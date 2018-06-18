#include "SynapseLocation.h"
#include <QDebug>
#include <QVector>
#include "CIS3DSparseField.h"
#include "Util.h"

void SynapseLocation::computeSynapses(const PropsMap& preNeurons, const PropsMap& postNeurons,
                                      const NetworkProps& networkProps,
                                      const QString& /*outputDir*/) {
    long numPairs = 0;
    const long totalNumPairs = preNeurons.size() * postNeurons.size();

    for (PropsMap::ConstIterator postIt = postNeurons.begin(); postIt != postNeurons.end();
         ++postIt) {
        const int postNeuronId = postIt.key();
        const NeuronProps& postProps = postIt.value();
        for (PropsMap::ConstIterator preIt = preNeurons.begin(); preIt != preNeurons.end();
             ++preIt) {
            const int preNeuronId = preIt.key();
            const NeuronProps& preProps = preIt.value();

            if (!Util::overlap(preProps, postProps)) {
                continue;
            }

            SparseField innervationField;
            if (networkProps.cellTypes.isExcitatory(preProps.cellTypeId)) {
                innervationField = multiply(*(preProps.boutons), *(postProps.pstExc));
            } else {
                innervationField = multiply(*(preProps.boutons), *(postProps.pstInh));
            }

            SparseField::Locations locations;
            SparseField::Field field;
            innervationField.getFieldValues(locations, field);

            const Vec3f somaPre = preProps.somaPos;
            const Vec3f somaPost = postProps.somaPos;

            QVector<Synapse> synapses;

            for (int i = 0; i < locations.size(); ++i) {
                const Vec3i& loc = locations[i];
                const Vec3f origin = innervationField.getOrigin();
                const Vec3f voxelSize = innervationField.getVoxelSize();
                const float x = origin.getX() + (float(loc.getX()) + 0.5f) * voxelSize.getX();
                const float y = origin.getY() + (float(loc.getY()) + 0.5f) * voxelSize.getY();
                const float z = origin.getZ() + (float(loc.getZ()) + 0.5f) * voxelSize.getZ();
                const float innervation = field[i];
                const int numSynapses = poisson(innervation);
                for (int s = 0; s < numSynapses; ++s) {
                    Synapse syn;
                    syn.preNeuronId = preNeuronId;
                    syn.postNeuronId = postNeuronId;
                    syn.pos = Vec3f(x, y, z);
                    syn.approxDistanceToSomaPre = (syn.pos - somaPre).length();
                    syn.approxDistanceToSomaPost = (syn.pos - somaPost).length();
                    synapses.append(syn);
                    printSynapse(syn);
                }
            }
            ++numPairs;
            if (numPairs % 10000 == 0) {
                qDebug() << "Processed neuron pairs:" << numPairs << "/" << totalNumPairs << "("
                         << double(numPairs) * 100. / double(totalNumPairs) << "%)";
            }
        }
    }
}

void SynapseLocation::printSynapse(const Synapse& syn) {
    const QString str = QString("PreID: %1.\tPostID: %2. (%3, %4, %5) SomaDist (%6, %7)\n")
                            .arg(syn.preNeuronId)
                            .arg(syn.postNeuronId)
                            .arg(syn.pos.getX())
                            .arg(syn.pos.getY())
                            .arg(syn.pos.getZ())
                            .arg(syn.approxDistanceToSomaPre)
                            .arg(syn.approxDistanceToSomaPost);
    qDebug() << str;
}

int SynapseLocation::poisson(const double mean) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::poisson_distribution<> d(mean);
    return d(gen);
}
