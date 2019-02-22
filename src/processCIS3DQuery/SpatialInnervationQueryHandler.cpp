#include "SpatialInnervationQueryHandler.h"
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

QJsonObject
SpatialInnervationQueryHandler::createJsonResult(
    const QString& keyView,
    const qint64 fileSizeBytes1,
    const QString& keyData,
    const qint64 fileSizeBytes2,
    int nVoxels)
{
    QJsonObject result;
    result.insert("voxelS3key", keyView);
    result.insert("voxelFileSize", fileSizeBytes1);
    result.insert("voxelCount", nVoxels);
    result.insert("dataS3key", keyData);
    result.insert("dataFileSize", fileSizeBytes2);
    return result;
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

    mTempFolder = QDir::cleanPath(mConfig["WORKER_TMP_DIR"].toString() + QDir::separator() + mQueryId);
    UtilIO::makeDir(mTempFolder);

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
                                    mNetworkManager,
                                    mConfig);

    QNetworkRequest request;
    request.setUrl(mQueryUrl);
    request.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
    request.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
    request.setAttribute(QNetworkRequest::User, QVariant("getSpatialInnervationQueryData"));
    QueryHelpers::setAuthorization(mConfig, request);

    connect(&mNetworkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyGetQueryFinished(QNetworkReply*)));
    mNetworkManager.get(request);
}

void
SpatialInnervationQueryHandler::replyGetQueryFinished(QNetworkReply* reply)
{
    QNetworkReply::NetworkError error = reply->error();
    if (error == QNetworkReply::NoError && !reply->request().attribute(QNetworkRequest::User).toString().contains("getSpatialInnervationQueryData"))
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

        const QString datasetShortName = jsonData["network"].toString();
        mDataRoot = QueryHelpers::getDatasetPath(datasetShortName, mConfig);
        qDebug() << "    Loading network data:" << datasetShortName << "Path: " << mDataRoot;
        mNetwork.setDataRoot(mDataRoot);
        mNetwork.loadFilesForSynapseComputation();

        QString dataFolder = QDir::cleanPath(mDataRoot + QDir::separator() + "spatialInnervation" + QDir::separator() + "innervation");
        QString voxelFolder = QDir::cleanPath(mDataRoot + QDir::separator() + "spatialInnervation" + QDir::separator() + "features_meta");

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

        selection.filterUniquePre(mNetwork);
        selection.setBBox(bbmin, bbmax);
        if (isSlice)
        {
            selection.filterInnervationSlice(mNetwork, sliceRef, tissueLowPre, tissueHighPre, tissueModePre, tissueLowPost, tissueHighPost, tissueModePost);
        }
        IdList preIds = selection.Presynaptic();
        IdList postIds = selection.Postsynaptic();

        qDebug() << "[*] SPATIAL QUERY " << preIds.size() << " presynaptic and " << postIds.size() << " postsynaptic neurons.";

        const QString zipFileName = QString("spatialInnervation_%1.json.zip").arg(mQueryId);
        const QString zipFullPath = QString("%1/%2").arg(mTempFolder).arg(zipFileName);
        const QString jsonFileName = QString(zipFileName).remove(".zip");
        const QString jsonFullPath = QString(zipFullPath).remove(".zip");

        const QString dataZipFileName = QString("spatialInnervation_%1.zip").arg(mQueryId);
        const QString dataZipFullPath = QString("%1/%2").arg(mTempFolder).arg(dataZipFileName);
        const QString dataFileName = QString(dataZipFileName).remove(".zip");
        const QString dataFullPath = QString(dataZipFullPath).remove(".zip");

        // ###################### LOOP OVER NEURONS ######################

        std::map<int, float> innervationPerVoxel;
        QVector<QString> fileNames;

        mAborted = false;
        for (int i = 0; i < preIds.size(); i++)
        {
            if (mAborted)
            {
                break;
            }

            QString dataFileName = QDir(dataFolder).filePath("preNeuronID_" + QString::number(preIds[i]));
            QString tempFileName = QDir(mTempFolder).filePath("preNeuronID_" + QString::number(preIds[i]));
            fileNames.append(tempFileName);

            QFile dataFile(dataFileName);
            if (!dataFile.open(QIODevice::ReadOnly))
            {
                const QString msg =
                    QString("Cannot open file %1 for writing.").arg(dataFileName);
                throw std::runtime_error(qPrintable(msg));
            }
            QTextStream inStream(&dataFile);

            QFile tempFile(tempFileName);
            if (!tempFile.open(QIODevice::WriteOnly))
            {
                const QString msg =
                    QString("Cannot open file %1 for writing.").arg(tempFileName);
                throw std::runtime_error(qPrintable(msg));
            }
            QTextStream outStream(&tempFile);

            QString line = inStream.readLine();
            int currentPostId = -1;
            while (!line.isNull())
            {
                QStringList parts = line.split(' ');
                bool isVoxelId;
                int voxelId = parts[0].toInt(&isVoxelId);
                if (isVoxelId)
                {
                    if (postIds.contains(currentPostId) && selection.getPresynapticBand(preIds[i]) == selection.getPostsynapticBand(currentPostId))
                    {
                        outStream << line << "\n";

                        float innervation = parts[1].toFloat();
                        auto it = innervationPerVoxel.find(voxelId);
                        if (it == innervationPerVoxel.end())
                        {
                            innervationPerVoxel[voxelId] = innervation;
                        }
                        else
                        {
                            innervationPerVoxel[voxelId] += innervation;
                        }
                    }
                }
                else
                {
                    currentPostId = parts[1].toInt();
                }

                line = inStream.readLine();
            }

            tempFile.close();

            qDebug() << "[*] Updating zip file:" << dataFullPath;
            QProcess zip2;
            zip2.setWorkingDirectory(mTempFolder);

            QStringList arguments2;
            QStringList rmArgs;
            if (i == 0)
            {
                arguments2.append("-j");
            }
            else
            {
                arguments2.append("-ju");
            }
            arguments2.append(dataZipFullPath);
            arguments2.append(fileNames[i]);

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

            // ###################### REPORT UPDATE ######################

            QJsonObject progress;
            progress.insert("completed", i + 1);
            progress.insert("total", preIds.size());
            const double percent = double(i + 1) * 100.0 / double(preIds.size());
            progress.insert("percent", percent);
            QJsonObject jobStatus;
            jobStatus.insert("status", "running");
            jobStatus.insert("progress", progress);

            QJsonObject payload;
            payload.insert("jobStatus", jobStatus);

            QNetworkRequest putRequest;
            putRequest.setUrl(mQueryUrl);
            putRequest.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
            putRequest.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
            putRequest.setAttribute(QNetworkRequest::User, QVariant("putSpatialInnervationResult"));
            putRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            QueryHelpers::setAuthorization(mConfig, putRequest);

            QJsonDocument putDoc(payload);
            QString putData(putDoc.toJson());

            qDebug() << "[*] Posting intermediate result:" << percent << "\%    (" << i + 1 << "/" << preIds.size() << ")";

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

            QString fileName = QDir(voxelFolder).filePath("voxel_pos.dat");
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

        qint64 fileSizeBytes1 = 0;
        qint64 fileSizeBytes2 = 0;

        if (mAborted)
        {
            logoutAndExit(1);
        }
        else
        {
            // ###################### ZIP FILES ######################

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

            // ###################### UPLOAD FILES ######################

            fileSizeBytes1 = QFileInfo(zipFullPath).size();
            int upload1 = QueryHelpers::uploadToS3(zipFileName, zipFullPath, mConfig);
            fileSizeBytes2 = QFileInfo(dataZipFullPath).size();
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
        }

        // ###################### CLEAN FILES ######################

        QProcess rm;
        QStringList rmArgs;
        rmArgs << "-rf" << mTempFolder;
        rm.start("rm", rmArgs);
        rm.waitForStarted();
        rm.waitForFinished();

        qDebug() << "[*] Removed original files";

        if (mAborted)
        {
            logoutAndExit(1);
        }
        else
        {
            // ###################### SIGNAL COMPLETION ######################

            int nVoxel = (int)innervationPerVoxel.size();
            const QJsonObject result = createJsonResult(zipFileName, fileSizeBytes1, dataZipFileName, fileSizeBytes2, nVoxel);

            QJsonObject progress;
            progress.insert("completed", preIds.size());
            progress.insert("total", preIds.size());
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
            putRequest.setAttribute(QNetworkRequest::User, QVariant("putSpatialInnervationResult"));
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
