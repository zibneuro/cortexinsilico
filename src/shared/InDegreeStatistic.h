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
                      bool sampleEnabled,
                      FormulaCalculator& calculator,
                      QueryHandler* handler);

protected:
    void doCalculate(const NeuronSelection& selection) override;
    void doCreateJson(QJsonObject& obj) const override;
    void doCreateCSV(FileHelper& fileHelper) const override;

private:
    void checkInput(const NeuronSelection& selection);
    QList<int> samplePostIds(QList<int> selectionC);
    double calculateCorrelation(std::vector<double>& valuesAC, std::vector<double>& valuesBC, double stdAC, double stdBC);
    double calculateMean(std::vector<double>& values);
    void writeDiagramOverlap(FileHelper& fileHelper) const;
    void writeDiagramProbability(FileHelper& fileHelper) const;

void calculateStatistics(); 

    Statistics mStatisticsAC;
    Statistics mStatisticsBC;
    Statistics mStatisticsACProb;
    Statistics mStatisticsBCProb;
    std::vector<int> mPostNeuronId;
    std::vector<double> mValuesAC;
    std::vector<double> mValuesBC;
    std::vector<std::vector<double> > mACProbFlat;
    std::vector<std::vector<double> > mBCProbFlat;
    std::vector<double> mValuesACProb;
    std::vector<double> mValuesBCProb;
    double mCorrelation;
    double mCorrelationProb;
    int mSampleSize;
    int mSampleSeed;
    bool mSampleEnabled;
    int mIterations;
    std::map<int,int> mMappingSliceRBC;
};

#endif // INDEGREESTATISTIC
