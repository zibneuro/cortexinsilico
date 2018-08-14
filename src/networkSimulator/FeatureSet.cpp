#include "FeatureSet.h"

FeatureSet::FeatureSet(float pstAllExc, float pstAllInh)
    : mPstAllExc(pstAllExc), mPstAllInh(pstAllInh) {}

void FeatureSet::addCalculationFeature(CalculationFeature feature) { mFeatures.push_back(feature); }

QList<CalculationFeature> FeatureSet::getFeatures() { return mFeatures; }

float FeatureSet::getPstAllExc() { return mPstAllExc; }

float FeatureSet::getPstAllInh() { return mPstAllInh; }