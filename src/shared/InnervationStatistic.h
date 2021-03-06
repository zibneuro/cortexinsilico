#ifndef INNERVATION_H
#define INNERVATION_H

#include "NetworkStatistic.h"
#include "Histogram.h"
#include "PyNNExport.h"
#include <vector>
#include <map>

class SparseVectorCache;

/**
    Represents the standard innervation statistic (as initially available
    in CortexInSilico).
*/
class InnervationStatistic : public NetworkStatistic
{
public:
    /**
        Constructor.
        @param networkProps The model data of the network.
        @param innervation BinSize Bin size of the innervation histogram.
        @param conProbBinSize Bin size of the connectionProbability histogram.
    */
    InnervationStatistic(const NetworkProps& networkProps,
                         FormulaCalculator& calculator,
                         QueryHandler* handler);

    bool hasSubquery(QString& subquery, QString& subqueryResultKey) override;
    
    void writeSubquery(FileHelper& fileHelper) override;

protected:
    /**
        Performs the actual computation based on the specified neurons.
        @param selection The selected neurons.
    */
    void doCalculate(const NeuronSelection& selection) override;

    /**
        Adds the result values to a JSON object
        @param obj: JSON object to which the values are appended
    */
    void doCreateJson(QJsonObject& obj) const override;

    /**
        Writes the result values to file stream (CSV).
        @param out The file stream to which the values are written.
        @param sep The separator between parameter name and value.
    */
    void doCreateCSV(FileHelper& fileHelper) const override;

private:

    Histogram innervationHisto;
    Histogram connProbHisto;

    Statistics innervation;
    Statistics innervationUnique;
    Statistics connProb;
    Statistics connProbUnique;

    Statistics innervationPerPre;
    Statistics innervationPerPreUnique;
    Statistics divergence;
    Statistics divergenceUnique;

    Statistics innervationPerPost;
    Statistics convergence;

    int numPreNeurons;
    int numPostNeurons;
    int numPreNeuronsUnique;
    
    int maxPynn;
    bool exportPynn;
    std::map<int, ExportData> pynnData;
    std::map<int, int> pynnPreIds;
    std::vector<int> pynnPostIds;
    
};

#endif // INNERVATION_H
