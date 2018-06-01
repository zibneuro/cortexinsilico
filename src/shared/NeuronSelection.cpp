#include "NeuronSelection.h"

/**
    Constructor for a second order network statistic, containing pre-
    and postsynaptic neuron selections.
    @param presynaptic The presynaptic neuron IDs.
    @param postsynaptic The postsynaptic neuron IDs.
*/
NeuronSelection::NeuronSelection(const IdList& presynaptic, const IdList& postsynaptic):
    mPresynaptic(presynaptic), mPostsynaptic(postsynaptic){
};

/**
    Constructor for a triplet motif statistic.
    @param motif1 The first subselection.
    @param motif2 The second subselection.
    @param motif3 The third subselection.
*/
NeuronSelection::NeuronSelection(const IdList& motif1, const IdList& motif2, const IdList& motif3):
    mMotif1(motif1), mMotif2(motif2), mMotif3(motif3){
};

/**
  Returns the presynaptic subselection.
*/
IdList NeuronSelection::Presynaptic() const{
    return mPresynaptic;
}

/**
  Returns the postynaptic subselection.
*/
IdList NeuronSelection::Postsynaptic() const{
    return mPostsynaptic;
}

/**
  Returns the first neuron subselection for motif statistics.
*/
IdList NeuronSelection::Motif1() const{
    return mMotif1;
}

/**
  Returns the second neuron subselection for motif statistics.
*/
IdList NeuronSelection::Motif2()const{
    return mMotif2;
}

/**
  Returns the third neuron subselection for motif statistics.
*/
IdList NeuronSelection::Motif3()const{
    return mMotif3;
}
