#ifndef SYNAPSELOCATION_H
#define SYNAPSELOCATION_H

#include <QString>
#include "CIS3DNetworkProps.h"
#include "Typedefs.h"

/*
    The explicit location of a synapse.
*/
struct Synapse {
    unsigned long id;
    unsigned int preNeuronId, postNeuronId;
    Vec3f pos;
    float distanceToSomaPre;
    float distanceToSomaPost;
    float approxDistanceToSomaPre;
    float approxDistanceToSomaPost;
};

/*
    Computes the explicit locations of synapses between pre- and postsynaptic
    neurons.
*/
class SynapseLocation {

public:    
    /**
        Performs the actual calculation of synapse locations.
        @preNeurons The presynaptic neurons.
        @postNeurons The postsynaptic neurons.
        @networkProps The model data.
        @outputDir The directoy to which the synapse properties are written.
    */
    static void computeSynapses(const PropsMap& preNeurons,
                         const PropsMap& postNeurons,
                         const NetworkProps& networkProps,
                         const QString& outputDir);

private:
    static void printSynapse(const Synapse& syn);
    static int poisson(const double mean);
};

#endif // SYNAPSELOCATION_H
