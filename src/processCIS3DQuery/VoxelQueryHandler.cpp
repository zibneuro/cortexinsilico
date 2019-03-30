#include "VoxelQueryHandler.h"
#include "CIS3DCellTypes.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DNeurons.h"
#include "CIS3DRegions.h"
#include "CIS3DSparseVectorSet.h"
#include "FeatureProvider.h"
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
#include <fstream>
#include <stdexcept>

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
VoxelQueryHandler::createJsonResult(bool createFile)
{
    Statistics preCellsPerVoxel;
    Histogram preCellsPerVoxelH(50);
    createStatistics(mMapPreCellsPerVoxel, preCellsPerVoxel, preCellsPerVoxelH);

    Statistics preBranchesPerVoxel;
    Histogram preBranchesPerVoxelH(50);
    createStatistics(mMapPreBranchesPerVoxel, preBranchesPerVoxel, preBranchesPerVoxelH);

    Statistics boutonsPerVoxel;
    Histogram boutonsPerVoxelH(500);
    createStatistics(mMapBoutonsPerVoxel, boutonsPerVoxel, boutonsPerVoxelH);

    Statistics postCellsPerVoxel;
    Histogram postCellsPerVoxelH(50);
    createStatistics(mMapPostCellsPerVoxel, postCellsPerVoxel, postCellsPerVoxelH);

    Statistics postBranchesPerVoxel;
    Histogram postBranchesPerVoxelH(50);
    createStatistics(mMapPostBranchesPerVoxel, postBranchesPerVoxel, postBranchesPerVoxelH);

    Statistics postsynapticSitesPerVoxel;
    Histogram postsynapticSitesPerVoxelH(500);
    createStatistics(mMapPostsynapticSitesPerVoxel, postsynapticSitesPerVoxel, postsynapticSitesPerVoxelH);

    Statistics synapsesPerVoxel;
    Histogram synapsesPerVoxelH(100);
    createStatistics(mSynapsesPerVoxel, synapsesPerVoxel, synapsesPerVoxelH);
    // qDebug() << synapsesPerVoxel.getMaximum() << synapsesPerVoxel.getMean() <<
    // synapsesPerVoxelH.getNumberOfBins();

    // Synapses per connection
    QJsonArray synapsesPerConnectionMin;
    QJsonArray synapsesPerConnectionMed;
    QJsonArray synapsesPerConnectionMax;
    for (auto it = mSynapsesPerConnectionOccurrences.begin();
         it != mSynapsesPerConnectionOccurrences.end();
         it++)
    {
        // qDebug() << it->first << it->second;
        float min, med, max;
        Util::getMinMedMax(it->second, min, med, max);
        synapsesPerConnectionMin.push_back(QJsonValue(min));
        synapsesPerConnectionMed.push_back(QJsonValue(med));
        synapsesPerConnectionMax.push_back(QJsonValue(max));
    }

    QJsonObject synapsesPerConnectionPlot;
    synapsesPerConnectionPlot.insert("min", synapsesPerConnectionMin);
    synapsesPerConnectionPlot.insert("med", synapsesPerConnectionMed);
    synapsesPerConnectionPlot.insert("max", synapsesPerConnectionMax);

    // qDebug() << synapsesPerConnectionPlot;

    QJsonObject result;
    result.insert("numberSelectedVoxels", (int)mSelectedVoxels.size());
    result.insert("numberPreInnervatedVoxels", (int)mPreInnervatedVoxels.size());
    result.insert("numberPostInnervatedVoxels",
                  (int)mPostInnervatedVoxels.size());
    result.insert("presynapticCellsPerVoxel",
                  Util::createJsonStatistic(preCellsPerVoxel));
    result.insert("presynapticCellsPerVoxelHisto",
                  Util::createJsonHistogram(preCellsPerVoxelH));
    result.insert("presynapticBranchesPerVoxel",
                  Util::createJsonStatistic(preBranchesPerVoxel));
    result.insert("presynapticBranchesPerVoxelHisto",
                  Util::createJsonHistogram(preBranchesPerVoxelH));
    result.insert("postsynapticCellsPerVoxel",
                  Util::createJsonStatistic(postCellsPerVoxel));
    result.insert("postsynapticCellsPerVoxelHisto",
                  Util::createJsonHistogram(postCellsPerVoxelH));
    result.insert("postsynapticBranchesPerVoxel",
                  Util::createJsonStatistic(postBranchesPerVoxel));
    result.insert("postsynapticBranchesPerVoxelHisto",
                  Util::createJsonHistogram(postBranchesPerVoxelH));
    result.insert("boutonsPerVoxel", Util::createJsonStatistic(boutonsPerVoxel));
    result.insert("boutonsPerVoxelHisto",
                  Util::createJsonHistogram(boutonsPerVoxelH));
    result.insert("postsynapticSitesPerVoxel",
                  Util::createJsonStatistic(postsynapticSitesPerVoxel));
    result.insert("postsynapticSitesPerVoxelHisto",
                  Util::createJsonHistogram(postsynapticSitesPerVoxelH));
    result.insert("synapsesPerVoxel",
                  Util::createJsonStatistic(synapsesPerVoxel));
    result.insert("synapsesPerVoxelHisto",
                  Util::createJsonHistogram(synapsesPerVoxelH));
    // result.insert("synapsesPerConnection",
    // Util::createJsonStatistic(mSynapsesPerConnection));
    result.insert("synapsesPerConnectionPlot", synapsesPerConnectionPlot);

    // ################# CREATE CSV FILE #################

    if (createFile)
    {
        const QString key = QString("voxel_%1.csv").arg(mQueryId);
        const QString tmpDir = mConfig["WORKER_TMP_DIR"].toString();
        QString filename = QString("%1/%2").arg(tmpDir).arg(key);
        QFile csv(filename);
        if (!csv.open(QIODevice::WriteOnly))
        {
            QString msg =
                QString("Cannot open file for saving csv: %1").arg(filename);
            throw std::runtime_error(qPrintable(msg));
        }
        const QChar sep(',');

        QTextStream out(&csv);
        out << mFilterString;
        out << "\n";
        out << "\n";

        out << "Voxels meeting spatial filter condition:" << sep
            << (int)mSelectedVoxels.size() << "\n";
        out << "Voxels with presynaptic cells:" << sep
            << (int)mPreInnervatedVoxels.size() << "\n";
        out << "Voxels with postsynaptic cells:" << sep
            << (int)mPostInnervatedVoxels.size() << "\n";
        out << "\n";

        preCellsPerVoxel.write(out, "Presynaptic cells per voxel");
        postCellsPerVoxel.write(out, "Postsynaptic cells per voxel");
        preBranchesPerVoxel.write(out, "Axon branches per voxel");
        postBranchesPerVoxel.write(out, "Dendrite branches per voxel");
        synapsesPerVoxel.write(out, "Synapses per voxel");

        out << "\n";
        preCellsPerVoxelH.write(out, "Presynaptic cells per voxel");
        postCellsPerVoxelH.write(out, "Postsynaptic cells per voxel");        
        preBranchesPerVoxelH.write(out, "Axon branches per voxel");
        postBranchesPerVoxelH.write(out, "Dendrite branches per voxel");
 
        // boutonsPerVoxelH.write(out, "Boutons per voxel");
        // postsynapticSitesPerVoxelH.write(out, "Postsynaptic sites per voxel");
        synapsesPerVoxelH.write(out, "Synapses per voxel");
        out << "\n";

        out << "Number of synapses (k) per neuron pair:\n";
        out << "k,min,med,max\n";
        for (int i = 0; i < synapsesPerConnectionMin.size(); i++)
        {
            out << i + 1 << sep << synapsesPerConnectionMin[i].toDouble() << sep
                << synapsesPerConnectionMed[i].toDouble() << sep
                << synapsesPerConnectionMax[i].toDouble() << "\n";
        }

        out.flush();
        csv.close();

        const qint64 fileSizeBytes = QFileInfo(filename).size();
        qDebug() << filename << fileSizeBytes;
        if (QueryHelpers::uploadToS3(key, filename, mConfig) != 0)
        {
            qDebug() << "Error uploading csv file to S3:" << filename;
        }

        result.insert("downloadS3key", key);
        result.insert("fileSize", fileSizeBytes);
        qDebug() << fileSizeBytes;
    }
    else
    {
        result.insert("downloadS3key", "");
        result.insert("fileSize", 0);
    }

    return result;
}

VoxelQueryHandler::VoxelQueryHandler()
    : QueryHandler()
{
}

void
VoxelQueryHandler::doProcessQuery()
{
    QJsonObject voxelSelection = mQuery["voxelSelection"].toObject();
    QJsonObject voxelFilter = voxelSelection["voxelFilter"].toObject();
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

    qDebug() << "[*] Voxel query: " << mode << voxelOrigin[0] << voxelOrigin[1]
             << voxelOrigin[2] << voxelDimensions[0] << voxelDimensions[1]
             << voxelDimensions[2];

    QString advancedSettings =
        Util::getAdvancedSettingsString(mQuery, true, false);

    // ################# DETERMINE NEURON IDS #################

    CIS3D::Structure postTarget = CIS3D::DEND;

    if (mode == "prePost" || mode == "prePostVoxel")
    {
        mSelection.setSelectionFromQuery(mQuery, mNetwork, mConfig);
    }
    else
    {
        mNetwork.setDataRoot(mDataRoot);
        mNetwork.loadFilesForQuery();
        mSelection.setFullModel(mNetwork, false);
        // setFilterString(mode, "", "", advancedSettings, voxelOrigin,
        //                voxelDimensions);
        int samplingFactor = -1;
        int randomSeed = -1;
        if (Util::isSampled(mNetworkSelection, mNetworkNumber, samplingFactor, randomSeed))
        {
            mSelection.sampleDownFactor(samplingFactor, randomSeed);
        }
    }

    QString postAllFolder =
        QDir::cleanPath(mDataRoot + QDir::separator() + "features_postAll");
    QString metaFolder =
        QDir::cleanPath(mDataRoot + QDir::separator() + "features_meta");
    QString voxelPosFile =
        QDir::cleanPath(metaFolder + QDir::separator() + "voxel_pos.dat");
    QString indexFile = QDir::cleanPath(metaFolder + QDir::separator() +
                                        Util::getIndexFileName(postTarget));
    QString indexFileBranch = QDir::cleanPath(metaFolder + QDir::separator() +
                                              Util::getBranchIndexFileName(postTarget));

    IdList preIds = mSelection.SelectionA();
    IdList postIds = mSelection.SelectionB();

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
            double voxelX = parts[1].toDouble() - 25;
            double voxelY = parts[2].toDouble() - 25;
            double voxelZ = parts[3].toDouble() - 25;

            if (voxelX >= minX && voxelX < maxX)
            {
                if (voxelY >= minY && voxelY < maxY)
                {
                    if (voxelZ >= minZ && voxelZ < maxZ)
                    {
                        mSelectedVoxels.insert(voxelId);
                        filteredVoxels.insert(voxelId);
                        // qDebug() << "[!] voxel ID" << voxelId;
                    }
                }
            }
        }
    }
    else
    {
        const QString msg =
            QString("Error reading index file. Could not open file %1")
                .arg(voxelPosFile);
        throw std::runtime_error(qPrintable(msg));
    }

    totalVoxelCount = filteredVoxels.size();
    int synK = 10;
    for (int q = 1; q <= synK; q++)
    {
        std::vector<float> foo;
        mSynapsesPerConnectionOccurrences[q] = foo;
    }

    // RandomGenerator generator(50000);

    QFile inputFile(indexFile);
    QFile inputFileBranch(indexFileBranch);

    if (!inputFileBranch.open(QIODevice::ReadOnly))
    {
        const QString msg =
            QString("Error reading index file. Could not open file %1")
                .arg(indexFileBranch);
        throw std::runtime_error(qPrintable(msg));
    }

    int lastUpdatedVoxelCount = -1;
    if (inputFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&inputFile);
        QTextStream inBranch(&inputFileBranch);

        // int absVoxelCount  = 0;
        while (!in.atEnd() && !mAborted)
        {
            QString line = in.readLine();
            line = line.trimmed();
            QStringList parts = line.split(" ");
            int voxelId = parts[0].toInt();

            QString lineBranch = inBranch.readLine();
            lineBranch = lineBranch.trimmed();
            QStringList partsBranch = lineBranch.split(" ");
            int voxelIdBranch = partsBranch[0].toInt();

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
                mSynapsesPerVoxel[voxelId] = 0;

                for (int i = 4; i < parts.size(); i += 2)
                {
                    int neuronId = parts[i].toInt();
                    if (neuronId > 0 &&
                        (mappedPreIds.find(neuronId) != mappedPreIds.end()))
                    {
                        // qDebug() << "unique preId" << neuronId;
                        mMapPreCellsPerVoxel[voxelId] += preMultiplicity[neuronId];
                        int mult = preMultiplicity[neuronId];
                        float boutons = parts[i + 1].toFloat();
                        boutonSum += mult * boutons;
                        mMapBoutonsPerVoxel[voxelId] += mult * boutons;
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

                std::map<int, float> synPerVoxel;

                for (int k = 1; k <= synK; k++)
                {
                    synPerVoxel[k] = 0;
                }

                int count = -1;

                for (auto preIt = pre.begin(); preIt != pre.end(); preIt++)
                {
                    for (auto postIt = post.begin(); postIt != post.end(); postIt++)
                    {
                        count++;
                        if (preIt->first != postIt->first)
                        {
                            float innervation = preIt->second * postIt->second;
                            int multipl = preMultiplicity[preIt->first];

                            for (int k = 1; k <= synK; k++)
                            {
                                float synap =
                                    mCalculator.calculateSynapseProbability(innervation, k);
                                synPerVoxel[k] += multipl * synap;
                                mSynapsesPerVoxel[voxelId] += /*k*/ * multipl * synap;
                            }

                            for (int j = 1; j < multipl; j++)
                            {
                                mSynapsesPerConnection.addSample(innervation);
                            }
                        }
                    }
                }

                for (int k = 1; k <= synK; k++)
                {
                    mSynapsesPerConnectionOccurrences[k].push_back(synPerVoxel[k]);
                }
            }

            // Register branch            
            if (filteredVoxels.find(voxelIdBranch) != filteredVoxels.end())
            {
                mMapPreBranchesPerVoxel[voxelIdBranch] = 0;
                mMapPostBranchesPerVoxel[voxelIdBranch] = 0;                
                for (int i = 4; i < partsBranch.size(); i += 2)
                {
                    int neuronId = partsBranch[i].toInt();
                    if (neuronId > 0 &&
                        (mappedPreIds.find(neuronId) != mappedPreIds.end()))
                    {
                        // qDebug() << "unique preId" << neuronId;
                        int branches = partsBranch[i + 1].toInt();
                        mMapPreBranchesPerVoxel[voxelIdBranch] += branches * preMultiplicity[neuronId];
                    }
                    else if (prunedPostIds.find(-neuronId) != prunedPostIds.end())
                    {
                        int branches = partsBranch[i + 1].toInt();
                        mMapPostBranchesPerVoxel[voxelIdBranch] += branches;
                    }
                }
                
            }
            

            // ################ REPORT UPDATE ################
            int updateRate = totalVoxelCount / 20;
            if (updateRate > 0 && (voxelCount % updateRate == 0) && (voxelCount != lastUpdatedVoxelCount))
            {
                lastUpdatedVoxelCount = voxelCount;
                double percent = double(voxelCount + 1) * 100 / (double)totalVoxelCount;
                QJsonObject result = createJsonResult(false);
                //qDebug() << "RESULT" << result;
                updateQuery(result, percent);
            }
        }
        inputFile.close();
        inputFileBranch.close();
    }
    else
    {
        const QString msg =
            QString("Error reading index file. Could not open file %1")
                .arg(indexFile);
        throw std::runtime_error(qPrintable(msg));
    }

    QJsonObject result = createJsonResult(true);
    updateQuery(result, 100);
}

float
VoxelQueryHandler::calculateSynapseProbability(float innervation, int k)
{
    int nfak = 1;
    float innervationPow = 1;
    for (int i = 1; i <= k; i++)
    {
        innervationPow *= innervation;
        nfak *= i;
    }
    float synapseProb = innervationPow * exp(-innervation) / nfak;

    // qDebug() << k << nfak << innervation << synapseProb <<
    // r.drawPoisson(innervation);
    return synapseProb;
}

bool
VoxelQueryHandler::initSelection()
{
    return false;
};

void
VoxelQueryHandler::setFilterString(QString mode, QString preFilter, QString postFilter, QString advancedSettings, std::vector<double> origin, std::vector<int> dimensions)
{
    const QChar sep(',');
    mFilterString += "Selection:\n";
    mFilterString += "Mode:,";
    if (mode == "voxel")
    {
        mFilterString += "Explicit voxel selection\n";
    }
    else if (mode == "prePostVoxel")
    {
        mFilterString += "Explicit voxel selection with neuron filter\n";
    }
    else
    {
        mFilterString += "Implicit voxel selection with neuron filter\n";
    }

    if (mode == "voxel" || mode == "prePostVoxel")
    {
        mFilterString += "Voxel cube origin:,";
        mFilterString +=
            QString("%1 %2 %3\n").arg(origin[0]).arg(origin[1]).arg(origin[2]);
        mFilterString += "Voxel cube dimensions:,";
        mFilterString += QString("%1 %2 %3\n")
                             .arg(dimensions[0])
                             .arg(dimensions[1])
                             .arg(dimensions[2]);
    }
    if (mode == "prePost" || mode == "prePostVoxel")
    {
        mFilterString += "Presynaptic selection:,";
        mFilterString += preFilter;
        mFilterString += "\n";
        mFilterString += "Postsynaptic selection:,";
        mFilterString += postFilter;
        mFilterString += "\n";
    }
    mFilterString += advancedSettings;
}

QString
VoxelQueryHandler::getResultKey()
{
    return "voxelResult";
}