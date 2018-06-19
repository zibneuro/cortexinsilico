#include "SynapseDistributor.h"

/*
    Constructor.

    @param features The neuron features.
*/
SynapseDistributor::SynapseDistributor(QList<Feature>& features) : mFeatures(features) {
    initHashMaps();
}

/*
    Calculates synase counts according to the specified mode
    and parameters.

    @param rule Calculation rule.
    @param parameters Rule parameters.
    @return A list of synapses.
*/
QList<Synapse> SynapseDistributor::apply(Rule /*rule*/, QVector<float> /*parameters*/) {
    QList<Synapse> synapses;
    std::random_device rd;
    std::mt19937 randomGenerator(rd());

    QSetIterator<int> voxelIt(mVoxels);
    while (voxelIt.hasNext()) {
        int voxelId = voxelIt.next();
        if (mPreNeuronsVoxelwise[voxelId].size() > 0 && mPostNeuronsVoxelwise[voxelId].size()) {
            QList<int> preFeatures = mPreNeuronsVoxelwise[voxelId].toList();
            QList<int> postFeatures = mPostNeuronsVoxelwise[voxelId].toList();
            for (int i = 0; i < preFeatures.size(); i++) {
                for (int j = 0; j < postFeatures.size(); j++) {
                    int preFeatureIdx = preFeatures[i];
                    int postFeatureIdx = postFeatures[j];

                    Feature preFeature = mFeatures[preFeatureIdx];
                    Feature postFeature = mFeatures[postFeatureIdx];

                    Synapse synapse;
                    synapse.voxelId = preFeature.voxelID;
                    synapse.voxelX = preFeature.voxelX;
                    synapse.voxelY = preFeature.voxelY;
                    synapse.voxelZ = preFeature.voxelZ;
                    synapse.preNeuronId = preFeature.neuronID;
                    synapse.postNeuronId = postFeature.neuronID;

                    if (synapse.preNeuronId != synapse.postNeuronId) {
                        float pre = preFeature.pre;
                        float post;
                        float postAll;
                        if (preFeature.functionalCellType == "exc") {
                            post = postFeature.postExc;
                            postAll = postFeature.postAllExc;
                        } else {
                            post = postFeature.postInh;
                            postAll = postFeature.postAllInh;
                        }

                        synapse.pre = pre;
                        synapse.post = post;
                        synapse.postAll = postAll;
                        if (postAll != 0) {
                            float innervationMean = pre * post / postAll;
                            std::poisson_distribution<> distribution(innervationMean);
                            synapse.count = distribution(randomGenerator);
                        } else {
                            synapse.count = 0;
                        }
                        synapses.append(synapse);
                    }
                }
            }
        }
    }
    return synapses;
}

/*
    Initializes hash sets that provide access to the pre-
    and postsynaptic neurons within each voxel. Also
    determines the number of voxels.
*/
void SynapseDistributor::initHashMaps() {
    // Init empty neuron hash tables for all voxels.
    for (int i = 0; i < mFeatures.size(); i++) {
        mVoxels.insert(mFeatures[i].voxelID);
    }
    QSetIterator<int> voxelIt(mVoxels);
    while (voxelIt.hasNext()) {
        int voxelId = voxelIt.next();
        QSet<int> empty;
        mPreNeuronsVoxelwise.insert(voxelId, empty);
        mPostNeuronsVoxelwise.insert(voxelId, empty);
    }

    // Register pre- and postsynaptic neuron features in all voxels.
    for (int i = 0; i < mFeatures.size(); i++) {
        Feature feature = mFeatures[i];
        int voxelId = feature.voxelID;
        if (feature.pre != 0) {
            mPreNeuronsVoxelwise[voxelId].insert(i);
        }
        if (feature.postExc != 0 || feature.postInh != 0) {
            mPostNeuronsVoxelwise[voxelId].insert(i);
        }
    }
}