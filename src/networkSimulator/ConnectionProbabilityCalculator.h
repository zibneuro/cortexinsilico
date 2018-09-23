#pragma once

#include "CIS3DSparseVectorSet.h"
#include "CIS3DStatistics.h"
#include "Distribution.h"
#include "FeatureProvider.h"
#include "Typedefs.h"
#include <QVector>

/*
    Computes the connection probability according to generalized Peters'
    rule.
*/
class ConnectionProbabilityCalculator
{
public:

    ConnectionProbabilityCalculator(FeatureProvider& featureProvider);

    void calculate(QVector<float> parameters, bool addIntercept, double maxInnervation);

private:
    double calculateProbability(double innervationMean);
    void writeSynapseMatrix(std::vector<std::vector<int> >& contacts);
    void writeInnervationMatrix(std::vector<std::vector<float> >& innervation);
    void writeStatistics(double connectionProbability, std::vector<double> sufficientStat);

    FeatureProvider& mFeatureProvider;
    int mNumPre;
    int mNumPost;
};
