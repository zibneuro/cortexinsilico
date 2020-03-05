#include "CIS3DNetworkProps.h"
#include "QDebug"

NetworkProps::NetworkProps(bool legacyPath)
{
    useLegacyPath = legacyPath;
}

/**
    Sets the root directory in which all the model data is located.
    @param dataRoot The directory path.
    @param resetCache Whether to reset internal flag that data has been loaded.
*/
void NetworkProps::setDataRoot(const QString &dataRoot, const bool resetCache)
{
    this->dataRoot = dataRoot;
    dataRootDir = QDir(dataRoot);
    if (resetCache)
    {
        mFilesForComputationLoaded = false;
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

void NetworkProps::loadFilesForQuery(QString networkName)
{
    
    const QString cellTypesFile = CIS3D::getCellTypesFileName(dataRootDir);
    qDebug() << dataRootDir << networkName << cellTypesFile;
    cellTypes.loadCSV(cellTypesFile);

    const QString regionsFile = CIS3D::getRegionsFileName(dataRootDir);
    regions.loadCSV(regionsFile);

    if (!networkName.isEmpty())
    {
        const QString neuronsFile = CIS3D::getNeuronsFileName(dataRootDir.filePath(networkName));
        neurons.loadCSV(neuronsFile);

        const QString redundancyFile = CIS3D::getAxonRedundancyMapFileName(dataRootDir.filePath(networkName));
        axonRedundancyMap.loadCSV(redundancyFile);
    }
}
