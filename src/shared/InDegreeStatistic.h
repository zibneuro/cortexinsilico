#ifndef INDEGREESTATISTIC_H
#define INDEGREESTATISTIC_H

#include "InnervationMatrix.h"
#include "NetworkStatistic.h"

class InDegreeStatistic : public NetworkStatistic
{
public:
    InDegreeStatistic(const NetworkProps& networkProps,
                      int sampleSize,
                      int sampleSeed,
                      FormulaCalculator& calculator,
                      QueryHandler* handler);

protected:
    void doCalculate(const NeuronSelection& selection) override;
    void doCreateJson(QJsonObject& obj) const override;
    void doCreateCSV(QTextStream& out, const QChar sep) const override;

private:
    void checkInput(const NeuronSelection& selection);
    QList<int> samplePostIds(QList<int> selectionC);
    void calculateCorrelation();
    double calculateMean(std::vector<double>& values);
    void writeDiagram(QTextStream& out) const;

void calculateStatistics(); 

    Statistics mStatisticsAC;
    Statistics mStatisticsBC;
    std::vector<int> mPostNeuronId;
    std::vector<double> mValuesAC;
    std::vector<double> mValuesBC;
    std::vector<double> mValuesACProb;
    std::vector<double> mValuesBCProb;
    double mCorrelation;
    double mCorrelationProb;
    int mSampleSize;
    int mSampleSeed;
    int mIterations;
};

#endif // INDEGREESTATISTIC
