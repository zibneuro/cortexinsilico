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

    /**
        Retrieves the mapped id from the original id.
        @param neuronId The original neuron id.
        @returns The mapped id.
        @throws runtime_error when id is not registered.
    */
    int getNeuronIdToUse(const int neuronId) const;

    /**
        Saves the axon redundancy map as a binary file.
        @param fileName Name of the file to write.
        @returns 1 if file was successfully written.
        @throws runtime_error in case of failure.
    */
    int saveBinary(const QString& fileName) const;

    /**
        Loads the axon redundancy map from a binary file.
        @param fileName Name of the file to load.
        @returns 1 if file was successfully loaded.
        @throws: runtime_error in case of failure.
    */
    int loadBinary(const QString& fileName);

private:
    QHash<int, int> mMap;

};
