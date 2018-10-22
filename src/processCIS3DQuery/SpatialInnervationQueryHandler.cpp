#include "SpatialInnervationQueryHandler.h"
#include "Histogram.h"
#include "QueryHelpers.h"
#include "CIS3DCellTypes.h"
#include "CIS3DNeurons.h"
#include "CIS3DRegions.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DSparseVectorSet.h"
#include "Util.h"
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
#include "FeatureProvider.h"
#include "NeuronSelection.h"

QJsonObject
createJsonResult(/*const IdsPerCellTypeRegion& idsPerCellTypeRegion,
                 const NetworkProps& network,*/
                 const QString& key,
                 const qint64 fileSizeBytes)
{
    /*
    QJsonArray entries;
    int total = 0;

    for (IdsPerCellTypeRegion::const_iterator it = idsPerCellTypeRegion.constBegin(); it != idsPerCellTypeRegion.constEnd(); ++it)
    {
        const CellTypeRegion ctr = it.key();
        const int numNeurons = it.value().size();
        const QString cellType = network.cellTypes.getName(ctr.first);
        const QString region = network.regions.getName(ctr.second);

        QJsonObject obj;
        obj["cellType"] = cellType;
        obj["column"] = region;
        obj["count"] = numNeurons;

        entries.append(obj);
        total += numNeurons;
    }

    QJsonObject selection;
    selection.insert("CountsPerCellTypeColumn", entries);
    selection.insert("Total", total);

    QJsonObject tables;
    tables.insert("selection", selection);
*/
    QJsonObject result;
    //result.insert("tables", tables);
    result.insert("voxelS3key", key);
    result.insert("voxelFileSize", fileSizeBytes);

    return result;
}

QString
createGeometryJSON(const QString& zipFileName,
                   FeatureProvider& featureProvider,
                   const QString& tmpDir)
{
    const QString zipFullPath = QString("%1/%2").arg(tmpDir).arg(zipFileName);
    const QString jsonFileName = QString(zipFileName).remove(".zip");
    const QString jsonFullPath = QString(zipFullPath).remove(".zip");

    QFile jsonFile(jsonFullPath);
    if (!jsonFile.open(QIODevice::WriteOnly))
    {
        const QString msg = QString("Cannot open file for saving json: %1").arg(jsonFullPath);
        throw std::runtime_error(qPrintable(msg));
    }

    // ###################### LOAD NONEMPTY VOXELS ######################

    std::vector<float> x;
    std::vector<float> y;
    std::vector<float> z;

    featureProvider.loadVoxelPositions(x, y, z);

    qDebug() << "Size" << x.size();

    QJsonArray positions;

    for (unsigned int i = 0; i < x.size(); ++i)
    {
        positions.push_back(QJsonValue(x[i]));
        positions.push_back(QJsonValue(y[i]));
        positions.push_back(QJsonValue(z[i]));
    }

    // ###################### LOAD FEATURES ######################

    std::map<int, std::map<int, float> > neuron_pre;
    std::map<int, std::map<int, float> > neuron_postExc;
    std::map<int, std::map<int, float> > neuron_postInh;
    std::map<int, float> voxel_postAllExc;
    std::map<int, float> voxel_postAllInh;
    std::map<int, int> neuron_funct;
    std::map<int, std::set<int> > voxel_neuronsPre;
    std::map<int, std::set<int> > voxel_neuronsPostExc;
    std::map<int, std::set<int> > voxel_neuronsPostInh;
    std::vector<int> voxel;

    featureProvider.load(neuron_pre, 1, neuron_postExc, 1, neuron_postInh, voxel_postAllExc, 1, voxel_postAllInh, neuron_funct, voxel_neuronsPre, voxel_neuronsPostExc, voxel_neuronsPostInh);

    std::vector<int> preIndices;
    for (auto it = neuron_pre.begin(); it != neuron_pre.end(); ++it)
    {
        preIndices.push_back(it->first);
    }

    std::vector<int> postIndices;
    for (auto it = neuron_postExc.begin(); it != neuron_postExc.end(); ++it)
    {
        postIndices.push_back(it->first);
    }

    // ###################### INIT OUTPUT FIELDS ######################

    std::vector<int> empty(postIndices.size(), 0);
    std::vector<float> emptyFloat(postIndices.size(), 0);
    std::vector<std::vector<int> > contacts(preIndices.size(), empty);
    std::vector<std::vector<float> > innervation(preIndices.size(), emptyFloat);

    std::vector<double> sufficientStat;
    sufficientStat.push_back(0);
    sufficientStat.push_back(0);
    sufficientStat.push_back(0);
    sufficientStat.push_back(0);

    Statistics connProbSynapse;
    Statistics connProbInnervation;

    // ###################### LOOP OVER NEURONS ######################


    for (unsigned int i = 0; i < preIndices.size(); i++)
    {
        std::map<int, std::map<int, float>> innervationPerPre;
        int preId = preIndices[i];
        for (unsigned int j = 0; j < postIndices.size(); j++)
        {
            int postId = postIndices[j];
            if (preId != postId)
            {
                std::map<int, float> innervationPerVoxel;
                for (auto pre = neuron_pre[preId].begin(); pre != neuron_pre[preId].end(); ++pre)
                {
                    if (neuron_postExc[postId].find(pre->first) != neuron_postExc[postId].end())
                    {
                        float preVal = pre->second;
                        float postVal = neuron_postExc[postId][pre->first];
                        float arg = preVal * postVal;
                        innervationPerVoxel[pre->first] = arg;
                    }
                }
                innervationPerPre[postId] = innervationPerVoxel;
            }
        }
    }

    // ###################### DETERMINE STATISTICS ######################

    /*
    for (unsigned int i = 0; i < preIndices.size(); i++)
    {
        int realizedConnections = 0;
        for (unsigned int j = 0; j < postIndices.size(); j++)
        {
            connProbInnervation.addSample(calculateProbability(innervation[i][j]));
            realizedConnections += contacts[i][j] > 0 ? 1 : 0;
        }
        double probability = (double)realizedConnections / (double)postIndices.size();
        connProbSynapse.addSample(probability);
    }*/

    // ###################### WRITE OUTPUT ######################

    QJsonObject metadata;
    metadata.insert("version", 1);
    metadata.insert("type", "BufferGeometry");
    metadata.insert("generator", "CortexInSilico3D");

    QJsonObject position;
    position.insert("itemSize", 3);
    position.insert("type", "Float32Array");
    position.insert("array", positions);

    /*
    QJsonObject cellTypeID;
    cellTypeID.insert("itemSize", 1);
    cellTypeID.insert("type", "Uint8Array");
    cellTypeID.insert("array", cellTypeIds);

    QJsonObject regionID;
    regionID.insert("itemSize", 1);
    regionID.insert("type", "Uint8Array");
    regionID.insert("array", regionIds);
    */

    QJsonObject attributes;
    attributes.insert("position", position);
    //attributes.insert("cellTypeID", cellTypeID);
    //attributes.insert("regionID", regionID);

    QJsonObject data;
    data.insert("attributes", attributes);

    QJsonObject result;
    result.insert("metadata", metadata);
    result.insert("data", data);

    QJsonDocument doc(result);
    QTextStream out(&jsonFile);
    out << doc.toJson(QJsonDocument::Compact);
    jsonFile.close();

    qDebug() << "[*] Zipping geometry file:" << jsonFullPath;
    QProcess zip;
    zip.setWorkingDirectory(tmpDir);

    QStringList arguments;
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
    qDebug() << "[*] Completed zipping";

    return zipFullPath;
}

SpatialInnervationQueryHandler::SpatialInnervationQueryHandler(QObject* parent)
    : QObject(parent)
{
}

void
SpatialInnervationQueryHandler::process(const QString& selectionQueryId,
                                        const QJsonObject& config)
{
    qDebug() << "Process spatial innervation query" << selectionQueryId;
    mConfig = config;
    mQueryId = selectionQueryId;

    const QString baseUrl = mConfig["METEOR_URL_CIS3D"].toString();
    const QString queryEndPoint = mConfig["METEOR_SPATIALINNERVATIONQUERY_ENDPOINT"].toString();
    const QString loginEndPoint = mConfig["METEOR_LOGIN_ENDPOINT"].toString();
    const QString logoutEndPoint = mConfig["METEOR_LOGOUT_ENDPOINT"].toString();

    if (baseUrl.isEmpty())
    {
        throw std::runtime_error("SelectionQueryHandler: Cannot find METEOR_URL_CIS3D");
    }
    if (queryEndPoint.isEmpty())
    {
        throw std::runtime_error("SelectionQueryHandler: Cannot find METEOR_SPATIALINNERVATIONQUERY_ENDPOINT");
    }
    if (loginEndPoint.isEmpty())
    {
        throw std::runtime_error("SelectionQueryHandler: Cannot find METEOR_LOGIN_ENDPOINT");
    }
    if (logoutEndPoint.isEmpty())
    {
        throw std::runtime_error("SelectionQueryHandler: Cannot find METEOR_LOGOUT_ENDPOINT");
    }

    mQueryUrl = baseUrl + queryEndPoint + mQueryId;
    mLoginUrl = baseUrl + loginEndPoint;
    mLogoutUrl = baseUrl + logoutEndPoint;

    mAuthInfo = QueryHelpers::login(mLoginUrl,
                                    mConfig["WORKER_USERNAME"].toString(),
                                    mConfig["WORKER_PASSWORD"].toString(),
                                    mNetworkManager);

    QNetworkRequest request;
    request.setUrl(mQueryUrl);
    request.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
    request.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
    request.setAttribute(QNetworkRequest::User, QVariant("getSpatialInnervationQueryData"));

    connect(&mNetworkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyGetQueryFinished(QNetworkReply*)));
    mNetworkManager.get(request);
}

void
SpatialInnervationQueryHandler::replyGetQueryFinished(QNetworkReply* reply)
{
    qDebug() << "SPATIAL response";
    QNetworkReply::NetworkError error = reply->error();
    if (error == QNetworkReply::NoError && !reply->request().attribute(QNetworkRequest::User).toString().contains("getSpatialInnervationQueryData"))
    {
        return;
    }
    else if (error == QNetworkReply::NoError)
    {
        qDebug() << "[*] SPATIAL PROCESSING";

        const QByteArray content = reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(content);
        QJsonObject jsonData = jsonResponse.object().value("data").toObject();
        mCurrentJsonData = jsonData;
        reply->deleteLater();

        const QString datasetShortName = jsonData["network"].toString();
        mDataRoot = QueryHelpers::getDatasetPath(datasetShortName, mConfig);
        qDebug() << "    Loading network data:" << datasetShortName << "Path: " << mDataRoot;
        mNetwork.setDataRoot(mDataRoot);
        mNetwork.loadFilesForSynapseComputation();

        // EXTRACT CONNECTION PROBABILITY FORMULA
        // QString connProbFormula = jsonData["connProbFormula"].toString();

        // EXTRACT SLICE PARAMETERS
        const double tissueLowPre = jsonData["tissueLowPre"].toDouble();
        const double tissueHighPre = jsonData["tissueHighPre"].toDouble();
        const double tissueLowPost = jsonData["tissueLowPost"].toDouble();
        const double tissueHighPost = jsonData["tissueHighPost"].toDouble();
        const double sliceRef = jsonData["sliceRef"].toDouble();
        //const bool isSlice = sliceRef != -9999;
        qDebug() << "Slice ref, Tissue depth" << sliceRef << tissueLowPre << tissueHighPre << tissueLowPost << tissueHighPost;

        QString preSelString = jsonData["presynapticSelectionFilter"].toString();
        QJsonDocument preDoc = QJsonDocument::fromJson(preSelString.toLocal8Bit());
        QJsonArray preArr = preDoc.array();

        SelectionFilter preFilter = Util::getSelectionFilterFromJson(preArr, mNetwork, CIS3D::PRESYNAPTIC);
        Util::correctVPMSelectionFilter(preFilter, mNetwork);
        Util::correctInterneuronSelectionFilter(preFilter, mNetwork);
        IdList preNeurons = mNetwork.neurons.getFilteredNeuronIds(preFilter);
        preNeurons = NeuronSelection::filterTissueDepth(mNetwork, preNeurons, sliceRef, tissueLowPre, tissueHighPre, CIS3D::SliceBand::FIRST);

        QString postSelString = jsonData["postsynapticSelectionFilter"].toString();
        QJsonDocument postDoc = QJsonDocument::fromJson(postSelString.toLocal8Bit());
        QJsonArray postArr = postDoc.array();
        SelectionFilter postFilter = Util::getSelectionFilterFromJson(postArr, mNetwork, CIS3D::POSTSYNAPTIC);
        Util::correctInterneuronSelectionFilter(postFilter, mNetwork);
        IdList postNeurons = mNetwork.neurons.getFilteredNeuronIds(postFilter);
        postNeurons = NeuronSelection::filterTissueDepth(mNetwork, postNeurons, sliceRef, tissueLowPost, tissueHighPost, CIS3D::SliceBand::FIRST);

        qDebug() << "[*] SPATIAL QUERY " << preNeurons.size() << " presynaptic and " << postNeurons.size() << " postsynaptic neurons.";
        NeuronSelection selection(preNeurons, postNeurons);
        QVector<float> bbmin;
        QVector<float> bbmax;
        bbmin.append(-10000);
        bbmin.append(-10000);
        bbmin.append(-10000);
        bbmax.append(10000);
        bbmax.append(10000);
        bbmax.append(10000);

        selection.sampleDown(10000, -1);
        selection.setBBox(bbmin, bbmax);

        QString workingFolder = QDir::cleanPath(mConfig["WORKER_TMP_DIR"].toString() + QDir::separator() + mQueryId);
        FeatureProvider featureProvider(workingFolder, false);
        featureProvider.preprocessFeatures(mNetwork, selection, 0.00000000000000001, false, true);

        const QString key = QString("spatialInnervation_%1.json.zip").arg(mQueryId);
        QString geometryFile;
        try
        {
            geometryFile = createGeometryJSON(key, featureProvider, mConfig["WORKER_TMP_DIR"].toString());
        }
        catch (std::runtime_error& e)
        {
            qDebug() << QString(e.what());
            logoutAndExit(1);
        }

        const qint64 fileSizeBytes = QFileInfo(geometryFile).size();
        if (QueryHelpers::uploadToS3(key, geometryFile, mConfig) != 0)
        {
            qDebug() << "Error uploading geometry json file to S3:" << geometryFile;
            logoutAndExit(1);
        }

        const QJsonObject result = createJsonResult(key, fileSizeBytes);
        reply->deleteLater();

        QNetworkRequest putRequest;
        putRequest.setUrl(mQueryUrl);
        putRequest.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
        putRequest.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
        putRequest.setAttribute(QNetworkRequest::User, QVariant("putSpatialInnervationResult"));
        putRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QJsonDocument putDoc(result);
        QString putData(putDoc.toJson());

        connect(&mNetworkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyPutResultFinished(QNetworkReply*)));
        mNetworkManager.put(putRequest, putData.toLocal8Bit());
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
SpatialInnervationQueryHandler::replyPutResultFinished(QNetworkReply* reply)
{
    QNetworkReply::NetworkError error = reply->error();
    const QString requestId = reply->request().attribute(QNetworkRequest::User).toString();
    if (error == QNetworkReply::NoError && !(requestId == "putSpatialInnervationResult"))
    {
        return;
    }
    else if (error == QNetworkReply::NoError)
    {
        qDebug() << "    Completed processing spatial innervation query" << mQueryId;
        reply->deleteLater();
        logoutAndExit(0);
    }
    else
    {
        qDebug() << "[-] Error putting spatial innervation query result (queryId" << mQueryId << "):";
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
SpatialInnervationQueryHandler::logoutAndExit(const int exitCode)
{
    QueryHelpers::logout(mLogoutUrl,
                         mAuthInfo,
                         mNetworkManager);

    if (exitCode == 0)
    {
        emit completedProcessing();
    }
    else
    {
        QCoreApplication::exit(exitCode);
    }
}
