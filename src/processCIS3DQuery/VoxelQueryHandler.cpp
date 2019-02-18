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
    double binSize = stat.getMaximum() / 29;
    binSize = binSize == 0 ? 1 : binSize;
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
    qDebug() << synapsesPerVoxel.getMaximum() << synapsesPerVoxel.getMean() << synapsesPerVoxelH.getNumberOfBins();

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
    result.insert("numberSelectedVoxels", (int)mSelectedVoxels.size());
    result.insert("numberPreInnervatedVoxels", (int)mPreInnervatedVoxels.size());
    result.insert("numberPostInnervatedVoxels", (int)mPostInnervatedVoxels.size());
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
    mAborted = false;
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
        if (mode == "voxel" || mode == "prePostVoxel")
        {
            voxelOrigin.push_back(voxelFilter["originX"].toDouble());
            voxelOrigin.push_back(voxelFilter["originY"].toDouble());
            voxelOrigin.push_back(voxelFilter["originZ"].toDouble());
            voxelDimensions.push_back(voxelFilter["dimX"].toInt(1));
            voxelDimensions.push_back(voxelFilter["dimY"].toInt(1));
            voxelDimensions.push_back(voxelFilter["dimZ"].toInt(1));
        }
        else
        {
            voxelOrigin.push_back(-5000);
            voxelOrigin.push_back(-5000);
            voxelOrigin.push_back(-5000);
            voxelDimensions.push_back(500);
            voxelDimensions.push_back(500);
            voxelDimensions.push_back(500);
        }

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
        QString voxelPosFile = QDir::cleanPath(metaFolder + QDir::separator() + "voxel_pos.dat");
        QString indexFile = QDir::cleanPath(metaFolder + QDir::separator() + "voxel_indexL.dat");
        QString innervationFolder = QDir::cleanPath(mDataRoot + QDir::separator() + "spatialInnervation" + QDir::separator() + "innervation");

        // ################# DETERMINE NEURON IDS #################

        NeuronSelection selection;
        if (mode == "prePost" || mode == "prePostVoxel")
        {
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

            selection = NeuronSelection(preNeurons, postNeurons);
            QVector<float> bbmin;
            QVector<float> bbmax;
            bbmin.append(-10000);
            bbmin.append(-10000);
            bbmin.append(-10000);
            bbmax.append(10000);
            bbmax.append(10000);
            bbmax.append(10000);
            selection.setBBox(bbmin, bbmax);
            if (isSlice)
            {
                selection.filterInnervationSlice(mNetwork, sliceRef, tissueLowPre, tissueHighPre, tissueModePre, tissueLowPost, tissueHighPost, tissueModePost);
            }
        }
        else
        {
            selection.setFullModel(mNetwork, false);
        }

        IdList preIds = selection.Presynaptic();
        IdList postIds = selection.Postsynaptic();

        qDebug() << preIds.size() << postIds.size();

        std::set<int> mappedPreIds;
        std::set<int> prunedPostIds;
        std::map<int, int> preMultiplicity;

        for (int i = 0; i < preIds.size(); i++)
        {
            int mappedId = mNetwork.axonRedundancyMap.getNeuronIdToUse(preIds[i]);
            if (preMultiplicity.find(mappedId) == preMultiplicity.end())
            {
                mappedPreIds.insert(mappedId);
                preMultiplicity[mappedId] = 1;
            }
            else
            {
                preMultiplicity[mappedId] += 1;
            }
        }

        for (int i = 0; i < postIds.size(); i++)
        {
            int postId = postIds[i];
            prunedPostIds.insert(postId);
        }

        // Read post all
        std::map<int, float> postAllField;
        FeatureProvider::readMapFloat(postAllField, postAllFolder, "voxel_postAllExc.dat");

        // ################# LOOP OVER VOXELS #################

        int voxelCount = -1;
        int totalVoxelCount = 0;

        double minX = voxelOrigin[0];
        double minY = voxelOrigin[1];
        double minZ = voxelOrigin[2];
        double maxX = voxelOrigin[0] + 50 * voxelDimensions[0];
        double maxY = voxelOrigin[1] + 50 * voxelDimensions[1];
        double maxZ = voxelOrigin[2] + 50 * voxelDimensions[2];

        std::set<int> filteredVoxels;
        QFile posFile(voxelPosFile);
        if (posFile.open(QIODevice::ReadOnly))
        {
            QTextStream in(&posFile);
            while (!in.atEnd())
            {
                totalVoxelCount++;

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
                            mSelectedVoxels.insert(voxelId);
                            filteredVoxels.insert(voxelId);
                        }
                    }
                }
            }
        }
        else
        {
            const QString msg =
                QString("Error reading index file. Could not open file %1").arg(voxelPosFile);
            throw std::runtime_error(qPrintable(msg));
        }

        totalVoxelCount = filteredVoxels.size();

        QFile inputFile(indexFile);
        if (inputFile.open(QIODevice::ReadOnly))
        {
            QTextStream in(&inputFile);
            while (!in.atEnd() && !mAborted)
            {
                QString line = in.readLine();
                line = line.trimmed();
                QStringList parts = line.split(" ");
                int voxelId = parts[0].toInt();

                if (filteredVoxels.find(voxelId) != filteredVoxels.end())
                {
                    voxelCount++;
                    // ################ PROCESS SINGLE VOXEL ################

                    std::map<int, float> pre;
                    std::map<int, float> post;
                    float boutonSum = 0;
                    float postSum = 0;

                    float postAll = postAllField[voxelId];

                    mMapPostCellsPerVoxel[voxelId] = 0;
                    mMapPreCellsPerVoxel[voxelId] = 0;
                    mMapBoutonsPerVoxel[voxelId] = 0;
                    mMapPostsynapticSitesPerVoxel[voxelId] = 0;

                    for (int i = 4; i < parts.size(); i += 2)
                    {
                        int neuronId = parts[i].toInt();
                        if (neuronId > 0 && (mappedPreIds.find(neuronId) != mappedPreIds.end()))
                        {
                            mMapPreCellsPerVoxel[voxelId] += preMultiplicity[neuronId];
                            float boutons = preMultiplicity[neuronId] * parts[i + 1].toFloat();
                            boutonSum += boutons;
                            mMapBoutonsPerVoxel[voxelId] += boutons;
                            pre[neuronId] = boutons;
                            mPreInnervatedVoxels.insert(voxelId);
                        }
                        else if (prunedPostIds.find(-neuronId) != prunedPostIds.end())
                        {
                            mMapPostCellsPerVoxel[voxelId] += 1;
                            float pst = parts[i + 1].toFloat();
                            postSum += pst;
                            post[-neuronId] = pst;
                            mMapPostsynapticSitesPerVoxel[voxelId] += postAll * pst;
                            mPostInnervatedVoxels.insert(voxelId);
                        }
                    }

                    float synapses = boutonSum * postSum;
                    mSynapsesPerVoxel[voxelId] = synapses;
                }

                // ################ REPORT UPDATE ################
                int updateRate = totalVoxelCount  / 20;
                if (voxelCount % updateRate == 0)
                {
                    const QJsonObject result = createJsonResult();
                    QJsonObject progress;
                    progress.insert("completed", voxelCount + 1);
                    progress.insert("total", totalVoxelCount);
                    const double percent = double(voxelCount + 1) * 100 / (double)totalVoxelCount;
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
            inputFile.close();
        }
        else
        {
            const QString msg =
                QString("Error reading index file. Could not open file %1").arg(indexFile);
            throw std::runtime_error(qPrintable(msg));
        }

        const QString zipFileName = QString("spatialInnervation_%1.json.zip").arg(mQueryId);
        const QString zipFullPath = QString("%1/%2").arg(mTempFolder).arg(zipFileName);
        const QString jsonFileName = QString(zipFileName).remove(".zip");
        const QString jsonFullPath = QString(zipFullPath).remove(".zip");

        const QString dataZipFileName = QString("spatialInnervation_%1.zip").arg(mQueryId);
        const QString dataZipFullPath = QString("%1/%2").arg(mTempFolder).arg(dataZipFileName);
        const QString dataFileName = QString(dataZipFileName).remove(".zip");
        const QString dataFullPath = QString(dataZipFullPath).remove(".zip");

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
            progress.insert("completed", voxelCount);
            progress.insert("total", totalVoxelCount);
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
