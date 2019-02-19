#pragma once

#include "CIS3DSparseVectorSet.h"
#include "CIS3DStatistics.h"
#include "RandomGenerator.h"
#include "FeatureProvider.h"
#include "Typedefs.h"
#include <QVector>

/*
    Computes neuron-to-neuron connectivity according to Generalized Peters' rule.    
*/
class Calculator
{
public:
    /*
        Constructor.
        @param featureProvider The features of the neuron selections.
        @param randomGenerator The random generator for synapse counts.
        @param runIndices The postfixes of the output files.
    */
    Calculator(FeatureProvider& featureProvider, RandomGenerator& randomGenerator, std::vector<int> runIndices);

    void calculateBatch(std::vector<QVector<float> > parametersBatch, double maxInnervation, QString mode, double boutonPSTRatio, bool checkProb, double maxFailedRatio);

    static void calculateSpatial(std::map<int, std::map<int, float> >& neuron_pre, std::map<int, std::map<int, float> >& neuron_postExc);

private:
    double calculateProbability(double innervationMean);
    void writeSynapseMatrix(int runIndex, std::vector<std::vector<int> >& contacts);
    void writeInnervationMatrix(int runIndex, std::vector<std::vector<float> >& innervation);
    void writeStatistics(int runIndex,
                         double connectionProbability,
                         double connectionProbabilityInnervation,
                         std::vector<double> sufficientStat,
                         bool failed);

    FeatureProvider& mFeatureProvider;
    RandomGenerator& mRandomGenerator;
    std::vector<int> mRunIndices;
};
