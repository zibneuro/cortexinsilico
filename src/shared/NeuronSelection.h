#include <QJsonObject>
#include <QString>
#include "CIS3DNetworkProps.h"
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
      Empty constructor.
    */
    NeuronSelection();

    /**
     Constructor for a second order network statistic, containing pre-
     and postsynaptic neuron selections.
     @param presynaptic The presynaptic neuron IDs.
     @param postsynaptic The postsynaptic neuron IDs.
    */
    NeuronSelection(const IdList& presynaptic, const IdList& postsynaptic);

    /**
        Determines a innervation statistic selection from a specification file.
        @param spec The spec file with the filter definition.
        @param networkProps the model data of the network.
    */
    void setInnervationSelection(const QJsonObject& spec, const NetworkProps& networkProps);

    /**
        Determines a triplet motif statistic selection from a specification file.
        @param spec The spec file with the filter definition.
        @param networkProps the model data of the network.
    */
    void setTripletSelection(const QJsonObject& spec, const NetworkProps& networkProps);

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
    IdList MotifA() const;

    /**
      Returns the second neuron subselection for motif statistics.
    */
    IdList MotifB() const;

    /**
      Returns the third neuron subselection for motif statistics.
    */
    IdList MotifC() const;

   private:
    IdList mPresynaptic;
    IdList mPostsynaptic;
    IdList mMotifA;
    IdList mMotifB;
    IdList mMotifC;
};

#endif
