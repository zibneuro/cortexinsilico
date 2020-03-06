#pragma once

#include <QHash>


/**
    For efficiency, NeuroNet deduplicates axons.
    This class maintains a mapping from a neuronId that is a duplicate
    to neuronIdToUse (the original for which data is available)
*/
class AxonRedundancyMap {

public:
    /**
        Registers a neuron id and its mapped id.
        @param neuronId The neuron id to register.
        @param neuronIdToUse The id to use for retrieving axon data.
    */
    void add(const int neuronId, const int neuronIdToUse);

    void updateMapping(const int neuronId, const int desiredMappedId);

    /**
        Retrieves the mapped id from the original id.
        @param neuronId The original neuron id.
        @returns The mapped id.
        @throws runtime_error when id is not registered.
    */
    int getNeuronIdToUse(const int neuronId) const;

    void loadCSV(QString fileName);

    bool isUnique(int neuronId);

private:

    QHash<int, int> mMap;    

};
