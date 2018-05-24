/*
    This tool computes the innervation matrix based on the generalized dense
    Peters' rule (parametrized by theta). Also extracts summary statistics.
    Usage:

    ./networkSimulator <specFile>

    <specFile>   The specification file containing;
                    - The model data directoy
                    - The connectivity rule parameters (theta)
                    - The definition of the summary statistics
*/

#include <QtCore>
#include <QDebug>
#include <QScopedPointer>
#include <QPair>
#include <QHash>
#include <QtAlgorithms>
#include <random>
#include <omp.h>

#include "CIS3DAxonRedundancyMap.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DSparseField.h"
#include "CIS3DSparseVectorSet.h"
#include "CIS3DVec3.h"
#include "CIS3DBoundingBoxes.h"
#include "CIS3DCellTypes.h"
#include "CIS3DNeurons.h"
#include "CIS3DRegions.h"
#include "CIS3DNetworkProps.h"
#include "Typedefs.h"
#include "Util.h"
#include "UtilIO.h"
#include "InnervationStatistic.h"
#include "Histogram.h"
#include "SparseVectorCache.h"

// Loads the postsynaptic target density field
SparseField* loadPSTAll(QString dataRoot, CIS3D::NeuronType functionalType){
    const QString pstAllFile = CIS3D::getPSTAllFullPath(dataRoot, functionalType);
    SparseField* pstAll = SparseField::load(pstAllFile);
    if(pstAll == 0){
        throw std::runtime_error(qPrintable(QString("PSTall file could not be loaded: %1.").arg(pstAllFile)));
    }
    return pstAll;
}

// Computes the innervation matrix by interating over the pre- and postsynaptic
// neurons and applying the connectivity rule parameters (theta).
void computeInnervationPost(const QList<int>& preNeurons,
                            const PropsMap& postNeurons,
                            SparseVectorCache& cache,
                            const NetworkProps& networkProps,
                            const QString& dataRoot,
                            const QString& outputDir,
                            const QVector<float>& theta)
{
    Histogram histo;
    SparseField* pstAllExc = loadPSTAll(dataRoot, CIS3D::EXCITATORY);
    SparseField* pstAllInh = loadPSTAll(dataRoot, CIS3D::INHIBITORY);

    qDebug() << "[*] Starting innervation computation";

    const IdsPerCellTypeRegion sortedByPost = Util::sortByCellTypeRegion(postNeurons);
    const QVector<CellTypeRegion> ctrs = sortedByPost.keys().toVector();

    qDebug() << "[*] Completed sorting by celltype-region";

    QSet<int> mappedIds;
    for (int i=0; i<preNeurons.size(); ++i) {
        const int preId = preNeurons[i];
        const int mappedId = networkProps.axonRedundancyMap.getNeuronIdToUse(preId);
        mappedIds.insert(mappedId);
    }
    QList<int> uniquePreNeuronsList = mappedIds.toList();

    PropsMap uniquePreNeurons;

    qDebug() << "[*] Start reading properties for" << uniquePreNeuronsList.size() << "unique presynaptic neurons";
    for (int i=0; i<uniquePreNeuronsList.size(); ++i) {
        NeuronProps props;
        props.id = uniquePreNeuronsList[i];
        props.mappedAxonId = networkProps.axonRedundancyMap.getNeuronIdToUse(props.id);
        props.boundingBox = networkProps.boundingBoxes.getAxonBox(props.id);
        if (props.boundingBox.isEmpty()) {
            throw std::runtime_error(qPrintable(QString("Empty axon bounding box for neuron %1.").arg(props.id)));
        }
        props.cellTypeId = networkProps.neurons.getCellTypeId(props.id);
        props.regionId = networkProps.neurons.getRegionId(props.id);
        props.somaPos = networkProps.neurons.getSomaPosition(props.id);

        const QString ctName = networkProps.cellTypes.getName(props.cellTypeId);
        const QString filePath = CIS3D::getBoutonsFileFullPath(dataRoot, ctName, props.mappedAxonId);
        props.boutons = SparseField::load(filePath);
        uniquePreNeurons.insert(props.id, props);

        if ((i % 1000 == 0) && (i > 0)) {
            qDebug() << "    Read properties for" << i << "unique presynaptic neurons";
        }
    }
    qDebug() << "[*] Completed reading properties for" << uniquePreNeuronsList.size() << "unique presynaptic neurons";

    long int numCtrsDone = 0;
    const long int totalNumCtrs = ctrs.size();

    for (int i=0; i<ctrs.size(); ++i) {
        const CellTypeRegion cellTypeRegion = ctrs[i];
        QList<int> postNeuronsList = sortedByPost.value(cellTypeRegion);
        SparseVectorSet* vectorSet = new SparseVectorSet();

        for (QList<int>::ConstIterator postIt=postNeuronsList.constBegin(); postIt!=postNeuronsList.constEnd(); ++postIt) {
            const int postNeuronId = *postIt;
            const NeuronProps& postProps = postNeurons.value(postNeuronId);
            //const NeuronProps& postPropsNormalized = postNeuronsNormalized.value(postNeuronId);
            vectorSet->addVector(postNeuronId);

            for (int pre=0; pre<uniquePreNeuronsList.size(); ++pre) {
                const int preNeuronId = uniquePreNeuronsList[pre];
                const NeuronProps& preProps = uniquePreNeurons.value(preNeuronId);
                if (Util::overlap(preProps, postProps)) {
                    float innervation;
                    //float innervationNormalized;
                    if (networkProps.cellTypes.isExcitatory(preProps.cellTypeId)) {
                        //const SparseField innervationFieldNormalized = multiply(*(preProps.boutons), *(postPropsNormalized.pstExc));
                        const SparseField innervationField = multiplyGenPeter(*(preProps.boutons),
                                                                            *(postProps.pstExc),
                                                                            *pstAllExc,
                                                                            theta[0],
                                                                            theta[1],
                                                                            theta[2],
                                                                            theta[3]);
                        innervation = innervationField.getFieldSum();
                        //innervationNormalized = innervationFieldNormalized.getFieldSum();
                        histo.addValue(innervation);
                        //if(innervation - innervationNormalized > 0.000001){
                        //    qDebug() << innervation - innervationNormalized;
                        //}
                    }
                    else {
                        qDebug() << "inhibitory";
                        const SparseField innervationField = multiplyGenPeter(*(preProps.boutons),
                                                                            *(postProps.pstInh),
                                                                            *pstAllInh,
                                                                            theta[0],
                                                                            theta[1],
                                                                            theta[2],
                                                                            theta[3]);
                        innervation = innervationField.getFieldSum();
                    }
                    if (innervation > 0.0f) {
                        //qDebug() << preNeuronId << " " << postNeuronId << " " << innervation;
                        vectorSet->setValue(postNeuronId, preNeuronId, innervation);
                    }
                }
            }
        }


        const QString cellTypeName = networkProps.cellTypes.getName(cellTypeRegion.first);
        const QString regionName = networkProps.regions.getName(cellTypeRegion.second);
        const QString fn = CIS3D::getInnervationPostFileName(QDir(outputDir), regionName, cellTypeName);

        numCtrsDone += 1;

        cache.add(fn,vectorSet);
        if (SparseVectorSet::save(vectorSet, fn)) {
            qDebug() << "[*] CellType-Region" << numCtrsDone << "/" << totalNumCtrs
                     << "\tSaved" << fn
                     << "\tNum postsyn neurons:" <<  postNeuronsList.size();
        }
        else {
            qDebug() << "Cannot save innervation file" << fn;
        }
    }

    for (PropsMap::Iterator preIt=uniquePreNeurons.begin(); preIt!=uniquePreNeurons.end(); ++preIt) {
        NeuronProps& props = preIt.value();
        if (props.boutons) delete props.boutons;
    }

    delete pstAllExc;
    delete pstAllInh;

    //qDebug() << histo.getNumberOfValues() << " " << histo.getNumberOfZeros() << " " << histo.getAverage();
}

// Checks that the specifcation file has the right format.
bool specConsistent(const QJsonObject spec){
    const QString dataRoot = spec["DATA_ROOT"].toString();
    if(!QDir(dataRoot).exists()){
        qDebug() << "specFile DATA_ROOT not found: " << dataRoot;
        return false;
    }
    return true;
}
\

void printUsage() {
    qDebug() << "Usage: ./networkSimulator <specFile>";
}

// Copies the model data meta files to the output dir, which contains the
// computed innervation matrix.
void copyToOutputDir(const QString dataRoot,
                        const QString outputDir,
                        const QString filename){
    const QString source = QDir::cleanPath(dataRoot + QDir::separator() + filename);
    const QString target = QDir::cleanPath(outputDir + QDir::separator() + filename);
    QDir(outputDir).remove(filename);
    QFile::copy(source,target);
}

// Initializes the output directory to which the innervation matrix and the
// summary statistics are written.
void initConectomeDir(const QJsonObject spec){
    const QString dataRoot = spec["DATA_ROOT"].toString();
    const QString outputDir = spec["OUTPUT_DIR"].toString();

    if(!QDir(outputDir).exists()){
        qDebug() << "[*] Creating output directory " << outputDir;
        QDir().mkpath(outputDir);
    }

    const QString innervationPostDir = QDir::cleanPath(outputDir + QDir::separator() + "InnervationPost");
    qDebug() << "[*] Initializing output directory";
    QDir(innervationPostDir).removeRecursively();

    copyToOutputDir(dataRoot, outputDir, "AxonRedundancyMap.dat");
    copyToOutputDir(dataRoot, outputDir, "BoundingBoxes.csv");
    copyToOutputDir(dataRoot, outputDir, "CellTypes.csv");
    copyToOutputDir(dataRoot, outputDir, "Neurons.csv");
    copyToOutputDir(dataRoot, outputDir, "Regions.csv");
}

// Writes the summary statistics into a JSON file.
void writeSummary(const QJsonObject spec){
    QJsonValue generationParameters = spec["GENERATION_PARAMETERS"];
    const QString outputDir = generationParameters.toObject()["OUTPUT_DIR"].toString();
    const QString filePath = QDir::cleanPath(outputDir + QDir::separator() + "summaryStatistics.json");
    QJsonDocument jsonDoc(spec);
    QString jsonString = jsonDoc.toJson();
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly)){
        throw std::runtime_error("Failed writing summary statistics.");
    }
    file.write(jsonString.toLocal8Bit());
    file.close();

    qDebug() << "[*] Written summary statistics to " << filePath;
}

void insertJson(QJsonObject& target, const QJsonObject toInsert, const QString propertyName){
    // Appends a JSON object as property to an existing JSON object.
    QJsonValue insertValue(toInsert);
    target.insert(propertyName, insertValue);
}

// Appends a JSON object as property to an existing JSON object, setting a
// specific property name.
void copyJson(QJsonObject& target, const QJsonObject toInsert,
    const QString propertyName, const QString targetPropertyName){
    QJsonValue insertValue(toInsert[propertyName]);
    target.insert(targetPropertyName, insertValue);
}

// Determines the intersection of two neuron ID lists.
IdList intersectIds(IdList stat, IdList ref){
    IdList intersected;
    for(int i=0; i<stat.size(); i++){
        if(ref.contains(stat[i])){
            intersected.push_back(stat[i]);
        }
    }
    return intersected;
}

// Computes the specified summary statistics.
void computeStatistics(QJsonObject& spec, const IdList preRef, const IdList postRef,
    const SparseVectorCache& cache, NetworkProps& networkProps) {
    QJsonObject summaryStatistics;
    QJsonArray statisticDefinitions = spec["STATISTIC_DEFINTIONS"].toArray();
    spec.remove("STATISTIC_DEFINTIONS");
    insertJson(summaryStatistics, spec, "GENERATION_PARAMETERS");
    QJsonArray results;

    networkProps.setDataRoot(spec["OUTPUT_DIR"].toString());

    for(int i=0; i<statisticDefinitions.size(); i++){
        QJsonObject definition = statisticDefinitions[i].toObject();
        definition.insert("DATA_ROOT", spec["DATA_ROOT"].toString());

        IdList preNeurons = UtilIO::getPreSynapticNeurons(definition, networkProps);
        IdList postNeurons = UtilIO::getPostSynapticNeuronIds(definition, networkProps);

        IdList preNeuronsIntersected = intersectIds(preNeurons, preRef);
        IdList postNeuronsIntersected = intersectIds(postNeurons, postRef);

        qDebug() << "[*] Computing statistic: " << definition["STATISTIC_NAME"].toString();

        if(preNeuronsIntersected.size() < preNeurons.size() ||
            postNeuronsIntersected.size() < postNeurons.size()){
            definition.insert("STATUS","WARNING statistic filter yielded more neurons than generation filter. Discarded additional neurons.");
        } else if (preNeuronsIntersected.size() == 0 || postNeuronsIntersected.size() == 0){
            definition.insert("STATUS","ERROR statistic filter yielded no neurons (compare with generation filter).");
            continue;
        }

        InnervationStatistic statistic(networkProps, cache);
        statistic.calculate(preNeuronsIntersected, postNeuronsIntersected);

        QJsonObject statReport = statistic.createJson();
        definition.remove("DATA_ROOT");
        const QString statisticType = definition["STATISTIC_TYPE"].toString();
        if(statReport.keys().contains(statisticType)){
            copyJson(definition, statReport, statisticType, "RESULT");
            definition.insert("PRE_NEURON_NUMBER", preNeuronsIntersected.size());
            definition.insert("POST_NEURON_NUMBER", postNeuronsIntersected.size());
            if(!definition.keys().contains("STATUS")){
                definition.insert("STATUS","OK");
            }
        } else {
            definition.insert("STATUS","ERROR statistic type unknown.");
        }
        results.push_back(definition);
    }
    summaryStatistics.insert("SUMMARY_STATISTICS", results);
    writeSummary(summaryStatistics);
}

// Extracts the theta-parameters from the specification file.
QVector<float> extractRuleParameters(const QJsonObject spec){
    QVector<float> theta;
    QJsonArray parameters = spec["CONNECTIVITY_RULE_PARAMETERS"].toArray();
    theta.append((float)parameters[0].toDouble());
    theta.append((float)parameters[1].toDouble());
    theta.append((float)parameters[2].toDouble());
    theta.append((float)parameters[3].toDouble());
    return theta;
}


int main(int argc, char ** argv)
{
    if (argc != 2) {
        printUsage();
        return 1;
    }

    const QString specFile = argv[1];
    qDebug() << specFile;
    const QJsonObject spec = UtilIO::parseSpecFile(specFile);

    if(!specConsistent(spec)){
        return 1;
    }

    // Initialize output directory.
    initConectomeDir(spec);

    // Load model data.
    const QString dataRoot = spec["DATA_ROOT"].toString();
    const QString connectomeDir = spec["OUTPUT_DIR"].toString();
    NetworkProps networkProps;
    networkProps.setDataRoot(dataRoot);
    networkProps.loadFilesForSynapseComputation();

    SparseVectorCache cache;

    qDebug() << "[*] Computing presynaptic selection";
    QList<int> preNeurons = UtilIO::getPreSynapticNeurons(spec, networkProps);
    qDebug() << "[*] Computing postsynaptic selection";
    PropsMap postNeurons = UtilIO::getPostSynapticNeurons(spec, networkProps, false);
    //PropsMap postNeuronsNormalized = UtilIO::getPostSynapticNeurons(spec, networkProps, true);

    qDebug() << "[*] Start processing " << preNeurons.size() << " presynaptic and " << postNeurons.size() << " postsynaptic neurons.";
    computeInnervationPost(preNeurons, postNeurons, cache, networkProps, dataRoot, connectomeDir, extractRuleParameters(spec));
    qDebug() << "[*] Computing summary statistics";

    QJsonObject summaryStatistics(spec);
    computeStatistics(summaryStatistics, preNeurons, postNeurons.keys(), cache, networkProps);

    // Clean up
    cache.clear();
    for (PropsMap::Iterator postIt=postNeurons.begin(); postIt!=postNeurons.end(); ++postIt) {
        NeuronProps& props = postIt.value();
        if (props.pstExc) delete props.pstExc;
        if (props.pstInh) delete props.pstInh;
    }

    return 0;
}
