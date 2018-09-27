/*
    This tool extracts features from the model and performs
    synaptic connectivity simulations.
*/

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
#include "ConnectionProbabilityCalculator.h"
#include "FeatureExtractor.h"
#include "FeatureProvider.h"
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
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QSet>
#include <QTextStream>
#include <random>

/*
    Prints the usage manual to the console.
*/
void
printUsage()
{
    qDebug() << "This tool provides three modes: INIT, SYNAPSE and SUBCUBE.";
    qDebug() << "";
    qDebug() << "In the SYNAPSE mode, the tool simulates connectivity formation"
             << "between neuron selections"
             << "according to generalized Peters' rule, which is parametrized by"
             << "theta. To do so,"
             << "the tool requires that are provided in file"
             << "init.csv, and which can be generated in the INIT mode.";
    qDebug() << "";
    qDebug() << "In the SUBCUBE mode, the tool extracts individual features per"
             << "voxel from a subcube of the compete model."
             << "The features are written to the files features.csv, "
             << "featuresSpatial.csv, voxels.csv and neurons.csv";
    qDebug() << "";
    qDebug() << "Usage:";
    qDebug() << "";
    qDebug() << "./networkSimulator INIT    <initSpecFile>";
    qDebug() << "./networkSimulator SYNAPSE <synapseSpecFile>";
    qDebug() << "./networkSimulator SUBCUBE <voxelSpecFile>";
    qDebug() << "";
    qDebug() << "The <initSpecFile> contains the neuron selection to be used for"
             << "simulation.";
    qDebug() << "The <synapseSpecFile> contains the theta parameters for Peter's"
             << "rule.";
    qDebug() << "The <voxelSpecFile> contains the model data directory and the "
             << "origin and"
             << "size of the subcube to be extracted from the model data.";
}

void
checkNumParams(QJsonArray parameters, int expected)
{
    if (parameters.size() != expected)
    {
        QString msg = QString("Invalid number of connectivity rule parameters, expected %1 got %2.").arg(expected).arg(parameters.size());
        throw std::runtime_error(qPrintable(msg));
    }
}

/*
    Extracts the THETA parameters from the spec file.

    @param spec The specification file as json object.
    @return A vector with the theta parameters.
*/
QVector<float>
extractRuleParameters(const QJsonObject spec, bool addIntercept, QString mode)
{
    int numParams;
    if (mode == "generalizedPeters")
    {
        numParams = addIntercept ? 4 : 3;
    }
    else if (mode == "generalizedPeters2Param")
    {
        numParams = addIntercept ? 3 : 2;
    }
    else if (mode == "weightedOverlap")
    {
        numParams = 2;
    }
    else
    {
        throw std::runtime_error("Invalid simulation mode");
    }

    QVector<float> theta;
    QJsonArray parameters = spec["CONNECTIVITY_RULE_PARAMETERS"].toArray();
    checkNumParams(parameters, numParams);
    for (int i = 0; i < parameters.size(); i++)
    {
        theta.append((float)parameters[i].toDouble());
    }

    return theta;
}

/*
    Reads the VOXEL_ORIGIN property from the specification file.

    @param spec The specification file as json object.
    @return The origin (x,y,z) as vector.
    @throws runtime_error if the VOXEL_ORIGIN property is not found.
*/
QVector<float>
extractOrigin(const QJsonObject spec)
{
    if (spec["VOXEL_ORIGIN"] == QJsonValue::Undefined)
    {
        throw std::runtime_error("Key VOXEL_ORIGIN not found in spec file.");
    };
    QVector<float> origin;
    QJsonArray parameters = spec["VOXEL_ORIGIN"].toArray();
    origin.append((float)parameters[0].toDouble());
    origin.append((float)parameters[1].toDouble());
    origin.append((float)parameters[2].toDouble());
    return origin;
}

QVector<float>
extractBBoxMin(const QJsonObject spec)
{
    QVector<float> min;
    if (spec["BBOX_MIN"] == QJsonValue::Undefined)
    {
        min.append(-10000);
        min.append(-10000);
        min.append(-10000);
    }
    else
    {
        QJsonArray parameters = spec["BBOX_MIN"].toArray();
        min.append((float)parameters[0].toDouble());
        min.append((float)parameters[1].toDouble());
        min.append((float)parameters[2].toDouble());
    }
    return min;
}

QVector<float>
extractPiaSomaDistancePre(const QJsonObject spec)
{
    QVector<float> range;
    if (spec["PRE_PIA_DISTANCE"] != QJsonValue::Undefined)
    {
        QJsonArray parameters = spec["PRE_PIA_DISTANCE"].toArray();
        range.append(float(parameters[0].toDouble()));
        range.append(float(parameters[1].toDouble()));
    }
    return range;
}

QVector<float>
extractPiaSomaDistancePost(const QJsonObject spec)
{
    QVector<float> range;
    if (spec["POST_PIA_DISTANCE"] != QJsonValue::Undefined)
    {
        QJsonArray parameters = spec["POST_PIA_DISTANCE"].toArray();
        range.append(float(parameters[0].toDouble()));
        range.append(float(parameters[1].toDouble()));
    }
    return range;
}

QVector<float>
extractBBoxMax(const QJsonObject spec)
{
    QVector<float> max;
    if (spec["BBOX_MAX"] == QJsonValue::Undefined)
    {
        max.append(10000);
        max.append(10000);
        max.append(10000);
    }
    else
    {
        QJsonArray parameters = spec["BBOX_MAX"].toArray();
        max.append((float)parameters[0].toDouble());
        max.append((float)parameters[1].toDouble());
        max.append((float)parameters[2].toDouble());
    }
    return max;
}

/*
    Reads the VOXEL_DIMENSIONS property from the specification file.

    @param spec The specification file as json object.
    @return The number of voxels in each direction (nx,ny,nz) as vector.
    @throws runtime_error if the VOXEL_DIMENSIONS property is not found.
*/
QVector<int>
extractDimensions(const QJsonObject spec)
{
    if (spec["VOXEL_DIMENSIONS"] == QJsonValue::Undefined)
    {
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
QSet<QString>
extractCellTypes(const QJsonObject spec)
{
    if (spec["CELLTYPES"] == QJsonValue::Undefined)
    {
        throw std::runtime_error("Key CELLTYPES not found in spec file.");
    };
    QSet<QString> cellTypes;
    QJsonArray parameters = spec["CELLTYPES"].toArray();
    for (int i = 0; i < parameters.size(); i++)
    {
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
QSet<QString>
extractRegions(const QJsonObject spec)
{
    if (spec["REGIONS"] == QJsonValue::Undefined)
    {
        throw std::runtime_error("Key REGIONS not found in spec file.");
    };
    QSet<QString> regions;
    QJsonArray parameters = spec["REGIONS"].toArray();
    for (int i = 0; i < parameters.size(); i++)
    {
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
QSet<int>
extractNeuronIds(const QJsonObject spec)
{
    QSet<int> neuronIds;
    if (spec["NEURON_IDS"] != QJsonValue::Undefined)
    {
        QJsonArray parameters = spec["NEURON_IDS"].toArray();
        for (int i = 0; i < parameters.size(); i++)
        {
            const int neuronId = parameters[i].toInt();
            neuronIds.insert(neuronId);
        }
    };
    return neuronIds;
}

QSet<int>
extractVoxelIds(const QJsonObject spec)
{
    QSet<int> neuronIds;
    if (spec["VOXEL_IDS"] != QJsonValue::Undefined)
    {
        QJsonArray parameters = spec["VOXEL_IDS"].toArray();
        for (int i = 0; i < parameters.size(); i++)
        {
            const int neuronId = parameters[i].toInt();
            neuronIds.insert(neuronId);
        }
    };
    return neuronIds;
}

QString
extractOutputMode(const QJsonObject spec)
{
    if (spec["OUTPUT_MODE"] != QJsonValue::Undefined)
    {
        const QString mode = spec["OUTPUT_MODE"].toString();
        return mode;
    }
    else
    {
        return "completePerVoxel";
    }
}

QString
extractSimulationMode(const QJsonObject spec)
{
    if (spec["SIMULATION_MODE"] != QJsonValue::Undefined)
    {
        const QString mode = spec["SIMULATION_MODE"].toString();
        return mode;
    }
    else
    {
        return "generalizedPeters";
    }
}

bool
extractCreateMatrix(const QJsonObject spec)
{
    if (spec["CREATE_MATRIX"] != QJsonValue::Undefined)
    {
        return "true" == spec["CREATE_MATRIX"].toString();
    }
    else
    {
        return false;
    }
}

bool
extractAddIntercept(const QJsonObject spec)
{
    if (spec["ADD_INTERCEPT"] != QJsonValue::Undefined)
    {
        return spec["ADD_INTERCEPT"].toBool();
    }
    else
    {
        return true;
    }
}

/*
    Reads the SAMPLING_FACTOR property from the specification file.

    @param spec The specification file as json object.
    @return The SAMPLING_FACTOR, 1 by default.
    @throws runtime_error if the smapling factor has an invalid value.
*/
int
extractSamplingFactor(const QJsonObject spec)
{
    if (spec["SAMPLING_FACTOR"] != QJsonValue::Undefined)
    {
        int samplingFactor = spec["SAMPLING_FACTOR"].toInt();
        if (samplingFactor == 0)
        {
            throw std::runtime_error(
                "Invalid value for sampling factor. Possible reason: \"-symbols.");
        }
        else
        {
            return samplingFactor;
        }
    }
    else
    {
        return 1;
    }
}

double
extractInnervationBound(const QJsonObject spec)
{
    if (spec["INNERVATION_BOUND"] != QJsonValue::Undefined)
    {
        return spec["INNERVATION_BOUND"].toDouble();
    }
    else
    {
        return 1000;
    }
}

void
writeOutputFile(double connectionProbability)
{
    QString fileName = "output.json";
    QFile json(fileName);
    if (!json.open(QIODevice::WriteOnly))
    {
        const QString msg =
            QString("Cannot open file %1 for writing.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    QTextStream out(&json);
    out << "{\"CONNECTION_PROBABILITY\":" << connectionProbability << "}";
}

void
makeCleanDir(QString dirname)
{
    QDir dir;
    if (dir.exists(dirname))
    {
        QDir outputDir(dirname);
        outputDir.setNameFilters(QStringList() << "*.*");
        outputDir.setFilter(QDir::Files);
        foreach (QString dirFile, outputDir.entryList())
        {
            outputDir.remove(dirFile);
        }
    }
    dir.mkdir(dirname);
}

/*
    Entry point for the console application.

    @param argc Number of arguments.
    @param argv Value of arguments.
    @return 0 if successfull, 1 in case of invalid arguments.
*/
int
main(int argc, char** argv)
{
    if (argc < 2)
    {
        printUsage();
        return 1;
    }

    const QString mode = argv[1];

    if (mode == "SYNAPSE")
    {
        if (argc != 3)
        {
            printUsage();
            return 1;
        }
        else
        {
            const QString specFile = argv[2];
            QJsonObject spec = UtilIO::parseSpecFile(specFile);
            bool addIntercept = extractAddIntercept(spec);
            double maxInnervation = extractInnervationBound(spec);
            QString mode = extractSimulationMode(spec);
            QVector<float> parameters = extractRuleParameters(spec, addIntercept, mode);
            FeatureProvider featureProvider;
            ConnectionProbabilityCalculator calculator(featureProvider);
            calculator.calculate(parameters, addIntercept, maxInnervation, mode);
        }
    }
    else if (mode == "SUBCUBE")
    {
        if (argc != 3)
        {
            printUsage();
            return 1;
        }
        else
        {
            const QString specFile = argv[2];
            QJsonObject spec = UtilIO::parseSpecFile(specFile);
            const QString dataRoot = spec["DATA_ROOT"].toString();
            NetworkProps networkProps;
            networkProps.setDataRoot(dataRoot);
            networkProps.loadFilesForSynapseComputation();
            QVector<int> dimensions = extractDimensions(spec);
            QSet<QString> cellTypes = extractCellTypes(spec);
            FeatureExtractor extractor(networkProps);
            if (dimensions[0] == -1 && dimensions[1] == -1 && dimensions[2] == -1)
            {
                extractor.extractAll(cellTypes);
            }
            else
            {
                QVector<float> origin = extractOrigin(spec);
                QSet<QString> regions = extractRegions(spec);
                QSet<int> neuronIds = extractNeuronIds(spec);
                int samplingFactor = extractSamplingFactor(spec);
                extractor.extract(origin, dimensions, cellTypes, regions, neuronIds, samplingFactor);
            }
            return 0;
        }
    }
    else if (mode == "INIT")
    {
        if (argc != 3)
        {
            printUsage();
            return 1;
        }
        else
        {
            const QString specFile = argv[2];
            QJsonObject spec = UtilIO::parseSpecFile(specFile);
            const QString dataRoot = spec["DATA_ROOT"].toString();
            NetworkProps networkProps;
            networkProps.setDataRoot(dataRoot);
            networkProps.loadFilesForSynapseComputation();
            FeatureProvider featureProvider;
            NeuronSelection selection;
            int samplingFactor = extractSamplingFactor(spec);
            QVector<float> bboxMin = extractBBoxMin(spec);
            QVector<float> bboxMax = extractBBoxMax(spec);
            QVector<float> rangePre = extractPiaSomaDistancePre(spec);
            QVector<float> rangePost = extractPiaSomaDistancePost(spec);
            QString mode = extractSimulationMode(spec);
            selection.setInnervationSelection(spec, networkProps, samplingFactor);
            selection.setBBox(bboxMin, bboxMax);
            selection.setPiaSomaDistance(rangePre, rangePost, networkProps);
            if (mode == "generalizedPeters")
            {
                featureProvider.preprocessFeatures(networkProps, selection, 0.0001, true, false);
            }
            else if (mode == "generalizedPeters2Param")
            {
                featureProvider.preprocessFeatures(networkProps, selection, 0.0001, true, false);
            }
            return 0;
        }
    }
    else
    {
        printUsage();
        return 1;
    }
}
