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
#include "RandomGenerator.h"

SelectionQueryHandler::SelectionQueryHandler()
    : QueryHandler()
{
}

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
            region = QString("%1 Barreloid").arg(parts[0]);
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

void
SelectionQueryHandler::doProcessQuery()
{
    IdList selectionA = mSelection.SelectionA();
    IdList selectionB = mSelection.SelectionB();
    IdList selectionC = mSelection.SelectionC();

    QJsonObject cellSelection = mQuery["cellSelection"].toObject();
    bool selectionAEnabled = cellSelection["selectionA"].toObject()["enabled"].toBool();
    bool selectionBEnabled = cellSelection["selectionB"].toObject()["enabled"].toBool();
    bool selectionCEnabled = cellSelection["selectionC"].toObject()["enabled"].toBool();

    QJsonObject result;
    if (selectionAEnabled)
    {
        const QString key = QString("selection_%1_A.json.zip").arg(mQueryId);
        QString geometryFile = createGeometryJSON(key, selectionA, mNetwork, mConfig["WORKER_TMP_DIR"].toString());
        if (uploadToS3(key, geometryFile) != 0)
        {
            abort("Failed uploading to S3 " + geometryFile);
            return;
        }
        const IdsPerCellTypeRegion idsPerCellTypeRegion = Util::sortByCellTypeRegionIDs(selectionA, mNetwork);
        const qint64 fileSizeBytes = QFileInfo(geometryFile).size();
        QJsonObject selectionAData = createJsonResult(idsPerCellTypeRegion, mNetwork, key, fileSizeBytes);
        result.insert("selectionA", selectionAData);
    }
    if (selectionBEnabled)
    {
        const QString key = QString("selection_%1_B.json.zip").arg(mQueryId);
        QString geometryFile = createGeometryJSON(key, selectionB, mNetwork, mConfig["WORKER_TMP_DIR"].toString());
        if (uploadToS3(key, geometryFile) != 0)
        {
            abort("Failed uploading to S3 " + geometryFile);
            return;
        }
        const IdsPerCellTypeRegion idsPerCellTypeRegion = Util::sortByCellTypeRegionIDs(selectionB, mNetwork);
        const qint64 fileSizeBytes = QFileInfo(geometryFile).size();
        QJsonObject selectionBData = createJsonResult(idsPerCellTypeRegion, mNetwork, key, fileSizeBytes);
        result.insert("selectionB", selectionBData);
    }
    if (selectionCEnabled)
    {
        const QString key = QString("selection_%1_C.json.zip").arg(mQueryId);
        QString geometryFile = createGeometryJSON(key, selectionC, mNetwork, mConfig["WORKER_TMP_DIR"].toString());
        if (uploadToS3(key, geometryFile) != 0)
        {
            abort("Failed uploading to S3 " + geometryFile);
            return;
        }
        const IdsPerCellTypeRegion idsPerCellTypeRegion = Util::sortByCellTypeRegionIDs(selectionC, mNetwork);
        const qint64 fileSizeBytes = QFileInfo(geometryFile).size();
        QJsonObject selectionCData = createJsonResult(idsPerCellTypeRegion, mNetwork, key, fileSizeBytes);
        result.insert("selectionC", selectionCData);
    }

    mUpdateCount++;
    mQuery.insert(getResultKey(), result);
    mQuery.insert("status", getCompletedStatus());
    writeResult(mQuery);
};

QString
SelectionQueryHandler::getResultKey()
{
    return "selectionResult";
}