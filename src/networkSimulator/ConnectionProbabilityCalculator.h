#pragma once

#include <QVector>
#include "CIS3DSparseVectorSet.h"
#include "Distribution.h"
#include "FeatureProvider.h"

class ConnectionProbabilityCalculator {
   public:
    ConnectionProbabilityCalculator(FeatureProvider featureProvider);

    double calculate(QVector<float> parameters);

    void calculateVoxel(FeatureSet& features, QVector<float>& parameters,
                        SparseVectorSet& synapses);

   private:
    double calculateProbability(SparseVectorSet& synapses);
    FeatureProvider& mFeatureProvider;
    Distribution mDistribution;
    float mEps = 0.000001;
};