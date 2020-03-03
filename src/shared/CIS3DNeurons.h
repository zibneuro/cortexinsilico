#ifndef NEURONS_H
#define NEURONS_H

#include <QMap>
#include <QList>
#include "CIS3DVec3.h"
#include "CIS3DConstantsHelpers.h" 
#include <QSharedMemory>
#include <vector>

/**
    A filter to select subgroups of neurons based on cell type, region,
    barrel column, laminar location, and synaptic side.
*/
struct SelectionFilter {
    SelectionFilter()
        : synapticSide(CIS3D::BOTH_SIDES)
    {}

    QList<int> cellTypeIds;
    QList<int> regionIds;
    QList<int> nearestColumnIds;
    QList<CIS3D::LaminarLocation> laminarLocations;
    CIS3D::SynapticSide synapticSide;    
    std::vector<float> corticalDepth;
};

/**
    The collection of neurons that form the network.
*/
class Neurons
{

public:
    /**
        Constructor.
    */
    Neurons();

    /**
        Constructor.
        @param csvFile The neuron file to load.
        @throws runtime_error if file could not be loaded or parsed.
    */
    Neurons(const QString& csvFile);

    /**
        The properties of a single neuron.
    */
    struct NeuronProperties {
        int id;
        float somaX;
        float somaY;
        float somaZ;
        CIS3D::LaminarLocation loc;
        int cellTypeId;
        int regionId;
        int nearestColumnId;
        CIS3D::SynapticSide synapticSide;
        float corticalDepth;
    };

    /**
        Adds another neuron to the collection.
        @param neuronProps The neuron to add.
    */
    void addNeuron(const NeuronProperties& neuronProps);

    /**
        Checks if the neuron with the specified ID exists.
        @param id The neuron ID.
        @return True if the neuron exists.
    */
    bool exists(const int id) const;

    /**
        Determines the cell type of the specified neuron.
        @param id The neuron ID.
        @return The ID of the cell type.
        @throws runtime_error if neuron ID does not exist.
    */
    int getCellTypeId(const int id) const;

    /**
        Determines the region of the specified neuron.
        @param id The neuron ID.
        @return The ID of the region.
        @throws runtime_error if neuron ID does not exist.
    */
    int getRegionId(const int id) const;

    /**
        Determines the soma position of the specified neuron.
        @param id The neuron ID.
        @return The soma position.
        @throws runtime_error if neuron ID does not exist.
    */
    Vec3f getSomaPosition(const int id) const;

    /**
        Determines the synaptic side of the specified neuron.
        @param id The neuron ID.
        @return The synaptic side (presy./postsy./both).
        @throws runtime_error if neuron ID does not exist.
    */
    CIS3D::SynapticSide getSynapticSide(const int id) const;

     /**
        Determines the laminar location of the specified neuron.
        @param id The neuron ID.
        @return The laminar location.
        @throws runtime_error if neuron ID does not exist.
    */
    CIS3D::LaminarLocation getLaminarLocation(const int id) const;

    /**
        Returns the IDs of all neurons with the specified properties.
        @includedCellTypeIds The desired cell types.
        @includedRegionIds The desired regions.
        @synapticSide The desired synaptic side.
        @return The neuron IDs.
    */
    QList<int> getFilteredNeuronIds(const QList<int>& includedCellTypeIds,
                                    const QList<int>& includedRegionIds,
                                    const CIS3D::SynapticSide synapticSide=CIS3D::BOTH_SIDES) const;

    /**
        Returns the IDs of all neurons meeting the filter condition.
        @filter The selection filter.
        @return The neuron IDs.
    */
    QList<int> getFilteredNeuronIds(const SelectionFilter& filter) const;

    /*
        Returns the properties of the specfied neuron.
        @param neuronId The ID of the neuron.
        @return The neuron properties.
    */
    NeuronProperties getNeuronProps(int neuronId) const;

    /**
        Loads the neurons from file.
        @param fileName The name of the neuron file.
        @throws runtime_error if file could not be loaded or parsed.
    */
    void loadCSV(const QString& fileName);

private:

    typedef QMap<int, NeuronProperties> PropsMap;
    PropsMap mPropsMap;    

};

#endif // NEURONS_H
