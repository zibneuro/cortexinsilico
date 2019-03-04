#include <QJsonObject>
#include <QString>
#include <QVector>
#include "CIS3DNetworkProps.h"
#include "Typedefs.h"
#include "CIS3DConstantsHelpers.h"
#include "RandomGenerator.h"

#ifndef NEURONSELECTION_H
#define NEURONSELECTION_H

/**
    Encapsulates the selected neuron subppulations that
    are part of an analytic query.
*/
class NeuronSelection
{
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
      Filters neurons in a slice model based on tissue depth.
      param side: 0 left band, 1 right band, 2 any band
    */
    static IdList filterTissueDepth(const NetworkProps& networkProps, IdList& preFiltered, double sliceRef, double low, double high, CIS3D::SliceBand band = CIS3D::SliceBand::BOTH);

    /**
        Determines a innervation statistic selection from a specification file.
        @param spec The spec file with the filter definition.
        @param networkProps the model data of the network.
        @param samplingFactor Sampling rate for selection (default: 1 = take all)
        @param seed The random seed for the sampling (-1, to use random seed)
    */
    void setInnervationSelection(const QJsonObject& spec, const NetworkProps& networkProps, int samplingFactor = 1, int seed = -1);

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
    void setTripletSelection(const QString selAString, const QString selBString, const QString selCString, const NetworkProps& networkProps);

    void setInDegreeSelection(const QString selAString,
                              const QString selBSelString,
                              const QString selCSelString,
                              const NetworkProps& networkProps);

    void setPiaSomaDistance(QVector<float> rangePre, QVector<float> rangePost, const NetworkProps& networkProps);

    void setFullModel(const NetworkProps& networkProps, bool uniquePre = true);

    void filterPiaSoma(IdList& neuronIds, QVector<float> range, const NetworkProps& networkProps);

    void filterTripletSlice(const NetworkProps& networkProps,
                            double sliceRef,
                            double tissueLowMotifA,
                            double tissueHighMotifA,
                            QString tissueModeMotifA,
                            double tissueLowMotifB,
                            double tissueHighMotifB,
                            QString tissueModeMotifB,
                            double tissueLowMotifC,
                            double tissueHighMotifC,
                            QString tissueModeMotifC);

    void filterInnervationSlice(const NetworkProps& networkProps,
                                double sliceRef,
                                double tissueLowPre,
                                double tissueHighPre,
                                QString tissueModePre,
                                double tissueLowPost,
                                double tissueHighPost,
                                QString tissueModePost);

    /**
        Determines neuron IDs based on a selection string;
        @param selectionString The selection string.
        @param networkProps The model data of the network.
        @return A list of neuron IDs.      
    */
    IdList getSelectedNeurons(const QString selectionString, const NetworkProps& networkProps, CIS3D::SynapticSide synapticSide = CIS3D::BOTH_SIDES);

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

    CIS3D::SliceBand getMotifABand(int id) const;

    CIS3D::SliceBand getMotifBBand(int id) const;

    CIS3D::SliceBand getMotifCBand(int id) const;

    CIS3D::SliceBand getPresynapticBand(int id) const;

    CIS3D::SliceBand getPostsynapticBand(int id) const;

    /*
      Prints the number of selected neurons for motif statistics.
    */
    void printMotifStats();

    void setBBox(QVector<float> min, QVector<float> max);

    QVector<float> getBBoxMin();

    QVector<float> getBBoxMax();

    QVector<float> getPiaSomaDistancePre();

    QVector<float> getPiaSomaDistancePost();

    void filterUniquePre(const NetworkProps& networkProps);

    void sampleDown(int maxSize, int seed);

    void sampleDownFactor(int samplingFactor, int seed);

    static IdList filterPreOrBoth(const NetworkProps& networkProps, IdList ids);

    void setPostTarget(CIS3D::Structure selectionA, CIS3D::Structure selectionB, CIS3D::Structure selectionC = CIS3D::DEND);

    CIS3D::Structure getPostTarget(int selectionIndex) const;

    static IdList getDownsampledFactor(IdList& original, int factor, RandomGenerator& randomGenerator);    

private:
    static bool inSliceBand(double somaX, double min, double max);

    static void inSliceRange(double somaX, double sliceRef, double low, double high, bool& first, bool& second);

    IdList getDownsampled(IdList& original, int maxSize, RandomGenerator& randomGenerator);      

    IdList mPresynaptic;
    IdList mPostsynaptic;
    IdList mMotifA;
    IdList mMotifB;
    IdList mMotifC;
    QVector<float> mBBoxMin;
    QVector<float> mBBoxMax;
    QVector<float> mPiaSomaDistancePre;
    QVector<float> mPiaSomaDistancePost;
    std::map<int, CIS3D::SliceBand> mMotifABand;
    std::map<int, CIS3D::SliceBand> mMotifBBand;
    std::map<int, CIS3D::SliceBand> mMotifCBand;
    std::map<int, CIS3D::SliceBand> mPresynapticBand;
    std::map<int, CIS3D::SliceBand> mPostsynapticBand;
    std::vector<CIS3D::Structure> mPostTarget;
};

#endif
