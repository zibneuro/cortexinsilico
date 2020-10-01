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
#include "Subvolume.h"
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
    Histogram boutonsPerVoxelH(100);
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
    result.insert("boutonsCubicMicron", mBoutonsCubicMicron.getJson());
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

        
        mFileHelper.openFile("testOutput.csv");
        mFileHelper.write("subvolume_id,length_dendrite,length_axon,variability_dendrite,variability_axon,cellbodies,variability_cellbody,branch_dendr,branch_axon,boutons\n");
        for (auto it = mTestOutput.begin(); it != mTestOutput.end(); it++)
        {
            int nAxons = mMapPreBranchesPerVoxel[it->first];
            int nDendr = mMapPostBranchesPerVoxel[it->first];
            float boutons = mMapBoutonsPerVoxel[it->first];
            QString line = QString::number(it->first) + "," + QString::number(it->second[0]) + "," + QString::number(it->second[1]) +
                           "," + QString::number(it->second[2]) + "," + QString::number(it->second[3]) + "," + QString::number(it->second[4]) + "," + QString::number(it->second[5])
                            + "," + QString::number(nDendr) + "," + QString::number(nAxons) + "," + QString::number(boutons) + "\n";
            mFileHelper.write(line);
        }
        mFileHelper.closeFile();
        

        mFileHelper.openFile("statistics.csv");
        mFileHelper.write(Statistics::getHeaderCsv());
        //mFileHelper.write(Statistics::getLineSingleValue("sub-volumes meeting spatial filter condition", (int)mSelectedVoxels.size()));
        mFileHelper.write(Statistics::getLineSingleValue("sub-volume with presynaptic cells [mm³]", Util::formatVolume((int)mPreInnervatedVoxels.size())));
        mFileHelper.write(Statistics::getLineSingleValue("sub-volume with postsynaptic cells [mm³]", Util::formatVolume((int)mPostInnervatedVoxels.size())));
        mFileHelper.write(preCellbodiesPerVoxel.getLineCsv("neuron density [somata/(50\u00B5m)³]"));
        mFileHelper.write(cellbodyVariabilityPerVoxel.getLineCsv("neuron diversity [1/(50\u00B5m)³]"));        
        //mFileHelper.write(postCellbodiesPerVoxel.getLineCsv("postsynaptic cell bodies per (50\u00B5m)³"));                
        mFileHelper.write(dendriteLengthPerVoxel.getLineCsv("dendrite density [cm/(50\u00B5m)³]"));        
        mFileHelper.write(dendriteVariabilityPerVoxel.getLineCsv("dendrite diversity [types/(50\u00B5m)³]"));
        mFileHelper.write(axonLengthPerVoxel.getLineCsv("axon density [m/(50\u00B5m)³]"));
        mFileHelper.write(axonVariabilityPerVoxel.getLineCsv("axon diversity [types/(50\u00B5m)³]"));
        mFileHelper.write(boutonsPerVoxel.getLineCsv("boutons [1/(50\u00B5m)³]"));   
        mFileHelper.write(postBranchesPerVoxel.getLineCsv("dendrite branchlets [1/(50\u00B5m)³]"));
        mFileHelper.write(preBranchesPerVoxel.getLineCsv("axon branchlets [1/(50\u00B5m)³]"));             
        // mFileHelper.write(postCellsPerVoxel.getLineCsv("innervating postsynaptic cells [1/(50\u00B5m)³]"));
        // mFileHelper.write(preCellsPerVoxel.getLineCsv("innervating presynaptic cells [1/(50\u00B5m)³]"));        
        mFileHelper.write(synapsesPerVoxel.getLineCsv("synapses [1/(50\u00B5m)³]")); 
        mFileHelper.closeFile();

        preCellbodiesPerVoxelH.writeFile(mFileHelper, "histogram_neuron_density.csv");
        //postCellbodiesPerVoxelH.writeFile(mFileHelper, "histogram_postsynaptic_cellbodies.csv");
        //preCellsPerVoxelH.writeFile(mFileHelper, "histogram_innervating_presynaptic_cells.csv");
        //postCellsPerVoxelH.writeFile(mFileHelper, "histogram_innervating_postsynaptic_cells.csv");
        preBranchesPerVoxelH.writeFile(mFileHelper, "histogram_axon_branches.csv");
        postBranchesPerVoxelH.writeFile(mFileHelper, "histogram_dendrite_branches.csv");
        axonLengthPerVoxelH.writeFile(mFileHelper, "histogram_axon_density.csv");
        dendriteLengthPerVoxelH.writeFile(mFileHelper, "histogram_dendrite_density.csv");
        synapsesPerVoxelH.writeFile(mFileHelper, "histogram_synapses.csv");
        cellbodyVariabilityPerVoxelH.writeFile(mFileHelper, "histogram_neuron_diversity.csv");
        axonVariabilityPerVoxelH.writeFile(mFileHelper, "histogram_axon_diversity.csv");
        dendriteVariabilityPerVoxelH.writeFile(mFileHelper, "histogram_dendrite_diversity.csv");

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
        mBoutonsCubicMicron.writeFile(mFileHelper, "boxplot_boutons.csv", "[1/\u00B5m³]");

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
    QString subvolumeDir = mNetwork.networkRootDir.filePath("subvolumes");

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
    double min_x, max_x, min_y, max_y, min_z, max_z, min_depth, max_depth, min_zAxis, max_zAxis;
    Util::getRange(subvolumeConditions, "rangeX", -1114, 1408, min_x, max_x);
    Util::getRange(subvolumeConditions, "rangeY", -759, 1497, min_y, max_y);
    Util::getRange(subvolumeConditions, "rangeZ", -1461, 708, min_z, max_z);
    Util::getRange(subvolumeConditions, "corticalDepth", -100, 2000, min_depth, max_depth);
    Util::getRange(subvolumeConditions, "rangeZAxis", -100000, 100000, min_zAxis, max_zAxis);
    gridFilter.min_x = min_x;
    gridFilter.max_x = max_x;
    gridFilter.min_y = min_y;
    gridFilter.max_y = max_y;
    gridFilter.min_z = min_z;
    gridFilter.max_z = max_z;
    gridFilter.min_depth = min_depth;
    gridFilter.max_depth = max_depth;
    gridFilter.whitelist_region = Util::getPermittedSubvolumeRegionIds(subvolumeConditions, mNetwork.regions);
    gridFilter.min_zAxis = min_zAxis;
    gridFilter.max_zAxis = max_zAxis;
    mSubvolumes = mNetwork.grid.filter(gridFilter);

    qDebug() << "SUBVOLUME" << mSubvolumes.size() << "PRE" << mPreIds.size() << "POST" << mPostIds.size();

    // ################# LOOP OVER SUBVOLUMES #################

    mSynK = 10;
    for (int q = 0; q <= mSynK; q++)
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
        Subvolume subvolume;
        subvolume.load(subvolumeDir, SID, mPreIds, mappedPreIds, mPostIds);

        determineBranches(subvolume, preMultiplicity);
        determineCellCounts(subvolume);
        determineSynapses(subvolume, preMultiplicity, pstAll);

        // ################ REPORT UPDATE ################

        if (subvolume_idx % 10 == 0)
        {
            double percent = 100 * static_cast<float>(subvolume_idx + 1) / static_cast<float>(mSubvolumes.size());
            qDebug() << "Percent" << percent;
            QJsonObject result = createJsonResult(false);
            updateQuery(result, percent);
        }
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

void VoxelQueryHandler::determineCellCounts(Subvolume &subvolume)
{
    int SID = subvolume.SID;

    std::set<int> celltypes;
    mPreCellbodiesPerVoxel[SID] = 0;
    mPostCellbodiesPerVoxel[SID] = 0;
    for (auto it = subvolume.cellbodies.begin(); it != subvolume.cellbodies.end(); it++)
    {        
        int cellTypeId = mNetwork.neurons.getCellTypeId(*it);
        if (cellTypeId < 10)
        {
            mPreCellbodiesPerVoxel[SID] += 1;
            celltypes.insert(cellTypeId);
        }
    }
    mVariabilityCellbodies[SID] = static_cast<float>(celltypes.size());
    mTestOutput[SID].push_back(static_cast<float>(mPreCellbodiesPerVoxel[SID]));
    mTestOutput[SID].push_back(static_cast<float>(mVariabilityCellbodies[SID]));
}

void VoxelQueryHandler::determineBranches(Subvolume &subvolume, std::map<int, int> &preDuplicity)
{
    int SID = subvolume.SID;

    mAxonLengthPerVoxel[SID] = 0;
    mDendriteLengthPerVoxel[SID] = 0;
    mMapPreBranchesPerVoxel[SID] = 0;
    mMapPostBranchesPerVoxel[SID] = 0;
    mMapPreCellsPerVoxel[SID] = 0;
    mMapPostCellsPerVoxel[SID] = 0;

    if (!subvolume.presynaptic.empty())
    {
        mPreInnervatedVoxels.insert(SID);
    }
    if (!subvolume.postsynaptic.empty())
    {
        mPostInnervatedVoxels.insert(SID);
    }

    std::set<int> celltypesAxon;
    std::set<int> celltypesDendrite;

    for (auto it = subvolume.presynaptic.begin(); it != subvolume.presynaptic.end(); it++)
    {
        int duplicity = preDuplicity[it->first];

        mMapPreBranchesPerVoxel[SID] += duplicity * it->second.branchlets;
        mMapPreCellsPerVoxel[SID] += duplicity;

        float axonLength = duplicity * it->second.length;
        mAxonLengthPerVoxel[SID] += 0.000001 * axonLength; //convert to [m]
        if (axonLength > 0)
        {
            int cellTypeId = mNetwork.neurons.getCellTypeId(it->first);
            if (cellTypeId <= 10)
            {
                celltypesAxon.insert(cellTypeId);
            }
        }
    }

    for (auto it = subvolume.postsynaptic.begin(); it != subvolume.postsynaptic.end(); it++)
    {
        mMapPostBranchesPerVoxel[SID] += it->second.branchlets;
        mMapPostCellsPerVoxel[SID] += 1;

        float dendriteLength = it->second.length;
        mDendriteLengthPerVoxel[SID] += 0.0001 * dendriteLength; //convert to [cm]
        if (dendriteLength > 0)
        {
            int cellTypeId = mNetwork.neurons.getCellTypeId(it->first);
            if (cellTypeId <= 10)
            {
                celltypesDendrite.insert(cellTypeId);
            }
        }
    }

    mVariabilityAxon[SID] = static_cast<float>(celltypesAxon.size());
    mVariabilityDendrite[SID] = static_cast<float>(celltypesDendrite.size());

    double axonBranches = mMapPreBranchesPerVoxel[SID];
    double dendriteBranches = mMapPostBranchesPerVoxel[SID];
    if (dendriteBranches != 0)
    {
        mAxonDendriteRatio.addSample(SID, axonBranches / dendriteBranches);
    }

    
    mTestOutput[SID].push_back(static_cast<float>(mDendriteLengthPerVoxel[SID]));
    mTestOutput[SID].push_back(static_cast<float>(mAxonLengthPerVoxel[SID]));
    mTestOutput[SID].push_back(static_cast<float>(mVariabilityDendrite[SID]));
    mTestOutput[SID].push_back(static_cast<float>(mVariabilityAxon[SID]));
}

void VoxelQueryHandler::determineSynapses(Subvolume &subvolume, std::map<int, int> &preDuplicity, PstAll& pstAll)
{
    int SID = subvolume.SID;

    mMapBoutonsPerVoxel[SID] = 0;
    mSynapsesPerVoxel[SID] = 0;

    std::map<int, float> synPerVoxel;
    for (int k = 0; k <= mSynK; k++)
    {
        synPerVoxel[k] = 0;
    }

    for (auto itPre = subvolume.presynaptic.begin(); itPre != subvolume.presynaptic.end(); itPre++)
    {
        int duplicity = preDuplicity[itPre->first];
        float boutons =  itPre->second.boutons;
        mMapBoutonsPerVoxel[SID] += duplicity * boutons;
        int cellTypeId =  mNetwork.neurons.getCellTypeId(itPre->first);
        bool excitatory = mNetwork.cellTypes.isExcitatory(cellTypeId);                
        float pstAllVal =  excitatory ? pstAll.get(SID).exc : pstAll.get(SID).inh;
        
        for (auto itPost = subvolume.postsynaptic.begin(); itPost != subvolume.postsynaptic.end(); itPost++)
        {
            float pst = excitatory ? itPost->second.pstExc : itPost->second.pstInh;            
            float innervation = boutons * pst / pstAllVal;            
            for (int k = 0; k <= mSynK; k++)
            {
                float synap =
                    mCalculator.calculateSynapseProbability(innervation, k);
                synPerVoxel[k] += duplicity * synap;
                mSynapsesPerVoxel[SID] += k * duplicity * synap;
            }                        
        }
    }

    for (int k = 0; k <= mSynK; k++)
    {
        mSynapsesPerConnectionOccurrences[k].push_back(synPerVoxel[k]);
    }

    mSynapsesCubicMicron.addSample(SID, Util::convertToCubicMicron(mSynapsesPerVoxel[SID]));    
    mBoutonsCubicMicron.addSample(SID, Util::convertToCubicMicron(mMapBoutonsPerVoxel[SID]));
}