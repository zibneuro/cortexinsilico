#pragma once

#include <QString>
#include <QList>
#include <random>
#include "CIS3DAxonRedundancyMap.h"
#include "CIS3DBoundingBoxes.h"
#include "CIS3DCellTypes.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DNetworkProps.h"
#include "CIS3DNeurons.h"
#include "CIS3DRegions.h"
#include "CIS3DSparseField.h"
#include "CIS3DSparseVectorSet.h"
#include "CIS3DVec3.h"
#include "Histogram.h"
#include "InnervationStatistic.h"
#include "NeuronSelection.h"
#include "SparseFieldCalculator.h"
#include "SparseVectorCache.h"
#include "Typedefs.h"
#include "Util.h"
#include "UtilIO.h"

/*
    This class writes synapses to a synapses.csv file.
*/
class SynapseWriter {
   public:

    void init();

    /*
        Writes a synapses.csv file.

        @param filename The name of the file.
        @param A list of synapses.
        @throws runtime_error if the file cannot be written.
    */
    void write(QString fileName, QList<Synapse> synapses);

    /*
        Comparator for two synapses. Synapse is considered smaller than other synapse, if voxel ID
        is lower. If the voxel IDs are identical the comparison is based on the pre- and then the 
        postsynaptic neuron ID.

        @param a First synapse.
        @param b Second synapse.
        @return True, if the first synapse is smaller than the second synapse.
    */
    static bool lessThan(Synapse& a, Synapse& b);
};
