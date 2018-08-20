#pragma once

#include <random>

/*
    This class represents a Poisson distribution.
*/
class Distribution {
public:
  /*
   Constructor.
  */
  Distribution();

  /*
      Selects the number of synapses for the specified
      mean innervation value.
      @param mu The mean innervation value.
  */
  int drawSynapseCount(float mu);

private:
  std::mt19937 mRandomGenerator;
  float mMaxMu = 1000000;
  int mMaxSynapseCount = 999999;
};