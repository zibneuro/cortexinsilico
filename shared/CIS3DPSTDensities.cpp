#include "CIS3DPSTDensities.h"
#include <stdexcept>
#include <QTextStream>
#include <QDebug>


float PSTDensities::getLengthDensity(const QString &postCtName,
                                     const CIS3D::Structure structure,
                                     const CIS3D::NeuronType preSynType) const
{
    if (!mDensityMap.contains(postCtName)) {
        const QString msg = QString("Error getting length density. Unknown cell type %1").arg(postCtName);
        throw std::runtime_error(qPrintable(msg));
    }
    PSTDensities::DensityKey key(structure, preSynType);
    return mDensityMap.value(postCtName).value(key).lengthDensity;
}


float PSTDensities::getAreaDensity(const QString &postCtName,
                                   const CIS3D::Structure structure,
                                   const CIS3D::NeuronType preSynType) const
{
    if (!mDensityMap.contains(postCtName)) {
        const QString msg = QString("Error getting area density. Unknown cell type %1").arg(postCtName);
        throw std::runtime_error(qPrintable(msg));
    }
    PSTDensities::DensityKey key(structure, preSynType);
    return mDensityMap.value(postCtName).value(key).areaDensity;
}



void PSTDensities::setLengthDensity(const QString &postCtName,
                                    const CIS3D::Structure structure,
                                    const CIS3D::NeuronType preSynType,
                                    const float density)
{
    if (!mDensityMap.contains(postCtName)) {
        mDensityMap.insert(postCtName, CellTypeDensityMap());
    }

    PSTDensities::DensityKey key(structure, preSynType);
    mDensityMap[postCtName][key].lengthDensity = density;
}


void PSTDensities::setAreaDensity(const QString &postCtName,
                                  const CIS3D::Structure structure,
                                  const CIS3D::NeuronType preSynType,
                                  const float density)
{
    if (!mDensityMap.contains(postCtName)) {
        mDensityMap.insert(postCtName, CellTypeDensityMap());
    }

    PSTDensities::DensityKey key(structure, preSynType);
    mDensityMap[postCtName][key].areaDensity = density;
}


void PSTDensities::saveCSV(const QString &fileName) const
{
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        const QString msg = QString("Error saving PSTDensities file. Could not open file %1").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    QTextStream out(&file);
    const QChar sep = ',';

    out << "Postsynaptic cell type" << sep << "Presynaptic type (inh/exc)" << sep
        << "Soma length" << sep << "Apical length" << sep << "Basal length" << sep
        << "Soma area"   << sep << "Apical area"   << sep << "Basal area"   << "\n";

    for (DensityMap::ConstIterator it = mDensityMap.begin(); it != mDensityMap.end(); ++it) {
        const QString ctName = it.key();
        const CellTypeDensityMap& densities = it.value();

        out << ctName << sep << "INHIBITORY" << sep
            << densities.value(DensityKey(CIS3D::SOMA,   CIS3D::INHIBITORY)).lengthDensity << sep
            << densities.value(DensityKey(CIS3D::APICAL, CIS3D::INHIBITORY)).lengthDensity << sep
            << densities.value(DensityKey(CIS3D::DEND,   CIS3D::INHIBITORY)).lengthDensity << sep
            << densities.value(DensityKey(CIS3D::SOMA,   CIS3D::INHIBITORY)).areaDensity << sep
            << densities.value(DensityKey(CIS3D::APICAL, CIS3D::INHIBITORY)).areaDensity << sep
            << densities.value(DensityKey(CIS3D::DEND,   CIS3D::INHIBITORY)).areaDensity << "\n";

        out << ctName << sep << "EXCITATORY" << sep
            << densities.value(DensityKey(CIS3D::SOMA,   CIS3D::EXCITATORY)).lengthDensity << sep
            << densities.value(DensityKey(CIS3D::APICAL, CIS3D::EXCITATORY)).lengthDensity << sep
            << densities.value(DensityKey(CIS3D::DEND,   CIS3D::EXCITATORY)).lengthDensity << sep
            << densities.value(DensityKey(CIS3D::SOMA,   CIS3D::EXCITATORY)).areaDensity << sep
            << densities.value(DensityKey(CIS3D::APICAL, CIS3D::EXCITATORY)).areaDensity << sep
            << densities.value(DensityKey(CIS3D::DEND,   CIS3D::EXCITATORY)).areaDensity << "\n";
    }
}


void PSTDensities::loadCSV(const QString &fileName)
{
    QFile file(fileName);
    QTextStream(stdout) << "[*] Reading PST densities from " << fileName << "\n";

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString msg = QString("Error reading PSTDensities file. Could not open file %1").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    const QChar sep = ',';
    QTextStream in(&file);

    int lineCount = 1;
    QString line = in.readLine();
    if (line.isNull()) {
        const QString msg = QString("Error reading PSTDensities file %1. No content.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    QStringList parts = line.split(sep);
    if (parts.size() != 8 ||
        parts[ 0] != "Postsynaptic cell type" ||
        parts[ 1] != "Presynaptic type (inh/exc)" ||
        parts[ 2] != "Soma length" ||
        parts[ 3] != "Apical length" ||
        parts[ 4] != "Basal length" ||
        parts[ 5] != "Soma area" ||
        parts[ 6] != "Apical area" ||
        parts[ 7] != "Basal area")
    {
        const QString msg = QString("Error reading PSTDensities file %1. Invalid column headers.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    line = in.readLine();
    lineCount += 1;

    while (!line.isNull()) {
        parts = line.split(sep);
        if (parts.size() != 8) {
            const QString msg = QString("Error reading PSTDensities file %1. Invalid columns.").arg(fileName);
            throw std::runtime_error(qPrintable(msg));
        }

        const QString ctName = parts[0];
        const QString type = parts[1];
        if (type != "INHIBITORY" && type != "EXCITATORY") {
            const QString msg = QString("Error reading PSTDensities file %1. Invalid presynaptic type.").arg(fileName);
            throw std::runtime_error(qPrintable(msg));
        }
        const CIS3D::NeuronType preType = (type == "INHIBITORY") ? CIS3D::INHIBITORY : CIS3D::EXCITATORY;

        const float somaLength   = parts[2].toFloat();
        const float apicalLength = parts[3].toFloat();
        const float basalLength  = parts[4].toFloat();
        const float somaArea     = parts[5].toFloat();
        const float apicalArea   = parts[6].toFloat();
        const float basalArea    = parts[7].toFloat();

        setLengthDensity(ctName, CIS3D::SOMA,   preType, somaLength);
        setLengthDensity(ctName, CIS3D::APICAL, preType, apicalLength);
        setLengthDensity(ctName, CIS3D::DEND,   preType, basalLength);

        setAreaDensity(ctName, CIS3D::SOMA,   preType, somaArea);
        setAreaDensity(ctName, CIS3D::APICAL, preType, apicalArea);
        setAreaDensity(ctName, CIS3D::DEND,   preType, basalArea);

        line = in.readLine();
        lineCount += 1;
    }
    QTextStream(stdout) << "[*] Completed reading PST densities\n";
}
