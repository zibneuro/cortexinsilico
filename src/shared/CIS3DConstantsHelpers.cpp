#include "CIS3DConstantsHelpers.h"

const int CIS3D::NeuronIdNumDigits = 7;


QDir CIS3D::getModelDataDir(const QDir& dataRootDir) {
    const QDir modelDataDir(dataRootDir.absolutePath() + "/modelData");
    return modelDataDir;
}


QDir CIS3D::getInnervationDataDir(const QDir& dataRootDir) {
    const QDir innervationDataDir(dataRootDir.absolutePath() + "/innervation");
    return innervationDataDir;
}


QDir CIS3D::getInnervationPostDataDir(const QDir& innervationDataDir) {
    const QDir innervationPostDataDir(innervationDataDir.absolutePath() + "/InnervationPost");
    return innervationPostDataDir;
}


QDir CIS3D::getInnervationPostDataDirFromRoot(const QDir& dataRootDir) {
    const QDir innervationDataDir = getInnervationDataDir(dataRootDir);
    const QDir innervationPostDataDir = getInnervationPostDataDir(innervationDataDir);
    return innervationPostDataDir;
}


QString CIS3D::getBoutonsFileName(const int neuronId) {
    return QString("Boutons_%1.dat").arg(neuronId, NeuronIdNumDigits, 10, QChar('0'));
}


QString CIS3D::getBoutonsFileFullPath(const QDir &modelDataDir,
                                      const QString &cellTypeName,
                                      const int neuronId)
{
    QString path = getBoutonsDir(modelDataDir, cellTypeName).absolutePath();
    path += "/" + getBoutonsFileName(neuronId);
    return path;
}


QDir CIS3D::getBoutonsRootDir(const QDir &modelDataDir) {
    QDir boutonsRootDir(modelDataDir.absolutePath() + "/Boutons");
    return boutonsRootDir;
}


QDir CIS3D::getBoutonsDir(const QDir &modelDataDir,
                          const QString &cellTypeName)
{
    QDir boutonsDir(getBoutonsRootDir(modelDataDir).absolutePath() + "/" + cellTypeName);
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


QDir CIS3D::getPSTRootDir(const QDir &modelDataDir) {
    QDir PSTRootDir(modelDataDir.absolutePath() + "/PSTs");
    return PSTRootDir;
}


QDir CIS3D::getNormalizedPSTRootDir(const QDir &modelDataDir) {
    QDir normalizedPSTRootDir(modelDataDir.absolutePath() + "/NormalizedPSTs");
    return normalizedPSTRootDir;
}


QDir CIS3D::getPSTDir(const QDir &modelDataDir,
                      const QString &regionName,
                      const QString &cellTypeName)
{
    QDir PSTDir(getPSTRootDir(modelDataDir).absolutePath() + "/" + regionName + "/" + cellTypeName);
    return PSTDir;
}


QDir CIS3D::getNormalizedPSTDir(const QDir &modelDataDir,
                                const QString &regionName,
                                const QString &cellTypeName)
{
    QDir normalizedPSTDir(getNormalizedPSTRootDir(modelDataDir).absolutePath() + "/" + regionName + "/" + cellTypeName);
    return normalizedPSTDir;
}


QString CIS3D::getPSTFileFullPath(const QDir &modelDataDir,
                                  const QString &regionName,
                                  const QString &cellTypeName,
                                  const int neuronId,
                                  const CIS3D::NeuronType presynapticNeuronType)
{
    QString path = getPSTDir(modelDataDir, regionName, cellTypeName).absolutePath();
    path += "/" + getPSTFileName(neuronId, presynapticNeuronType);
    return path;
}


QString CIS3D::getNormalizedPSTFileFullPath(const QDir &modelDataDir,
                                            const QString &regionName,
                                            const QString &cellTypeName,
                                            const int neuronId,
                                            const CIS3D::NeuronType presynapticNeuronType)
{
    QString path = getNormalizedPSTDir(modelDataDir, regionName, cellTypeName).absolutePath();
    path += "/" + getNormalizedPSTFileName(neuronId, presynapticNeuronType);
    return path;
}


QString CIS3D::getRegionsFileName(const QDir& modelDataDir) {
    return modelDataDir.absolutePath() + "/Regions.csv";
}


QString CIS3D::getColumnsFileName(const QDir& modelDataDir) {
    return modelDataDir.absolutePath() + "/Columns.csv";
}


QString CIS3D::getNeuronsFileName(const QDir& modelDataDir) {
    return modelDataDir.absolutePath() + "/Neurons.csv";
}


QString CIS3D::getBoundingBoxesFileName(const QDir& modelDataDir) {
    return modelDataDir.absolutePath() + "/BoundingBoxes.csv";
}


QString CIS3D::getCellTypesFileName(const QDir& modelDataDir) {
    return modelDataDir.absolutePath() + "/CellTypes.csv";
}


QString CIS3D::getAxonRedundancyMapFileName(const QDir& modelDataDir) {
    return modelDataDir.absolutePath() + "/AxonRedundancyMap.dat";
}


QDir CIS3D::getPSTAllDir(const QDir &modelDataDir) {
    QDir PSTRootDir(modelDataDir.absolutePath() + "/PSTall");
    return PSTRootDir;
}


QString CIS3D::getPSTAllFullPath(const QDir &modelDataDir, const CIS3D::NeuronType presynapticNeuronType) {
    if (presynapticNeuronType == EXCITATORY) {
        return getPSTAllDir(modelDataDir).absolutePath() + "/PSTall_excitatoryPre.dat";
    }
    else {
        return getPSTAllDir(modelDataDir).absolutePath() + "/PSTall_inhibitoryPre.dat";
    }
}


QString CIS3D::getInnervationPostFileName(const QDir &innervationDir,
                                          const QString &regionName,
                                          const QString &cellTypeName)
{
    const QDir innervationPostDir = getInnervationPostDataDir(innervationDir);
    const QString fileName = innervationPostDir.absolutePath() + "/" + regionName + "/" + regionName + "__" + cellTypeName + ".dat";
    return fileName;
}


QString CIS3D::getPSTDensitiesFileName(const QDir &modelDataDir)
{
    return modelDataDir.absolutePath() + "/PSTDensities.csv";
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
