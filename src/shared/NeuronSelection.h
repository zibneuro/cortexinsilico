#include <QJsonObject>
#include <QString>
#include <QVector>
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
        @param samplingFactor Sampling rate for selection (default: 1 = take all)
    */
    void setInnervationSelection(const QJsonObject& spec, const NetworkProps& networkProps, int samplingFactor = 1);

    /**
        Determines a triplet motif statistic selection from a specification file.
        @param spec The spec file with the filter definition.
        @param networkProps the model data of the network.
    */
    void setTripletSelection(const QJsonObject& spec, const NetworkProps& networkProps);

    /**
        Determines a triplet motif statistic selection from selection strings.
        @param motifASelString The first selection string.
        @param motifBSelString The second selection string.
        @param motifASelString The third selection string.
        @param networkProps the model data of the network.
    */
    void setTripletSelection(const QString selAString, const QString selBString,
                             const QString selCString, const NetworkProps& networkProps);
    
    void setInDegreeSelection(const QString selAString,
                                     const QString selBSelString,
                                     const QString selCSelString,
                                     const NetworkProps& networkProps);

    void setPiaSomaDistance(QVector<float> rangePre, QVector<float> rangePost, const NetworkProps& networkProps);

    void filterPiaSoma(IdList& neuronIds, QVector<float> range, const NetworkProps& networkProps);

    /**
        Determines neuron IDs based on a selection string;
        @param selectionString The selection string.
        @param networkProps The model data of the network.
        @return A list of neuron IDs.      
    */
    IdList getSelectedNeurons(const QString selectionString, const NetworkProps& networkProps, CIS3D::SynapticSide synapticSide=CIS3D::BOTH_SIDES);                             

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

    /*
      Prints the number of selected neurons for motif statistics.
    */
    void printMotifStats();

    void setBBox(QVector<float> min, QVector<float> max);

    QVector<float> getBBoxMin();

    QVector<float> getBBoxMax();

    QVector<float> getPiaSomaDistancePre();

    QVector<float> getPiaSomaDistancePost();

   private:
    IdList mPresynaptic;
    IdList mPostsynaptic;
    IdList mMotifA;
    IdList mMotifB;
    IdList mMotifC;
    QVector<float> mBBoxMin;
    QVector<float> mBBoxMax;
    QVector<float> mPiaSomaDistancePre;
    QVector<float> mPiaSomaDistancePost;
};

#endif
