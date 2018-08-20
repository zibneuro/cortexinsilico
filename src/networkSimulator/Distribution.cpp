#include "Distribution.h"
#include <algorithm>

/*
    Constructor.
*/
Distribution::Distribution() {
  std::random_device rd;
  mRandomGenerator = std::mt19937(rd());
};

/*
    Selects the number of synapses for the specified
    mean innervation value.
    @param mu The mean innervation value.
    @return The number of synapses.
*/
int Distribution::drawSynapseCount(float mu) {
  if (mu > 0) {
    std::poisson_distribution<> distribution(mu);
    return std::min(mMaxSynapseCount, distribution(mRandomGenerator));
  } else {
    return 0;
  }
};