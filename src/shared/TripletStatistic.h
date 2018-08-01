#ifndef TRIPLETSTATISTIC_H
#define TRIPLETSTATISTIC_H

#include "InnervationMatrix.h"
#include "NetworkStatistic.h"
#include "TripletMotif.h"

/**
    Computes the probability distribution of triplet motifs.
*/
class TripletStatistic : public NetworkStatistic {
   public:
    /**
        Constructor.
        @param networkProps The model data of the network.
        @param sampleSize The number of triplets to draw.
    */
    TripletStatistic(const NetworkProps& networkProps, int sampleSize);

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

    /**
        Determines the average innervation values between the three neuron
        subselections using the randomly drawn triplets for sampling.
        @param triplets The randomly drawn triplets.
        @return The average innervation values.
    */
    std::vector<std::vector<double> > getAverageInnervation(QList<CellTriplet>& triplets);

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

    QList<Statistics> mMotifProbabilities;
    QList<Statistics> mMotifExpectedProbabilities;
    int mSampleSize;
};

#endif  // TRIPLETSTATISTIC
