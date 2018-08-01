#ifndef UTIL_H
#define UTIL_H

#include <QList>
#include <QString>
#include "CIS3DNetworkProps.h"
#include "CIS3DNeurons.h"
#include "Typedefs.h"
#include "CIS3DStatistics.h"
#include "Histogram.h"

class QJsonObject;
class QJsonArray;
class NetworkProps;

/**
    A set of utility functions.
*/
namespace Util {

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
    QList<int> getUniquePreNeurons(const QList<int>& preNeuronsList,
                                   const NetworkProps& networkProps);

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
    IdsPerCellTypeRegion sortByCellTypeRegionIDs(const IdList& neuronIds, const NetworkProps& networkProps);

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


}

#endif // UTIL_H
