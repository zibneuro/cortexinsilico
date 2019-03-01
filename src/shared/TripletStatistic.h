#ifndef TRIPLETSTATISTIC_H
#define TRIPLETSTATISTIC_H

#include "InnervationMatrix.h"
#include "NetworkStatistic.h"
#include "TripletMotif.h"
#include "Util.h"

/**
    Computes the probability distribution of triplet motifs.
*/
class TripletStatistic : public NetworkStatistic
{
public:
    /**
        Constructor.
        @param networkProps The model data of the network.
        @param sampleSize The number of triplets to draw.
        @param iterations The number of iterations.
    */
    TripletStatistic(const NetworkProps& networkProps,
                     int sampleSize,
                     int iterations,                     
                     FormulaCalculator& calculator);

    /**
        Writes result to file, for testing purposes.
    */
    void writeResult() const;

protected:
    /**
        Performs the actual computation based on the specified neurons.
        @param selection The selected neurons.
    */
    void doCalculate(const NeuronSelection& selection) override;

    /**
        Adds the result values to a JSON object
        @param obj JSON object to which the values are appended
    */
    void doCreateJson(QJsonObject& obj) const override;

    /**
        Writes the result values to file stream (CSV).
        @param out The file stream to which the values are written.
        @param sep The separator between parameter name and value.
    */
    void doCreateCSV(QTextStream& out, const QChar sep) const override;

private:
    /**
        Checks wheter the neuron selectio is valid.
        @throws runtime_error if selection is ncot valid.
    */
    void checkInput(const NeuronSelection& selection);

    /**
        Randomly select triplets of the specified neuron type.
        @param selection The neuron selection.
        @param nrOfTriples The number of triplets to draw.
        @return List of triplets.
    */
    QList<CellTriplet> drawTriplets(const NeuronSelection& selection);

    /**
        Sets the innervation values in the specified triplets.
        @param triplets The uninitialized triplets.
    */
    void setInnervation(QList<CellTriplet>& triplets);

    /**
        Initializes empty statistics.
    */
    void initializeStatistics();

    /*
        Calculates the connection probabiltiy for the specfied innervation.
        @param innervation The innervation.
        @return The connection probability.
    */
    double calculateConnectionProbability(double innervation);

    /**
        Calculates the convergence to the specified postsynaptic neuron.
        @param presynapticNeurons The IDs of the presynaptic neurons.
        @param postsynapticNeuronId The ID of the postsynaptic neuron.
        @return  The convergence to the postsynaptic neuron.
    */
    double calculateConvergence(IdList& presynapticNeurons, int postsynapticNeuronId, int preSelectionIndex, int postSelectionIndex);

    /**
        Calculates the average convergence to for all combinations by
        sampling a subset of the postsynaptic neurons in each group.
        @param selection The selected neuron groups.
    */
    void calculateAverageConvergence(const NeuronSelection& selection);

    /**
        Computes the occurrence probability of the specified motifs based
        on a random selection of neuron triplets.
        @param triplets The random selection of neurons.
        @param tripletMotifs The motif combinations.
    */
    void computeProbabilities(QList<CellTriplet>& triplets,
                              std::map<unsigned int, std::list<TripletMotif*> > tripletMotifs);

    /**
        Computes the expected occurrence probability of each motif based
        on the average connection probability between the neuron subselections.
        @param avgInnervation The average connection probabilties.
        @param tripletMotifs The motif combinations.
    */
    void computeExpectedProbabilities(
        std::vector<std::vector<double> > avgInnervation,
        std::map<unsigned int, std::list<TripletMotif*> > tripletMotifs);

    /**
        Computes the expected occurrence probability of each motif based
        on the average convergence between the neuron subselections.
        @param tripletMotifs The motif combinations.
    */
    void computeExpectedProbabilities(
        std::map<unsigned int, std::list<TripletMotif*> > tripletMotifs);

    /**
        Deletes the motif combinations.
        @param Map of motif combinations.
    */
    void deleteMotifCombinations(std::map<unsigned int, std::list<TripletMotif*> > tripletMotifs);

    /**
        Determines the deviation for the specified motif.
        @param motif The number of the motif.
        @return The deviation.
    */
    double getDeviation(int motif) const;

    /**
        Draws a random selection of neuron IDs.
        @param neuronIds The complete ID list.
        @param number The number of neurons to draw.
        @return The selected IDs.
    */
    IdList drawRandomly(const IdList& neuronIds, int number);

    /**
        Draws a random selection of neuron IDs, if the number of elements 
        in the specified list exceeds a certain limit.
        @param neuronIds The complete ID list.        
        @param limit The permissible number of elements.
        @return The selected IDs.
    */
    IdList drawRandomlyExceeds(const IdList& neuronIds, int limit);

    /**
        Prints the average convergence values between the subselections.
    */
    void printAverageConvergence();

    QList<Statistics> mMotifProbabilities;
    QList<Statistics> mMotifExpectedProbabilities;
    std::vector<std::vector<Statistics> > mConvergences;
    int mSampleSize;
    int mIterations;
    std::vector<CIS3D::Structure> mPostTargets;
};

#endif // TRIPLETSTATISTIC
