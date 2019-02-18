#include "VoxelQueryHandler.h"
#include "Histogram.h"
#include "QueryHelpers.h"
#include "CIS3DCellTypes.h"
#include "CIS3DNeurons.h"
#include "CIS3DRegions.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DSparseVectorSet.h"
#include "Util.h"
#include "UtilIO.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCoreApplication>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QProcess>
#include <stdexcept>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "NeuronSelection.h"
#include "FeatureProvider.h"
#include <fstream>

template <typename T>
void
VoxelQueryHandler::createStatistics(std::map<int, T>& values,
                                    Statistics& stat,
                                    Histogram& histogram)
{
    for (auto it = values.begin(); it != values.end(); it++)
    {
        stat.addSample(it->second);
    }
    double binSize = stat.getMean() / 15;
    histogram = Histogram(binSize);
    for (auto it = values.begin(); it != values.end(); it++)
    {
        histogram.addValue(it->second);
    }
}

QJsonObject
VoxelQueryHandler::createJsonResult()
{
    Statistics preCellsPerVoxel;
    Histogram preCellsPerVoxelH(50);
    createStatistics(mMapPreCellsPerVoxel, preCellsPerVoxel, preCellsPerVoxelH);

    Statistics boutonsPerVoxel;
    Histogram boutonsPerVoxelH(500);
    createStatistics(mMapBoutonsPerVoxel, boutonsPerVoxel, boutonsPerVoxelH);

    Statistics postCellsPerVoxel;
    Histogram postCellsPerVoxelH(50);
    createStatistics(mMapPostCellsPerVoxel, postCellsPerVoxel, postCellsPerVoxelH);

    Statistics postsynapticSitesPerVoxel;
    Histogram postsynapticSitesPerVoxelH(500);
    createStatistics(mMapPostsynapticSitesPerVoxel, postsynapticSitesPerVoxel, postsynapticSitesPerVoxelH);    

    Statistics synapsesPerVoxel;
    Histogram synapsesPerVoxelH(100);
    createStatistics(mSynapsesPerVoxel, synapsesPerVoxel, synapsesPerVoxelH);    

    // Synapses per connection
    Histogram synapsesPerConnectionMinH(1);
    Histogram synapsesPerConnectionMedH(1);
    Histogram synapsesPerConnectionMaxH(1);
    for (auto it_k = mSynapsesPerConnectionOccurrences.begin(); it_k != mSynapsesPerConnectionOccurrences.end(); it_k++)
    {
        int nonZero = 0;
        std::vector<float> occurences;

        //qDebug() << it_k->first << it_k->second.size();

        for (auto it_v = it_k->second.begin(); it_v != it_k->second.end(); it_v++)
        {
            occurences.push_back((float)it_v->second);
            nonZero++;
        }

        float min, med, max;
        Util::getMinMedMax(occurences, min, med, max);
        synapsesPerConnectionMinH.addValues((float)it_k->first, min);
        synapsesPerConnectionMedH.addValues((float)it_k->first, med);
        synapsesPerConnectionMaxH.addValues((float)it_k->first, max);
    }

    //qDebug() << synapsesPerVoxel.getMinimum();

    QJsonObject result;
    result.insert("numberInnervatedVoxels", (int)mInnervatedVoxels.size());
    result.insert("numberTotalVoxels", 0);
    result.insert("presynapticCellsPerVoxel", Util::createJsonStatistic(preCellsPerVoxel));
    result.insert("presynapticCellsPerVoxelHisto", Util::createJsonHistogram(preCellsPerVoxelH));
    result.insert("postsynapticCellsPerVoxel", Util::createJsonStatistic(postCellsPerVoxel));
    result.insert("postsynapticCellsPerVoxelHisto", Util::createJsonHistogram(postCellsPerVoxelH));
    result.insert("boutonsPerVoxel", Util::createJsonStatistic(boutonsPerVoxel));
    result.insert("boutonsPerVoxelHisto", Util::createJsonHistogram(boutonsPerVoxelH));
    result.insert("postsynapticSitesPerVoxel", Util::createJsonStatistic(postsynapticSitesPerVoxel));
    result.insert("postsynapticSitesPerVoxelHisto", Util::createJsonHistogram(postsynapticSitesPerVoxelH));
    result.insert("synapsesPerVoxel", Util::createJsonStatistic(synapsesPerVoxel));
    result.insert("synapsesPerVoxelHisto", Util::createJsonHistogram(synapsesPerVoxelH));
    result.insert("synapsesPerConnection", Util::createJsonStatistic(mSynapsesPerConnection));
    result.insert("synapsesPerConnectionMedHisto", Util::createJsonHistogram(synapsesPerConnectionMedH));
    result.insert("synapsesPerConnectionMinHisto", Util::createJsonHistogram(synapsesPerConnectionMinH));
    result.insert("synapsesPerConnectionMaxHisto", Util::createJsonHistogram(synapsesPerConnectionMaxH));
    result.insert("downloadS3key", "");
    result.insert("fileSize", 0);
    return result;
}

VoxelQueryHandler::VoxelQueryHandler(QObject* parent)
    : QObject(parent)
{
}

void
VoxelQueryHandler::process(const QString& selectionQueryId,
                           const QJsonObject& config)
{
    //qDebug() << "Process voxel query" << selectionQueryId;
    mConfig = config;
    mQueryId = selectionQueryId;

    mTempFolder = QDir::cleanPath(mConfig["WORKER_TMP_DIR"].toString() + QDir::separator() + mQueryId);
    UtilIO::makeDir(mTempFolder);

    const QString baseUrl = mConfig["METEOR_URL_CIS3D"].toString();
    const QString queryEndPoint = mConfig["METEOR_VOXELQUERY_ENDPOINT"].toString();
    const QString loginEndPoint = mConfig["METEOR_LOGIN_ENDPOINT"].toString();
    const QString logoutEndPoint = mConfig["METEOR_LOGOUT_ENDPOINT"].toString();

    if (baseUrl.isEmpty())
    {
        throw std::runtime_error("VoxelQueryHandler: Cannot find METEOR_URL_CIS3D");
    }
    if (queryEndPoint.isEmpty())
    {
        throw std::runtime_error("VoxelQueryHandler: Cannot find METEOR_VOXELQUERY_ENDPOINT");
    }
    if (loginEndPoint.isEmpty())
    {
        throw std::runtime_error("VoxelQueryHandler: Cannot find METEOR_LOGIN_ENDPOINT");
    }
    if (logoutEndPoint.isEmpty())
    {
        throw std::runtime_error("VoxelQueryHandler: Cannot find METEOR_LOGOUT_ENDPOINT");
    }

    mQueryUrl = baseUrl + queryEndPoint + mQueryId;
    mLoginUrl = baseUrl + loginEndPoint;
    mLogoutUrl = baseUrl + logoutEndPoint;

    mAuthInfo = QueryHelpers::login(mLoginUrl,
                                    mConfig["WORKER_USERNAME"].toString(),
                                    mConfig["WORKER_PASSWORD"].toString(),
                                    mNetworkManager,
                                    mConfig);

    QNetworkRequest request;
    request.setUrl(mQueryUrl);
    request.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
    request.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
    request.setAttribute(QNetworkRequest::User, QVariant("getVoxelQueryData"));
    QueryHelpers::setAuthorization(mConfig, request);

    connect(&mNetworkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyGetQueryFinished(QNetworkReply*)));
    mNetworkManager.get(request);
}

void
VoxelQueryHandler::readIndex(QString indexFile,
                             std::vector<double> origin,
                             std::vector<int> dimensions,
                             std::set<int>& voxelIds,
                             std::set<int>& neuronIds)
{
    double minX = origin[0];
    double minY = origin[1];
    double minZ = origin[2];
    double maxX = origin[0] + 50 * dimensions[0];
    double maxY = origin[1] + 50 * dimensions[1];
    double maxZ = origin[2] + 50 * dimensions[2];
    QFile inputFile(indexFile);
    if (inputFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&inputFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            line = line.trimmed();
            QStringList parts = line.split(" ");
            int voxelId = parts[0].toInt();
            double voxelX = parts[1].toDouble();
            double voxelY = parts[2].toDouble();
            double voxelZ = parts[3].toDouble();

            if (voxelX >= minX && voxelX <= maxX)
            {
                if (voxelY >= minY && voxelY <= maxY)
                {
                    if (voxelZ >= minZ && voxelZ <= maxZ)
                    {
                        voxelIds.insert(voxelId);
                        for (int i = 4; i < parts.size(); i++)
                        {
                            int neuronId = parts[i].toInt();
                            neuronIds.insert(neuronId);
                        }
                    }
                }
            }
        }
        inputFile.close();
    }
}

void
VoxelQueryHandler::replyGetQueryFinished(QNetworkReply* reply)
{
    QNetworkReply::NetworkError error = reply->error();
    if (error == QNetworkReply::NoError && !reply->request().attribute(QNetworkRequest::User).toString().contains("getVoxelQueryData"))
    {
        return;
    }
    else if (error == QNetworkReply::NoError)
    {
        const QByteArray content = reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(content);
        QJsonObject jsonData = jsonResponse.object().value("data").toObject();
        mCurrentJsonData = jsonData;
        reply->deleteLater();

        QJsonObject voxelFilter = jsonData["voxelFilter"].toObject();
        QString mode = voxelFilter["mode"].toString();
        std::vector<double> voxelOrigin;
        std::vector<int> voxelDimensions;
        voxelOrigin.push_back(voxelFilter["originX"].toDouble());
        voxelOrigin.push_back(voxelFilter["originY"].toDouble());
        voxelOrigin.push_back(voxelFilter["originZ"].toDouble());
        voxelDimensions.push_back(voxelFilter["dimX"].toInt(1));
        voxelDimensions.push_back(voxelFilter["dimY"].toInt(1));
        voxelDimensions.push_back(voxelFilter["dimZ"].toInt(1));

        qDebug() << "[*] Voxel query: " << mode << voxelOrigin[0] << voxelOrigin[1] << voxelOrigin[2] << voxelDimensions[0] << voxelDimensions[1] << voxelDimensions[2];

        const QString datasetShortName = jsonData["network"].toString();
        mDataRoot = QueryHelpers::getDatasetPath(datasetShortName, mConfig);
        qDebug() << "    Loading network data:" << datasetShortName << "Path: " << mDataRoot;
        mNetwork.setDataRoot(mDataRoot);
        mNetwork.loadFilesForSynapseComputation();

        QString preFolder = QDir::cleanPath(mDataRoot + QDir::separator() + "spatialInnervation" + QDir::separator() + "features_pre");
        QString postFolder = QDir::cleanPath(mDataRoot + QDir::separator() + "spatialInnervation" + QDir::separator() + "features_postExc");
        QString postAllFolder = QDir::cleanPath(mDataRoot + QDir::separator() + "spatialInnervation" + QDir::separator() + "features_postAll");
        QString metaFolder = QDir::cleanPath(mDataRoot + QDir::separator() + "spatialInnervation" + QDir::separator() + "features_meta");
        QString indexFile = QDir::cleanPath(metaFolder + QDir::separator() + "voxel_index.dat");
        QString innervationFolder = QDir::cleanPath(mDataRoot + QDir::separator() + "spatialInnervation" + QDir::separator() + "innervation");

        // EXTRACT SLICE PARAMETERS
        const double tissueLowPre = jsonData["tissueLowPre"].toDouble();
        const double tissueHighPre = jsonData["tissueHighPre"].toDouble();
        QString tissueModePre = jsonData["tissueModePre"].toString();
        const double tissueLowPost = jsonData["tissueLowPost"].toDouble();
        const double tissueHighPost = jsonData["tissueHighPost"].toDouble();
        QString tissueModePost = jsonData["tissueModePost"].toString();
        const double sliceRef = jsonData["sliceRef"].toDouble();
        const bool isSlice = sliceRef != -9999;
        qDebug() << "Slice ref, Tissue depth" << sliceRef << tissueLowPre << tissueHighPre << tissueModePre << tissueLowPost << tissueHighPost << tissueModePost;

        QString preSelString = jsonData["presynapticSelectionFilter"].toString();
        QJsonDocument preDoc = QJsonDocument::fromJson(preSelString.toLocal8Bit());
        QJsonArray preArr = preDoc.array();

        SelectionFilter preFilter = Util::getSelectionFilterFromJson(preArr, mNetwork, CIS3D::PRESYNAPTIC);
        Util::correctVPMSelectionFilter(preFilter, mNetwork);
        Util::correctInterneuronSelectionFilter(preFilter, mNetwork);
        IdList preNeurons = mNetwork.neurons.getFilteredNeuronIds(preFilter);

        QString postSelString = jsonData["postsynapticSelectionFilter"].toString();
        QJsonDocument postDoc = QJsonDocument::fromJson(postSelString.toLocal8Bit());
        QJsonArray postArr = postDoc.array();
        SelectionFilter postFilter = Util::getSelectionFilterFromJson(postArr, mNetwork, CIS3D::POSTSYNAPTIC);
        Util::correctInterneuronSelectionFilter(postFilter, mNetwork);
        IdList postNeurons = mNetwork.neurons.getFilteredNeuronIds(postFilter);

        NeuronSelection selection(preNeurons, postNeurons);
        QVector<float> bbmin;
        QVector<float> bbmax;
        bbmin.append(-10000);
        bbmin.append(-10000);
        bbmin.append(-10000);
        bbmax.append(10000);
        bbmax.append(10000);
        bbmax.append(10000);

        RandomGenerator generator(50000);

        //selection.filterUniquePre(mNetwork);
        selection.setBBox(bbmin, bbmax);
        if (isSlice)
        {
            selection.filterInnervationSlice(mNetwork, sliceRef, tissueLowPre, tissueHighPre, tissueModePre, tissueLowPost, tissueHighPost, tissueModePost);
        }

        if (mode == "voxel")
        {
            selection.setFullModel(mNetwork, false);
        }

        std::set<int> voxelIds;
        std::set<int> neuronIdsVoxel;
        if (mode == "voxel" || mode == "prePostVoxel")
        {
            readIndex(indexFile, voxelOrigin, voxelDimensions, voxelIds, neuronIdsVoxel);
            //qDebug() << voxelIds.size() << neuronIdsVoxel.size();
        }

        IdList preIds = selection.Presynaptic();
        IdList postIds = selection.Postsynaptic();

        //qDebug() << "[*] VOXEL QUERY " << preIds.size() << " presynaptic and " << postIds.size() << " postsynaptic neurons.";

        const QString zipFileName = QString("spatialInnervation_%1.json.zip").arg(mQueryId);
        const QString zipFullPath = QString("%1/%2").arg(mTempFolder).arg(zipFileName);
        const QString jsonFileName = QString(zipFileName).remove(".zip");
        const QString jsonFullPath = QString(zipFullPath).remove(".zip");

        const QString dataZipFileName = QString("spatialInnervation_%1.zip").arg(mQueryId);
        const QString dataZipFullPath = QString("%1/%2").arg(mTempFolder).arg(dataZipFileName);
        const QString dataFileName = QString(dataZipFileName).remove(".zip");
        const QString dataFullPath = QString(dataZipFullPath).remove(".zip");

        // ###################### LOOP OVER NEURONS ######################

        std::vector<int> mappedIds;
        std::vector<int> prunedPostIds;
        std::map<int, int> preMultiplicity;
        bool filterVoxel = mode == "voxel" || mode == "prePostVoxel";

        for (int i = 0; i < preIds.size(); i++)
        {
            int q = mNetwork.axonRedundancyMap.getNeuronIdToUse(preIds[i]);
            if (filterVoxel)
            {
                if (neuronIdsVoxel.find(q) == neuronIdsVoxel.end())
                {
                    continue;
                }
            }
            if (preMultiplicity.find(q) == preMultiplicity.end())
            {
                mappedIds.push_back(q);
                preMultiplicity[q] = 1;
            }
            else
            {
                preMultiplicity[q] += 1;
            }
        }

        for (int i = 0; i < postIds.size(); i++)
        {
            int postId = postIds[i];
            if (filterVoxel)
            {
                if (neuronIdsVoxel.find(postId) == neuronIdsVoxel.end())
                {
                    continue;
                }
            }
            prunedPostIds.push_back(postId);
        }

        int maxIndex = mappedIds.size() > prunedPostIds.size() ? mappedIds.size() : prunedPostIds.size();

        qDebug() << mappedIds.size() << prunedPostIds.size() << maxIndex;

        // Read post all
        std::map<int, float> postAll;
        FeatureProvider::readMapFloat(postAll, postAllFolder, "voxel_postAllExc.dat");

        mAborted = false;
        for (int i = 0; i < maxIndex; i++)
        {
            qDebug() << "loop" << i;

            if (mAborted)
            {
                break;
            }

            if (i < (int)mappedIds.size())
            {
                qDebug() << "pre" << i;
                int mappedPreId = mappedIds[i];
                int multiplicity = preMultiplicity[mappedPreId];
                QString dataFileName = QDir(preFolder).filePath(QString::number(mappedPreId) + ".dat");
                QString innervationFileName = QDir(innervationFolder).filePath("preNeuronID_" + QString::number(mappedPreId));

                QFile dataFile(dataFileName);
                if (!dataFile.open(QIODevice::ReadOnly))
                {
                    const QString msg =
                        QString("Cannot open file %1 for reading.").arg(dataFileName);
                    throw std::runtime_error(qPrintable(msg));
                }
                QTextStream inStream(&dataFile);

                QString line = inStream.readLine();
                while (!line.isNull())
                {
                    QStringList parts = line.split(' ');
                    bool isVoxelId;
                    int voxelId = parts[0].toInt(&isVoxelId);
                    float boutons = parts[1].toFloat();

                    if (!filterVoxel || (voxelIds.find(voxelId) != voxelIds.end()))
                    {
                        if (mMapPreCellsPerVoxel.find(voxelId) == mMapPreCellsPerVoxel.end())
                        {
                            mMapPreCellsPerVoxel[voxelId] = multiplicity;
                            mInnervatedVoxels.insert(voxelId);
                        }
                        else
                        {
                            mMapPreCellsPerVoxel[voxelId] += multiplicity;
                        }

                        if (mMapBoutonsPerVoxel.find(voxelId) == mMapBoutonsPerVoxel.end())
                        {
                            mMapBoutonsPerVoxel[voxelId] = multiplicity * boutons;
                        }
                        else
                        {
                            mMapBoutonsPerVoxel[voxelId] += multiplicity * boutons;
                        }
                    }

                    line = inStream.readLine();
                }

                // Synapses per voxel
                QFile dataFile2(innervationFileName);
                if (!dataFile2.open(QIODevice::ReadOnly))
                {
                    const QString msg =
                        QString("Cannot open file %1 for writing.").arg(innervationFileName);
                    throw std::runtime_error(qPrintable(msg));
                }
                QTextStream inStream2(&dataFile2);

                /*
                qDebug() << "pre innervation" << i;                

                QString line2 = inStream2.readLine();
                std::map<int, float> synapsesVoxel;
                int currentPostId = -1;
                while (!line2.isNull())
                {
                    QStringList parts = line2.split(' ');
                    bool isVoxelId;
                    int voxelId = parts[0].toInt(&isVoxelId);
                    if (isVoxelId && currentPostId != -1)
                    {
                        if (!filterVoxel || (voxelIds.find(voxelId) != voxelIds.end()))
                        {
                            float innervation = multiplicity * parts[1].toFloat();
                            if (mSynapsesPerVoxel.find(voxelId) == mSynapsesPerVoxel.end())
                            {
                                mSynapsesPerVoxel[voxelId] = innervation;
                            }
                            else
                            {
                                mSynapsesPerVoxel[voxelId] += innervation;
                            }
                            //synapsesVoxel[voxelId] += innervation;                            
                                for (int k = 0; k < multiplicity; k++)
                                {
                                    float synapses = calculateSynapse(generator, innervation);
                                    if (synapses > 0)
                                    {
                                        int count = (int)synapses;
                                        if (mSynapsesPerConnectionOccurrences.find(count) == mSynapsesPerConnectionOccurrences.end())
                                        {
                                            std::map<int, int> foo;
                                            mSynapsesPerConnectionOccurrences[count] = foo;
                                        }
                                        auto occ = mSynapsesPerConnectionOccurrences.find(count);
                                        if (occ->second.find(voxelId) == occ->second.end())
                                        {
                                            occ->second[voxelId] = 1;
                                        }
                                        else
                                        {
                                            occ->second[voxelId] += 1;
                                        }

                                        mSynapsesPerConnection.addSample(synapses);
                                    }
                                    if (synapsesVoxel.find(voxelId) == synapsesVoxel.end())
                                    {
                                        synapsesVoxel[voxelId] = synapses;
                                    }
                                    else
                                    {
                                        synapsesVoxel[voxelId] += synapses;
                                    }
                                }                                
                        }
                    }
                    else
                    {
                        currentPostId = parts[1].toInt();
                        if (!postIds.contains(currentPostId))
                        {
                            currentPostId = -1;
                        }
                    }
                    line2 = inStream2.readLine();
                }
                */
            }

            if (i < (int)prunedPostIds.size())
            {
                qDebug() << "post" << i;
                int postId = prunedPostIds[i];
                QString dataFileName = QDir(postFolder).filePath(QString::number(postId) + ".dat");

                QFile dataFile(dataFileName);
                if (!dataFile.open(QIODevice::ReadOnly))
                {
                    const QString msg =
                        QString("Cannot open file %1 for reading.").arg(dataFileName);
                    throw std::runtime_error(qPrintable(msg));
                }
                QTextStream inStream(&dataFile);

                QString line = inStream.readLine();
                while (!line.isNull())
                {
                    QStringList parts = line.split(' ');
                    bool isVoxelId;
                    int voxelId = parts[0].toInt(&isVoxelId);

                    if (!filterVoxel || (voxelIds.find(voxelId) != voxelIds.end()))
                    {
                        float pstNorm = parts[1].toFloat();
                        float pst = postAll[voxelId] * pstNorm;

                        if (mMapPostCellsPerVoxel.find(voxelId) == mMapPostCellsPerVoxel.end())
                        {
                            mMapPostCellsPerVoxel[voxelId] = 1;
                            mInnervatedVoxels.insert(voxelId);
                        }
                        else
                        {
                            mMapPostCellsPerVoxel[voxelId] += 1;
                        }

                        if (mMapPostsynapticSitesPerVoxel.find(voxelId) == mMapPostsynapticSitesPerVoxel.end())
                        {
                            mMapPostsynapticSitesPerVoxel[voxelId] = pst;
                        }
                        else
                        {
                            mMapPostsynapticSitesPerVoxel[voxelId] += pst;
                        }
                    }

                    line = inStream.readLine();
                }
            }

            if (i % 10 == 0)
            {
                const QJsonObject result = createJsonResult();
                QJsonObject progress;
                progress.insert("completed", i + 1);
                progress.insert("total", maxIndex);
                const double percent = double(i + 1) * 100.0 / (double)maxIndex;
                progress.insert("percent", percent);
                QJsonObject jobStatus;
                jobStatus.insert("status", "running");
                jobStatus.insert("progress", progress);

                QJsonObject payload;
                payload.insert("jobStatus", jobStatus);
                payload.insert("result", result);

                QNetworkRequest putRequest;
                putRequest.setUrl(mQueryUrl);
                putRequest.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
                putRequest.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
                putRequest.setAttribute(QNetworkRequest::User, QVariant("putSpatialInnervationResult"));
                putRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
                QueryHelpers::setAuthorization(mConfig, putRequest);

                QJsonDocument putDoc(payload);
                QString putData(putDoc.toJson());

                //qDebug() << "[*] Posting intermediate result:" << percent << "\%    (" << i + 1 << "/" << preIds.size() << ")";

                QEventLoop loop;
                QNetworkReply* reply = mNetworkManager.put(putRequest, putData.toLocal8Bit());
                connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
                loop.exec();

                QNetworkReply::NetworkError error = reply->error();
                const QString requestId = reply->request().attribute(QNetworkRequest::User).toString();
                if (error != QNetworkReply::NoError)
                {
                    qDebug() << "[-] Error putting Evaluation result (queryId" << mQueryId << "):";
                    qDebug() << reply->errorString();
                    mAborted = true;
                }
            }
        }

        /*
        if (!mAborted)
        {
            // ###################### PROCESS VOXELS ######################

            std::vector<int> voxelIds;
            std::vector<float> x;
            std::vector<float> y;
            std::vector<float> z;

            QJsonArray positionsJson;
            QJsonArray voxelIdsJson;
            QJsonArray colorsJson;

            QString fileName = QDir(metaFolder).filePath("voxel_pos.dat");
            QFile file(fileName);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                const QString msg =
                    QString("Error reading features file. Could not open file %1").arg(fileName);
                throw std::runtime_error(qPrintable(msg));
            }

            QTextStream in(&file);
            QString line = in.readLine();
            while (!line.isNull())
            {
                QStringList parts = line.split(' ');
                int voxelId = parts[0].toInt();
                auto it = innervationPerVoxel.find(voxelId);
                if (it != innervationPerVoxel.end())
                {
                    voxelIds.push_back(it->first);
                    x.push_back(parts[1].toFloat());
                    y.push_back(parts[2].toFloat());
                    z.push_back(parts[3].toFloat());
                    positionsJson.push_back(QJsonValue(parts[1].toFloat()));
                    positionsJson.push_back(QJsonValue(parts[2].toFloat()));
                    positionsJson.push_back(QJsonValue(parts[3].toFloat()));
                    voxelIdsJson.push_back(QJsonValue(it->first));
                    colorsJson.push_back(it->second);
                    colorsJson.push_back(it->second);
                    colorsJson.push_back(it->second);
                }
                line = in.readLine();
            }

            QString voxelFileName = QDir(mTempFolder).filePath("voxelPositions");
            fileNames.append(voxelFileName);
            QFile voxelFile(voxelFileName);
            if (!voxelFile.open(QIODevice::WriteOnly))
            {
                const QString msg =
                    QString("Cannot open file %1 for writing.").arg(voxelFileName);
                throw std::runtime_error(qPrintable(msg));
            }
            QTextStream stream(&voxelFile);
            for (unsigned int i = 0; i < voxelIds.size(); i++)
            {
                //qDebug() << i;
                stream << voxelIds[i] << " " << x[i] << " " << y[i] << " " << z[i] << "\n";
            }
            voxelFile.close();

            qDebug() << "[*] Add voxel positions to zip file:" << dataFullPath;
            QProcess zip3;
            zip3.setWorkingDirectory(mTempFolder);

            QStringList arguments3;

            arguments3.append("-ju");

            arguments3.append(dataZipFullPath);
            arguments3.append(voxelFileName);

            zip3.start("zip", arguments3);

            if (!zip3.waitForStarted())
            {
                throw std::runtime_error("Error starting zip process");
            }

            if (!zip3.waitForFinished())
            {
                throw std::runtime_error("Error completing zip process");
            }

            QJsonObject metadata;
            metadata.insert("version", 1);
            metadata.insert("type", "BufferGeometry");
            metadata.insert("generator", "CortexInSilico3D");

            QJsonObject voxelIdsField;
            voxelIdsField.insert("array", voxelIdsJson);

            QJsonObject position;
            position.insert("itemSize", 3);
            position.insert("type", "Float32Array");
            position.insert("array", positionsJson);

            QJsonObject color;
            color.insert("itemSize", 3);
            color.insert("type", "Float32Array");
            color.insert("array", colorsJson);

            QJsonObject attributes;
            attributes.insert("position", position);
            attributes.insert("color", color);

            QJsonObject data;
            data.insert("attributes", attributes);

            QJsonObject result;
            result.insert("metadata", metadata);
            result.insert("data", data);

            QFile jsonFile(jsonFullPath);
            if (!jsonFile.open(QIODevice::WriteOnly))
            {
                const QString msg = QString("Cannot open file for saving json: %1").arg(jsonFullPath);
                throw std::runtime_error(qPrintable(msg));
            }

            QJsonDocument doc(result);
            QTextStream out(&jsonFile);
            out << doc.toJson(QJsonDocument::Compact);
            jsonFile.close();
        }
        */

        //qint64 fileSizeBytes1 = 0;
        //qint64 fileSizeBytes2 = 0;

        if (mAborted)
        {
            logoutAndExit(1);
        }
        else
        {
            // ###################### ZIP FILES ######################

            /*
            qDebug() << "[*] Zipping view file:" << jsonFullPath;
            QProcess zip;
            zip.setWorkingDirectory(mTempFolder);

            QStringList arguments;
            arguments.append("-j");
            arguments.append(zipFileName);
            arguments.append(jsonFileName);

            qDebug() << "Arguments" << arguments;
            zip.start("zip", arguments);

            if (!zip.waitForStarted())
            {
                throw std::runtime_error("Error starting zip process");
            }

            if (!zip.waitForFinished())
            {
                throw std::runtime_error("Error completing zip process");
            }
            */

            // ###################### UPLOAD FILES ######################

            /*
            //fileSizeBytes1 = QFileInfo(zipFullPath).size();
            int upload1 = QueryHelpers::uploadToS3(zipFileName, zipFullPath, mConfig);
            //fileSizeBytes2 = QFileInfo(dataZipFullPath).size();
            int upload2 = QueryHelpers::uploadToS3(dataZipFileName, dataZipFullPath, mConfig);

            if (upload1 != 0)
            {
                qDebug() << "Error uploading geometry json file to S3:" << zipFullPath;
            }
            if (upload2 != 0)
            {
                qDebug() << "Error uploading geometry json file to S3:" << dataZipFullPath;
            }
            if (upload1 != 0 || upload2 != 0)
            {
                mAborted = true;
            }
            */
        }

        // ###################### CLEAN FILES ######################

        /*
        QProcess rm;
        QStringList rmArgs;
        rmArgs << "-rf" << mTempFolder;
        rm.start("rm", rmArgs);
        rm.waitForStarted();
        rm.waitForFinished();

        qDebug() << "[*] Removed original files";
        */

        if (mAborted)
        {
            logoutAndExit(1);
        }
        else
        {
            // ###################### SIGNAL COMPLETION ######################

            const QJsonObject result = createJsonResult();

            QJsonObject progress;
            progress.insert("completed", maxIndex);
            progress.insert("total", maxIndex);
            progress.insert("percent", 100);
            QJsonObject jobStatus;
            jobStatus.insert("status", "completed");
            jobStatus.insert("progress", progress);

            QJsonObject payload;
            payload.insert("result", result);
            payload.insert("jobStatus", jobStatus);

            reply->deleteLater();

            QNetworkRequest putRequest;
            putRequest.setUrl(mQueryUrl);
            putRequest.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
            putRequest.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
            putRequest.setAttribute(QNetworkRequest::User, QVariant("putVoxelResult"));
            putRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            QueryHelpers::setAuthorization(mConfig, putRequest);

            QJsonDocument putDoc(payload);
            QString putData(putDoc.toJson());

            connect(&mNetworkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyPutResultFinished(QNetworkReply*)));
            mNetworkManager.put(putRequest, putData.toLocal8Bit());
        }
    }
    else
    {
        qDebug() << "[-] Error obtaining EvaluationQuery data:";
        qDebug() << reply->errorString();
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404)
        {
            qDebug() << QString(reply->readAll().replace("\"", ""));
        }
        reply->deleteLater();
        logoutAndExit(1);
    }
}

void
VoxelQueryHandler::replyPutResultFinished(QNetworkReply* reply)
{
    QNetworkReply::NetworkError error = reply->error();
    const QString requestId = reply->request().attribute(QNetworkRequest::User).toString();
    if (error == QNetworkReply::NoError && !(requestId == "putVoxelResult"))
    {
        return;
    }
    else if (error == QNetworkReply::NoError)
    {
        qDebug() << "    Completed processing voxel query" << mQueryId;
        reply->deleteLater();
        logoutAndExit(0);
    }
    else
    {
        qDebug() << "[-] Error putting voxel query result (queryId" << mQueryId << "):";
        qDebug() << reply->errorString();
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404)
        {
            qDebug() << QString(reply->readAll().replace("\"", ""));
        }
        reply->deleteLater();
        logoutAndExit(1);
    }
}

void
VoxelQueryHandler::logoutAndExit(const int exitCode)
{
    QueryHelpers::logout(mLogoutUrl,
                         mAuthInfo,
                         mNetworkManager,
                         mConfig);

    if (exitCode == 0)
    {
        emit completedProcessing();
    }
    else
    {
        QCoreApplication::exit(exitCode);
    }
}

float
VoxelQueryHandler::calculateSynapse(RandomGenerator& generator, float innervation)
{
    // TODO: apply user formula
    int count = generator.drawPoisson(innervation);
    return (float)count;
}
