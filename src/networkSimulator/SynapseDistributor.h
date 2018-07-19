#pragma once

#include <QList>
#include <QSet>
#include <QVector>
#include <QHash>
#include <QString>
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
            Generalized Peters' rule parametrized by theta_1 to theta_4.
        */
        GeneralizedPeters,
    };
    /*
        Constructor.

        @param features The neuron features.
    */
    SynapseDistributor(QList<Feature>& features, QSet<int> voxelIds, QString outputMode);

    /*
        Calculates synase counts according to the specified mode
        and parameters.

        @param rule Calculation rule.
        @param parameters Rule parameters.
        @return A list of synapses.
    */
    void apply(Rule rule, QVector<float> parameters);

   private:
    /*
        Caculates the synaptic innervation value according to the
        specified rule.

        @param rule Selected rule.
        @param parameters The rule parameters.
        @param pre Presynaptic bouton count.
        @param post Postsynaptic target count.
        @param postAll Overall postsynaptic target count.
        @return The mean innervation value.
    */
    float determineInnervationMean(Rule rule, QVector<float> parameters, float pre, float post,
                                   float postAll);

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
    bool mSparse;
    bool mPerVoxel;
};
