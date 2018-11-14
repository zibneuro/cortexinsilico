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
        @param runIndex A postfix for output files.
    */
    Calculator(FeatureProvider& featureProvider, RandomGenerator& randomGenerator, int runIndex);

    /*
        Constructor.
        @param featureProvider The features of the neuron selections.
        @param randomGenerator The random generator for synapse counts.
        @param runIndices The postfixes of the output files.
    */
    Calculator(FeatureProvider& featureProvider, RandomGenerator& randomGenerator, std::vector<int> runIndices);

    void calculate(QVector<float> parameters, bool addIntercept, double maxInnervation, QString mode);

    void calculateBatch(std::vector<QVector<float> > parametersBatch, bool addIntercept, double maxInnervation, QString /*mode*/);   

    static void calculateSpatial(std::map<int, std::map<int, float> >& neuron_pre, std::map<int, std::map<int, float> >& neuron_postExc); 

private:
    double calculateProbability(double innervationMean);
    void writeSynapseMatrix(std::vector<std::vector<int> >& contacts);
    void writeInnervationMatrix(std::vector<std::vector<float> >& innervation);
    void writeStatistics(double connectionProbability, double connectionProbabilityInnervation, std::vector<double> sufficientStat);

    FeatureProvider& mFeatureProvider;
    RandomGenerator& mRandomGenerator;
    int mRunIndex;
    std::vector<int> mRunIndices;
};
