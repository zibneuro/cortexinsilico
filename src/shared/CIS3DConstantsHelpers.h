#ifndef CIS3DCONSTANTSHELPERS_H
#define CIS3DCONSTANTSHELPERS_H

#include "CIS3DVec3.h"
#include <QString>
#include <QDir>
#include <QDir>
#include <QPair>

/**
    A set of enums to specify structural features of the neurons.
    Provides a collection of utility functions to obtain standard file names.


    The canonical directory structure is:

    <dataRootDir>/<innervationDataDir>/<InnervationPostDataDir>/...
                  <modelDataDir>/Boutons/...
                                 PSTall/...
                                 *.csv
                                 ...
*/

namespace CIS3D
{
    /**
        Number of digits to use for a neuron ID (pad with zeros in front)
    */
    extern const int NeuronIdNumDigits;

    enum SliceBand
    {
        NONE,
        FIRST,
        SECOND,
        BOTH
    };

    /**
        Categorization of neurons based on exc. vs. inh. property.
    */
    enum NeuronType
    {
        EXCITATORY,
        INHIBITORY,
        EXC_OR_INH
    };

    /**
        Categorization of neurons based on synaptic side.
    */
    enum SynapticSide
    {
        PRESYNAPTIC = 0,
        POSTSYNAPTIC = 1,
        BOTH_SIDES = 2
    };

    /**
        Neuron location with respect to layers.
    */
    enum LaminarLocation
    {
        UNKNOWN_LL = 0,
        INFRAGRANULAR = 1,
        GRANULAR = 2,
        SUPRAGRANULAR = 3
    };

    /**
       Structural feature of the neuron.
   */
    enum Structure
    {
        UNKNOWN = 0,
        SOMA = 1,
        AXON = 2,
        DEND = 3,
        APICAL = 4,
        BASAL = 5
    };

    /**
         Neuron location with respect to rat vS1.
     */
    enum S1Location
    {
        UNKNOWN_S1 = 0,
        INSIDE_S1,
        OUTSIDE_S1
    };

    /**
        Represents explicit synapse location.
    */
    struct Synapse
    {
        unsigned long id;
        unsigned int preNeuronId;
        Vec3f pos;
        float euclideanDistanceToSomaPre;
        float distanceToSomaPost;
        int sectionID;
        QString sectionName;
        float sectionX;
        Structure structure;
    };

    /*
        Returns a string representation of functional cell type.

        @param typeFunctional The functional cell type as enum.
        @return The functional cell type as string.
    */
    QString getFunctionalType(NeuronType functionalType);

    /*
        Returns a string representation of the synaptic side.

        @param side The synaptic side as enum.
        @return The synaptic side as string.
    */
    QString getSynapticSide(SynapticSide side);

    /*
        Returns a string representation of the laminar location.

        @param location The laminar location as enum.
        @return The laminar location as string.
    */
    QString getLaminarLocation(LaminarLocation location);

    /**
        A tuple of cell type ID and region ID
    */
    typedef QPair<int, int> CellTypeRegion;

    /**
        Get the directory containing the model data, relative to the data root
        @param dataRootDir The data root directory
    */
    QDir getModelDataDir(const QDir& dataRootDir, bool legacyPath = false);

    /**
        Get the directory containing the innervation data, relative to the data root
        @param dataRootDir The data root directory
    */
    QDir getInnervationDataDir(const QDir& dataRootDir);

    QString
    getNeuronIdFilePath(const QDir& dataDir, int neuronId);

    /**
        Get the directory containing the innervationPost data, relative to the innervation data directory.
        @param innervationDataDir The innervation directory
    */
    QDir getInnervationPostDataDir(const QDir& innervationDataDir);

    /**
        Get the directory containing the innervationPost data, relative to the data root
        @param dataRootDir The data root directory
    */
    QDir getInnervationPostDataDirFromRoot(const QDir& dataRootDir);

    /**
        Determines the name of the file with the bouton densities.
        @param neuronId The ID of the neuron.
        @return The file name.
    */
    QString getBoutonsFileName(const int neuronId);

    /**
        Determines the name of the directory with all bouton subdirs.
        @param modelDataDir The model data directory.
        @return The bouton root directory.
    */
    QDir getBoutonsRootDir(const QDir& modelDataDir);

    /**
        Determines the directory with all bouton files for the
        specified cell type.
        @param modelDataDir The model data directory.
        @param cellTypeName The cell type.
        @return The bouton directory.
    */
    QDir getBoutonsDir(const QDir& modelDataDir,
                       const QString& cellTypeName);

    /**
        Determines the bouton file for the specified neuron.
        @param modelDataDir The model data directory.
        @param cellTypeName The cell type.
        @param neuronId The neuron ID.
        @return The bouton file.
    */
    QString getBoutonsFileFullPath(const QDir& modelDataDir,
                                   const QString& cellTypeName,
                                   const int neuronId);

    /**
        Determines the name of the file with the PST density
        for the the specified neuron.
        @param neuronId The neuron ID.
        @param presynapticNeuronType The neuron type.
        @return The PST file name.
   */
    QString getPSTFileName(const int neuronId, const NeuronType presynapticNeuronType);

    /**
        Determines the name of the file with the post synaptic target density normalized
        by the overall post synaptic target density for the the specified neuron.
        @param neuronId The neuron ID.
        @param presynapticNeuronType The neuron type.
        @return The normalized PST file name.
    */
    QString getNormalizedPSTFileName(const int neuronId, const NeuronType presynapticNeuronType);

    QString getNormalizedApicalPSTFileName(const int neuronId, const NeuronType presynapticNeuronType);

    QString getNormalizedBasalPSTFileName(const int neuronId, const NeuronType presynapticNeuronType);

    /**
        Determines directory with all PST subdirs.
        @param modelDataDir The model data directory.
        @return The PST directory.
    */
    QDir getPSTRootDir(const QDir& modelDataDir);

    /**
        Determines the directory containing the normalized PST subdirs.
        @param modelDataDir The model data directory.
        @return The normalized PST root directory.
    */
    QDir getNormalizedPSTRootDir(const QDir& modelDataDir);

    QDir getNormalizedApicalPSTRootDir(const QDir& modelDataDir);

    QDir getNormalizedBasalPSTRootDir(const QDir& modelDataDir);

    /**
        Determines the directory containing the PST densities for
        the specified region and cell type.
        @param modelDataDir The model data directory.
        @param regionName The name of the region.
        @param cellTypeName The name of the cell type.
        @return The PST directory.
    */
    QDir getPSTDir(const QDir& modelDataDir,
                   const QString& regionName,
                   const QString& cellTypeName);

    /**
        Determines the directory containing the normalized PST densities
        for the specified cells.
        @param modelDataDir The model data directory.
        @param regionName The name of the region.
        @param cellTypeName The name of the cell type.
        @return The PST directory.
    */
    QDir getNormalizedPSTDir(const QDir& modelDataDir,
                             const QString& regionName,
                             const QString& cellTypeName);

    QDir getNormalizedApicalPSTDir(const QDir& modelDataDir,
                                   const QString& regionName,
                                   const QString& cellTypeName);

    QDir getNormalizedBasalPSTDir(const QDir& modelDataDir,
                                  const QString& regionName,
                                  const QString& cellTypeName);
    /**
        Determines PST file path for the specified neuron.
        @param modelDataDir The model data directory.
        @param regionName The name of the region.
        @param cellTypeName The name of the cell type.
        @param neuronId The neuron ID.
        @param presynapticNeuronType The neuron type.
        @return The PST file path.
    */
    QString getPSTFileFullPath(const QDir& modelDataDir,
                               const QString& regionName,
                               const QString& cellTypeName,
                               const int neuronId,
                               const NeuronType presynapticNeuronType);
    /**
       Determines normalized PST file path for the specified neuron.
       @param modelDataDir The model data directory.
       @param regionName The name of the region.
       @param cellTypeName The name of the cell type.
       @param neuronId The neuron ID.
       @param presynapticNeuronType The neuron type.
       @return The PST file path.
   */
    QString getNormalizedPSTFileFullPath(const QDir& modelDataDir,
                                         const QString& regionName,
                                         const QString& cellTypeName,
                                         const int neuronId,
                                         const NeuronType presynapticNeuronType);

    QString getNormalizedApicalPSTFileFullPath(const QDir& modelDataDir,
                                               const QString& regionName,
                                               const QString& cellTypeName,
                                               const int neuronId,
                                               const NeuronType presynapticNeuronType);

    QString getNormalizedBasalPSTFileFullPath(const QDir& modelDataDir,
                                              const QString& regionName,
                                              const QString& cellTypeName,
                                              const int neuronId,
                                              const NeuronType presynapticNeuronType);

    /**
        Determines the directory containing the overall PST density files.
        @param modelDataDir The model data directory.
        @return The overall PST density directory.
    */
    QDir getPSTAllDir(const QDir& modelDataDir);

    /**
        Determines the path of the overall PST density file for the specified
        neuron type.
        @param modelDataDir The model data directory.
        @param presynapticNeuronType The neuron type.
        @return The overall PST density file path.
    */
    QString getPSTAllFullPath(const QDir& modelDataDir, const NeuronType presynapticNeuronType);

    /**
        Determines the file containing the defintions of all barrel
        columns.
        @param modelDataDir The model data directory.
        @return The file name.
    */
    QString getColumnsFileName(const QDir& modelDataDir);

    /**
        Determines the file containing the hierarchical definition
        of regions contained in the model.
        @param modelDataDir The model data directory.
        @return The file name.
    */
    QString getRegionsFileName(const QDir& modelDataDir);

    /**
        Determines the file containing the neuron defintions.
        @param modelDataDir The model data directory.
        @return The file name.
    */
    QString getNeuronsFileName(const QDir& modelDataDir);

    /**
        Determines the file containing the bounding boxes for
        all neurons.
        @param modelDataDir The model data directory.
        @return The file name.
    */
    QString getBoundingBoxesFileName(const QDir& modelDataDir);

    /**
        Determines the file containing the cell type definitions of
        the model.
        @param modelDataDir The model data directory.
        @return The file name.
    */
    QString getCellTypesFileName(const QDir& modelDataDir);

    /**
        Determines the file containing the neuron ID mapping for
        duplicated axons.
        @param modelDataDir The model data directory.
        @return The file name.
    */
    QString getAxonRedundancyMapFileName(const QDir& modelDataDir);

    QString getMappingFilePath(QDir& dataRootDir, QString& network1, QString& network2);

    /**
        Determines the file containing calculated innervation for the specified
        region and cell type.
        @param innervationPostDataDir The InnervationPost data directory.
        @param regionName The name of the region.
        @param cellTypeName The name of the cell type.
        @return The file name.
    */
    QString getInnervationPostFileName(const QDir& innervationPostDataDir,
                                       const QString& regionName,
                                       const QString& cellTypeName);

    /**
        Determines the file containing the cell type specific PST densities.
        @param modelDataDir The model data directory.
        @return The file name.
    */
    QString getPSTDensitiesFileName(const QDir& modelDataDir);

    /**
        Converts structure enum to text string.
        @param structure The structure as enum.
        @return The structure as text string.
    */
    QString getStructureName(const Structure structure);
} // namespace CIS3D

#endif // CIS3DCONSTANTSHELPERS_H
