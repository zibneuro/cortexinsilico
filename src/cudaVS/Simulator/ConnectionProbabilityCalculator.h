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
class ConnectionProbabilityCalculator {
public:

    struct Contact{
        int pre;
        int post;
        float preVal;
        float postVal;
        float postAllVal;
        float mu;
        int count;
    };

  ConnectionProbabilityCalculator(FeatureProvider &featureProvider);

  double calculateSynapse(QVector<float> parameters, bool matrix=false);
  double distributeSynapses(QVector<float> parameters);

private:
  double calculateProbability(double innervationMean);

  FeatureProvider &mFeatureProvider;
  int mNumPre;
  int mNumPost;
};
