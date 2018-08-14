#include "ConnectionProbabilityCalculator.h"
#include <omp.h>
#include <mutex>
#include "CIS3DConstantsHelpers.h"
#include "CIS3DSparseField.h"
#include "CIS3DSparseVectorSet.h"
#include "SparseFieldCalculator.h"
#include "Typedefs.h"

ConnectionProbabilityCalculator::ConnectionProbabilityCalculator(FeatureProvider featureProvider)
    : mFeatureProvider(featureProvider) {
    mNeuronSelection = mFeatureProvider.getSelection();
    mPresynaptic = mFeatureProvider.getUniquePresynaptic();
    mPostsynaptic = mNeuronSelection.Postsynaptic();
}

double ConnectionProbabilityCalculator::calculate(QVector<float> parameters) {
    SparseField* postAll = mFeatureProvider.getPostAllExc();
    Statistics innervationHistogram;
    SparseFieldCalculator fieldCalculator;
    std::mutex mutex;    
    for (int i = 0; i < mPostsynaptic.size(); i++) {
        int postNeuron = mPostsynaptic[i];
        #pragma omp parallel for schedule(dynamic)
        for (int j = 0; j < mPresynaptic.size(); j++) {
            int preNeuron = mPresynaptic[j];
            SparseField* pre = mFeatureProvider.getPre(preNeuron);
            SparseField* post = mFeatureProvider.getPostExc(postNeuron);
            int multiplicity = mFeatureProvider.getPresynapticMultiplicity(preNeuron);
            float innervation = fieldCalculator.calculatePetersRule(
                *pre, *post, *postAll, parameters[0], parameters[1], parameters[2], parameters[3]);
            mutex.lock();
            for (int k = 0; k < multiplicity; k++) {
                innervationHistogram.addSample(innervation);
            }
            mutex.unlock();
        }
    }
    qDebug() << innervationHistogram.getNumberOfSamples();
    return calculateProbability(innervationHistogram.getMean());
}

double ConnectionProbabilityCalculator::calculateProbability(double innervationMean) {
    return 1 - exp(-1 * innervationMean);
}
