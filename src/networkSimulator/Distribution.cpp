#include "Distribution.h"

Distribution::Distribution() {
    std::random_device rd;
    mRandomGenerator = std::mt19937(rd());
};

int Distribution::drawSynapseCount(float mu) {
    if (mu > 0 && mu <= mMaxMu) {
        std::poisson_distribution<> distribution(mu);
        return distribution(mRandomGenerator);
    } else if (mu > mMaxMu) {
        return mMaxSynapseCount;
    } else {
        return 0;
    }
};