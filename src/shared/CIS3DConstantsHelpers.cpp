#include "CIS3DConstantsHelpers.h"

const int CIS3D::NeuronIdNumDigits = 7;

/**
    Determines the name of the file with the bouton densities.
    @param neuronId The ID of the neuron.
    @return The file name.
*/
QString CIS3D::getBoutonsFileName(const int neuronId) {
    return QString("Boutons_%1.dat").arg(neuronId, NeuronIdNumDigits, 10, QChar('0'));
}

/**
    Determines the name of the directory with all bouton subdirs.
    @param dataRootDir The base data directory.
    @return The bouton root directory.
*/
QString CIS3D::getBoutonsFileFullPath(const QDir &dataRootDir,
                                      const QString &cellTypeName,
                                      const int neuronId)
{
    QString path = getBoutonsDir(dataRootDir, cellTypeName).absolutePath();
    path += "/" + getBoutonsFileName(neuronId);
    return path;
}

/**
    Determines the directory with all bouton files for the
    specified cell type.
    @param dataRootDir The base data directory.
    @param cellTypeName The cell type.
    @return The bouton directory.
*/
QDir CIS3D::getBoutonsRootDir(const QDir &dataRootDir) {
    QDir boutonsRootDir(dataRootDir.absolutePath() + "/Boutons");
    return boutonsRootDir;
}

/**
    Determines the bouton file for the specified neuron.
    @param rootDir The base data directory.
    @param cellTypeName The cell type.
    @param neuronId The neuron ID.
    @return The bouton file.
*/
QDir CIS3D::getBoutonsDir(const QDir &dataRootDir,
                          const QString &cellTypeName)
{
    QDir boutonsDir(getBoutonsRootDir(dataRootDir).absolutePath() + "/" + cellTypeName);
    return boutonsDir;
}

/**
     Determines the name of the file with the PST density
     for the the specified neuron.
     @param neuronId The neuron ID.
     @param presynapticNeuronType The neuron type.
     @return The PST file name.
*/
QString CIS3D::getPSTFileName(const int neuronId, const NeuronType presynapticNeuronType) {
    if (presynapticNeuronType == EXCITATORY) {
        return QString("PST_excitatoryPre_%1.dat").arg(neuronId, CIS3D::NeuronIdNumDigits, 10, QChar('0'));
    }
    else {
        return QString("PST_inhibitoryPre_%1.dat").arg(neuronId, CIS3D::NeuronIdNumDigits, 10, QChar('0'));
    }
}

/**
    Determines the name of the file with the post synaptic target density normalized
    by the overall post synaptic target density for the the specified neuron.
    @param neuronId The neuron ID.
    @param presynapticNeuronType The neuron type.
    @return The normalized PST file name.
*/
QString CIS3D::getNormalizedPSTFileName(const int neuronId, const CIS3D::NeuronType presynapticNeuronType) {
    if (presynapticNeuronType == EXCITATORY) {
        return QString("NormalizedPST_excitatoryPre_%1.dat").arg(neuronId, CIS3D::NeuronIdNumDigits, 10, QChar('0'));
    }
    else {
        return QString("NormalizedPST_inhibitoryPre_%1.dat").arg(neuronId, CIS3D::NeuronIdNumDigits, 10, QChar('0'));
    }
}

/**
    Determines directory with all PST subdirs.
    @param dataRootDir The base data directory.
    @return The PST directory.
*/
QDir CIS3D::getPSTRootDir(const QDir &dataRootDir) {
    QDir PSTRootDir(dataRootDir.absolutePath() + "/PSTs");
    return PSTRootDir;
}

/**
    Determines the directory containing the normalized PST subdirs.
    @param dataRootDir The base data directory.
    @return The normalized PST root directory.
*/
QDir CIS3D::getNormalizedPSTRootDir(const QDir &dataRootDir) {
    QDir normalizedPSTRootDir(dataRootDir.absolutePath() + "/NormalizedPSTs");
    return normalizedPSTRootDir;
}

/**
    Determines the directory containing the PST densities for
    the specified region and cell type.
    @param dataRootDir The base data directory.
    @param regionName The name of the region.
    @param cellTypeName The name of the cell type.
    @return The PST directory.
*/
QDir CIS3D::getPSTDir(const QDir &dataRootDir,
                      const QString &regionName,
                      const QString &cellTypeName)
{
    QDir PSTDir(getPSTRootDir(dataRootDir).absolutePath() + "/" + regionName + "/" + cellTypeName);
    return PSTDir;
}

/**
    Determines the directory containing the normalized PST densities
    for the specified cells.
    @param dataRootDir The base data directory.
    @param regionName The name of the region.
    @param cellTypeName The name of the cell type.
    @return The PST directory.
*/
QDir CIS3D::getNormalizedPSTDir(const QDir &dataRootDir,
                                const QString &regionName,
                                const QString &cellTypeName)
{
    QDir normalizedPSTDir(getNormalizedPSTRootDir(dataRootDir).absolutePath() + "/" + regionName + "/" + cellTypeName);
    return normalizedPSTDir;
}

/**
    Determines PST file path for the specified neuron.
    @param dataRootDir The base data directory.
    @param regionName The name of the region.
    @param cellTypeName The name of the cell type.
    @param neuronId The neuron ID.
    @param presynapticNeuronType The neuron type.
    @return The PST file path.
*/
QString CIS3D::getPSTFileFullPath(const QDir &dataRootDir,
                                  const QString &regionName,
                                  const QString &cellTypeName,
                                  const int neuronId,
                                  const CIS3D::NeuronType presynapticNeuronType)
{
    QString path = getPSTDir(dataRootDir, regionName, cellTypeName).absolutePath();
    path += "/" + getPSTFileName(neuronId, presynapticNeuronType);
    return path;
}

/**
    Determines normalized PST file path for the specified neuron.
    @param dataRootDir The base data directory.
    @param regionName The name of the region.
    @param cellTypeName The name of the cell type.
    @param neuronId The neuron ID.
    @param presynapticNeuronType The neuron type.
    @return The PST file path.
*/
QString CIS3D::getNormalizedPSTFileFullPath(const QDir &dataRootDir,
                                            const QString &regionName,
                                            const QString &cellTypeName,
                                            const int neuronId,
                                            const CIS3D::NeuronType presynapticNeuronType)
{
    QString path = getNormalizedPSTDir(dataRootDir, regionName, cellTypeName).absolutePath();
    path += "/" + getNormalizedPSTFileName(neuronId, presynapticNeuronType);
    return path;
}

/**
    Determines the file containing the hierarchical definition
    of regions contained in the model.
    @param dataRootDir The base data directory.
    @return The file name.
*/
QString CIS3D::getRegionsFileName(const QDir& dataRootDir) {
    return dataRootDir.absolutePath() + "/Regions.csv";
}

/**
    Determines the file containing the defintions of all barrel
    columns.
    @param dataRootDir The base data directory.
    @return The file name.
*/
QString CIS3D::getColumnsFileName(const QDir& dataRootDir) {
    return dataRootDir.absolutePath() + "/Columns.csv";
}

/**
    Determines the file containing the neuron defintions.
    @param dataRootDir The base data directory.
    @return The file name.
*/
QString CIS3D::getNeuronsFileName(const QDir& dataRootDir) {
    return dataRootDir.absolutePath() + "/Neurons.csv";
}

/**
    Determines the file containing the bounding boxes for
    all neurons.
    @param dataRootDir The base data directory.
    @return The file name.
*/
QString CIS3D::getBoundingBoxesFileName(const QDir& dataRootDir) {
    return dataRootDir.absolutePath() + "/BoundingBoxes.csv";
}

/**
    Determines the file containing the cell type definitions of
    the model.
    @param dataRootDir The base data directory.
    @return The file name.
*/
QString CIS3D::getCellTypesFileName(const QDir& dataRootDir) {
    return dataRootDir.absolutePath() + "/CellTypes.csv";
}

/**
    Determines the file containing the neuron ID mapping for
    duplicated axons.
    @param dataRootDir The base data directory.
    @return The file name.
*/
QString CIS3D::getAxonRedundancyMapFileName(const QDir& dataRootDir) {
    return dataRootDir.absolutePath() + "/AxonRedundancyMap.dat";
}

/**
    Determines the directory containing the overall PST density files.
    @param dataRootDir The base data directory.
    @return The overall PST density directory.
*/
QDir CIS3D::getPSTAllDir(const QDir &dataRootDir) {
    QDir PSTRootDir(dataRootDir.absolutePath() + "/PSTall");
    return PSTRootDir;
}

/**
    Determines the path of the overall PST density file for the specified
    neuron type.
    @param dataRootDir The base data directory.
    @param presynapticNeuronType The neuron type.
    @return The overall PST density file path.
*/
QString CIS3D::getPSTAllFullPath(const QDir &dataRootDir, const CIS3D::NeuronType presynapticNeuronType) {
    if (presynapticNeuronType == EXCITATORY) {
        return getPSTAllDir(dataRootDir).absolutePath() + "/PSTall_excitatoryPre.dat";
    }
    else {
        return getPSTAllDir(dataRootDir).absolutePath() + "/PSTall_inhibitoryPre.dat";
    }
}

/**
    Determines the file containing calculated innervation for the specified
    region and cell type.
    @param dataRootDir The base data directory.
    @param regionName The name of the region.
    @param cellTypeName The name of the cell type.
    @return The file name.
*/
QString CIS3D::getInnervationPostFileName(const QDir &dataRootDir,
                                          const QString &regionName,
                                          const QString &cellTypeName)
{
    QDir innervationPostDir(dataRootDir.absolutePath() + "/InnervationPost");
    QString fileName = innervationPostDir.absolutePath() + "/" + regionName + "/" + regionName + "__" + cellTypeName + ".dat";
    return fileName;
}

/**
    Determines the file containing the cell type specific PST densities.
    @param dataRootDir The base data directory.
    @return The file name.
*/
QString CIS3D::getPSTDensitiesFileName(const QDir &dataRootDir)
{
    return dataRootDir.absolutePath() + "/PSTDensities.csv";
}

/**
    Converts structure enum to text string.
    @param structure The structure as enum.
    @return The structure as text string.
*/
QString CIS3D::getStructureName(const CIS3D::Structure structure)
{
    switch (structure) {
        case CIS3D::SOMA: return "Soma";
        case CIS3D::APICAL: return "Apical";
        case CIS3D::DEND: return "Basal";
        case CIS3D::AXON: return "Axon";
        default: return "Unknown";
    }
}
