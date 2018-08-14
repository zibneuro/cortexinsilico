#pragma once

#include "NeuronSelection.h"

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
    FeatureProvider(NetworkProps& networkProps);

    /*
        Initializes the features according to the specified collection.
        @selection The neuron selection.
    */
    void init(NeuronSelection& selection);

    NeuronSelection getSelection();

    SparseField* getPre(int neuronId);

    SparseField* getPostExc(int neuronId);

    SparseField* getPostInh(int neuronId);

    SparseField* getPostAllExc();

    SparseField* getPostAllInh();

    int getPresynapticMultiplicity(int neuronId);

    QList<int> getUniquePresynaptic();

private:
    NeuronSelection mSelection;
    NetworkProps& mNetworkProps;

    SparseField* mPostsynapticAllExc;
    SparseField* mPostsynapticAllInh;
    QMap<int, SparseField*> mPresynaptic;
    QMap<int, int> mPresynapticMultiplicity;
    QMap<int, SparseField*> mPostsynapticExc;
    QMap<int, SparseField*> mPostsynapticInh;
};