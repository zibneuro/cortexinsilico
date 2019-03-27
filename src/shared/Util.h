#ifndef UTIL_H
#define UTIL_H

#include <QList>
#include <QString>
#include "CIS3DNetworkProps.h"
#include "CIS3DNeurons.h"
#include "CIS3DStatistics.h"
#include "Histogram.h"
#include "Typedefs.h"
#include "CIS3DConstantsHelpers.h"
#include <QJsonObject>

class QJsonObject;
class QJsonArray;
class NetworkProps;

/**
    A set of utility functions.
*/
namespace Util
{
    /**
    Checks whether two neurons overlap based on their bounding box.
    @param n1 Properties of first neurons.
    @param n2 Properties of second neurons.
    @returns True if the neurons overlap.
*/
    bool overlap(const NeuronProps& n1, const NeuronProps& n2);

    /**
    Determines the unique neurons accounting for axon redundancy.
    @param preNeuronList The IDs of presynaptic neurons.
    @param networkProps The model data of the network.
    @returns The IDs of unique presynaptic neurons.
*/
    QList<int> getUniquePreNeurons(const QList<int>& preNeuronsList, const NetworkProps& networkProps);

    /**
    Creates a mapping (cellType,region) -> (neuron ids).
    @param propsMap The neuron properties.
    @returns A mapping (hash) with (cellType, region) as hash-key.
*/
    IdsPerCellTypeRegion sortByCellTypeRegion(const PropsMap& propsMap);

    /**
    Creates a mapping (cellType,region) -> (neuron ids) for the
    specified subset of neurons.
    @param neuronIds A subset of neuron IDs.
    @param networkProps The model data of the network.
    @returns A mapping (hash) with (cellType, region) as hash-key.
*/
    IdsPerCellTypeRegion sortByCellTypeRegionIDs(const IdList& neuronIds,
                                                 const NetworkProps& networkProps);

    /**
    Creates a selection filter for neurons.
    @param jsonArray The filter query as received from  the webframework.
    @param network The model data of the network.
    @param synapticSide Specifies whether selection is pre- or postsynaptic.
    @returns A selection filter that can be applied to the (CIS3D)Neurons class.
    @throws runtime_error if the selection filter is not valid.
*/
    SelectionFilter getSelectionFilterFromJson(const QJsonArray& jsonArray, const NetworkProps& network, const CIS3D::SynapticSide synapticSide);

    /**
    Determines and sets the neuron filter for the generation of the connectome
    based on the filters defined in the various statistics (the generation filter
    is the union of all statistic filters).
    @param spec The JSON specification object with the statistic definitions.
        The fields of the generation filter are appended.
*/
    void addGenerationFilter(QJsonObject& spec);

    /**
    Creates a JSON report of a statistic.

    @param statistics The statistics to report.
    @return The JSON object.
*/
    QJsonObject createJsonStatistic(const Statistics& statistics);

    QJsonArray createJsonArray(const std::vector<double>& vector);

    /**
    Creates a JSON report of a histogram

    @param statistics The histogram to report.
    @return The JSON object.
*/
    QJsonObject createJsonHistogram(const Histogram& histogram);

    /**
    Checks whether two values are almost equal.
    @param a First value.
    @param b Second value.
    @param eps Tolerance.
    @return True, if the values are almost equal.
*/
    bool almostEqual(double a, double b, double eps);

    /*
    Handles case that only VPM is selected in combination with nearest column.
    In this case, the nearest column name X is replaced by X_Barreloid and set as region filter.
    @param selection The selection filter.
    @param networkProps The model data.
*/
    void correctVPMSelectionFilter(SelectionFilter& filter, const NetworkProps& networkProps);

    void correctInterneuronSelectionFilter(SelectionFilter& filter, const NetworkProps& networkProps);

    std::vector<double> getHeatMap(double value, double min, double max);

    void getMinMedMax(std::vector<float> in, float& min, float& med, float& max);

    QString getAdvancedSettingsString(const QJsonObject& spec, bool hasSynapseDistribution = false, bool hasConnectionProbability = true);

    CIS3D::Structure getPostsynapticTarget(QString selectionString);

    QString getPostFolderName(CIS3D::Structure target);

    QString getIndexFileName(CIS3D::Structure target);

    QString getBranchIndexFileName(CIS3D::Structure target);

    QString getInnervationFolderName(CIS3D::Structure target);

    QString getNetwork(QJsonObject& spec, int& samplingFactor);

    int getSliceRef(QString network);

    bool isSlice(QJsonObject& networkSpec, int number, double& sliceRef);

    bool isFull(QJsonObject& networkSpec, int number);

    bool matchCells(QJsonObject& networkSpec, int number);

    int getOppositeNetworkNumber(int number);

    bool isSampled(QJsonObject& networkSpec, int number, int& samplingFactor, int& randomSeed);

    QString getLongName(QJsonObject& networkSpec, int number);

    QString getShortName(QJsonObject& networkSpec, int number);

    QString writeNetworkDescription(QJsonObject& networkSelection, int number, QJsonObject& preSelection, QJsonObject& postSelection);

    void getSampleSettings(QJsonObject& sampleSettings, int network, int& sampleSize, int& randomSeed);

    CIS3D::SynapticSide getSynapticSide(QJsonObject& selectionFilter);

    QString getDatasetPath(const QString& datasetShortName,
                           const QJsonObject& config);

} // namespace Util

#endif // UTIL_H
