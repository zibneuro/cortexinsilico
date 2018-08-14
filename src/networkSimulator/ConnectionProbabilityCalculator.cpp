#include "ConnectionProbabilityCalculator.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DSparseVectorSet.h"
#include "FeatureSet.h"
#include "Typedefs.h"

ConnectionProbabilityCalculator::ConnectionProbabilityCalculator(FeatureProvider featureProvider)
    : mFeatureProvider(featureProvider) {}

double ConnectionProbabilityCalculator::calculate(QVector<float> parameters) {
    IdList presynaptic = mFeatureProvider.getPre();
    SparseVectorSet synapses;
    for (int i = 0; i < presynaptic.size(); i++) {
        synapses.addVector(presynaptic[i]);
    }
    QList<FeatureSet> featureSets = mFeatureProvider.getVoxelFeatures();
    for (int i = 0; i < featureSets.size(); i++) {
        calculateVoxel(featureSets[i], parameters, synapses);
    }
    return calculateProbability(synapses);
}

void ConnectionProbabilityCalculator::calculateVoxel(FeatureSet& featureSet,
                                                     QVector<float>& parameters,
                                                     SparseVectorSet& synapses) {
    QList<CalculationFeature> features = featureSet.getFeatures();
    float pstAllExc = featureSet.getPstAllExc();
    float pstAllInh = featureSet.getPstAllInh();

    float theta1 = parameters[0];
    float theta2 = parameters[1];
    float theta3 = parameters[2];
    float theta4 = parameters[3];

    for (int i = 0; i < features.size(); i++) {
        CalculationFeature neuronA = features[i];
        if (neuronA.synapticSide != CIS3D::POSTSYNAPTIC) {
            float pre = neuronA.pre;
            for (int j = 0; j < features.size(); j++) {
                CalculationFeature neuronB = features[j];
                if (neuronB.synapticSide != CIS3D::PRESYNAPTIC &&
                    neuronA.neuronId != neuronB.neuronId) {

                    if(synapses.getEntryIds(neuronA.neuronId).contains(neuronB.neuronId)){
                        continue;
                    }

                    float pstAll =
                        neuronB.functionalType == CIS3D::EXCITATORY ? pstAllExc : pstAllInh;

                    float post = neuronB.functionalType == CIS3D::EXCITATORY ? neuronB.pstExc
                                                                             : neuronB.pstInh;
                    float mu = 0;
                    if ((pre > mEps) && (post > mEps) && (pstAll > mEps)) {
                        mu = exp(theta1 + theta2 * log(pre) + theta3 * log(post) +
                                 theta4 * log(pstAll));
                    }
                    int count = mDistribution.drawSynapseCount(mu);
                    if (count > 0) {
                        synapses.setValue(neuronA.neuronId, neuronB.neuronId, 1);
                    }
                }
            }
        }
    }
}

double ConnectionProbabilityCalculator::calculateProbability(SparseVectorSet& synapses) {
    int numPotentialConnections = mFeatureProvider.getNumPre() * mFeatureProvider.getNumPost();
    int realizedConnections = 0;
    QList<int> presynapticNeurons = synapses.getVectorIds();
    for (int i = 0; i < presynapticNeurons.size(); i++) {
        realizedConnections += synapses.getNumberOfEntries(presynapticNeurons[i]);
    }
    return (double)realizedConnections / (double)numPotentialConnections;
}