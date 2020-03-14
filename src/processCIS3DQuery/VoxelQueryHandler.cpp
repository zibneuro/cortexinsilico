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
#include "PstAll.h"
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
void VoxelQueryHandler::createStatistics(std::map<int, T> &values,
                                         Statistics &stat,
                                         Histogram &histogram)
{
    for (auto it = values.begin(); it != values.end(); it++)
    {
        stat.addSample(it->second);
    }
    double binSize = stat.getMaximum() / 1000;
    binSize = binSize == 0 ? 1 : binSize;
    histogram = Histogram(binSize);
    for (auto it = values.begin(); it != values.end(); it++)
    {
        histogram.addValue(it->second);
    }
}

QString VoxelQueryHandler::getMode(QJsonObject &mQuery)
{
    bool hasPre = !mQuery["cellSelection"].toObject()["selectionA"].toObject()["conditions"].toArray().empty();
    bool hasPost = !mQuery["cellSelection"].toObject()["selectionB"].toObject()["conditions"].toArray().empty();
    bool hasSubvolume = !mQuery["cellSelection"].toObject()["selectionC"].toObject()["conditions"].toArray().empty();
    if (!hasSubvolume)
    {
        return "prePost";
    }
    else if (hasPre || hasPost)
    {
        return "prePostVoxel";
    }
    else
    {
        return "voxel";
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

    Statistics preCellbodiesPerVoxel;
    Histogram preCellbodiesPerVoxelH(50);
    createStatistics(mPreCellbodiesPerVoxel, preCellbodiesPerVoxel, preCellbodiesPerVoxelH);

    Statistics postCellbodiesPerVoxel;
    Histogram postCellbodiesPerVoxelH(50);
    createStatistics(mPostCellbodiesPerVoxel, postCellbodiesPerVoxel, postCellbodiesPerVoxelH);

    Statistics axonLengthPerVoxel;
    Histogram axonLengthPerVoxelH(50);
    createStatistics(mAxonLengthPerVoxel, axonLengthPerVoxel, axonLengthPerVoxelH);

    Statistics dendriteLengthPerVoxel;
    Histogram dendriteLengthPerVoxelH(50);
    createStatistics(mDendriteLengthPerVoxel, dendriteLengthPerVoxel, dendriteLengthPerVoxelH);

    Statistics cellbodyVariabilityPerVoxel;
    Histogram cellbodyVariabilityPerVoxelH(50);
    createStatistics(mVariabilityCellbodies, cellbodyVariabilityPerVoxel, cellbodyVariabilityPerVoxelH);

    Statistics axonVariabilityPerVoxel;
    Histogram axonVariabilityPerVoxelH(50);
    createStatistics(mVariabilityAxon, axonVariabilityPerVoxel, axonVariabilityPerVoxelH);

    Statistics dendriteVariabilityPerVoxel;
    Histogram dendriteVariabilityPerVoxelH(50);
    createStatistics(mVariabilityDendrite, dendriteVariabilityPerVoxel, dendriteVariabilityPerVoxelH);

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
    result.insert("presynapticCellsPerVoxelHisto", preCellsPerVoxelH.createJson());
    result.insert("presynapticBranchesPerVoxel", Util::createJsonStatistic(preBranchesPerVoxel));
    result.insert("presynapticBranchesPerVoxelHisto", preBranchesPerVoxelH.createJson());
    result.insert("postsynapticCellsPerVoxel",
                  Util::createJsonStatistic(postCellsPerVoxel));
    result.insert("postsynapticCellsPerVoxelHisto", postCellsPerVoxelH.createJson());
    result.insert("postsynapticBranchesPerVoxel",
                  Util::createJsonStatistic(postBranchesPerVoxel));
    result.insert("postsynapticBranchesPerVoxelHisto", postBranchesPerVoxelH.createJson());
    result.insert("boutonsPerVoxel", Util::createJsonStatistic(boutonsPerVoxel));
    result.insert("boutonsPerVoxelHisto", boutonsPerVoxelH.createJson());
    result.insert("postsynapticSitesPerVoxel",
                  Util::createJsonStatistic(postsynapticSitesPerVoxel));
    result.insert("postsynapticSitesPerVoxelHisto", postsynapticSitesPerVoxelH.createJson());
    result.insert("synapsesPerVoxel",
                  Util::createJsonStatistic(synapsesPerVoxel));
    result.insert("synapsesPerVoxelHisto", synapsesPerVoxelH.createJson());
    // result.insert("synapsesPerConnection",
    // Util::createJsonStatistic(mSynapsesPerConnection));
    result.insert("synapsesPerConnectionPlot", synapsesPerConnectionPlot);
    result.insert("synapsesCubicMicron", mSynapsesCubicMicron.getJson());
    result.insert("axonDendriteRatio", mAxonDendriteRatio.getJson());
    result.insert("presynapticCellbodiesPerVoxel", Util::createJsonStatistic(preCellbodiesPerVoxel));
    result.insert("postsynapticCellbodiesPerVoxel", Util::createJsonStatistic(postCellbodiesPerVoxel));
    result.insert("presynapticCellbodiesPerVoxelHisto", preCellbodiesPerVoxelH.createJson());
    result.insert("postsynapticCellbodiesPerVoxelHisto", postCellbodiesPerVoxelH.createJson());
    result.insert("axonLengthPerVoxel", Util::createJsonStatistic(axonLengthPerVoxel));
    result.insert("dendriteLengthPerVoxel", Util::createJsonStatistic(dendriteLengthPerVoxel));
    result.insert("axonLengthPerVoxelHisto", axonLengthPerVoxelH.createJson());
    result.insert("dendriteLengthPerVoxelHisto", dendriteLengthPerVoxelH.createJson());
    result.insert("cellbodyVariabilityPerVoxel",
                  Util::createJsonStatistic(cellbodyVariabilityPerVoxel));
    result.insert("cellbodyVariabilityPerVoxelHisto", cellbodyVariabilityPerVoxelH.createJson());
    result.insert("axonVariabilityPerVoxel",
                  Util::createJsonStatistic(axonVariabilityPerVoxel));
    result.insert("axonVariabilityPerVoxelHisto", axonVariabilityPerVoxelH.createJson());
    result.insert("dendriteVariabilityPerVoxel",
                  Util::createJsonStatistic(dendriteVariabilityPerVoxel));

    result.insert("dendriteVariabilityPerVoxelHisto", dendriteVariabilityPerVoxelH.createJson());

    // ################# CREATE CSV FILE #################

    if (createFile)
    {
        mFileHelper.initFolder(mConfig, mQueryId);
        mFileHelper.openFile("specifications.csv");
        mFileHelper.write(mResultFileHeader);
        mFileHelper.closeFile();

        /*
        mFileHelper.openFile("subvolumes.csv");
        mFileHelper.write("subvolume_id,x,y,z,cortical_depth,region_id\n");
        for (auto it = mSubvolumes.begin(); it != mSubvolumes.end(); it++)
        {
            mFileHelper.write(*it);
        }
        mFileHelper.closeFile();
        */

        /*
        mFileHelper.openFile("testOutput.csv");
        mFileHelper.write("subvolume_id,cellbodies,variability_cellbody,length_dendrite,length_axon,variability_dendrite,variability_axon,branch_dendr,branch_axon\n");
        for (auto it = mTestOutput.begin(); it != mTestOutput.end(); it++)
        {
            int nAxons = mMapPreBranchesPerVoxel[it->first];
            int nDendr = mMapPostBranchesPerVoxel[it->first];
            QString line = QString::number(it->first) + "," + QString::number(it->second[0]) + "," + QString::number(it->second[1]) +
                           "," + QString::number(it->second[2]) + "," + QString::number(it->second[3]) + "," + QString::number(it->second[4]) + "," + QString::number(it->second[5]) + "," + QString::number(nDendr) + "," + QString::number(nAxons) + "\n";
            mFileHelper.write(line);
        }
        mFileHelper.closeFile();
        */

        mFileHelper.openFile("statistics.csv");
        mFileHelper.write(Statistics::getHeaderCsv());
        mFileHelper.write(Statistics::getLineSingleValue("sub-volumes meeting spatial filter condition", (int)mSelectedVoxels.size()));
        mFileHelper.write(Statistics::getLineSingleValue("sub-volume with presynaptic cells [mm³]", Util::formatVolume((int)mPreInnervatedVoxels.size())));
        mFileHelper.write(Statistics::getLineSingleValue("sub-volume with postsynaptic cells [mm³]", Util::formatVolume((int)mPostInnervatedVoxels.size())));
        mFileHelper.write(preCellbodiesPerVoxel.getLineCsv("cell bodies [1/(50\u00B5m)³]"));
        mFileHelper.write(cellbodyVariabilityPerVoxel.getLineCsv("cell body heterogeneity [1/(50\u00B5m)³]"));
        //mFileHelper.write(postCellbodiesPerVoxel.getLineCsv("postsynaptic cell bodies per (50\u00B5m)³"));
        mFileHelper.write(preCellsPerVoxel.getLineCsv("innervating presynaptic cells [1/(50\u00B5m)³]"));
        mFileHelper.write(postCellsPerVoxel.getLineCsv("innervating postsynaptic cells [1/(50\u00B5m)³]"));
        mFileHelper.write(preBranchesPerVoxel.getLineCsv("axon branchlets [1/(50\u00B5m)³]"));
        mFileHelper.write(postBranchesPerVoxel.getLineCsv("dendrite branchlets [1/(50\u00B5m)³]"));
        mFileHelper.write(axonVariabilityPerVoxel.getLineCsv("axon heterogeneity [1/(50\u00B5m)³]"));
        mFileHelper.write(dendriteVariabilityPerVoxel.getLineCsv("dendrite heterogeneity [1/(50\u00B5m)³]"));
        mFileHelper.write(axonLengthPerVoxel.getLineCsv("axon length [m/(50\u00B5m)³]"));
        mFileHelper.write(dendriteLengthPerVoxel.getLineCsv("dendrite length [cm/(50\u00B5m)³]"));
        mFileHelper.write(synapsesPerVoxel.getLineCsv("synapses [1/(50\u00B5m)³]"));
        mFileHelper.closeFile();

        preCellbodiesPerVoxelH.writeFile(mFileHelper, "histogram_cellbodies.csv");
        //postCellbodiesPerVoxelH.writeFile(mFileHelper, "histogram_postsynaptic_cellbodies.csv");
        preCellsPerVoxelH.writeFile(mFileHelper, "histogram_innervating_presynaptic_cells.csv");
        postCellsPerVoxelH.writeFile(mFileHelper, "histogram_innervating_postsynaptic_cells.csv");
        preBranchesPerVoxelH.writeFile(mFileHelper, "histogram_axon_branches.csv");
        postBranchesPerVoxelH.writeFile(mFileHelper, "histogram_dendrite_branches.csv");
        axonLengthPerVoxelH.writeFile(mFileHelper, "histogram_axon_length.csv");
        dendriteLengthPerVoxelH.writeFile(mFileHelper, "histogram_dendrite_length.csv");
        synapsesPerVoxelH.writeFile(mFileHelper, "histogram_synapses.csv");
        cellbodyVariabilityPerVoxelH.writeFile(mFileHelper, "histogram_cellbody_heterogeneity.csv");
        axonVariabilityPerVoxelH.writeFile(mFileHelper, "histogram_axon_heterogeneity.csv");
        dendriteVariabilityPerVoxelH.writeFile(mFileHelper, "histogram_dendrite_heterogeneity.csv");

        mFileHelper.openFile("linechart_synapses_perconnection.csv");
        mFileHelper.write("k,min,med,max\n");
        for (int i = 0; i < synapsesPerConnectionMin.size(); i++)
        {
            mFileHelper.write(QString::number(i) + "," + QString::number(synapsesPerConnectionMin[i].toDouble()) +
                              "," + QString::number(synapsesPerConnectionMed[i].toDouble()) + "," + QString::number(synapsesPerConnectionMax[i].toDouble()) + "\n");
        }
        mFileHelper.closeFile();

        mSynapsesCubicMicron.writeFile(mFileHelper, "boxplot_synapses.csv", "[1/\u00B5m³]");
        mAxonDendriteRatio.writeFile(mFileHelper, "boxplot_axon_dendrite_ratio.csv", "[1/(50\u00B5m)³]");

        mFileHelper.uploadFolder(result);
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

void VoxelQueryHandler::doProcessQuery()
{
    mQuery["cellSelection"].toObject()["selectionC"].toObject().insert("enabled", false);
    mSelection.setSelectionFromQuery(mQuery, mNetwork, mConfig);
    IdList preIds = mSelection.SelectionA();
    IdList postIds = mSelection.SelectionB();
    PstAll pstAll;
    pstAll.load(mNetwork.networkRootDir.filePath("agg_pst.csv"));

    // ################# FILTER NEURONS #################
    for (int i = 0; i < preIds.size(); i++)
    {
        mPreIds.insert(preIds[i]);
    }
    for (int i = 0; i < postIds.size(); i++)
    {
        mPostIds.insert(postIds[i]);
    }

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

    // ################# FILTER SUBVOLUMES #################

    GridFilter gridFilter;
    QJsonArray subvolumeConditions = mQuery["cellSelection"].toObject()["selectionC"].toObject()["conditions"].toArray();
    double min_x, max_x, min_y, max_y, min_z, max_z, min_depth, max_depth;
    Util::getRange(subvolumeConditions, "rangeX", -1114, 1408, min_x, max_x);
    Util::getRange(subvolumeConditions, "rangeY", -759, 1497, min_y, max_y);
    Util::getRange(subvolumeConditions, "rangeZ", -1461, 708, min_z, max_z);
    Util::getRange(subvolumeConditions, "corticalDepth", -100, 2000, min_depth, max_depth);
    gridFilter.min_x = min_x;
    gridFilter.max_x = max_x;
    gridFilter.min_y = min_y;
    gridFilter.max_y = max_y;
    gridFilter.min_z = min_z;
    gridFilter.max_z = max_z;
    gridFilter.min_depth = min_depth;
    gridFilter.max_depth = max_depth;
    gridFilter.whitelist_region = Util::getPermittedSubvolumeRegionIds(subvolumeConditions, mNetwork.regions);
    mSubvolumes = mNetwork.grid.filter(gridFilter);

    qDebug() << "SUBVOLUME" << mSubvolumes.size() << "PRE" << mPreIds.size() << "POST" << mPostIds.size();

    // ################# LOOP OVER SUBVOLUMES #################

    int synK = 10;
    for (int q = 0; q <= synK; q++)
    {
        std::vector<float> foo;
        mSynapsesPerConnectionOccurrences[q] = foo;
    }

    for (unsigned int subvolume_idx = 0; subvolume_idx < mSubvolumes.size(); subvolume_idx++)
    {
        if (mAborted)
        {
            break;
        }

        int SID = mSubvolumes[subvolume_idx];

        /*        
        determineCellCounts(voxelId);
        determineBranchLengths(voxelId);

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

        mSynapsesCubicMicron.addSample(voxelId, Util::convertToCubicMicron(mMapBoutonsPerVoxel[voxelId]));

        std::map<int, float> synPerVoxel;

        for (int k = 0; k <= synK; k++)
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

                    for (int k = 0; k <= synK; k++)
                    {
                        float synap =
                            mCalculator.calculateSynapseProbability(innervation, k);
                        synPerVoxel[k] += multipl * synap;
                        mSynapsesPerVoxel[voxelId] += k * multipl * synap;
                    }

                    for (int j = 1; j < multipl; j++)
                    {
                        mSynapsesPerConnection.addSample(innervation);
                    }
                }
            }
        }

        for (int k = 0; k <= synK; k++)
        {
            mSynapsesPerConnectionOccurrences[k].push_back(synPerVoxel[k]);
        }

        mMapPreBranchesPerVoxel[voxelIdBranch] = 0;
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
        }
        determineSnippets(voxelIdBranch);
        double axonBranches = mMapPreBranchesPerVoxel[voxelIdBranch];
        double dendriteBranches = mMapPostBranchesPerVoxel[voxelIdBranch];
        if (dendriteBranches != 0)
        {
            mAxonDendriteRatio.addSample(voxelIdBranch, axonBranches / dendriteBranches);
        }

        // ################ REPORT UPDATE ################
        int updateRate = totalVoxelCount / 20;
        if (updateRate > 0 && (voxelCount % updateRate == 0) && (voxelCount != lastUpdatedVoxelCount))
        {
            lastUpdatedVoxelCount = voxelCount;
            double percent = double(voxelCount + 1) * 100 / (double)totalVoxelCount;
            QJsonObject result = createJsonResult(false);
            updateQuery(result, percent);
        }
        */
    }

    QJsonObject result = createJsonResult(true);
    updateQuery(result, 100);
}

float VoxelQueryHandler::calculateSynapseProbability(float innervation, int k)
{
    int nfak = 1;
    float innervationPow = 1;
    for (int i = 1; i <= k; i++)
    {
        innervationPow *= innervation;
        nfak *= i;
    }
    float synapseProb = innervationPow * exp(-innervation) / nfak;

    return synapseProb;
}

bool VoxelQueryHandler::initSelection()
{
    return false;
};

QString
VoxelQueryHandler::getResultKey()
{
    return "voxelResult";
}

void VoxelQueryHandler::determineCellCounts(int voxelId)
{
    QString filename = QDir::cleanPath(mDataRoot + QDir::separator() + "subvolume_neuronIds" + QDir::separator() + QString::number(voxelId + 1));
    std::vector<std::vector<double>> data = UtilIO::readCsv(filename, true);
    std::set<int> celltypes;
    mPreCellbodiesPerVoxel[voxelId] = 0;
    mPostCellbodiesPerVoxel[voxelId] = 0;
    for (auto it = data.begin(); it != data.end(); it++)
    {
        int neuronId = static_cast<int>((*it)[0]);
        if (mPreIds.find(neuronId) != mPreIds.end() || mPostIds.find(neuronId) != mPostIds.end())
        {
            mPreCellbodiesPerVoxel[voxelId] += 1;
            celltypes.insert(mNetwork.neurons.getCellTypeId(neuronId));
        }
    }
    mVariabilityCellbodies[voxelId] = static_cast<float>(celltypes.size()) / 10;
    mTestOutput[voxelId].push_back(static_cast<float>(mPreCellbodiesPerVoxel[voxelId]));
    mTestOutput[voxelId].push_back(static_cast<float>(mVariabilityCellbodies[voxelId]));
}

void VoxelQueryHandler::determineBranchLengths(int voxelId)
{
    QString filename = QDir::cleanPath(mDataRoot + QDir::separator() + "subvolume_stats" + QDir::separator() + QString::number(voxelId + 1));
    std::vector<std::vector<double>> data = UtilIO::readCsv(filename, true);
    mAxonLengthPerVoxel[voxelId] = 0;
    mDendriteLengthPerVoxel[voxelId] = 0;
    std::set<int> celltypesAxon;
    std::set<int> celltypesDendrite;

    for (auto it = data.begin(); it != data.end(); it++)
    {
        int neuronId = static_cast<int>((*it)[0]);
        double apicalLength = (*it)[1];
        double basalLength = (*it)[3];
        double axonLength = (*it)[5];
        if (mPreIds.find(neuronId) != mPreIds.end())
        {
            mAxonLengthPerVoxel[voxelId] += 0.000001 * axonLength; //convert to [m]
            if (axonLength > 0)
            {
                celltypesAxon.insert(mNetwork.neurons.getCellTypeId(neuronId));
            }
        }
        if (mPostIds.find(neuronId) != mPostIds.end())
        {
            mDendriteLengthPerVoxel[voxelId] += 0.0001 * (apicalLength + basalLength); //convert to [cm]
            if (apicalLength + basalLength > 0)
            {
                celltypesDendrite.insert(mNetwork.neurons.getCellTypeId(neuronId));
            }
        }
    }

    mVariabilityAxon[voxelId] = static_cast<float>(celltypesAxon.size()) / 11;
    mVariabilityDendrite[voxelId] = static_cast<float>(celltypesDendrite.size()) / 10;
    mTestOutput[voxelId].push_back(static_cast<float>(mDendriteLengthPerVoxel[voxelId]));
    mTestOutput[voxelId].push_back(static_cast<float>(mAxonLengthPerVoxel[voxelId]));
    mTestOutput[voxelId].push_back(static_cast<float>(mVariabilityDendrite[voxelId]));
    mTestOutput[voxelId].push_back(static_cast<float>(mVariabilityAxon[voxelId]));
}

void VoxelQueryHandler::determineSnippets(int voxelId)
{
    mMapPostBranchesPerVoxel[voxelId] = 0;
    QString voxelStr = mVoxelIdVoxelStr[voxelId];
    QString filename = QDir::cleanPath(mDataRoot + QDir::separator() + "features_subvolume_cube" + QDir::separator() + UtilIO::getSubvolumeFileName(voxelStr));
    std::vector<std::vector<double>> data = UtilIO::readCsv(filename, true);
    for (auto it = data.begin(); it != data.end(); it++)
    {
        int neuronId = static_cast<int>((*it)[0]);
        int dendriteSnippets = static_cast<int>((*it)[1]);
        if (mPostIds.find(neuronId) != mPostIds.end())
        {
            mMapPostBranchesPerVoxel[voxelId] += dendriteSnippets;
        }
    }
}
