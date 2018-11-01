/*
    This tool computes the innervation matrix. Usage:

    ./computeStatistic INNERVATION <spec_file>

    <spec_file> Specification file that contains the data directory path
                and filter definitions that determine which segments of
                the innervation matrix are computed.

    Note: Currently, only innervation values are computed, not explicit
    synapse locations.
*/

#include <omp.h>
#include <QDebug>
#include <QHash>
#include <QPair>
#include <QScopedPointer>
#include <QtAlgorithms>
#include <QtCore>
#include <QString>
#include <random>

#include "CIS3DAxonRedundancyMap.h"
#include "CIS3DBoundingBoxes.h"
#include "CIS3DCellTypes.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DNetworkProps.h"
#include "CIS3DNeurons.h"
#include "CIS3DRegions.h"
#include "CIS3DSparseField.h"
#include "CIS3DSparseVectorSet.h"
#include "CIS3DVec3.h"
#include "Typedefs.h"
#include "Util.h"
#include "UtilIO.h"

enum FileType
{
    BOUTON_FILE,
    NORMALIZED_PST_EXC_FILE,
    NORMALIZED_PST_INH_FILE
};

void
printErrorAndExit(const std::runtime_error& e)
{
    qDebug() << QString(e.what());
    qDebug() << "Aborting.";
    exit(1);
}

void
printUsage()
{
    qDebug() << "Usage: ./computeSynapses INNERVATION <specfile>";
    qDebug() << "Usage: ./computeSynapses INNERVATION_SINGLE <dataRoot> <preId> <postId>";
}

void
computeSingle(const NetworkProps& networkProps, int preId, int postId)
{
    QDir modelDataDir = CIS3D::getModelDataDir(networkProps.dataRoot);

    int preCellTypeId = networkProps.neurons.getCellTypeId(preId);
    QString preCellTypeName = networkProps.cellTypes.getName(preCellTypeId);
    QString preFilePath =
        CIS3D::getBoutonsFileFullPath(modelDataDir, preCellTypeName, preId);

    int postCellTypeId = networkProps.neurons.getCellTypeId(postId);
    int postRegionId = networkProps.neurons.getRegionId(postId);
    QString postCellTypeName = networkProps.cellTypes.getName(postCellTypeId);
    QString postRegionName = networkProps.regions.getName(postRegionId);
    QString postFilePath = CIS3D::getNormalizedPSTFileFullPath(
        modelDataDir, postRegionName, postCellTypeName, postId, CIS3D::EXCITATORY);

    SparseField* preField = SparseField::load(preFilePath);
    SparseField* postField = SparseField::load(postFilePath);

    qDebug() << "BB from file";
    BoundingBox preBoxFile = networkProps.boundingBoxes.getAxonBox(preId);
    preBoxFile.print();
    BoundingBox postBoxFile = networkProps.boundingBoxes.getDendriteBox(postId);
    postBoxFile.print();

    qDebug() << "BB from field";
    BoundingBox preBox = preField->getNonZeroBox();
    preBox.print();
    BoundingBox postBox = postField->getNonZeroBox();
    postBox.print();
    qDebug() << preBox.intersects(postBox);

    const SparseField innervationField =
        multiply(*preField, *postField);
    float innervation = innervationField.getFieldSum();

    qDebug() << preId << postId << innervation;
}

// Performs the actual computation by iterating over the pre- and postsynaptic
// neurons.
void
computeInnervationPost(const QList<int>& preNeurons, const PropsMap& postNeurons, const NetworkProps& networkProps, const QString& dataRoot, const QString& outputDir)
{
    qDebug() << "[*] Starting innervation computation";

    QTime startTime = QTime::currentTime();
    // QReadWriteLock lock;

    const IdsPerCellTypeRegion sortedByPost = Util::sortByCellTypeRegion(postNeurons);
    const QVector<CellTypeRegion> ctrs = sortedByPost.keys().toVector();
    qDebug() << "[*] Completed sorting by celltype-region";

    const QList<int> uniquePreNeuronsList = Util::getUniquePreNeurons(preNeurons, networkProps);
    PropsMap uniquePreNeurons;

    const QDir modelDataDir = CIS3D::getModelDataDir(dataRoot);

    // Map presynpatic neuron IDs taking advantage of the duplicated axon
    // morphologies.
    qDebug() << "[*] Start reading properties for" << uniquePreNeuronsList.size()
             << "unique presynaptic neurons";
    for (int i = 0; i < uniquePreNeuronsList.size(); ++i)
    {
        NeuronProps props;
        props.id = uniquePreNeuronsList[i];
        props.mappedAxonId = networkProps.axonRedundancyMap.getNeuronIdToUse(props.id);
        props.boundingBox = networkProps.boundingBoxes.getAxonBox(props.id);
        if (props.boundingBox.isEmpty())
        {
            throw std::runtime_error(
                qPrintable(QString("Empty axon bounding box for neuron %1.").arg(props.id)));
        }
        props.cellTypeId = networkProps.neurons.getCellTypeId(props.id);
        props.regionId = networkProps.neurons.getRegionId(props.id);
        props.somaPos = networkProps.neurons.getSomaPosition(props.id);

        const QString ctName = networkProps.cellTypes.getName(props.cellTypeId);
        const QString filePath =
            CIS3D::getBoutonsFileFullPath(modelDataDir, ctName, props.mappedAxonId);
        props.boutons = SparseField::load(filePath);
        uniquePreNeurons.insert(props.id, props);

        if (i % 1000 == 0)
        {
            qDebug() << "    Read properties for" << i << "unique presynaptic neurons";
        }
    }
    qDebug() << "[*] Completed reading properties for" << uniquePreNeuronsList.size()
             << "unique presynaptic neurons";

    long int numCtrsDone = 0;
    const long int totalNumCtrs = ctrs.size();

    //#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < ctrs.size(); ++i)
    {
        const CellTypeRegion cellTypeRegion = ctrs[i];
        QList<int> postNeuronsList = sortedByPost.value(cellTypeRegion);
        // qSort(postNeuronsList);
        SparseVectorSet vectorSet;

        for (QList<int>::ConstIterator postIt = postNeuronsList.constBegin();
             postIt != postNeuronsList.constEnd();
             ++postIt)
        {
            const int postNeuronId = *postIt;
            const NeuronProps& postProps = postNeurons.value(postNeuronId);
            vectorSet.addVector(postNeuronId);

            for (int pre = 0; pre < uniquePreNeuronsList.size(); ++pre)
            {
                const int preNeuronId = uniquePreNeuronsList[pre];
                const NeuronProps& preProps = uniquePreNeurons.value(preNeuronId);

                if (Util::overlap(preProps, postProps))
                {
                    float innervation;
                    if (networkProps.cellTypes.isExcitatory(preProps.cellTypeId))
                    {
                        const SparseField innervationField =
                            multiply(*(preProps.boutons), *(postProps.pstExc));
                        innervation = innervationField.getFieldSum();
                    }
                    else
                    {
                        const SparseField innervationField =
                            multiply(*(preProps.boutons), *(postProps.pstInh));
                        innervation = innervationField.getFieldSum();
                    }

                    if (innervation > 0.0f)
                    {
                        vectorSet.setValue(postNeuronId, preNeuronId, innervation);
                    }
                }
            }
        }

        const QString cellTypeName = networkProps.cellTypes.getName(cellTypeRegion.first);
        const QString regionName = networkProps.regions.getName(cellTypeRegion.second);
        const QString fn =
            CIS3D::getInnervationPostFileName(QDir(outputDir), regionName, cellTypeName);

        // QWriteLocker locker(&lock);

        //#pragma omp atomic
        numCtrsDone += 1;

        if (SparseVectorSet::save(&vectorSet, fn))
        {
            qDebug() << "[*] CellType-Region" << numCtrsDone << "/" << totalNumCtrs << "\tSaved"
                     << fn << "\tNum postsyn neurons:" << postNeuronsList.size();
        }
        else
        {
            qDebug() << "Cannot save innervation file" << fn;
        }
    }

    for (PropsMap::Iterator preIt = uniquePreNeurons.begin(); preIt != uniquePreNeurons.end();
         ++preIt)
    {
        NeuronProps& props = preIt.value();
        if (props.boutons)
            delete props.boutons;
    }

    QTime endTime = QTime::currentTime();
    qDebug() << "Time: " << startTime.secsTo(endTime) << " sec. ("
             << startTime.secsTo(endTime) / 60. << " min.)";
}

int
main(int argc, char* argv[])
{
    if (argc != 3 && argc != 5)
    {
        printUsage();
        return 1;
    }

    const QString computeType = argv[1];
    if (computeType == "INNERVATION")
    {
        const QString specFile = argv[2];
        const QJsonObject spec = UtilIO::parseSpecFile(specFile);
        const QString dataRoot = spec["DATA_ROOT"].toString();
        const QString outputDir = spec["OUTPUT_DIR"].toString();

        NetworkProps networkProps;
        networkProps.setDataRoot(dataRoot);
        networkProps.loadFilesForSynapseComputation();

        qDebug() << "[*] Computing presynaptic selection";
        QList<int> preNeurons = UtilIO::getPreSynapticNeurons(spec, networkProps);
        qDebug() << "[*] Computing postsynaptic selection";
        PropsMap postNeurons = UtilIO::getPostSynapticNeurons(spec, networkProps);

        qDebug() << "[*] Start processing " << preNeurons.size() << " presynaptic and "
                 << postNeurons.size() << " postsynaptic neurons.";

        computeInnervationPost(preNeurons, postNeurons, networkProps, dataRoot, outputDir);

        // Clean up
        for (PropsMap::Iterator postIt = postNeurons.begin(); postIt != postNeurons.end(); ++postIt)
        {
            NeuronProps& props = postIt.value();
            if (props.pstExc)
                delete props.pstExc;
            if (props.pstInh)
                delete props.pstInh;
        }
    }
    else if (computeType == "INNERVATION_SINGLE")
    {
        const QString dataRoot = argv[2];
        int preId = atoi(argv[3]);
        int postId = atoi(argv[4]);

        NetworkProps networkProps;
        networkProps.setDataRoot(dataRoot);
        networkProps.loadFilesForSynapseComputation();

        computeSingle(networkProps, preId, postId);
    }
    else
    {
        printUsage();
        return 1;
    }

    return 0;
}
