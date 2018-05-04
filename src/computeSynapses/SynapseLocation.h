#ifndef SYNAPSELOCATION_H
#define SYNAPSELOCATION_H

#include <QString>
#include "CIS3DNetworkProps.h"
#include "Typedefs.h"

struct Synapse {
    unsigned long id;
    unsigned int preNeuronId, postNeuronId;
    Vec3f pos;
    float distanceToSomaPre;
    float distanceToSomaPost;
    float approxDistanceToSomaPre;
    float approxDistanceToSomaPost;
};

class SynapseLocation {

public:
    static void computeSynapses(const PropsMap& preNeurons,
                         const PropsMap& postNeurons,
                         const NetworkProps& networkProps,
                         const QString& outputDir);

private:
    static void printSynapse(const Synapse& syn);
    static int poisson(const double mean);
};

#endif // SYNAPSELOCATION_H
