/*
    This tool provides two modes: SYNAPSE and SUBCUBE.

    In the SYNAPSE mode, the tool computes synapse counts between neurons
    according to generalized Peters' rule, which is parametrized by theta. To do so,
    the tool requires neuron features that must be provided in file features.csv
    (in the same directory).

    In the SUBCUBE mode, the tool creates a features.csv file by extracting a
    sucube from the complete model.

    Usage:

    ./networkSimulator SYNAPSE <synapseSpecFile>
    ./networkSimulator SUBCUBE <voxelSpecFile>

    The <synapseSpecFile> contains the theta parameters for Peter's rule.

    The <voxelSpecFile> contains the model data directory and the origin and
    size of the subcube to be extracted from the model data.
*/

#include <omp.h>
#include <QDebug>
#include <QHash>
#include <QPair>
#include <QScopedPointer>
#include <QtAlgorithms>
#include <QtCore>
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
#include "FeatureExtractor.h"
#include "FeatureReader.h"
#include "Histogram.h"
#include "InnervationStatistic.h"
#include "NeuronSelection.h"
#include "SparseFieldCalculator.h"
#include "SparseVectorCache.h"
#include "SynapseDistributor.h"
#include "SynapseWriter.h"
#include "Typedefs.h"
#include "Util.h"
#include "UtilIO.h"

/*
    Prints the usage manual to the console.
*/
void printUsage() {
    qDebug() << "This tool provides two modes: SYNAPSE and SUBCUBE.";
    qDebug() << "";
    qDebug() << "In the SYNAPSE mode, the tool computes synapse counts between neurons";
    qDebug() << "according to generalized Peters' rule, which is parametrized by theta. To do so,";
    qDebug() << "the tool requires neuron features that must be provided in file features.csv";
    qDebug() << "(in the same directory).";
    qDebug() << "";
    qDebug() << "In the SUBCUBE mode, the tool creates a features.csv file by extracting a";
    qDebug() << "sucube from the complete model.";
    qDebug() << "";
    qDebug() << "Usage:";
    qDebug() << "";
    qDebug() << "./networkSimulator SYNAPSE <synapseSpecFile>";
    qDebug() << "./networkSimulator SUBCUBE <voxelSpecFile>";
    qDebug() << "";
    qDebug() << "The <synapseSpecFile> contains the theta parameters for Peter's rule.";
    qDebug() << "";
    qDebug() << "The <voxelSpecFile> contains the model data directory and the origin and";
    qDebug() << "size of the subcube to be extracted from the model data.";
}

/*
    Extracts the THETA parameters from the spec file.

    @param spec The specification file as json object.
    @return A vector with the theta parameters.
*/
QVector<float> extractRuleParameters(const QJsonObject spec) {
    QVector<float> theta;
    QJsonArray parameters = spec["CONNECTIVITY_RULE_PARAMETERS"].toArray();
    theta.append((float)parameters[0].toDouble());
    theta.append((float)parameters[1].toDouble());
    theta.append((float)parameters[2].toDouble());
    theta.append((float)parameters[3].toDouble());
    return theta;
}

/*
    Reads the VOXEL_ORIGIN property from the specification file.

    @param spec The specification file as json object.
    @return The origin (x,y,z) as vector.
    @throws runtime_error if the VOXEL_ORIGIN property is not found.
*/
QVector<float> extractOrigin(const QJsonObject spec) {
    if (spec["VOXEL_ORIGIN"] == QJsonValue::Undefined) {
        throw std::runtime_error("Key VOXEL_ORIGIN not found in spec file.");
    };
    QVector<float> origin;
    QJsonArray parameters = spec["VOXEL_ORIGIN"].toArray();
    origin.append((float)parameters[0].toDouble());
    origin.append((float)parameters[1].toDouble());
    origin.append((float)parameters[2].toDouble());
    return origin;
}

/*
    Reads the VOXEL_DIMENSIONS property from the specification file.

    @param spec The specification file as json object.
    @return The number of voxels in each direction (nx,ny,nz) as vector.
    @throws runtime_error if the VOXEL_DIMENSIONS property is not found.
*/
QVector<int> extractDimensions(const QJsonObject spec) {
    if (spec["VOXEL_DIMENSIONS"] == QJsonValue::Undefined) {
        throw std::runtime_error("Key VOXEL_DIMENSIONS not found in spec file.");
    };
    QVector<int> origin;
    QJsonArray parameters = spec["VOXEL_DIMENSIONS"].toArray();
    origin.append((int)parameters[0].toDouble());
    origin.append((int)parameters[1].toDouble());
    origin.append((int)parameters[2].toDouble());
    return origin;
}

/*
    Reads the CELLTYPES property from the specification file.

    @param spec The specification file as json object.
    @return The cell type names as set.
    @throws runtime_error if the CELLTYPES property is not found.
*/
QSet<QString> extractCellTypes(const QJsonObject spec) {
    if (spec["CELLTYPES"] == QJsonValue::Undefined) {
        throw std::runtime_error("Key CELLTYPES not found in spec file.");
    };
    QSet<QString> cellTypes;
    QJsonArray parameters = spec["CELLTYPES"].toArray();
    for (int i = 0; i < parameters.size(); i++) {
        const QString cellType = parameters[i].toString();
        cellTypes.insert(cellType);
    }
    return cellTypes;
}

/*
    Reads the REGIONS property from the specification file.

    @param spec The specification file as json object.
    @return The region names as set.
    @throws runtime_error if the REGIONS property is not found.
*/
QSet<QString> extractRegions(const QJsonObject spec) {
    if (spec["REGIONS"] == QJsonValue::Undefined) {
        throw std::runtime_error("Key REGIONS not found in spec file.");
    };
    QSet<QString> regions;
    QJsonArray parameters = spec["REGIONS"].toArray();
    for (int i = 0; i < parameters.size(); i++) {
        const QString region = parameters[i].toString();
        regions.insert(region);
    }
    return regions;
}

/*
    Reads the NEURON_IDS property from the specification file.

    @param spec The specification file as json object.
    @return The NEURON_IDS as set, empty by default.
*/
QSet<int> extractNeuronIds(const QJsonObject spec) {
    QSet<int> neuronIds;
    if (spec["NEURON_IDS"] != QJsonValue::Undefined) {
        QJsonArray parameters = spec["NEURON_IDS"].toArray();
        for (int i = 0; i < parameters.size(); i++) {
            const int neuronId = parameters[i].toInt();
            neuronIds.insert(neuronId);
        }
    };
    return neuronIds;
}

/*
    Reads the SAMPLING_FACTOR property from the specification file.

    @param spec The specification file as json object.
    @return The SAMPLING_FACTOR, 1 by default.
    @throws runtime_error if the smapling factor has an invalid value.
*/
int extractSamplingFactor(const QJsonObject spec) {
    if (spec["SAMPLING_FACTOR"] != QJsonValue::Undefined) {        
        int samplingFactor = spec["SAMPLING_FACTOR"].toInt();
        if(samplingFactor == 0){
            throw std::runtime_error("Invalid value for sampling factor. Possible reason: \"-symbols.");
        } else {
            return samplingFactor;
        }
    } else {
        return 1;
    }
}

/*
    Entry point for the console application.

    @param argc Number of arguments.
    @param argv Value of arguments.
    @return 0 if successfull, 1 in case of invalid arguments.
*/
int main(int argc, char **argv) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    const QString mode = argv[1];

    if (mode == "SYNAPSE") {
        if (argc != 3) {
            printUsage();
            return 1;
        } else {
            FeatureReader reader;
            QList<Feature> features = reader.load("features.csv");
            SynapseDistributor distributor(features);
            const QString specFile = argv[2];
            QJsonObject spec = UtilIO::parseSpecFile(specFile);
            QVector<float> parameters = extractRuleParameters(spec);
            QList<Synapse> synapses =
                distributor.apply(SynapseDistributor::Rule::GeneralizedPeters, parameters);
            SynapseWriter writer;
            writer.write("synapses.csv", synapses);
        }
    } else if (mode == "SUBCUBE") {
        if (argc != 3) {
            printUsage();
            return 1;
        } else {
            const QString specFile = argv[2];
            QJsonObject spec = UtilIO::parseSpecFile(specFile);
            const QString dataRoot = spec["DATA_ROOT"].toString();
            NetworkProps networkProps;
            networkProps.setDataRoot(dataRoot);
            networkProps.loadFilesForSynapseComputation();
            QVector<float> origin = extractOrigin(spec);
            QVector<int> dimensions = extractDimensions(spec);
            QSet<QString> cellTypes = extractCellTypes(spec);
            QSet<QString> regions = extractRegions(spec);
            QSet<int> neuronIds = extractNeuronIds(spec);
            int samplingFactor = extractSamplingFactor(spec);
            FeatureExtractor extractor(networkProps);
            extractor.extract(origin, dimensions, cellTypes, regions, neuronIds, samplingFactor);
            return 0;
        }
    } else {
        printUsage();
        return 1;
    }
}
