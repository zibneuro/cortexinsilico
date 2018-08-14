#pragma once

#include <random>

class Distribution {
   public:
    Distribution();

    int drawSynapseCount(float mu);

   private:
    std::mt19937 mRandomGenerator;
    float mMaxMu = 1000000;
    int mMaxSynapseCount = 999999;
};