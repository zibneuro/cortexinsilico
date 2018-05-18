#include "CIS3DNetworkProps.h"

/**
    Sets the root directory in which all the model data is located.
    @param dataRoot The directory path.
*/
void NetworkProps::setDataRoot(const QString& dataRoot) {
    this->dataRoot = dataRoot;
    dataRootDir = QDir(dataRoot);
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
    const QString cellTypesFile = CIS3D::getCellTypesFileName(dataRootDir);
    cellTypes.loadCSV(cellTypesFile);

    const QString neuronsFile = CIS3D::getNeuronsFileName(dataRootDir);
    neurons.loadCSV(neuronsFile);

    const QString boxesFile = CIS3D::getBoundingBoxesFileName(dataRootDir);
    boundingBoxes.loadCSV(boxesFile);

    const QString regionsFile = CIS3D::getRegionsFileName(dataRootDir);
    regions.loadCSV(regionsFile);

    const QString redundancyFile = CIS3D::getAxonRedundancyMapFileName(dataRootDir);
    axonRedundancyMap.loadBinary(redundancyFile);
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
    const QString cellTypesFile = CIS3D::getCellTypesFileName(dataRootDir);
    cellTypes.loadCSV(cellTypesFile);

    const QString neuronsFile = CIS3D::getNeuronsFileName(dataRootDir);
    neurons.loadCSV(neuronsFile);

    const QString regionsFile = CIS3D::getRegionsFileName(dataRootDir);
    regions.loadCSV(regionsFile);

    const QString redundancyFile = CIS3D::getAxonRedundancyMapFileName(dataRootDir);
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
    const QString cellTypesFile = CIS3D::getCellTypesFileName(dataRootDir);
    cellTypes.loadCSV(cellTypesFile);

    const QString neuronsFile = CIS3D::getNeuronsFileName(dataRootDir);
    neurons.loadCSV(neuronsFile);

    const QString boxesFile = CIS3D::getBoundingBoxesFileName(dataRootDir);
    boundingBoxes.loadCSV(boxesFile);

    const QString regionsFile = CIS3D::getRegionsFileName(dataRootDir);
    regions.loadCSV(regionsFile);

    const QString redundancyFile = CIS3D::getAxonRedundancyMapFileName(dataRootDir);
    axonRedundancyMap.loadBinary(redundancyFile);

    const QString pstDensitiesFile = CIS3D::getPSTDensitiesFileName(dataRootDir);
    densities.loadCSV(pstDensitiesFile);
}
