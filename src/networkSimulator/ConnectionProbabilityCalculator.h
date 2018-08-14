#pragma once

#include <QVector>
#include "CIS3DSparseVectorSet.h"
#include "CIS3DStatistics.h"
#include "Distribution.h"
#include "FeatureProvider.h"
#include "Typedefs.h"

class ConnectionProbabilityCalculator {
   public:
    ConnectionProbabilityCalculator(FeatureProvider featureProvider);

    double calculate(QVector<float> parameters);

   private:    

    double calculateProbability(double innervationMean);

    FeatureProvider& mFeatureProvider;    
    NeuronSelection mNeuronSelection;    
    IdList mPresynaptic;
    IdList mPostsynaptic;
};