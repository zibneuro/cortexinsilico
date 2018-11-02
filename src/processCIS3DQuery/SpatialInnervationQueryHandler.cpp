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
#include "NeuronSelection.h"

QJsonObject
SpatialInnervationQueryHandler::createJsonResult(
    const QString& keyView,
    const qint64 fileSizeBytes1,
    const QString& keyData,
    const qint64 fileSizeBytes2)
{
    QJsonObject result;
    result.insert("voxelS3key", keyView);
    result.insert("voxelFileSize", fileSizeBytes1);
    result.insert("voxelCount", (int)mStatistics.getNumberOfSamples());
    result.insert("dataS3key", keyData);
    result.insert("dataFileSize", fileSizeBytes2);
    return result;
}

QVector<QString>
SpatialInnervationQueryHandler::createGeometryJSON(const QString& zipFileName,
                                                   const QString& dataZipFileName,
                                                   FeatureProvider& featureProvider,
                                                   const QString& tmpDir)
{
    const QString zipFullPath = QString("%1/%2").arg(tmpDir).arg(zipFileName);    
    const QString jsonFileName = QString(zipFileName).remove(".zip");
    const QString jsonFullPath = QString(zipFullPath).remove(".zip");

    const QString dataZipFullPath = QString("%1/%2").arg(tmpDir).arg(dataZipFileName);
    const QString dataFileName = QString(dataZipFileName).remove(".zip");
    const QString dataFullPath = QString(dataZipFullPath).remove(".zip");

    QVector<QString> resultFiles;
    resultFiles.append(zipFullPath);
    resultFiles.append(dataZipFullPath);


    QFile jsonFile(jsonFullPath);
    if (!jsonFile.open(QIODevice::WriteOnly))
    {
        const QString msg = QString("Cannot open file for saving json: %1").arg(jsonFullPath);
        throw std::runtime_error(qPrintable(msg));
    }

    // ###################### LOAD NONEMPTY VOXELS ######################

    std::vector<int> voxelIds;
    std::vector<float> x;
    std::vector<float> y;
    std::vector<float> z;

    featureProvider.loadVoxelPositions(voxelIds, x, y, z);

    qDebug() << "Size" << x.size();

    QJsonArray positions;
    QJsonArray voxelIdsJson;
    QJsonArray colors;

    for (auto it = voxelIds.begin(); it != voxelIds.end(); ++it)
    {
        voxelIdsJson.push_back(QJsonValue(*it));
    }

    // ###################### LOAD FEATURES ######################

    std::map<int, std::map<int, float> > neuron_pre;
    std::map<int, std::map<int, float> > neuron_postExc;

    featureProvider.loadCIS3D(neuron_pre, neuron_postExc);

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

    Statistics connProbSynapse;
    Statistics connProbInnervation;

    qDebug() << "loop" << preIndices.size() << postIndices.size();

    // ###################### LOOP OVER NEURONS ######################

    std::set<QString> fileNames;
    std::map<int, float> innervationSum;
    for (auto it = voxelIds.begin(); it != voxelIds.end(); ++it)
    {
        innervationSum[*it] = 0;
    }

    for (unsigned int i = 0; i < preIndices.size(); i++)
    {
        std::map<int, std::map<int, float> > innervationPerPre;
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
                        innervationSum[pre->first] += arg;
                    }
                }
                innervationPerPre[postId] = innervationPerVoxel;
            }
        }
        QString fileName = QDir(tmpDir).filePath("preNeuronID_" + QString::number(preId));
        fileNames.insert(fileName);
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly))
        {
            const QString msg =
                QString("Cannot open file %1 for writing.").arg(fileName);
            throw std::runtime_error(qPrintable(msg));
        }
        QTextStream stream(&file);

        for (auto itPost = innervationPerPre.begin(); itPost != innervationPerPre.end(); ++itPost)
        {
            if (itPost->second.begin() != itPost->second.end())
            {
                stream << "postNeuronID"
                       << " " << itPost->first << "\n";
            }
            for (auto itVoxel = itPost->second.begin(); itVoxel != itPost->second.end(); ++itVoxel)
            {
                stream << itVoxel->first << " " << itVoxel->second << "\n";
            }
        }
    }

    // ###################### DETERMINE STATISTICS ######################

    for (auto it = innervationSum.begin(); it != innervationSum.end(); it++)
    {
        mStatistics.addSample(it->second);
    }

    for (unsigned int i = 0; i < x.size(); ++i)
    {
        positions.push_back(QJsonValue(x[i]));
        positions.push_back(QJsonValue(y[i]));
        positions.push_back(QJsonValue(z[i]));
        int voxelId = voxelIds[i];
        double innervation = (double)innervationSum[voxelId];
        std::vector<double> rgb = Util::getHeatMap(innervation, mStatistics.getMinimum(), mStatistics.getMaximum());

        colors.push_back(QJsonValue(rgb[0]));
        colors.push_back(QJsonValue(rgb[1]));
        colors.push_back(QJsonValue(rgb[2]));
    }

    // ###################### WRITE OUTPUT ######################

    QString voxelFileName = QDir(tmpDir).filePath("voxelPositions");
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
        stream << voxelIds[i] << " " << x[i] << " " << y[i] << " " << z[i] << "\n";
    }
    fileNames.insert(voxelFileName);

    QJsonObject metadata;
    metadata.insert("version", 1);
    metadata.insert("type", "BufferGeometry");
    metadata.insert("generator", "CortexInSilico3D");

    QJsonObject voxelIdsField;
    voxelIdsField.insert("array", voxelIdsJson);

    QJsonObject position;
    position.insert("itemSize", 3);
    position.insert("type", "Float32Array");
    position.insert("array", positions);

    QJsonObject color;
    color.insert("itemSize", 3);
    color.insert("type", "Float32Array");
    color.insert("array", colors);

    QJsonObject attributes;
    attributes.insert("position", position);
    attributes.insert("color", color);

    QJsonObject data;
    data.insert("attributes", attributes);

    QJsonObject result;
    result.insert("metadata", metadata);
    result.insert("data", data);

    QJsonDocument doc(result);
    QTextStream out(&jsonFile);
    out << doc.toJson(QJsonDocument::Compact);
    jsonFile.close();

    // ###################### ZIP FILES ######################

    qDebug() << "[*] Zipping view file:" << jsonFullPath;
    QProcess zip;
    zip.setWorkingDirectory(tmpDir);

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

    qDebug() << "[*] Zipping data file:" << dataFullPath;
    QProcess zip2;
    zip2.setWorkingDirectory(tmpDir);

    QStringList arguments2;
    arguments2.append("-j");
    arguments2.append(dataFileName); 
    
    for (auto itFile = fileNames.begin(); itFile != fileNames.end(); ++itFile)
    {
        arguments2.append(*itFile);
    }
   

    zip2.start("zip", arguments2);

    if (!zip2.waitForStarted())
    {
        throw std::runtime_error("Error starting zip process");
    }

    if (!zip2.waitForFinished())
    {
        throw std::runtime_error("Error completing zip process");
    }

    qDebug() << "[*] Completed zipping";

    return resultFiles;
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

        selection.sampleDown(40000, -1);
        selection.setBBox(bbmin, bbmax);

        QString metaFolder = QDir::cleanPath(mConfig["WORKER_TMP_DIR"].toString() + QDir::separator() + mQueryId);
        QString preFolder = metaFolder.append("_pre");
        QString postFolder = metaFolder.append("_post");
        metaFolder.append("_meta");
        FeatureProvider featureProvider(metaFolder, preFolder, postFolder, false, true);
        featureProvider.preprocessFeatures(mNetwork, selection, 0.00000000000000001, false, true);

        const QString keyView = QString("spatialInnervation_%1.json.zip").arg(mQueryId);
        const QString keyData = QString("spatialInnervation_%1.zip").arg(mQueryId);
        QVector<QString> filePaths;
        try
        {
            filePaths = createGeometryJSON(keyView, keyData, featureProvider, mConfig["WORKER_TMP_DIR"].toString());
        }
        catch (std::runtime_error& e)
        {
            qDebug() << QString(e.what());
            logoutAndExit(1);
        }

        const qint64 fileSizeBytes1 = QFileInfo(filePaths[0]).size();
        int upload1 = QueryHelpers::uploadToS3(keyView, filePaths[0], mConfig);
        const qint64 fileSizeBytes2 = QFileInfo(filePaths[1]).size();
        int upload2 = QueryHelpers::uploadToS3(keyData, filePaths[1], mConfig);

        if (upload1 != 0)
        {
            qDebug() << "Error uploading geometry json file to S3:" << filePaths[0];
        }
        if (upload2 != 0)
        {
            qDebug() << "Error uploading geometry json file to S3:" << filePaths[1];
        }
        if (upload1 != 0 || upload2 != 0)
        {
            logoutAndExit(1);
        }

        const QJsonObject result = createJsonResult(keyView, fileSizeBytes1, keyData, fileSizeBytes2);
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
