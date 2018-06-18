#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include "CIS3DBoundingBoxes.h"
#include <QHash>

class Vec3f;
class SparseField;

/**
    Holds the properties of a single neuron.
*/
struct NeuronProps {
    NeuronProps() :
        id(-1),
        mappedAxonId(-1),
        cellTypeId(-1),
        regionId(-1),
        pstExc(0),
        pstInh(0),
        boutons(0)
    {}

    /**
        Unique ID of the neuron.
    */
    int id;

    /**
        The mapped neuron ID associated with the axon morphology.
    */
    int mappedAxonId;

    /**
        ID of the neuron's cell type
    */
    int cellTypeId;

    /**
        ID of the region in which the neuron is located.
    */
    int regionId;

    /**
        Bounding box containing all morphological features of the neuron.
    */
    BoundingBox boundingBox;

    /**
        Spatial position of the soma.
    */
    Vec3f somaPos;

    /**
        In excitatory cells: The postsynaptic target density field.
    */
    SparseField* pstExc;

    /**
        In inhibitory cells: The postsynaptic target density field.
    */
    SparseField* pstInh;

    /**
        The presynaptic bouton density field.
    */
    SparseField* boutons;
};

/**
    Associates neuron IDs with neuron properties.
*/
typedef QHash<int, NeuronProps> PropsMap;

/**
    A tuple of cell type ID and region ID in which these cells exists.
*/
typedef QPair<int, int>  CellTypeRegion;

/**
    Associates a cell type region with the neuron IDs in this region.
*/
typedef QHash<CellTypeRegion, QList<int> > IdsPerCellTypeRegion;

/**
    A list of unique IDs.
*/
typedef QList<int> IdList;

/**
    The features associated with each neuron in each voxel.
    Additionally: The overall postsynaptic target counts (referring to one voxel).
*/
struct Feature {
    int voxelID;
    int voxelX;
    int voxelY;
    int voxelZ;
    int neuronID;
    QString cellType;
    QString region;
    int synapticSide;
    float pre;
    float postExc;
    float postAllExc;
    float postInh;
    float postAllInh;
};

/*
    Voxel ID and location. Pre- and postsynaptic target counts.
*/
struct Voxel {
    int voxelId;
    int voxelX;
    int voxelY;
    int voxelZ;
    float pre;
    float postExc;
    float postAllExc;
    float postInh;
    float postAllInh;
};

#endif // TYPEDEFS_H
