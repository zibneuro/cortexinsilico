#ifndef NETWORKPROPS_H
#define NETWORKPROPS_H

#include "CIS3DAxonRedundancyMap.h"
#include "CIS3DBoundingBoxes.h"
#include "CIS3DCellTypes.h"
#include "CIS3DNeurons.h"
#include "CIS3DRegions.h"
#include "CIS3DPSTDensities.h"
#include <QDir>
#include <QString>

/**
    Manages the model data of the network.
*/
class NetworkProps {

public:
    /**
        Sets the root directory in which all the model data is located.
        @param dataRoot The directory path.
        @param resetCache Whether to reset internal flag that data has been loaded.
    */
    void setDataRoot(const QString& dataRoot, const bool resetCache=true);

    /**
        Loads the data required for computing the innervation.
        - CellTypes
        - Neurons
        - BoundingBoxes
        - Regions
        - AxonRedundancyMap
    */
    void loadFilesForSynapseComputation();

    /**
        Loads the data required for computing a summary statistic about the
        network.
        - CellTypes
        - Neurons
        - Regions
        - AxonRedundancyMap
    */
    void loadFilesForQuery();

    /**
        Loads the data required for registering another neuron into an existing
        network.
        - CellTypes
        - Neurons
        - BoundingBoxes
        - Regions
        - AxonRedundancyMap
        - PSTDensity
    */
    void loadFilesForInputMapping();

    /**
        Path to the root directory with the model data.
    */
    QString dataRoot;

    /**
        The root directory with the model data.
    */
    QDir dataRootDir;

    /**
        The AxonRedundancyMap containing neuron ID mappings for the duplication
        of axons.
    */
    AxonRedundancyMap axonRedundancyMap;

    /**
        The bounding boxes associated with all neurons.
    */
    BoundingBoxes boundingBoxes;

    /**
        Meta information on the cell types that are part of the model.
    */
    CellTypes cellTypes;

    /**
        The neurons that are part of the model.
    */
    Neurons neurons;

    /**
        Hierarchical definition of the brain regions incorporated in the model.
    */
    Regions regions;

    /**
        The postsynaptic target density values based on cell type.
    */
    PSTDensities densities;

private:
    bool mFilesForComputationLoaded;
};


#endif // NETWORKPROPS_H
