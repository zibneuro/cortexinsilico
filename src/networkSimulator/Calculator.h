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

    void calculateBatch(std::vector<QVector<float> > parametersBatch, QString mode, std::map<QString, float>& propsFloat, std::map<QString, bool>& propsBoolean);

    static void calculateSpatial(std::map<int, std::map<int, float> >& neuron_pre, std::map<int, std::map<int, float> >& neuron_postExc, QString innervationDir);

private:

    struct rawDataItem {
        int voxelId;
        float pre;
        float post;
        float postAll;        
    };

    double calculateProbability(double innervationMean);
    void writeSynapseMatrix(int runIndex, std::vector<std::vector<int> >& contacts);
    void writeInnervationMatrix(int runIndex, std::vector<std::vector<float> >& innervation);
    void writeStatistics(int runIndex,
                         double connectionProbability,
                         double connectionProbabilityInnervation,
                         std::vector<double> sufficientStat,
                         std::vector<long> constraintCounts);
    bool updateConstraintCount(std::vector<long>& count,
                               bool magnitudeBoundViolated,
                               bool probabilityBoundViolated,
                               bool maxInnervationBoundViolated);

    FeatureProvider& mFeatureProvider;
    RandomGenerator& mRandomGenerator;
    std::vector<int> mRunIndices;
};
