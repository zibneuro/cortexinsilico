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


SparseField* loadPSTAll(QString dataRoot, CIS3D::NeuronType functionalType){
    const QString pstAllFile = CIS3D::getPSTAllFullPath(dataRoot, functionalType);
    SparseField* pstAll = SparseField::load(pstAllFile);
    if(pstAll == 0){
        throw std::runtime_error(qPrintable(QString("PSTall file could not be loaded: %1.").arg(pstAllFile)));
    }
    return pstAll;
}


void computeInnervationPost(const QList<int>& preNeurons,
                            const PropsMap& postNeurons,
                            const NetworkProps& networkProps,
                            const QString& dataRoot,
                            const QString& outputDir,
                            const QVector<float>& theta)
{

    SparseField* pstAllExc = loadPSTAll(dataRoot, CIS3D::EXCITATORY);
    SparseField* pstAllInh = loadPSTAll(dataRoot, CIS3D::INHIBITORY);

    qDebug() << "[*] Starting innervation computation";

    const IdsPerCellTypeRegion sortedByPost = Util::sortByCellTypeRegion(postNeurons);
    const QVector<CellTypeRegion> ctrs = sortedByPost.keys().toVector();
    qDebug() << "[*] Completed sorting by celltype-region";

    const QList<int> uniquePreNeuronsList = Util::getUniquePreNeurons(preNeurons, networkProps);
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
        SparseVectorSet vectorSet;

        for (QList<int>::ConstIterator postIt=postNeuronsList.constBegin(); postIt!=postNeuronsList.constEnd(); ++postIt) {
            const int postNeuronId = *postIt;
            const NeuronProps& postProps = postNeurons.value(postNeuronId);
            vectorSet.addVector(postNeuronId);

            for (int pre=0; pre<uniquePreNeuronsList.size(); ++pre) {
                const int preNeuronId = uniquePreNeuronsList[pre];
                const NeuronProps& preProps = uniquePreNeurons.value(preNeuronId);

                if (Util::overlap(preProps, postProps)) {
                    float innervation;
                    if (networkProps.cellTypes.isExcitatory(preProps.cellTypeId)) {
                        const SparseField innervationField = multiplyGenPeter(*(preProps.boutons),
                                                                            *(postProps.pstExc),
                                                                            *pstAllExc,
                                                                            theta[0],
                                                                            theta[1],
                                                                            theta[2],
                                                                            theta[3]);
                        innervation = innervationField.getFieldSum();
                    }
                    else {
                        const SparseField innervationField = multiplyGenPeter(*(preProps.boutons),
                                                                            *(postProps.pstInh),
                                                                            *pstAllExc,
                                                                            theta[0],
                                                                            theta[1],
                                                                            theta[2],
                                                                            theta[3]);
                        innervation = innervationField.getFieldSum();
                    }

                    if (innervation > 0.0f) {
                        vectorSet.setValue(postNeuronId, preNeuronId, innervation);
                    }
                }
            }
        }


        const QString cellTypeName = networkProps.cellTypes.getName(cellTypeRegion.first);
        const QString regionName = networkProps.regions.getName(cellTypeRegion.second);
        const QString fn = CIS3D::getInnervationPostFileName(QDir(outputDir), regionName, cellTypeName);

        numCtrsDone += 1;

        if (SparseVectorSet::save(&vectorSet, fn)) {
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
}


bool specConsistent(const QJsonObject spec){

    const QString dataRoot = spec["DATA_ROOT"].toString();
    if(!QDir(dataRoot).exists()){
        qDebug() << "specFile DATA_ROOT not found: " << dataRoot;
        return false;
    }

    return true;
}


void printUsage() {
    qDebug() << "Usage: ./computeConnectome <specFile>";
}


void copyToOutputDir(const QString dataRoot,
                        const QString outputDir,
                        const QString filename){
    const QString source = QDir::cleanPath(dataRoot + QDir::separator() + filename);
    const QString target = QDir::cleanPath(outputDir + QDir::separator() + filename);
    QDir(outputDir).remove(filename);
    QFile::copy(source,target);
}


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


void writeSummary(const QJsonObject spec){
    QJsonValue generationParameters = spec["GENERATION_PARAMETERS"];
    const QString outputDir = generationParameters.toObject()["OUTPUT_DIR"].toString();
    const QString filePath = QDir::cleanPath(outputDir + QDir::separator() + "summaryStatistics.json");
    QJsonDocument jsonDoc(spec);
    QString jsonString = jsonDoc.toJson();
    QFile file(filePath);

    qDebug() << "[*] Writing summary statistics to " << filePath;

    if(!file.open(QIODevice::WriteOnly)){
        throw std::runtime_error("Failed writing summary statistics.");
    }
    file.write(jsonString.toLocal8Bit());
    file.close();
}


void insertJson(QJsonObject& target, const QJsonObject toInsert, const QString propertyName){
    QJsonValue insertValue(toInsert);
    target.insert(propertyName, insertValue);
}


void copyJson(QJsonObject& target, const QJsonObject toInsert,
    const QString propertyName, const QString targetPropertyName){
    QJsonValue insertValue(toInsert[propertyName]);
    target.insert(targetPropertyName, insertValue);
}


IdList intersectIds(IdList stat, IdList ref){
    IdList intersected;
    for(int i=0; i<stat.size(); i++){
        if(ref.contains(stat[i])){
            intersected.push_back(stat[i]);
        }
    }
    return intersected;
}


void computeStatistics(QJsonObject& spec, const IdList preRef, const IdList postRef) {
    QJsonObject summaryStatistics;
    QJsonArray statisticDefinitions = spec["STATISTIC_DEFINTIONS"].toArray();
    spec.remove("STATISTIC_DEFINTIONS");
    insertJson(summaryStatistics, spec, "GENERATION_PARAMETERS");
    QJsonArray results;

    for(int i=0; i<statisticDefinitions.size(); i++){
        QJsonObject definition = statisticDefinitions[i].toObject();
        definition.insert("DATA_ROOT", spec["DATA_ROOT"].toString());

        NetworkProps networkProps;
        networkProps.setDataRoot(spec["OUTPUT_DIR"].toString());
        networkProps.loadFilesForSynapseComputation();

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

        InnervationStatistic statistic(networkProps);
        statistic.calculate(preNeuronsIntersected, postNeuronsIntersected);

        QJsonObject statReport = statistic.createJson();
        definition.remove("DATA_ROOT");
        const QString statisticType = definition["STATISTIC_TYPE"].toString();
        if(statReport.keys().contains(statisticType)){
            copyJson(definition, statReport, statisticType, "RESULT");
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

    initConectomeDir(spec);

    const QString dataRoot = spec["DATA_ROOT"].toString();
    const QString connectomeDir = spec["OUTPUT_DIR"].toString();

    NetworkProps networkProps;
    networkProps.setDataRoot(dataRoot);
    networkProps.loadFilesForSynapseComputation();

    qDebug() << "[*] Computing presynaptic selection";
    QList<int> preNeurons = UtilIO::getPreSynapticNeurons(spec, networkProps);
    qDebug() << "[*] Computing postsynaptic selection";
    PropsMap postNeurons = UtilIO::getPostSynapticNeurons(spec, networkProps, false);

    qDebug() << "[*] Start processing " << preNeurons.size() << " presynaptic and " << postNeurons.size() << " postsynaptic neurons.";

    computeInnervationPost(preNeurons, postNeurons, networkProps, dataRoot, connectomeDir, extractRuleParameters(spec));

    qDebug() << "[*] Computing summary statistics";

    QJsonObject summaryStatistics(spec);
    computeStatistics(summaryStatistics, preNeurons, postNeurons.keys());

    // Clean up
    for (PropsMap::Iterator postIt=postNeurons.begin(); postIt!=postNeurons.end(); ++postIt) {
        NeuronProps& props = postIt.value();
        if (props.pstExc) delete props.pstExc;
        if (props.pstInh) delete props.pstInh;
    }

    return 0;
}
