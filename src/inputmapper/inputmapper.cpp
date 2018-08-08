/*
    This tool registers a new neuron into the existing model.
    application. Usage:

    ./inputmapper <hocfile> <celltype> <datadir> <outputdir>"

    <hocfile>   The morphology of the neuron to register.
    <celltype>  The cell type of the neuron to register.
    <datadir>   The directory containing the model data.
    <outputdir> The output directort to which the csv-result of the registration
                is written.
*/

#include <QDebug>
#include <QtCore>
#include <random>

#include "CIS3DAxonRedundancyMap.h"
#include "CIS3DBoundingBoxes.h"
#include "CIS3DCellTypes.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DMorphology.h"
#include "CIS3DNetworkProps.h"
#include "CIS3DNeurons.h"
#include "CIS3DPSTDensities.h"
#include "CIS3DRegions.h"
#include "CIS3DSparseField.h"
#include "CIS3DSparseVectorSet.h"
#include "CIS3DSynapseStatistics.h"
#include "CIS3DVec3.h"

QTextStream& qStdOut() {
    static QTextStream ts(stdout);
    return ts;
}

QTextStream& qStdErr() {
    static QTextStream ts(stdout);
    return ts;
}

int poisson(const double mean) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::poisson_distribution<> d(mean);
    return d(gen);
}

float randomUniform(const float maxVal) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> unif(0, maxVal);
    return unif(gen);
}

void printUsage() {
    qStdErr() << "Usage: ./inputmapper <hocfile> <celltype> <datadir> <outputdir>\n";
}

typedef QList<CIS3D::Synapse> SynapseList;

SynapseList generateSynapses(const SegmentList& segments,
                             const SparseFieldCoordinates& segmentGridCoords,
                             const SparseField& innervation, const CIS3D::Structure structure,
                             const int presynapticId, const Vec3f preSoma) {
    SynapseList result;

    QMap<Vec3i, int> numSynapsesPerVoxel;
    for (SparseField::ConstIterator it = innervation.constBegin(); it != innervation.constEnd();
         ++it) {
        const int numSynapses = poisson(it.value);
        if (numSynapses > 0) {
            numSynapsesPerVoxel.insert(it.location, numSynapses);
        }
    }

    // Often no synapses have to be computed at all => return asap
    if (numSynapsesPerVoxel.isEmpty()) {
        return result;
    }

    // Create list of segments of the right structure passing through each voxel that contains
    // synapses
    const Vec3i offset = segmentGridCoords.getOffsetTo(innervation.getCoordinates());
    QMap<Vec3i, QList<int>> segmentsPerVoxel;
    for (int s = 0; s < segments.size(); ++s) {
        if (segments[s].structure == structure) {
            const Vec3i loc = segments[s].voxel + offset;
            if (numSynapsesPerVoxel.contains(loc)) {
                segmentsPerVoxel[loc].append(s);
            }
        }
    }

    for (QMap<Vec3i, int>::ConstIterator it = numSynapsesPerVoxel.constBegin();
         it != numSynapsesPerVoxel.constEnd(); ++it) {
        const Vec3i& location = it.key();
        const int numSynapses = it.value();

        const QList<int> segmentIds = segmentsPerVoxel.value(location);
        if (segmentIds.size() == 0) {
            qStdErr() << "[-] WARNING: no segments for voxel" << location.getX() << location.getY()
                      << location.getZ() << "\n";
        }

        QList<float> cumulativeLength;
        for (int s = 0; s < segmentIds.size(); ++s) {
            if (s == 0) {
                cumulativeLength.append(segments[segmentIds[s]].length);
            } else {
                cumulativeLength.append(cumulativeLength[s - 1] + segments[segmentIds[s]].length);
            }
        }

        for (int n = 0; n < numSynapses; ++n) {
            const float targetLength = randomUniform(cumulativeLength.last());

            for (int i = 0; i < cumulativeLength.size(); ++i) {
                if (cumulativeLength[i] >= targetLength) {
                    const Segment& synSeg = segments[segmentIds[i]];
                    const float start = (i == 0) ? 0.0f : cumulativeLength[i - 1];
                    const float alpha =
                        fabs(synSeg.length) > 1.e-4 ? (targetLength - start) / synSeg.length : 0.0f;
                    if (alpha < 0.0f || alpha > 1.0f) {
                        qStdErr() << "[-] WARNING: invalid alpha" << alpha << "\n";
                    }

                    CIS3D::Synapse s;
                    s.preNeuronId = presynapticId;
                    s.structure = structure;
                    s.sectionID = synSeg.sectionID;
                    s.sectionX =
                        synSeg.p0.sectionX + alpha * (synSeg.p1.sectionX - synSeg.p0.sectionX);
                    s.distanceToSomaPost =
                        synSeg.p0.distanceToSoma +
                        alpha * (synSeg.p1.distanceToSoma - synSeg.p0.distanceToSoma);
                    s.pos = synSeg.p0.coords + alpha * (synSeg.p1.coords - synSeg.p0.coords);
                    s.euclideanDistanceToSomaPre = (s.pos - preSoma).length();
                    result.append(s);

                    if (s.sectionX < 0.0 || s.sectionX > 1.0) {
                        qStdErr() << "[-] WARNING: invalid location on section:" << synSeg.sectionID
                                  << "Location: " << s.sectionX << "p0:" << synSeg.p0.sectionX
                                  << "p1:" << synSeg.p1.sectionX << "alpha:" << alpha << "\n";
                    }
                    break;
                }
            }
        }
    }

    return result;
}

SparseField computeLengthField(const SegmentList& segments, const CIS3D::Structure structure,
                               const SparseFieldCoordinates& coords) {
    SparseField field(coords);
    for (SegmentList::const_iterator it = segments.constBegin(); it != segments.constEnd(); ++it) {
        const Segment& seg = *it;
        if (seg.structure == structure) {
            field.addValue(seg.voxel, seg.length);
        }
    }
    return field;
}

SparseField computeAreaField(const SegmentList& segments, const CIS3D::Structure structure,
                             const SparseFieldCoordinates& coords) {
    SparseField field(coords);
    for (SegmentList::const_iterator it = segments.constBegin(); it != segments.constEnd(); ++it) {
        const Segment& seg = *it;
        if (seg.structure == structure) {
            field.addValue(seg.voxel, seg.area);
        }
    }
    return field;
}

SparseField computeInnervation(const SparseField& psts, const SparseField& boutons,
                               const SparseField& populationPsts) {
    const SparseField innervationUnnormalized = multiply(boutons, psts);
    SparseField innervation = divide(innervationUnnormalized, populationPsts);
    return innervation;
}

struct SynapseSectionProps {
    int sectionID;
    float sectionX;
};

typedef QList<SynapseSectionProps> SynapseSectionPropsList;
typedef QMap<QString, QMap<int, SynapseSectionPropsList>> SortedSynapseSectionPropsMap;

void writeSynConFiles(const SynapseList& synapses, const NetworkProps& network,
                      const QString& hocFileName, const QString& outputDir) {
    const QFileInfo hocInfo(hocFileName);
    const QString outFilePrefix = hocInfo.baseName();

    // Sort all synapses so they can be easily written to .con file.
    // A list of Synapse SectionInfos is saved for each presynaptic cell ID.
    // Presynaptic cell IDs are sorted by a string '<CT>__<REGION>'.
    SortedSynapseSectionPropsMap sectionInfosSortedByCtCol;
    for (SynapseList::ConstIterator it = synapses.constBegin(); it != synapses.constEnd(); ++it) {
        const CIS3D::Synapse& syn = *it;
        const int ctId = network.neurons.getCellTypeId(syn.preNeuronId);
        const QString ctName = network.cellTypes.getName(ctId);
        const int regionId = network.neurons.getRegionId(syn.preNeuronId);
        const QString regionName = network.regions.getName(regionId);

        const QString ctReg = QString("%1__%2").arg(ctName).arg(regionName);

        SortedSynapseSectionPropsMap::Iterator ctRegIt = sectionInfosSortedByCtCol.find(ctReg);
        if (ctRegIt == sectionInfosSortedByCtCol.end()) {
            // Insert new celltype-region string if it does not exist
            ctRegIt = sectionInfosSortedByCtCol.insert(ctReg, QMap<int, SynapseSectionPropsList>());
        }

        QMap<int, SynapseSectionPropsList>& preNeuronsPerCtReg = ctRegIt.value();
        QMap<int, SynapseSectionPropsList>::iterator preIdIt =
            preNeuronsPerCtReg.find(syn.preNeuronId);
        if (preIdIt == preNeuronsPerCtReg.end()) {
            // Insert new presynaptic cell ID for celltype-region if it does not exist
            preIdIt = preNeuronsPerCtReg.insert(syn.preNeuronId, SynapseSectionPropsList());
        }

        SynapseSectionPropsList& synapseSectionPropsList = preIdIt.value();
        SynapseSectionProps info;
        info.sectionID = syn.sectionID;
        info.sectionX = syn.sectionX;
        synapseSectionPropsList.append(info);
    }

    const QString conFileName = outFilePrefix + ".con";
    const QString synFileName = outFilePrefix + ".syn";

    const QString conFileFullPath = outputDir + '/' + conFileName;
    const QString synFileFullPath = outputDir + '/' + synFileName;

    qStdOut() << "[*] Saving " << conFileFullPath << "\n";
    qStdOut() << "[*] Saving " << synFileFullPath << "\n";

    QFile conFile(conFileFullPath);
    QFile synFile(synFileFullPath);

    if (!conFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        const QString msg =
            QString("Error saving .con file. Could not open file %1").arg(conFileFullPath);
        throw std::runtime_error(qPrintable(msg));
    }

    if (!synFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        const QString msg =
            QString("Error saving .syn file. Could not open file %1").arg(synFileFullPath);
        throw std::runtime_error(qPrintable(msg));
    }

    const QChar sep = '\t';

    QTextStream conOut(&conFile);
    conOut << "# Anatomical connectivity realization file. Only valid with realization:\n";
    conOut << "# " << synFileName << "\n";
    conOut << "# Generated by inputmapper on " << QDateTime::currentDateTime().toString(Qt::ISODate)
           << "\n";
    conOut << "# Type - CellID - synapseID\n\n";

    QTextStream synOut(&synFile);
    synOut << "# Synapse distribution file. Only valid with hoc file:\n";
    synOut << "# " << hocFileName << "\n";
    synOut << "# Generated by inputmapper on " << QDateTime::currentDateTime().toString(Qt::ISODate)
           << "\n";
    synOut << "# Type - Section - section.x\n\n";

    for (SortedSynapseSectionPropsMap::ConstIterator it = sectionInfosSortedByCtCol.constBegin();
         it != sectionInfosSortedByCtCol.constEnd(); ++it) {
        const QString& ctReg = it.key();
        const QMap<int, SynapseSectionPropsList>& sortedByPreId = it.value();
        int synIdCon = 0;
        int preIdCon = 0;
        for (QMap<int, SynapseSectionPropsList>::ConstIterator preIdIt = sortedByPreId.constBegin();
             preIdIt != sortedByPreId.constEnd(); ++preIdIt) {
            const SynapseSectionPropsList& synapseSectionPropsList = preIdIt.value();
            for (int i = 0; i < synapseSectionPropsList.size(); ++i) {
                conOut << ctReg << sep << preIdCon << sep << synIdCon << "\n";
                synOut << ctReg << sep << synapseSectionPropsList[i].sectionID << sep
                       << synapseSectionPropsList[i].sectionX << "\n";
                ++synIdCon;
            }
            ++preIdCon;
        }
    }
}

void writeCSVFile(const SynapseList& synapses, const NetworkProps& networkProps,
                  const Morphology& morph, const QString& hocFileName, const QString& outputDir) {
    const QFileInfo hocInfo(hocFileName);
    const QString outFilePrefix = hocInfo.baseName();
    const QString csvFileName = outFilePrefix + ".csv";
    const QString csvFileFullPath = outputDir + '/' + csvFileName;

    QFile csv(csvFileFullPath);
    if (!csv.open(QIODevice::WriteOnly | QIODevice::Text)) {
        const QString msg =
            QString("Error saving .csv file. Could not open file %1").arg(csvFileFullPath);
        throw std::runtime_error(qPrintable(msg));
    }

    const QChar sep = ',';

    qStdOut() << "[*] Saving " << csvFileFullPath << "\n";
    QTextStream out(&csv);
    out << "# Synapse list\n";
    out << "# Generated by inputmapper on " << QDateTime::currentDateTime().toString(Qt::ISODate)
        << "\n";
    out << "# Only valid with hoc file:\n";
    out << "# " << hocInfo.fileName() << "\n\n";
    out << "ID" << sep << "X" << sep << "Y" << sep << "Z" << sep << "Presynaptic cellID" << sep
        << "Presynaptic soma X" << sep << "Presynaptic soma Y" << sep << "Presynaptic soma Z" << sep
        << "Presynaptic cell type" << sep << "Presynaptic region" << sep
        << "Euclidean distance to presynaptic soma" << sep
        << "Distance to postsynaptic soma along morphology" << sep << "SectionID" << sep
        << "SectionName" << sep << "SectionX" << sep << "Structure"
        << "\n";

    for (int s = 0; s < synapses.size(); ++s) {
        const CIS3D::Synapse& syn = synapses[s];
        const int preCtId = networkProps.neurons.getCellTypeId(syn.preNeuronId);
        const QString preCtName = networkProps.cellTypes.getName(preCtId);
        const int preRegionId = networkProps.neurons.getRegionId(syn.preNeuronId);
        const QString preRegionName = networkProps.regions.getName(preRegionId);
        const QString sectionName = morph.getSectionName(syn.sectionID);

        out << syn.id << sep << syn.pos.getX() << sep << syn.pos.getY() << sep << syn.pos.getZ()
            << sep << syn.preNeuronId << sep
            << networkProps.neurons.getSomaPosition(syn.preNeuronId).getX() << sep
            << networkProps.neurons.getSomaPosition(syn.preNeuronId).getY() << sep
            << networkProps.neurons.getSomaPosition(syn.preNeuronId).getZ() << sep << preCtName
            << sep << preRegionName << sep << syn.euclideanDistanceToSomaPre << sep
            << syn.distanceToSomaPost << sep << syn.sectionID << sep << sectionName << sep
            << syn.sectionX << sep << CIS3D::getStructureName(syn.structure) << "\n";
    }
}

void writeCSVSummaryFile(const SynapseList& synapses, const NetworkProps& networkProps,
                         const QString& hocFileName, const QString& outputDir) {
    const QFileInfo hocInfo(hocFileName);
    const QString outFilePrefix = hocInfo.baseName();
    const QString csvFileName = outFilePrefix + "_summary.csv";
    const QString csvFileFullPath = outputDir + '/' + csvFileName;

    QFile csv(csvFileFullPath);
    if (!csv.open(QIODevice::WriteOnly | QIODevice::Text)) {
        const QString msg =
            QString("Error saving summary .csv file. Could not open file %1").arg(csvFileFullPath);
        throw std::runtime_error(qPrintable(msg));
    }

    QMap<int, SynapseStatistics> statsPerCellType;
    QMap<CIS3D::CellTypeRegion, SynapseStatistics> statsPerCellTypeRegion;

    const QList<int> allCellTypeIds = networkProps.cellTypes.getAllCellTypeIds();
    for (int i = 0; i < allCellTypeIds.size(); ++i) {
        statsPerCellType.insert(allCellTypeIds[i], SynapseStatistics());
    }

    for (int s = 0; s < synapses.size(); ++s) {
        const CIS3D::Synapse& syn = synapses[s];
        const int preCellTypeId = networkProps.neurons.getCellTypeId(syn.preNeuronId);
        const int preRegionId = networkProps.neurons.getRegionId(syn.preNeuronId);

        SynapseStatistics& statsCT = statsPerCellType[preCellTypeId];
        statsCT.addSynapse(syn);

        CIS3D::CellTypeRegion ctr(preCellTypeId, preRegionId);
        SynapseStatistics& statsCTR = statsPerCellTypeRegion[ctr];
        statsCTR.addSynapse(syn);
    }

    const QString sep = ",";

    qStdOut() << "[*] Saving " << csvFileFullPath << "\n";
    QTextStream out(&csv);
    out << "# Synapse realization summary\n";
    out << "# Generated by inputmapper on " << QDateTime::currentDateTime().toString(Qt::ISODate)
        << "\n";
    out << "# Only valid with hoc file:\n";
    out << "# " << hocInfo.fileName() << "\n\n";

    QString columnHeaders =
        "Number of synapses" + sep + "Mean path length to soma" + sep + "SD path length to soma" +
        sep + "Connected presynaptic cells" + sep + "Total presynaptic cells" + sep +
        "Convergence" + sep

        + "Number of synapses (apical)" + sep + "Mean path length to soma (apical)" + sep +
        "SD path length to soma (apical)" + sep + "Connected presynaptic cells (apical)" + sep +
        "Convergence (apical)" + sep

        + "Number of synapses (basal)" + sep + "Mean path length to soma (basal)" + sep +
        "SD path length to soma (basal)" + sep + "Connected presynaptic cells (basal)" + sep +
        "Convergence (basal)" + sep

        + "Number of synapses (soma)" + sep + "Mean path length to soma (soma)" + sep +
        "SD path length to soma (soma)" + sep + "Connected presynaptic cells (soma)" + sep +
        "Convergence (soma)" + "\n";

    out << "Presynaptic type" << sep << columnHeaders;

    for (QMap<int, SynapseStatistics>::ConstIterator it = statsPerCellType.constBegin();
         it != statsPerCellType.constEnd(); ++it) {
        const int preCtId = it.key();
        const SynapseStatistics& stats = it.value();

        const QString preCtName = networkProps.cellTypes.getName(preCtId);
        QList<int> cellTypesToSelect;
        cellTypesToSelect.append(preCtId);
        const int totalNumPresynapticNeurons =
            networkProps.neurons
                .getFilteredNeuronIds(cellTypesToSelect, QList<int>(), CIS3D::PRESYNAPTIC)
                .size();

        float convergence = 0.0f;
        float convergenceApical = 0.0f;
        float convergenceBasal = 0.0f;
        float convergenceSoma = 0.0f;

        if (totalNumPresynapticNeurons > 0) {
            convergence =
                float(stats.getNumberOfConnectedNeurons()) / float(totalNumPresynapticNeurons);
            convergenceApical = float(stats.getNumberOfConnectedNeurons(CIS3D::APICAL)) /
                                float(totalNumPresynapticNeurons);
            convergenceBasal = float(stats.getNumberOfConnectedNeurons(CIS3D::DEND)) /
                               float(totalNumPresynapticNeurons);
            convergenceSoma = float(stats.getNumberOfConnectedNeurons(CIS3D::SOMA)) /
                              float(totalNumPresynapticNeurons);
        }

        out << preCtName << sep

            << stats.getNumberOfSynapses() << sep << stats.getDistanceToSomaStatistics().getMean()
            << sep << stats.getDistanceToSomaStatistics().getStandardDeviation() << sep
            << stats.getNumberOfConnectedNeurons() << sep << totalNumPresynapticNeurons << sep
            << convergence << sep

            << stats.getNumberOfSynapses(CIS3D::APICAL) << sep
            << stats.getDistanceToSomaStatistics(CIS3D::APICAL).getMean() << sep
            << stats.getDistanceToSomaStatistics(CIS3D::APICAL).getStandardDeviation() << sep
            << stats.getNumberOfConnectedNeurons(CIS3D::APICAL) << sep << convergenceApical << sep

            << stats.getNumberOfSynapses(CIS3D::DEND) << sep
            << stats.getDistanceToSomaStatistics(CIS3D::DEND).getMean() << sep
            << stats.getDistanceToSomaStatistics(CIS3D::DEND).getStandardDeviation() << sep
            << stats.getNumberOfConnectedNeurons(CIS3D::DEND) << sep << convergenceBasal << sep

            << stats.getNumberOfSynapses(CIS3D::SOMA) << sep
            << stats.getDistanceToSomaStatistics(CIS3D::SOMA).getMean() << sep
            << stats.getDistanceToSomaStatistics(CIS3D::SOMA).getStandardDeviation() << sep
            << stats.getNumberOfConnectedNeurons(CIS3D::SOMA) << sep << convergenceSoma << "\n";
    }

    out << "\n";
    out << "Presynaptic type" << sep << "Presynaptic region" << sep << columnHeaders;

    for (QMap<CIS3D::CellTypeRegion, SynapseStatistics>::ConstIterator it =
             statsPerCellTypeRegion.constBegin();
         it != statsPerCellTypeRegion.constEnd(); ++it) {
        const CIS3D::CellTypeRegion ctr = it.key();
        const SynapseStatistics& stats = it.value();

        const QString preCtName = networkProps.cellTypes.getName(ctr.first);
        const QString preRegionName = networkProps.regions.getName(ctr.second);

        QList<int> cellTypesToSelect;
        cellTypesToSelect.append(ctr.first);

        QList<int> regionsToSelect;
        regionsToSelect.append(ctr.second);

        const int totalNumPresynapticNeurons =
            networkProps.neurons
                .getFilteredNeuronIds(cellTypesToSelect, regionsToSelect, CIS3D::PRESYNAPTIC)
                .size();

        float convergence = 0.0f;
        float convergenceApical = 0.0f;
        float convergenceBasal = 0.0f;
        float convergenceSoma = 0.0f;

        if (totalNumPresynapticNeurons > 0) {
            convergence =
                float(stats.getNumberOfConnectedNeurons()) / float(totalNumPresynapticNeurons);
            convergenceApical = float(stats.getNumberOfConnectedNeurons(CIS3D::APICAL)) /
                                float(totalNumPresynapticNeurons);
            convergenceBasal = float(stats.getNumberOfConnectedNeurons(CIS3D::DEND)) /
                               float(totalNumPresynapticNeurons);
            convergenceSoma = float(stats.getNumberOfConnectedNeurons(CIS3D::SOMA)) /
                              float(totalNumPresynapticNeurons);
        }

        out << preCtName << sep << preRegionName << sep

            << stats.getNumberOfSynapses() << sep << stats.getDistanceToSomaStatistics().getMean()
            << sep << stats.getDistanceToSomaStatistics().getStandardDeviation() << sep
            << stats.getNumberOfConnectedNeurons() << sep << totalNumPresynapticNeurons << sep
            << convergence << sep

            << stats.getNumberOfSynapses(CIS3D::APICAL) << sep
            << stats.getDistanceToSomaStatistics(CIS3D::APICAL).getMean() << sep
            << stats.getDistanceToSomaStatistics(CIS3D::APICAL).getStandardDeviation() << sep
            << stats.getNumberOfConnectedNeurons(CIS3D::APICAL) << sep << convergenceApical << sep

            << stats.getNumberOfSynapses(CIS3D::DEND) << sep
            << stats.getDistanceToSomaStatistics(CIS3D::DEND).getMean() << sep
            << stats.getDistanceToSomaStatistics(CIS3D::DEND).getStandardDeviation() << sep
            << stats.getNumberOfConnectedNeurons(CIS3D::DEND) << sep << convergenceBasal << sep

            << stats.getNumberOfSynapses(CIS3D::SOMA) << sep
            << stats.getDistanceToSomaStatistics(CIS3D::SOMA).getMean() << sep
            << stats.getDistanceToSomaStatistics(CIS3D::SOMA).getStandardDeviation() << sep
            << stats.getNumberOfConnectedNeurons(CIS3D::SOMA) << sep << convergenceSoma << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        printUsage();
        return 1;
    }

    const QTime startTime = QTime::currentTime();

    const QString hoc = argv[1];
    const QString celltype = argv[2];
    const QString dataRoot = argv[3];
    const QString outputDir = argv[4];
    const QDir modelDataDir = CIS3D::getModelDataDir(dataRoot);

    NetworkProps networkProps;
    networkProps.setDataRoot(dataRoot);
    networkProps.loadFilesForInputMapping();

    const float apicalLengthDensityExc =
        networkProps.densities.getLengthDensity(celltype, CIS3D::APICAL, CIS3D::EXCITATORY);
    const float basalLengthDensityExc =
        networkProps.densities.getLengthDensity(celltype, CIS3D::DEND, CIS3D::EXCITATORY);
    const float somaLengthDensityExc =
        networkProps.densities.getLengthDensity(celltype, CIS3D::SOMA, CIS3D::EXCITATORY);

    // const float apicalLengthDensityInh = networkProps.densities.getLengthDensity(celltype,
    // CIS3D::APICAL, CIS3D::INHIBITORY); const float basalLengthDensityInh  =
    // networkProps.densities.getLengthDensity(celltype, CIS3D::DEND,   CIS3D::INHIBITORY); const
    // float somaLengthDensityInh   = networkProps.densities.getLengthDensity(celltype, CIS3D::SOMA,
    // CIS3D::INHIBITORY);

    const float apicalAreaDensityExc =
        networkProps.densities.getAreaDensity(celltype, CIS3D::APICAL, CIS3D::EXCITATORY);
    const float basalAreaDensityExc =
        networkProps.densities.getAreaDensity(celltype, CIS3D::DEND, CIS3D::EXCITATORY);
    const float somaAreaDensityExc =
        networkProps.densities.getAreaDensity(celltype, CIS3D::SOMA, CIS3D::EXCITATORY);

    // const float apicalAreaDensityInh   = networkProps.densities.getAreaDensity  (celltype,
    // CIS3D::APICAL, CIS3D::INHIBITORY); const float basalAreaDensityInh    =
    // networkProps.densities.getAreaDensity  (celltype, CIS3D::DEND,   CIS3D::INHIBITORY); const
    // float somaAreaDensityInh     = networkProps.densities.getAreaDensity  (celltype, CIS3D::SOMA,
    // CIS3D::INHIBITORY);

    const SparseField* normalizedPSTexc =
        SparseField::load(CIS3D::getPSTAllFullPath(modelDataDir, CIS3D::EXCITATORY));
    // const SparseField* normalizedPSTinh = SparseField::load(CIS3D::getPSTAllFullPath(dataRoot,
    // CIS3D::INHIBITORY));

    const Morphology morph(hoc);
    if (morph.getNumberOfSections() == 0) {
        throw std::runtime_error("Morphology has no sections");
    }
    const BoundingBox postBox = morph.getBoundingBox();
    const SparseFieldCoordinates segmentsGridCoords = normalizedPSTexc->getCoordinates();
    const SegmentList segmentList = morph.getGridSegments(*normalizedPSTexc);

    const SparseField apicalLength =
        computeLengthField(segmentList, CIS3D::APICAL, segmentsGridCoords);
    const SparseField apicalArea = computeAreaField(segmentList, CIS3D::APICAL, segmentsGridCoords);
    const SparseField basalLength =
        computeLengthField(segmentList, CIS3D::DEND, segmentsGridCoords);
    const SparseField basalArea = computeAreaField(segmentList, CIS3D::DEND, segmentsGridCoords);
    const SparseField somaLength = computeLengthField(segmentList, CIS3D::SOMA, segmentsGridCoords);
    const SparseField somaArea = computeAreaField(segmentList, CIS3D::SOMA, segmentsGridCoords);

    const SparseField apicalPSTexc =
        apicalLength.multiply(apicalLengthDensityExc) + apicalArea.multiply(apicalAreaDensityExc);
    const SparseField basalPSTexc =
        basalLength.multiply(basalLengthDensityExc) + basalArea.multiply(basalAreaDensityExc);
    const SparseField somaPSTexc =
        somaLength.multiply(somaLengthDensityExc) + somaArea.multiply(somaAreaDensityExc);
    // const SparseField apicalPSTinh = apicalLength.multiply(apicalLengthDensityInh) +
    // apicalArea.multiply(apicalAreaDensityInh); const SparseField basalPSTinh  =
    // basalLength.multiply(basalLengthDensityInh)   + basalArea.multiply(basalAreaDensityInh); const
    // SparseField somaPSTinh   = somaLength.multiply(somaLengthDensityInh)     +
    // somaArea.multiply(somaAreaDensityInh);

    const QList<int> preNeuronIds =
        networkProps.neurons.getFilteredNeuronIds(QList<int>(), QList<int>(), CIS3D::PRESYNAPTIC);

    // Create map: (axonIdToUse)-> [preId] to process all cells with that bouton file to avoid
    // re-reading file multiple times
    QMap<int, QList<int>> idsPerAxonToUse;
    for (int i = 0; i < preNeuronIds.size(); ++i) {
        const int preId = preNeuronIds[i];
        const int axonIdToUse = networkProps.axonRedundancyMap.getNeuronIdToUse(preId);
        QMap<int, QList<int>>::iterator it = idsPerAxonToUse.find(axonIdToUse);
        if (it == idsPerAxonToUse.end()) {
            QList<int> list;
            list.append(preId);
            idsPerAxonToUse.insert(axonIdToUse, list);
        } else {
            it.value().append(preId);
        }
    }

    qStdOut() << "[*] Computing synapses with " << preNeuronIds.size()
              << " presynaptic neurons. Unique: " << idsPerAxonToUse.size() << "\n";

    SynapseList synapses;
    int processed = 0;

    for (QMap<int, QList<int>>::ConstIterator it = idsPerAxonToUse.constBegin();
         it != idsPerAxonToUse.constEnd(); ++it) {
        const int axonIdToUse = it.key();
        const BoundingBox preBox = networkProps.boundingBoxes.getAxonBox(axonIdToUse);
        if (!preBox.intersects(postBox)) {
            processed += it.value().size();
            continue;
        }

        const int preCT = networkProps.neurons.getCellTypeId(axonIdToUse);
        const QString preCTname = networkProps.cellTypes.getName(preCT);

        const SparseField* boutons =
            SparseField::load(CIS3D::getBoutonsFileFullPath(modelDataDir, preCTname, axonIdToUse));
        const bool isExc = networkProps.cellTypes.isExcitatory(preCT);

        if (!isExc) {
            continue;
        }

        const SparseField apicalInnervation =
            computeInnervation(apicalPSTexc, *boutons, *normalizedPSTexc);
        const SparseField basalInnervation =
            computeInnervation(basalPSTexc, *boutons, *normalizedPSTexc);
        const SparseField somaInnervation =
            computeInnervation(somaPSTexc, *boutons, *normalizedPSTexc);

        // const SparseField apicalInnervation = isExc ? computeInnervation(apicalPSTexc, *boutons,
        // *normalizedPSTexc)
        //                                            : computeInnervation(apicalPSTinh, *boutons,
        //                                            *normalizedPSTinh);

        // const SparseField basalInnervation  = isExc ? computeInnervation(basalPSTexc,  *boutons,
        // *normalizedPSTexc)
        //                                            : computeInnervation(basalPSTinh,  *boutons,
        //                                            *normalizedPSTinh);

        // const SparseField somaInnervation   = isExc ? computeInnervation(somaPSTexc,   *boutons,
        // *normalizedPSTexc)
        //                                            : computeInnervation(somaPSTinh,   *boutons,
        //                                            *normalizedPSTinh);

        const QList<int>& preIds = it.value();

        for (QList<int>::ConstIterator idIt = preIds.constBegin(); idIt != preIds.constEnd();
             ++idIt) {
            const int preId = *idIt;
            const Vec3f preSoma = networkProps.neurons.getSomaPosition(preId);
            const SynapseList apicalSyn = generateSynapses(
                segmentList, segmentsGridCoords, apicalInnervation, CIS3D::APICAL, preId, preSoma);
            const SynapseList basalSyn = generateSynapses(
                segmentList, segmentsGridCoords, basalInnervation, CIS3D::DEND, preId, preSoma);
            const SynapseList somaSyn = generateSynapses(
                segmentList, segmentsGridCoords, somaInnervation, CIS3D::SOMA, preId, preSoma);

            synapses.append(apicalSyn);
            synapses.append(basalSyn);
            synapses.append(somaSyn);

            processed += 1;
            if (processed % 100000 == 0) {
                qStdOut() << "    Number of presynaptic cells processed: " << processed
                          << "\tNum synapses: " << synapses.size() << "\n";
            }
        }

        delete boutons;
    }

    qStdOut() << "    Number of presynaptic cells processed: " << processed
              << "\tNum synapses: " << synapses.size() << "\n";

    // Assign synapse IDs
    int synapseId = 0;
    for (int i = 0; i < synapses.size(); ++i) {
        synapses[i].id = synapseId;
        synapseId += 1;
    }

    writeSynConFiles(synapses, networkProps, hoc, outputDir);
    writeCSVFile(synapses, networkProps, morph, hoc, outputDir);
    writeCSVSummaryFile(synapses, networkProps, hoc, outputDir);

    delete normalizedPSTexc;
    // delete normalizedPSTinh;

    const QTime endTime = QTime::currentTime();
    qStdOut() << "[*] Time: " << startTime.secsTo(endTime) << "sec.\n";

    return 0;
}
