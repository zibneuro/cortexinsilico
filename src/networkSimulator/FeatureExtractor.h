#pragma once

#include <omp.h>
#include <QDebug>
#include <QHash>
#include <QPair>
#include <QScopedPointer>
#include <QtAlgorithms>
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
    This class extracts features (e.g., number of boutons, dendritic spines)
    from the model data. All features are provided voxelwise. The voxel grid
    to be extracted from the complete model is specified by an origin (spatial
    coordinates) and the number of voxels in each direction. The output is
    written into the file "features.csv".
*/
class FeatureExtractor {
   public:
    /*
        Constructor.

        @param props The complete model data.
    */
    FeatureExtractor(NetworkProps& props);

    /*
        Extracts the model features from the specified subvolume and
        writes them into a file named features.csv.

        @param origin Origin of the subvolume (in spatial coordinates).
        @param dimensions Number of voxels in each direction from the origin.
    */
    void writeFeaturesToFile(QVector<float> origin, QVector<int> dimensions);

   private:
    /*
        Determines all neurons within the subvolume (based on soma position)

        @param origin Origin of the voxelcube.
        @param dimensions Number of voxels in each direction.
        @return List of neuron IDs.
    */
    QList<int> getNeuronsWithinVolume(QVector<float> origin, QVector<int> dimensions);

    /*
        Determines unique ID for the specified voxel.

        @param locationLocal Position of the voxel relative to the origin.
        @param dimensions Number of voxels in each direction.
        @return The ID of the voxel.
    */
    int getVoxelId(Vec3i locationLocal, QVector<int> dimensions);

    /*
        Retrieves the pre- and postsynaptic target counts for one neuron within all voxels.

        @param pre The presynaptic target counts.
        @param postExc The postsynaptic target counts for (presynaptic) excitatory neurons.
        @param postAllExc The overall postsynaptic target counts for (presynaptic) excitatory
        neurons.
        @param postInh The postsynaptic target counts for (presynaptic) inhibitory neurons.
        @param postAllInh The overall postsynaptic target counts for (presynaptic) inhibitory
        neurons.
        @param origin The origin of the voxel grid.
        @param dimensions The number of voxels in each direction.
        @return All voxels that with the pre- and postsynaptic target counts.

    */
    QList<Voxel> determineVoxels(SparseField* pre, SparseField* postExc, SparseField* postAllExc,
                                 SparseField* postInh, SparseField* postAllInh,
                                 QVector<float> origin, QVector<int> dimensions);

    /*
        Retrieves the features for all neurons and all voxels.

        @param neurons The neurons within the subvolume.
        @param origin Origin of the subvolume.
        @param dimensions Number of voxels in each direction.
        @return A list of features.
    */
    QList<Feature> extractFeatures(QList<int> neurons, QVector<float> origin,
                                   QVector<int> dimensions);

    /*
        Writes the features into a file named features.csv

        @param features A list with the features.
    */
    void writeCSV(QList<Feature>& features);

    /*
        Comparator for two features. Feature is considered smaller than other feature, if voxel ID
        is lower. If the voxel IDs are identical the comparison is based on the neuron ID.

        @param a First feature.
        @param b Second feature.
        @return True, if the first feature is smaller than the second feature.
    */
    static bool lessThan(Feature& a, Feature& b);

    /*
        The model data.
    */
    NetworkProps& mNetworkProps;
};
