#include "CIS3DNetworkProps.h"


void NetworkProps::setDataRoot(const QString& dataRoot) {
    this->dataRoot = dataRoot;
    dataRootDir = QDir(dataRoot);
}


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
