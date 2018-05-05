#ifndef MYSTATISTIC_H
#define MYSTATISTIC_H

#include "NetworkStatistic.h"

/**
    Example class for a customly defined statistc
*/
class MyStatistic : public NetworkStatistic
{
public:
    /**
        Constructor.
        @param networkProps The model data of the network.
    */
    MyStatistic(const NetworkProps& networkProps);

    /**
        Returns internal result, for testing purposes.
        @return The statistic.
    */
    Statistics getResult() const;

protected:
    /**
        Performs the actual computation based on the specified neurons.
        @param preNeurons The presynaptic neurons.
        @param postNeurons The postsynaptic neurons.
    */
    void doCalculate(const IdList& preNeurons, const IdList& postNeurons) override;

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

#endif // MYSTATISTIC
