#include "CIS3DNetworkProps.h"
#include "QDebug"

NetworkProps::NetworkProps(bool legacyPath){
    useLegacyPath = legacyPath;
}

/**
    Sets the root directory in which all the model data is located.
    @param dataRoot The directory path.
    @param resetCache Whether to reset internal flag that data has been loaded.
*/
void NetworkProps::setDataRoot(const QString& dataRoot, const bool resetCache) {
    this->dataRoot = dataRoot;
    dataRootDir = QDir(dataRoot);
    if(resetCache){
        mFilesForComputationLoaded = false;
    }
}

/**
    Loads the data required for computing the innervation.
    - CellTypes
    - Neurons
    - BoundingBoxes
    - Regions
    - AxonRedundancyMap
*/
void NetworkProps::loadFilesForSynapseComputation()
{
    if(!mFilesForComputationLoaded){
        const QDir modelDataDir = CIS3D::getModelDataDir(dataRootDir,useLegacyPath);
        

        const QString cellTypesFile = CIS3D::getCellTypesFileName(modelDataDir);
        cellTypes.loadCSV(cellTypesFile);

        const QString neuronsFile = CIS3D::getNeuronsFileName(modelDataDir);
        neurons.loadCSV(neuronsFile);

        const QString boxesFile = CIS3D::getBoundingBoxesFileName(modelDataDir);
        boundingBoxes.loadCSV(boxesFile);

        const QString regionsFile = CIS3D::getRegionsFileName(modelDataDir);
        regions.loadCSV(regionsFile);

        const QString redundancyFile = CIS3D::getAxonRedundancyMapFileName(modelDataDir);
        axonRedundancyMap.loadBinary(redundancyFile);

        qDebug() << "[*] Finished loading network props." << modelDataDir;

        mFilesForComputationLoaded = true;
    }
}

/**
    Loads the data required for computing a summary statistic about the
    network.
    - CellTypes
    - Neurons
    - Regions
    - AxonRedundancyMap
*/
void NetworkProps::loadFilesForQuery()
{
    const QDir modelDataDir = CIS3D::getModelDataDir(dataRootDir);

    const QString cellTypesFile = CIS3D::getCellTypesFileName(modelDataDir);
    cellTypes.loadCSV(cellTypesFile);

    const QString neuronsFile = CIS3D::getNeuronsFileName(modelDataDir);
    neurons.loadCSV(neuronsFile);

    const QString regionsFile = CIS3D::getRegionsFileName(modelDataDir);
    regions.loadCSV(regionsFile);

    const QString redundancyFile = CIS3D::getAxonRedundancyMapFileName(modelDataDir);
    axonRedundancyMap.loadBinary(redundancyFile);
}

/**
    Loads the data required for registering another neuron into an existing
    network.
    - CellTypes
    - Neurons
    - BoundingBoxes
    - Regions
    - AxonRedundancyMap
    - PSTDensity
*/
void NetworkProps::loadFilesForInputMapping()
{
   const QDir modelDataDir = CIS3D::getModelDataDir(dataRootDir);

    const QString cellTypesFile = CIS3D::getCellTypesFileName(modelDataDir);
    cellTypes.loadCSV(cellTypesFile);

    const QString neuronsFile = CIS3D::getNeuronsFileName(modelDataDir);
    neurons.loadCSV(neuronsFile);

    const QString boxesFile = CIS3D::getBoundingBoxesFileName(modelDataDir);
    boundingBoxes.loadCSV(boxesFile);

    const QString regionsFile = CIS3D::getRegionsFileName(modelDataDir);
    regions.loadCSV(regionsFile);

    const QString redundancyFile = CIS3D::getAxonRedundancyMapFileName(modelDataDir);
    axonRedundancyMap.loadBinary(redundancyFile);

    const QString pstDensitiesFile = CIS3D::getPSTDensitiesFileName(modelDataDir);
    densities.loadCSV(pstDensitiesFile);
}
