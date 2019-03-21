#pragma once

#include <QDebug>
#include <QHash>
#include <QPair>
#include <QMap>
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
    written into the file "features.csv", "featuresSpatial.csv", "neurons.csv"
    and "voxels.csv".
*/
class FeatureExtractor {
   public:
    /*
        Constructor.

        @param props The complete model data.
    */
    FeatureExtractor(NetworkProps& props);

    /*
        Extracts the model features from the specified subvolume according
        to the specified filters and writes the results to file. Creates
        the files features.csv, neurons.csv, voxels.csv.

        @param origin Origin of the subvolume (in spatial coordinates).
        @param dimensions Number of voxels in each direction from the origin.
        @param cellTypes The cell types filter.
        @param regions The regions filter.
        @param neuronIds The neuron IDs filter (overrules all others, if set).
        @param samplingFactor The neuron sampling factor, 1: take all, 2: take half, etc.
    */
    void extract(QVector<float> origin, QVector<int> dimensions, QSet<QString> cellTypes,
                 QSet<QString> regions, QSet<int> neuronIds, int samplingFactor);

    /*
        Extracts all features from the model data. Writes the features per voxel
        into the folder "voxelFeatures", one file per voxel. Creates an index file
        "index.dat" with a mapping (neuronID -> voxel_1, ..., voxel_k).
        @param cellTypes The cell types filter.
    */
    void extractAll(QSet<QString> cellTypes);

   private:
    /*
         Determines all neurons that meet the specified properties.

         @param origin Origin of the subvolume (in spatial coordinates).
         @param dimensions Number of voxels in each direction from the origin.
         @param cellTypes The cell types filter.
         @param regions The regions filter.
         @param neuronIds The neuron IDs filter.
     */
    QList<int> determineNeurons(QVector<float> origin, QVector<int> dimensions,
                                QSet<QString> cellTypes, QSet<QString> regions,
                                QSet<int> neuronIds);

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
        @param samplingFactor The sampling factor, 1: take all, 2: take half, etc.
        @return A list of features.
    */
    QList<Feature> extractFeatures(QList<int> neurons, QVector<float> origin,
                                   QVector<int> dimensions, int samplingFactor);

    /*
        Writes the features into a csv file.

        @param fileName The name of the file.
        @param features A list with the features.
    */
    void writeFeatures(QString fileName, QList<Feature>& features);

    /*
        Writes the features into a csv file using spatial coordinates.

        @param fileName The name of the file.
        @param origin The corrected origin.
        @param features A list with the features.
    */
    void writeFeaturesSpatial(QString fileName, QList<Feature>& features, QVector<float> origin);

    /*
        Writes the features into a csv file without coordinates or voxel ID.

        @param fileName The name of the file.
        @param features A list with the features.
    */
    void writeFeaturesSparse(QString fileName, QList<Feature>& features);

    /*
        Writes properties of the extracted neurons into a csv file.

        @param fileName The name of the file.
        @param features A list with the features.
        @param origin Origin of the subvolume (in spatial coordinates).
        @param dimensions Number of voxels in each direction from the origin.
    */
    void writeNeurons(QString fileName, QList<Feature>& features, QVector<float> origin,
                      QVector<int> dimensions);

    /*
        Writes properties of the extracted voxels into a csv file.

        @param fileName The name of the file.
        @param features The extracted features.
    */
    void writeVoxels(QString fileName, QList<Feature>& features);

    /*
        Comparator for two features. Feature is considered smaller than other feature, if voxel ID
        is lower. If the voxel IDs are identical, the comparison is based on the neuron ID.

        @param a First feature.
        @param b Second feature.
        @return True, if the first feature is smaller than the second feature.
    */
    static bool lessThan(Feature& a, Feature& b);

    /*
        Determines all neurons that have intersecting bounding boxes with the
        specified subcube.

        @param origin The origin of the subcube.
        @param dimensions The number of voxels in each dimension.
        @return The intersecting neurons.
    */
    QList<int> determineIntersectingNeurons(QVector<float> origin, QVector<int> dimensions);

    /*
        Determines whether the soma is located within the specified subcube.

        @param somaPosition Spatial location of the soma.
        @param origin The origin of the subcube.
        @param dimensions The number of voxels in each dimension.
        @return True, if the soma is located within the subcube.
    */
    bool somaWithinSubcube(Vec3f somaPosition, QVector<float> origin, QVector<int> dimensions);

    /*
        Corrects the user specified origin such that it lies in the
        centre of the voxel (according to the grid exported from Amira).
        @param origin The user specified origin.
    */
    void correctOrigin(QVector<float>& origin);

    /*
        Creates a directory with the specified name. If the directory already exists,
        all files in the directory are deleted.
        @param dirName The name of the directory.
    */
    void createOutputDir(QString dirName);

    /*
        Determines the overall model size.
        @param origin The model origin (ouput).
        @param dimensions The model dimensions (ouput).
        @param voxelSize The voxel size (ouput).
    */
    void determineModelDimensions(QVector<float>& origin, QVector<int>& dimensions, QVector<float>& voxelSize);

    /*
        Writes the index to file, which maps neuron IDs to the voxels
        that contain features from the neuron.
        @fileName The name of the file.
        @index The index to write.
    */
    void writeIndexFile(QString fileName, QMap<int, QSet<QString> >& index);

    /*
        Loads a sparse field using a cache.
    */
    SparseField* loadSparseField(QString fileName);

    /*
        The model data.
    */
    NetworkProps& mNetworkProps;

    /*
        Sparse field cache. FileName -> SparseField
    */
    QMap<QString, SparseField*> mSparseFieldCache;

    /*
        Info level.
    */
    bool mVerbose;
};
