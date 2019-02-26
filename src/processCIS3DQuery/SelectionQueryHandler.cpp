#include "SelectionQueryHandler.h"
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

QJsonObject
createJsonResult(const IdsPerCellTypeRegion& idsPerCellTypeRegion,
                 const NetworkProps& network,
                 const QString& key,
                 const qint64 fileSizeBytes)
{
    QJsonArray entries;
    int total = 0;

    for (IdsPerCellTypeRegion::const_iterator it = idsPerCellTypeRegion.constBegin(); it != idsPerCellTypeRegion.constEnd(); ++it)
    {
        const CellTypeRegion ctr = it.key();
        const int numNeurons = it.value().size();
        QString cellType = network.cellTypes.getName(ctr.first);

        QString region = network.regions.getName(ctr.second);

        if (region.contains("Septum"))
        {
            QStringList parts = region.split("_");
            region = QString("Septum of %1").arg(parts[2]);
        }

        if (region.contains("Surrounding"))
        {
            QStringList parts = region.split("_");
            region = QString("Surrounding of %1").arg(parts[2]);
        }

        if (region.contains("Barreloid"))
        {
            QStringList parts = region.split("_");
            region = QString("Barreloid %1").arg(parts[0]);
        }

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

    QJsonObject result;
    result.insert("tables", tables);
    result.insert("geometryS3key", key);
    result.insert("geometryFileSize", fileSizeBytes);

    return result;
}

QString
createGeometryJSON(const QString& zipFileName,
                   const IdList& neurons,
                   const NetworkProps& network,
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

    QJsonArray positions;
    QJsonArray cellTypeIds;
    QJsonArray regionIds;

    const int vpmTypeId = network.cellTypes.getId("VPM");

    for (int i = 0; i < neurons.size(); ++i)
    {
        const int neuronId = neurons[i];
        const Vec3f somaPos = network.neurons.getSomaPosition(neuronId);
        const int cellTypeId = network.neurons.getCellTypeId(neuronId);
        const int regionId = network.neurons.getRegionId(neuronId);

        if (cellTypeId != vpmTypeId)
        {
            positions.push_back(QJsonValue(somaPos.getX()));
            positions.push_back(QJsonValue(somaPos.getY()));
            positions.push_back(QJsonValue(somaPos.getZ()));
            cellTypeIds.push_back(QJsonValue(cellTypeId));
            regionIds.push_back(QJsonValue(regionId));
        }
    }

    QJsonObject metadata;
    metadata.insert("version", 1);
    metadata.insert("type", "BufferGeometry");
    metadata.insert("generator", "CortexInSilico3D");

    QJsonObject position;
    position.insert("itemSize", 3);
    position.insert("type", "Float32Array");
    position.insert("array", positions);

    QJsonObject cellTypeID;
    cellTypeID.insert("itemSize", 1);
    cellTypeID.insert("type", "Uint8Array");
    cellTypeID.insert("array", cellTypeIds);

    QJsonObject regionID;
    regionID.insert("itemSize", 1);
    regionID.insert("type", "Uint8Array");
    regionID.insert("array", regionIds);

    QJsonObject attributes;
    attributes.insert("position", position);
    attributes.insert("cellTypeID", cellTypeID);
    attributes.insert("regionID", regionID);

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

SelectionQueryHandler::SelectionQueryHandler(QObject* parent)
    : QObject(parent)
{
}

void
SelectionQueryHandler::process(const QString& selectionQueryId,
                               const QJsonObject& config)
{
    qDebug() << "Process selection query" << selectionQueryId;
    mConfig = config;
    mQueryId = selectionQueryId;

    const QString baseUrl = mConfig["METEOR_URL_CIS3D"].toString();
    const QString queryEndPoint = mConfig["METEOR_SELECTIONQUERYFILTER_ENDPOINT"].toString();
    const QString loginEndPoint = mConfig["METEOR_LOGIN_ENDPOINT"].toString();
    const QString logoutEndPoint = mConfig["METEOR_LOGOUT_ENDPOINT"].toString();

    if (baseUrl.isEmpty())
    {
        throw std::runtime_error("SelectionQueryHandler: Cannot find METEOR_URL_CIS3D");
    }
    if (queryEndPoint.isEmpty())
    {
        throw std::runtime_error("SelectionQueryHandler: Cannot find METEOR_SELECTIONQUERYFILTER_ENDPOINT");
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
    request.setAttribute(QNetworkRequest::User, QVariant("getSelectionQueryData"));
    QueryHelpers::setAuthorization(mConfig, request);

    connect(&mNetworkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyGetQueryFinished(QNetworkReply*)));
    mNetworkManager.get(request);
}

void
SelectionQueryHandler::replyGetQueryFinished(QNetworkReply* reply)
{
    QNetworkReply::NetworkError error = reply->error();
    if (error == QNetworkReply::NoError && !reply->request().attribute(QNetworkRequest::User).toString().contains("getSelectionQueryData"))
    {
        return;
    }
    else if (error == QNetworkReply::NoError)
    {
        const QByteArray content = reply->readAll();
        const QJsonDocument jsonResponse = QJsonDocument::fromJson(content);
        const QString status = jsonResponse.object().value("status").toString();

        if (status == "success")
        {
            QJsonDocument jsonResponse = QJsonDocument::fromJson(content);
            QString selectionString = jsonResponse.object().value("data").toString();
            qDebug() << "    Starting computation:" << mQueryId << selectionString;

            const QString datasetShortName = jsonResponse.object().value("network").toString();

            // EXTRACT SLICE PARAMETERS
            const double tissueLow = jsonResponse.object().value("tissueLow").toDouble();
            const double tissueHigh = jsonResponse.object().value("tissueHigh").toDouble();
            const double sliceRef = jsonResponse.object().value("sliceRef").toDouble();
            QString mode = jsonResponse.object().value("tissueMode").toString();
            const bool isSlice = sliceRef != -9999;
            qDebug() << "Slice ref, Tissue depth" << sliceRef << tissueLow << tissueHigh << mode;

            mDataRoot = QueryHelpers::getDatasetPath(datasetShortName, mConfig);
            qDebug() << "    Loading network data:" << datasetShortName << "Path: " << mDataRoot;

            mNetwork.setDataRoot(mDataRoot);
            mNetwork.loadFilesForQuery();

            CIS3D::SynapticSide synapticSide = CIS3D::BOTH_SIDES;
            QString synapticSideString = jsonResponse.object().value("synapticSide").toString();
            qDebug() << "SYNAPTIC SIDE" << synapticSideString;
            if (!isSlice && synapticSideString == "presynaptic")
            {
                synapticSide = CIS3D::PRESYNAPTIC;
            }
            else if (!isSlice && synapticSideString == "postsynaptic")
            {
                synapticSide = CIS3D::POSTSYNAPTIC;
            }
            else if (isSlice || synapticSideString == "both")
            {
                synapticSide = CIS3D::BOTH_SIDES;
            }
            else
            {
                const QString msg = QString("[-] Invalid synaptic side string: %1").arg(synapticSideString);
                throw std::runtime_error(qPrintable(msg));
            }

            QJsonDocument selectionDoc = QJsonDocument::fromJson(selectionString.toLocal8Bit());
            QJsonArray selectionArr = selectionDoc.array();
            SelectionFilter filter = Util::getSelectionFilterFromJson(selectionArr, mNetwork, synapticSide);
            Util::correctVPMSelectionFilter(filter, mNetwork);
            Util::correctInterneuronSelectionFilter(filter, mNetwork);
            IdList neurons = mNetwork.neurons.getFilteredNeuronIds(filter);

            if (isSlice)
            {
                CIS3D::SliceBand band = mode == "twoSided" ? CIS3D::SliceBand::BOTH : CIS3D::SliceBand::FIRST;
                neurons = NeuronSelection::filterTissueDepth(mNetwork, neurons, sliceRef, tissueLow, tissueHigh, band);
            }
            qDebug() << "    Start sorting " << neurons.size() << " neurons.";

            const QString key = QString("neuronselection_%1.json.zip").arg(mQueryId);
            QString geometryFile;
            try
            {
                geometryFile = createGeometryJSON(key, neurons, mNetwork, mConfig["WORKER_TMP_DIR"].toString());
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

            const IdsPerCellTypeRegion idsPerCellTypeRegion = Util::sortByCellTypeRegionIDs(neurons, mNetwork);
            const QJsonObject result = createJsonResult(idsPerCellTypeRegion, mNetwork, key, fileSizeBytes);
            reply->deleteLater();

            QNetworkRequest putRequest;
            putRequest.setUrl(mQueryUrl);
            putRequest.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
            putRequest.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
            putRequest.setAttribute(QNetworkRequest::User, QVariant("putSelectionResult"));
            putRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            QueryHelpers::setAuthorization(mConfig, putRequest);

            QJsonDocument putDoc(result);
            QString putData(putDoc.toJson());

            connect(&mNetworkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyPutResultFinished(QNetworkReply*)));
            mNetworkManager.put(putRequest, putData.toLocal8Bit());
        }
        else
        {
            qDebug() << "[-] Error obtaining SelectionQuery data (invalid status):";
            qDebug() << reply->errorString();
            qDebug() << QString(reply->readAll().replace("\"", ""));

            reply->deleteLater();
            logoutAndExit(1);
        }
    }
    else
    {
        qDebug() << "[-] Error obtaining SelectionQuery data (network error):";
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
SelectionQueryHandler::replyPutResultFinished(QNetworkReply* reply)
{
    QNetworkReply::NetworkError error = reply->error();
    const QString requestId = reply->request().attribute(QNetworkRequest::User).toString();
    if (error == QNetworkReply::NoError && !(requestId == "putSelectionResult"))
    {
        return;
    }
    else if (error == QNetworkReply::NoError)
    {
        qDebug() << "    Completed processing Selection query" << mQueryId;
        reply->deleteLater();
        logoutAndExit(0);
    }
    else
    {
        qDebug() << "[-] Error putting Selection result (queryId" << mQueryId << "):";
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
SelectionQueryHandler::logoutAndExit(const int exitCode)
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
