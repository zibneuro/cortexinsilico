#include "ConnectionProbabilityCalculator.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DSparseField.h"
#include "CIS3DSparseVectorSet.h"
#include "SparseFieldCalculator.h"
#include "Typedefs.h"
#include <mutex>
#include <omp.h>

/*
    Constructor.
    @param featureProvider The features of the neuron selections.
*/
ConnectionProbabilityCalculator::ConnectionProbabilityCalculator(
    FeatureProvider &featureProvider)
    : mFeatureProvider(featureProvider) {
  mNumPre = mFeatureProvider.getNumPre();
  mNumPost = mFeatureProvider.getNumPost();
}

/*
    Calculates the connection probability for the specified
    rule parameters.
    @param The connectivity rule parameters.
    @return The connection probability.
*/
double ConnectionProbabilityCalculator::calculate(QVector<float> parameters) {

  // qDebug() << "[*] Start simulation.";

  SparseField *postAll = mFeatureProvider.getPostAllExc();
  Statistics innervationHistogram;
  SparseFieldCalculator fieldCalculator;
  std::mutex mutex;
  std::mutex mutex2;
  for (int i = 0; i < mNumPre; i++) {
//#pragma omp parallel for schedule(dynamic)
    for (int j = 0; j < mNumPost; j++) {
      mutex.lock();
      SparseField *pre = mFeatureProvider.getPre(i);
      SparseField *post = mFeatureProvider.getPostExc(j);
      int multiplicity = mFeatureProvider.getPreMultiplicity(i);
      mutex.unlock();
      float innervation = fieldCalculator.calculatePetersRule(
          *pre, *post, *postAll, parameters[0], parameters[1], parameters[2],
          parameters[3]);
      mutex2.lock();
      for (int k = 0; k < multiplicity; k++) {
        innervationHistogram.addSample(innervation);
      }
      mutex2.unlock();
    }
  }

  // qDebug() << "[*] Finish simulation.";

  return calculateProbability(innervationHistogram.getMean());
}

double
ConnectionProbabilityCalculator::calculateProbability(double innervationMean) {
  return 1 - exp(-1 * innervationMean);
}
