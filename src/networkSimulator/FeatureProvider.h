#pragma once

#include "NeuronSelection.h"
#include "FeatureSet.h"

/**
    This class provides memory efficient access to neuron features.
    Pre-filters the features according to the current pre- and 
    postsynaptic selection.
*/
class FeatureProvider {

public:

    /*
        Constructor.
    */
    FeatureProvider();

    /*
        Initializes the features according to the specified collection.
        @selection The neuron selection.
    */
    void init(NeuronSelection selection);

    /*
        Returns the filtered neuron features for each voxel.
        @return The neuron features per voxel.
    */
    QList<FeatureSet> getVoxelFeatures();

    /*
        Determines the number of presynaptic neurons.
        @return The numner of neurons.
    */
    int getNumPre();

    /*
        Determines the presynaptic neuron IDs of the current 
        selection.
        @return The neuron IDs.
    */
    IdList getPre();

    /*
        Determines the number of postsynaptic neurons.
        @return The numner of neurons.
    */
    int getNumPost();

private:
    NeuronSelection mSelection;
    QList<FeatureSet> mVoxelFeatures;
};