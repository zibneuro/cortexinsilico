#ifndef INDEGREESTATISTIC_H
#define INDEGREESTATISTIC_H

#include "InnervationMatrix.h"
#include "NetworkStatistic.h"

class InDegreeStatistic : public NetworkStatistic
{
public:
    InDegreeStatistic(const NetworkProps& networkProps,
                      int sampleSize,
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

    Statistics mStatisticsAC;
    Statistics mStatisticsBC;
    std::vector<double> mValuesAC;
    std::vector<double> mValuesBC;
    double mCorrelation;
    int mSampleSize;
    int mIterations;
};

#endif // INDEGREESTATISTIC
