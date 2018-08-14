#pragma once

#include <QList>
#include "Typedefs.h"

class FeatureSet {
   public:
    FeatureSet(float pstAllExc, float pstAllInh);
    void addCalculationFeature(CalculationFeature feature);
    QList<CalculationFeature> getFeatures();
    float getPstAllExc();
    float getPstAllInh();

   private:
    QList<CalculationFeature> mFeatures;
    float mPstAllExc;
    float mPstAllInh;
};