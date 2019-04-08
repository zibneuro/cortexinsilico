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

    struct RawDataItem {
        float beta0;
        float beta1;
        float beta2;
        float beta3;
        int voxelId;
        int preId;
        int postId;
        float pre;
        float post;
        float postAll;
        float postNorm;
        float innervation;
        long synapses;
        bool magnitudeBound;
        bool probabilityBound;
        bool innervationBound;    
        bool discarded;    
    };

    double calculateProbability(double innervationMean);
    void writeSynapseMatrix(int runIndex, std::vector<std::vector<long> >& contacts);
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
    void writeRawData(int runIndex, std::vector<RawDataItem>& rawData); 

    FeatureProvider& mFeatureProvider;
    RandomGenerator& mRandomGenerator;
    std::vector<int> mRunIndices;
};
