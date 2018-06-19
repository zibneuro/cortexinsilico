#pragma once

#include <QtCore>
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
    This class assigns synapse counts to each neuron pair within
    each voxel based on a synapse formation rule and structural
    neuron features.
*/
class SynapseDistributor {
   public:
    /*
        The synapse formation rule to be applied.
    */
    enum Rule {
        /*
            Synaptic innervation proportional to pre- and postsynaptic target counts.
        */
        PetersDefault
    };
    /*
        Constructor.

        @param features The neuron features.
    */
    SynapseDistributor(QList<Feature>& features);

    /*
        Calculates synase counts according to the specified mode
        and parameters.

        @param rule Calculation rule.
        @param parameters Rule parameters.
        @return A list of synapses.
    */
    QList<Synapse> apply(Rule rule, QVector<float> parameters);

   private:

    /*
        Initializes hash sets that provide access to the pre-
        and postsynaptic neurons within each voxel. Registers
        all voxel IDs.
    */
    void initHashMaps();

    QList<Feature>& mFeatures;
    QHash<int, QSet<int> > mPreNeuronsVoxelwise;
    QHash<int, QSet<int> > mPostNeuronsVoxelwise;
    QSet<int> mVoxels;
};