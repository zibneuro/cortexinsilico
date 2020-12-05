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
    : QueryHandler() {}

void SpatialInnervationQueryHandler::doProcessQuery()
{
  qDebug() << "spatial innervation";

  mTempFolder = QDir::cleanPath(mConfig["WORKER_TMP_DIR"].toString() +
                                QDir::separator() + mQueryId);
  UtilIO::makeDir(mTempFolder);

  std::map<int, int> preIds = mSelection.getMultiplicities(mNetwork, "A");
  IdList postIds = mSelection.SelectionB();
  std::set<int> postIdsSet = Util::listToSet(postIds);

  CIS3D::Structure postTarget = mSelection.getPostTarget(1);

  // ###################### LOOP OVER NEURONS ######################

  QString dataFolder = mNetwork.networkRootDir.absoluteFilePath("DSC");
  QString voxelFile = QDir::cleanPath(mDataRoot + QDir::separator() + "grid_vS1.csv");

  const QString zipFileName =
      QString("spatialInnervation_%1.json.zip").arg(mQueryId);
  const QString zipFullPath =
      QString("%1/%2").arg(mTempFolder).arg(zipFileName);
  const QString jsonFileName = QString(zipFileName).remove(".zip");
  const QString jsonFullPath = QString(zipFullPath).remove(".zip");

  const QString dataZipFileName =
      QString("spatialDistribution_%1.zip").arg(mQueryId);
  const QString dataZipFullPath =
      QString("%1/%2").arg(mTempFolder).arg(dataZipFileName);
  const QString dataFileName = QString(dataZipFileName).remove(".zip");
  const QString dataFullPath = QString(dataZipFullPath).remove(".zip");

  std::map<int, float> innervationPerVoxel;
  QVector<QString> fileNames;

  // write multiplicities
  QString multiplicityFileName =
      QDir(mTempFolder).filePath("presynaptic_duplication_factors.csv");
  QFile multFile(multiplicityFileName);
  if (!multFile.open(QIODevice::WriteOnly))
  {
    const QString msg =
        QString("Cannot open file %1 for writing.").arg(multiplicityFileName);
    throw std::runtime_error(qPrintable(msg));
  }
  QTextStream multStream(&multFile);
  multStream << "id,multiplicity\n";
  for (auto it = preIds.begin(); it != preIds.end(); it++)
  {
    multStream << it->first << "," << it->second << "\n";
  }
  multFile.close();
  fileNames.append(multiplicityFileName);

  mAborted = false;
  int i = 0;
  int fileCounter = 0;
  for (auto it = preIds.begin(); it != preIds.end(); it++, i++)
  {
    if (mAborted)
    {
      break;
    }

    bool isExcitatory = mNetwork.cellTypes.isExcitatory(mNetwork.neurons.getCellTypeId(it->first));

    QString dataFileName =
        QDir(dataFolder).filePath(QString::number(it->first) + "_DSC.csv");

    std::vector<DSCEntry> dsc = loadDSC(dataFileName, postIdsSet, postTarget);

    for (auto itDsc = dsc.begin(); itDsc != dsc.end(); itDsc++)
    {
      int subvolume = (*itDsc).subvolumeId;
      float dsc = (*itDsc).dsc;
      if (innervationPerVoxel.find(subvolume) == innervationPerVoxel.end())
      {
        innervationPerVoxel[subvolume] = dsc;
      }
      else
      {
        innervationPerVoxel[subvolume] += dsc;
      }
    }

    if (isExcitatory)
    {
      QString tempFileName =
          QDir(mTempFolder).filePath(QString::number(it->first) + "_DSC.csv");
      fileNames.append(tempFileName);
      saveDSC(tempFileName, dsc, postIdsSet);

      //qDebug() << "[*] Updating zip file:" << dataFullPath;
      QProcess zip2;
      zip2.setWorkingDirectory(mTempFolder);

      QStringList arguments2;
      QStringList rmArgs;
      if (fileCounter == 0)
      {
        arguments2.append("-j");
      }
      else
      {
        arguments2.append("-ju");
      }
      arguments2.append(dataZipFullPath);
      arguments2.append(fileNames[fileCounter + 1]);

      zip2.start("zip", arguments2);

      if (!zip2.waitForStarted())
      {
        throw std::runtime_error("Error starting zip process");
      }

      if (!zip2.waitForFinished())
      {
        throw std::runtime_error("Error completing zip process");
      }
      fileCounter++;

      //qDebug() << "[*] Completed zipping";
    }

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
    QString readmeFileName = QDir(mTempFolder).filePath("README.txt");
    writeReadme(readmeFileName);

    // ###################### PROCESS VOXELS ######################

    std::set<int> voxelIds;
    QJsonArray viewerJson;
    for (auto it = innervationPerVoxel.begin(); it != innervationPerVoxel.end(); it++)
    {
      voxelIds.insert(it->first);
      QJsonArray entryJson;
      entryJson.append(it->first);
      entryJson.append(it->second);
      viewerJson.append(entryJson);
    }

    QString voxelFileName = QDir(mTempFolder).filePath("grid.csv");
    mNetwork.grid.save(voxelFileName, voxelIds);

    qDebug() << "[*] Add voxel positions to zip file:" << dataFullPath;
    QProcess zip3;
    zip3.setWorkingDirectory(mTempFolder);

    QStringList arguments3;

    arguments3.append("-ju");

    arguments3.append(dataZipFullPath);
    arguments3.append(voxelFileName);
    arguments3.append(multiplicityFileName);
    arguments3.append(readmeFileName);

    zip3.start("zip", arguments3);    

    if (!zip3.waitForStarted())
    {
      throw std::runtime_error("Error starting zip process");
    }

    if (!zip3.waitForFinished())
    {
      throw std::runtime_error("Error completing zip process");
    }

    QFile jsonFile(jsonFullPath);
    if (!jsonFile.open(QIODevice::WriteOnly))
    {
      const QString msg =
          QString("Cannot open file for saving json: %1").arg(jsonFullPath);
      throw std::runtime_error(qPrintable(msg));
    }

    QJsonDocument doc(viewerJson);
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

    fileSizeBytes1 = QFileInfo(dataZipFullPath).size();
    int upload1 = QueryHelpers::uploadToS3(dataZipFileName, dataZipFullPath, mConfig);
    fileSizeBytes2 = QFileInfo(jsonFullPath).size();
    int upload2 =
        QueryHelpers::uploadToS3(jsonFileName, jsonFullPath, mConfig);

    if (upload1 != 0)
    {
      qDebug() << "Error uploading geometry json file to S3:" << dataZipFullPath;
    }
    if (upload2 != 0)
    {
      qDebug() << "Error uploading viewer json file to S3:"
               << jsonFullPath;
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
        jsonFileName, fileSizeBytes2, dataZipFileName, fileSizeBytes1, nVoxel);
    updateQuery(result, 100);
  }
}

QString SpatialInnervationQueryHandler::getResultKey()
{
  return "spatialResult";
}

QJsonObject SpatialInnervationQueryHandler::createJsonResult(
    const QString &keyView, const qint64 fileSizeBytes1, const QString &keyData,
    const qint64 fileSizeBytes2, int nVoxels)
{
  QJsonObject result;
  result.insert("voxelS3key", keyView);
  result.insert("voxelFileSize", fileSizeBytes1);
  result.insert("voxelCount", nVoxels);
  result.insert("downloadS3key", keyData);
  result.insert("fileSize", fileSizeBytes2);
  return result;
}

std::vector<SpatialInnervationQueryHandler::DSCEntry> SpatialInnervationQueryHandler::loadDSC(QString filename, std::set<int> &postIds, CIS3D::Structure postTarget)
{
  QFile dataFile(filename);
  if (!dataFile.open(QIODevice::ReadOnly))
  {
    const QString msg =
        QString("Cannot open file %1 for writing.").arg(filename);
    throw std::runtime_error(qPrintable(msg));
  }
  QTextStream inStream(&dataFile);
  std::vector<DSCEntry> data;
  QString line = inStream.readLine();
  line = inStream.readLine();
  while (!line.isNull())
  {
    QStringList parts = line.split(',');
    int postId = parts[0].toInt();
    int subvolumeId = parts[1].toInt();
    float dscSoma = parts[2].toFloat();
    float dscApical = parts[3].toFloat();
    float dscBasal = parts[4].toFloat();
    float innervation = dscSoma + dscApical + dscBasal;
    if (postTarget == CIS3D::BASAL)
    {
      innervation = dscBasal;
    }
    else if (postTarget == CIS3D::APICAL)
    {
      innervation = dscApical;
    }
    DSCEntry entry;
    entry.postId = postId;
    entry.subvolumeId = subvolumeId;
    entry.dsc = innervation;
    data.push_back(entry);

    line = inStream.readLine();
  }

  return data;
}

void SpatialInnervationQueryHandler::saveDSC(QString filename, std::vector<SpatialInnervationQueryHandler::DSCEntry> &data, std::set<int> &postIds)
{
  QFile dataFile(filename);
  if (!dataFile.open(QIODevice::WriteOnly))
  {
    const QString msg =
        QString("Cannot open file %1 for writing.").arg(filename);
    throw std::runtime_error(qPrintable(msg));
  }
  QTextStream outStream(&dataFile);
  outStream.setRealNumberNotation(QTextStream::FixedNotation);
  outStream.setRealNumberPrecision(6);
  outStream << "post_id,subvolume_id,DSC\n";
  for (unsigned i = 0; i < data.size(); i++)
  {
    if(postIds.find(data[i].postId) != postIds.end())
    outStream << data[i].postId << "," << data[i].subvolumeId << "," << data[i].dsc << "\n";
  }
  dataFile.close();
}

void SpatialInnervationQueryHandler::writeReadme(QString filename){
  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly))
  {
    const QString msg =
        QString("Cannot open file %1 for writing.").arg(filename);
    throw std::runtime_error(qPrintable(msg));
  }
  QTextStream outStream(&file);
  outStream << "For each selected presynaptic neuron the DSC to all selected postsynaptic neurons\n";
  outStream << "is listed in the file NID_DSC.csv, where NID is the ID of the presynaptic neuron.\n";
  outStream << "The DSC values are calculated separately for each subvolume. The grid cells defining\n";
  outStream << "these subvolumes are listed in the file grid.csv\n";
  outStream << "\n";
  outStream << "Axon and dendrite morphologies are duplicated to match the number of cells in the\n";
  outStream << "model. In contrast to dendrite morphologies, duplicated axon morphologies are not\n";
  outStream << "displaced to the soma location of the assigned cell, allowing for a more compact\n";
  outStream << "representation in the model, where only the duplication factors of the respective\n";
  outStream << "axon morphologies need to be recorded (presynaptic_duplication_factors.csv).\n";
  outStream << "\n";
  outStream << "Note that due to space considerations, the NID_DSC.csv files of presynaptic inhibitory\n";
  outStream << "neurons are omitted from this downloadable zip archive. However, all data including\n";
  outStream << "inhibitory neurons can be obtained in the download area of the 'Digital Barrel Cortex\n";
  outStream << "Anatomy' section.\n";
  file.close();

}

void SpatialInnervationQueryHandler::setFormulas() {
  
}