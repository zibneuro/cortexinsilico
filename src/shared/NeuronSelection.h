#include <QString>
#include "Typedefs.h"

#ifndef NEURONSELECTION_H
#define NEURONSELECTION_H

/**
    Encapsulates the selected neuron subppulations that
    are part of an analytic query.
*/
class NeuronSelection {
public:

/**
    Constructor for a second order network statistic, containing pre-
    and postsynaptic neuron selections.
    @param presynaptic The presynaptic neuron IDs.
    @param postsynaptic The postsynaptic neuron IDs.
*/
NeuronSelection(const IdList& presynaptic, const IdList& postsynaptic);

/**
    Constructor for a triplet motif statistic.
    @param motif1 The first subselection.
    @param motif2 The second subselection.
    @param motif3 The third subselection.
*/
NeuronSelection(const IdList& motif1, const IdList& motif2, const IdList& motif3);

/**
  Returns the presynaptic subselection.
*/
IdList Presynaptic() const;

/**
  Returns the postynaptic subselection.
*/
IdList Postsynaptic() const;

/**
  Returns the first neuron subselection for motif statistics.
*/
IdList Motif1() const;

/**
  Returns the second neuron subselection for motif statistics.
*/
IdList Motif2() const;

/**
  Returns the third neuron subselection for motif statistics.
*/
IdList Motif3() const;


private:
IdList mPresynaptic;
IdList mPostsynaptic;
IdList mMotif1;
IdList mMotif2;
IdList mMotif3;
};


#endif
