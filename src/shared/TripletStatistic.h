#ifndef TRIPLETSTATISTIC_H
#define TRIPLETSTATISTIC_H

#include "NetworkStatistic.h"

/**
    Computes the probability distribution of triplet motifs.
*/
class TripletStatistic : public NetworkStatistic
{
public:
    /**
        Constructor.
        @param networkProps The model data of the network.
    */
    TripletStatistic(const NetworkProps& networkProps);

    /**
        Returns internal result, for testing purposes.
        @return The statistic.
    */
    Statistics getResult() const;

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
    Statistics mStandardStatistic;
};

#endif // TRIPLETSTATISTIC
