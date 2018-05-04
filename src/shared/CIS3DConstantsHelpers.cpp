#include "CIS3DConstantsHelpers.h"

const int CIS3D::NeuronIdNumDigits = 7;


QString CIS3D::getBoutonsFileName(const int neuronId) {
    return QString("Boutons_%1.dat").arg(neuronId, NeuronIdNumDigits, 10, QChar('0'));
}


QString CIS3D::getBoutonsFileFullPath(const QDir &dataRootDir,
                                      const QString &cellTypeName,
                                      const int neuronId)
{
    QString path = getBoutonsDir(dataRootDir, cellTypeName).absolutePath();
    path += "/" + getBoutonsFileName(neuronId);
    return path;
}


QDir CIS3D::getBoutonsRootDir(const QDir &dataRootDir) {
    QDir boutonsRootDir(dataRootDir.absolutePath() + "/Boutons");
    return boutonsRootDir;
}


QDir CIS3D::getBoutonsDir(const QDir &dataRootDir,
                          const QString &cellTypeName)
{
    QDir boutonsDir(getBoutonsRootDir(dataRootDir).absolutePath() + "/" + cellTypeName);
    return boutonsDir;
}


QString CIS3D::getPSTFileName(const int neuronId, const NeuronType presynapticNeuronType) {
    if (presynapticNeuronType == EXCITATORY) {
        return QString("PST_excitatoryPre_%1.dat").arg(neuronId, CIS3D::NeuronIdNumDigits, 10, QChar('0'));
    }
    else {
        return QString("PST_inhibitoryPre_%1.dat").arg(neuronId, CIS3D::NeuronIdNumDigits, 10, QChar('0'));
    }
}


QString CIS3D::getNormalizedPSTFileName(const int neuronId, const CIS3D::NeuronType presynapticNeuronType) {
    if (presynapticNeuronType == EXCITATORY) {
        return QString("NormalizedPST_excitatoryPre_%1.dat").arg(neuronId, CIS3D::NeuronIdNumDigits, 10, QChar('0'));
    }
    else {
        return QString("NormalizedPST_inhibitoryPre_%1.dat").arg(neuronId, CIS3D::NeuronIdNumDigits, 10, QChar('0'));
    }
}


QDir CIS3D::getPSTRootDir(const QDir &dataRootDir) {
    QDir PSTRootDir(dataRootDir.absolutePath() + "/PSTs");
    return PSTRootDir;
}


QDir CIS3D::getNormalizedPSTRootDir(const QDir &dataRootDir) {
    QDir normalizedPSTRootDir(dataRootDir.absolutePath() + "/NormalizedPSTs");
    return normalizedPSTRootDir;
}


QDir CIS3D::getPSTDir(const QDir &dataRootDir,
                      const QString &regionName,
                      const QString &cellTypeName)
{
    QDir PSTDir(getPSTRootDir(dataRootDir).absolutePath() + "/" + regionName + "/" + cellTypeName);
    return PSTDir;
}


QDir CIS3D::getNormalizedPSTDir(const QDir &dataRootDir,
                                const QString &regionName,
                                const QString &cellTypeName)
{
    QDir normalizedPSTDir(getNormalizedPSTRootDir(dataRootDir).absolutePath() + "/" + regionName + "/" + cellTypeName);
    return normalizedPSTDir;
}


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


QString CIS3D::getRegionsFileName(const QDir& dataRootDir) {
    return dataRootDir.absolutePath() + "/Regions.csv";
}


QString CIS3D::getColumnsFileName(const QDir& dataRootDir) {
    return dataRootDir.absolutePath() + "/Columns.csv";
}


QString CIS3D::getNeuronsFileName(const QDir& dataRootDir) {
    return dataRootDir.absolutePath() + "/Neurons.csv";
}


QString CIS3D::getBoundingBoxesFileName(const QDir& dataRootDir) {
    return dataRootDir.absolutePath() + "/BoundingBoxes.csv";
}


QString CIS3D::getCellTypesFileName(const QDir& dataRootDir) {
    return dataRootDir.absolutePath() + "/CellTypes.csv";
}


QString CIS3D::getAxonRedundancyMapFileName(const QDir& dataRootDir) {
    return dataRootDir.absolutePath() + "/AxonRedundancyMap.dat";
}


QDir CIS3D::getPSTAllDir(const QDir &dataRootDir) {
    QDir PSTRootDir(dataRootDir.absolutePath() + "/PSTall");
    return PSTRootDir;
}


QString CIS3D::getPSTAllFullPath(const QDir &dataRootDir, const CIS3D::NeuronType presynapticNeuronType) {
    if (presynapticNeuronType == EXCITATORY) {
        return getPSTAllDir(dataRootDir).absolutePath() + "/PSTall_excitatoryPre.dat";
    }
    else {
        return getPSTAllDir(dataRootDir).absolutePath() + "/PSTall_inhibitoryPre.dat";
    }
}


QString CIS3D::getInnervationPostFileName(const QDir &dataRootDir,
                                          const QString &regionName,
                                          const QString &cellTypeName)
{
    QDir innervationPostDir(dataRootDir.absolutePath() + "/InnervationPost");
    QString fileName = innervationPostDir.absolutePath() + "/" + regionName + "/" + regionName + "__" + cellTypeName + ".dat";
    return fileName;
}


QString CIS3D::getPSTDensitiesFileName(const QDir &dataRootDir)
{
    return dataRootDir.absolutePath() + "/PSTDensities.csv";
}


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
