#include "SpatialInnervationQueryHandler.h"
#include "CIS3DCellTypes.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DNeurons.h"
#include "CIS3DRegions.h"
#include "CIS3DSparseVectorSet.h"
#include "FormulaCalculator.h"
#include "Histogram.h"
#include "NeuronSelection.h"
#include "QueryHelpers.h"
#include "Util.h"
#include "UtilIO.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QTextStream>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <stdexcept>

SpatialInnervationQueryHandler::SpatialInnervationQueryHandler()
    : QueryHandler()
{
}

void
SpatialInnervationQueryHandler::doProcessQuery()
{
    mTempFolder = QDir::cleanPath(mConfig["WORKER_TMP_DIR"].toString() +
                                  QDir::separator() + mQueryId);
    UtilIO::makeDir(mTempFolder);

    IdList preIds = mSelection.SelectionA();
    IdList postIds = mSelection.SelectionB();

    // ###################### LOOP OVER NEURONS ######################

    QString dataFolder = QDir::cleanPath(
        mDataRoot + QDir::separator() +
        Util::getInnervationFolderName(mSelection.getPostTarget(1)));
    QString voxelFolder =
        QDir::cleanPath(mDataRoot + QDir::separator() + "features_meta");

    const QString zipFileName =
        QString("spatialInnervation_%1.json.zip").arg(mQueryId);
    const QString zipFullPath =
        QString("%1/%2").arg(mTempFolder).arg(zipFileName);
    const QString jsonFileName = QString(zipFileName).remove(".zip");
    const QString jsonFullPath = QString(zipFullPath).remove(".zip");

    const QString dataZipFileName =
        QString("spatialInnervation_%1.zip").arg(mQueryId);
    const QString dataZipFullPath =
        QString("%1/%2").arg(mTempFolder).arg(dataZipFileName);
    const QString dataFileName = QString(dataZipFileName).remove(".zip");
    const QString dataFullPath = QString(dataZipFullPath).remove(".zip");

    std::map<int, float> innervationPerVoxel;
    QVector<QString> fileNames;

    mAborted = false;
    for (int i = 0; i < preIds.size(); i++)
    {
        if (mAborted)
        {
            break;
        }

        QString dataFileName =
            QDir(dataFolder).filePath("preNeuronID_" + QString::number(preIds[i]));
        QString tempFileName =
            QDir(mTempFolder).filePath("preNeuronID_" + QString::number(preIds[i]));
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
        outStream << "voxelID "
                  << "postNeuronID "
                  << "overlap\n";
        QString line = inStream.readLine();
        int currentPostId = -1;
        while (!line.isNull())
        {
            QStringList parts = line.split(' ');
            bool isVoxelId;
            int voxelId = parts[0].toInt(&isVoxelId);
            if (isVoxelId)
            {
                if (postIds.contains(currentPostId) &&
                    mSelection.getBandA(preIds[i]) ==
                        mSelection.getBandB(currentPostId))
                {
                    outStream << voxelId << " " << QString::number(currentPostId) << " " << parts[1] << "\n";

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

        const double percent = double(i + 1) * 100.0 / double(preIds.size());
        if (percent < 100)
        {
            QJsonObject intermediateResult = createJsonResult("", 0, "", 0, 0);
            updateQuery(intermediateResult, percent);
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
                QString("Error reading features file. Could not open file %1")
                    .arg(fileName);
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
            // qDebug() << i;
            stream << voxelIds[i] << " " << x[i] << " " << y[i] << " " << z[i]
                   << "\n";
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
            const QString msg =
                QString("Cannot open file for saving json: %1").arg(jsonFullPath);
            throw std::runtime_error(qPrintable(msg));
        }

        QJsonDocument doc(result);
        QTextStream out(&jsonFile);
        out << doc.toJson(QJsonDocument::Compact);
        jsonFile.close();
    }

    qint64 fileSizeBytes1 = 0;
    qint64 fileSizeBytes2 = 0;

    if (!mAborted)
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
        int upload2 =
            QueryHelpers::uploadToS3(dataZipFileName, dataZipFullPath, mConfig);

        if (upload1 != 0)
        {
            qDebug() << "Error uploading geometry json file to S3:" << zipFullPath;
        }
        if (upload2 != 0)
        {
            qDebug() << "Error uploading geometry json file to S3:"
                     << dataZipFullPath;
        }
        if (upload1 != 0 || upload2 != 0)
        {
            abort("Failed uploading files");
        }

        // ###################### CLEAN FILES ######################

        QProcess rm;
        QStringList rmArgs;
        rmArgs << "-rf" << mTempFolder;
        rm.start("rm", rmArgs);
        rm.waitForStarted();
        rm.waitForFinished();

        qDebug() << "[*] Removed original files";

        // ###################### SIGNAL COMPLETION ######################

        int nVoxel = (int)innervationPerVoxel.size();
        QJsonObject result = createJsonResult(
            zipFileName, fileSizeBytes1, dataZipFileName, fileSizeBytes2, nVoxel);
        updateQuery(result, 100);
    }
}

QString
SpatialInnervationQueryHandler::getResultKey()
{
    return "spatialResult";
}

QJsonObject
SpatialInnervationQueryHandler::createJsonResult(
    const QString& keyView, const qint64 fileSizeBytes1, const QString& keyData, const qint64 fileSizeBytes2, int nVoxels)
{
    QJsonObject result;
    result.insert("voxelS3key", keyView);
    result.insert("voxelFileSize", fileSizeBytes1);
    result.insert("voxelCount", nVoxels);
    result.insert("downloadS3key", keyData);
    result.insert("fileSize", fileSizeBytes2);
    return result;
}
