#include "CIS3DNeurons.h"
#include <QBitArray>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <stdexcept>
#include <QBuffer>
#include <QDataStream>
#include <QDebug>

/**
    Constructor.
*/
Neurons::Neurons()
{
}

/**
    Constructor.
    @param csvFile The neuron file to load.
    @throws runtime_error if file could not be loaded or parsed.
*/
Neurons::Neurons(const QString& csvFile)
{
    loadCSV(csvFile);
}

/**
    Adds another neuron to the collection.
    @param neuronProps The neuron to add.
*/
void
Neurons::addNeuron(const NeuronProperties& neuronProps)
{
    const int id = neuronProps.id;
    if (mPropsMap.contains(id))
    {
        throw std::runtime_error("Error in addNeuron: Neuron ID exists");
    }
    mPropsMap.insert(id, neuronProps);
}

/**
    Checks if the neuron with the specified ID exists.
    @param id The neuron ID.
    @return True if the neuron exists.
*/
bool
Neurons::exists(const int id) const
{
    return mPropsMap.contains(id);
}

/**
    Determines the cell type of the specified neuron.
    @param id The neuron ID.
    @return The ID of the cell type.
    @throws runtime_error if neuron ID does not exist.
*/
int
Neurons::getCellTypeId(const int id) const
{
    if (mPropsMap.contains(id))
    {
        return mPropsMap.value(id).cellTypeId;
    }
    QString msg = QString("Error in getCellTypeId: Neuron ID %1 does not exist.").arg(id);
    throw std::runtime_error(qPrintable(msg));
}

/**
    Determines the region of the specified neuron.
    @param id The neuron ID.
    @return The ID of the region.
    @throws runtime_error if neuron ID does not exist.
*/
int
Neurons::getRegionId(const int id) const
{
    const PropsMap::const_iterator it = mPropsMap.constFind(id);
    if (it == mPropsMap.constEnd())
    {
        const QString msg = QString("Error in getRegion: Neuron ID %1 does not exist.").arg(id);
        throw std::runtime_error(qPrintable(msg));
    }
    return it->regionId;
}

/**
    Determines the soma position of the specified neuron.
    @param id The neuron ID.
    @return The soma position.
    @throws runtime_error if neuron ID does not exist.
*/
Vec3f
Neurons::getSomaPosition(const int id) const
{
    if (mPropsMap.contains(id))
    {
        const NeuronProperties& props = mPropsMap.value(id);
        return Vec3f(props.somaX, props.somaY, props.somaZ);
    }
    QString msg = QString("Error in getCellTypeId: Neuron ID %1 does not exist.").arg(id);
    throw std::runtime_error(qPrintable(msg));
}

/**
    Determines the synaptic side of the specified neuron.
    @param id The neuron ID.
    @return The synaptic side (presy./postsy./both).
    @throws runtime_error if neuron ID does not exist.
*/
CIS3D::SynapticSide
Neurons::getSynapticSide(const int id) const
{
    const PropsMap::const_iterator it = mPropsMap.constFind(id);
    if (it == mPropsMap.constEnd())
    {
        const QString msg =
            QString("Error in getSynapticSide: Neuron ID %1 does not exist.").arg(id);
        throw std::runtime_error(qPrintable(msg));
    }
    return it->synapticSide;
}

/**
    Determines the laminar location of the specified neuron.
    @param id The neuron ID.
    @return The laminar location.
    @throws runtime_error if neuron ID does not exist.
*/
CIS3D::LaminarLocation
Neurons::getLaminarLocation(const int id) const
{
    const PropsMap::const_iterator it = mPropsMap.constFind(id);
    if (it == mPropsMap.constEnd())
    {
        const QString msg =
            QString("Error in getLaminarLocation: Neuron ID %1 does not exist.").arg(id);
        throw std::runtime_error(qPrintable(msg));
    }
    return it->loc;
}

/**
    Returns the IDs of all neurons with the specified properties.
    @includedCellTypeIds The desired cell types.
    @includedRegionIds The desired regions.
    @synapticSide The desired synaptic side.
    @return The neuron IDs.
*/
QList<int>
Neurons::getFilteredNeuronIds(const QList<int>& includedCellTypeIds,
                              const QList<int>& includedRegionIds,
                              const CIS3D::SynapticSide synapticSide) const
{
    const bool allCellTypesIncluded = (includedCellTypeIds.size() == 0);
    const bool allRegionsIncluded = (includedRegionIds.size() == 0);

    QBitArray selectedCTs;
    if (!allCellTypesIncluded)
    {
        int maxCT = -1;
        for (int i = 0; i < includedCellTypeIds.size(); ++i)
        {
            if (includedCellTypeIds[i] > maxCT)
            {
                maxCT = includedCellTypeIds[i];
            }
        }
        selectedCTs.resize(maxCT + 1);
        selectedCTs.fill(false);
        for (int i = 0; i < includedCellTypeIds.size(); ++i)
        {
            selectedCTs.setBit(includedCellTypeIds[i], true);
        }
    }

    QBitArray selectedRegions;
    if (!allRegionsIncluded)
    {
        int maxReg = -1;
        for (int i = 0; i < includedRegionIds.size(); ++i)
        {
            if (includedRegionIds[i] > maxReg)
            {
                maxReg = includedRegionIds[i];
            }
        }
        selectedRegions.resize(maxReg + 1);
        selectedRegions.fill(false);
        for (int i = 0; i < includedRegionIds.size(); ++i)
        {
            selectedRegions.setBit(includedRegionIds[i], true);
        }
    }

    QList<int> result;

    for (QMap<int, NeuronProperties>::ConstIterator it = mPropsMap.constBegin();
         it != mPropsMap.constEnd();
         ++it)
    {
        const int neuronId = it.key();
        const NeuronProperties& props = it.value();
        if ((allCellTypesIncluded ||
             (props.cellTypeId < selectedCTs.size() && selectedCTs.at(props.cellTypeId))) &&
            (allRegionsIncluded ||
             (props.regionId < selectedRegions.size() && selectedRegions.at(props.regionId))) &&
            ((synapticSide == CIS3D::BOTH_SIDES) ||
             (getSynapticSide(neuronId) == CIS3D::BOTH_SIDES) ||
             (synapticSide == getSynapticSide(neuronId))))
        {
            result.append(neuronId);
        }
    }

    return result;
}

/**
    Returns the IDs of all neurons meeting the filter condition.
    @filter The selection filter.
    @return The neuron IDs.
*/
QList<int>
Neurons::getFilteredNeuronIds(const SelectionFilter& filter) const
{
    const bool allCellTypesIncluded = (filter.cellTypeIds.size() == 0);
    const bool allRegionsIncluded = (filter.regionIds.size() == 0);
    const bool allNearestColumnsIncluded = (filter.nearestColumnIds.size() == 0);
    const bool allLaminarLocationsIncluded = (filter.laminarLocations.size() == 0);

    QBitArray selectedCTs;
    if (!allCellTypesIncluded)
    {
        int maxCT = -1;
        for (int i = 0; i < filter.cellTypeIds.size(); ++i)
        {
            if (filter.cellTypeIds[i] > maxCT)
            {
                maxCT = filter.cellTypeIds[i];
            }
        }
        selectedCTs.resize(maxCT + 1);
        selectedCTs.fill(false);
        for (int i = 0; i < filter.cellTypeIds.size(); ++i)
        {
            selectedCTs.setBit(filter.cellTypeIds[i], true);
        }
    }

    QBitArray selectedRegions;
    if (!allRegionsIncluded)
    {
        int maxReg = -1;
        for (int i = 0; i < filter.regionIds.size(); ++i)
        {
            if (filter.regionIds[i] > maxReg)
            {
                maxReg = filter.regionIds[i];
            }
        }
        selectedRegions.resize(maxReg + 1);
        selectedRegions.fill(false);
        for (int i = 0; i < filter.regionIds.size(); ++i)
        {
            selectedRegions.setBit(filter.regionIds[i], true);
        }
    }

    QBitArray selectedNearestColumns;
    if (!allNearestColumnsIncluded)
    {
        int maxNC = -1;
        for (int i = 0; i < filter.nearestColumnIds.size(); ++i)
        {
            if (filter.nearestColumnIds[i] > maxNC)
            {
                maxNC = filter.nearestColumnIds[i];
            }
        }
        selectedNearestColumns.resize(maxNC + 1);
        selectedNearestColumns.fill(false);
        for (int i = 0; i < filter.nearestColumnIds.size(); ++i)
        {
            selectedNearestColumns.setBit(filter.nearestColumnIds[i], true);
        }
    }

    QBitArray selectedLaminarLocations;
    if (!allLaminarLocationsIncluded)
    {
        selectedLaminarLocations.resize(4);
        selectedLaminarLocations.fill(false);
        for (int i = 0; i < filter.laminarLocations.size(); ++i)
        {
            selectedLaminarLocations.setBit(int(filter.laminarLocations[i]), true);
        }
    }

    QList<int> result;

    for (QMap<int, NeuronProperties>::ConstIterator it = mPropsMap.constBegin();
         it != mPropsMap.constEnd();
         ++it)
    {
        const int neuronId = it.key();
        const NeuronProperties& props = it.value();
        if ((allCellTypesIncluded ||
             (props.cellTypeId < selectedCTs.size() && selectedCTs.at(props.cellTypeId))) &&
            (allRegionsIncluded ||
             (props.regionId < selectedRegions.size() && selectedRegions.at(props.regionId))) &&
            (allNearestColumnsIncluded || (props.nearestColumnId < selectedNearestColumns.size() &&
                                           selectedNearestColumns.at(props.nearestColumnId))) &&
            (allLaminarLocationsIncluded || selectedLaminarLocations.at(int(props.loc))) &&
            ((filter.synapticSide == CIS3D::BOTH_SIDES) ||
             (getSynapticSide(neuronId) == CIS3D::BOTH_SIDES) ||
             (filter.synapticSide == getSynapticSide(neuronId))))
        {
            result.append(neuronId);
        }
    }

    return result;
}

/*
    Returns the properties of the specfied neuron.
    @param neuronId The ID of the neuron.
    @return The neuron properties.
*/
Neurons::NeuronProperties
Neurons::getNeuronProps(int neuronId) const
{
    const PropsMap::const_iterator it = mPropsMap.constFind(neuronId);
    if (it == mPropsMap.constEnd())
    {
        const QString msg =
            QString("Error in getNeuronProps: Neuron ID %1 does not exist.").arg(neuronId);
        throw std::runtime_error(qPrintable(msg));
    }
    return *it;
}

/**
    Saves the neurons to file.
    @param fileName The file name.
    @throws runtime_error if file could not saved.
*/
void
Neurons::saveCSV(const QString& fileName) const
{
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        const QString msg =
            QString("Error saving neurons file. Could not open file %1").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    QTextStream out(&file);
    const QChar sep = ',';

    out << "ID" << sep;
    out << "SomaX" << sep;
    out << "SomaY" << sep;
    out << "SomaZ" << sep;
    out << "CellTypeID" << sep;
    out << "NearestColumnID" << sep;
    out << "RegionID" << sep;
    out << "LaminarLocation" << sep;
    out << "SynapticSide"
        << "\n";

    for (PropsMap::ConstIterator it = mPropsMap.begin(); it != mPropsMap.end(); ++it)
    {
        const NeuronProperties& props = it.value();
        out << props.id << sep;
        out << props.somaX << sep;
        out << props.somaY << sep;
        out << props.somaZ << sep;
        out << props.cellTypeId << sep;
        out << props.nearestColumnId << sep;
        out << props.regionId << sep;
        out << int(props.loc) << sep;
        out << int(props.synapticSide) << "\n";
    }
}

/**
    Loads the neurons from file.
    @param fileName The name of the neuron file.
    @throws runtime_error if file could not be loaded or parsed.
*/
void
Neurons::loadCSV(const QString& fileName)
{
    QFile file(fileName);
    QTextStream(stdout) << "[*] Reading neurons from " << fileName << "\n";

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        const QString msg =
            QString("Error reading neurons file. Could not open file %1").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    const QChar sep = ',';
    QTextStream in(&file);

    int lineCount = 1;
    QString line = in.readLine();
    if (line.isNull())
    {
        const QString msg = QString("Error reading neurons file %1. No content.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    QStringList parts = line.split(sep);
    if (parts.size() != 9 || parts[0] != "ID" || parts[1] != "SomaX" || parts[2] != "SomaY" ||
        parts[3] != "SomaZ" || parts[4] != "CellTypeID" || parts[5] != "NearestColumnID" ||
        parts[6] != "RegionID" || parts[7] != "LaminarLocation" || parts[8] != "SynapticSide")
    {
        const QString msg =
            QString("Error reading neurons file %1. Invalid header columns.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    line = in.readLine();
    lineCount += 1;

    while (!line.isNull())
    {
        parts = line.split(sep);
        if (parts.size() != 9)
        {
            const QString msg =
                QString("Error reading neurons file %1. Invalid columns.").arg(fileName);
            throw std::runtime_error(qPrintable(msg));
        }

        NeuronProperties props;
        props.id = parts[0].toInt();
        props.somaX = parts[1].toFloat();
        props.somaY = parts[2].toFloat();
        props.somaZ = parts[3].toFloat();
        props.cellTypeId = parts[4].toInt();
        props.nearestColumnId = parts[5].toInt();
        props.regionId = parts[6].toInt();
        props.loc = static_cast<CIS3D::LaminarLocation>(parts[7].toInt());
        props.synapticSide = static_cast<CIS3D::SynapticSide>(parts[8].toInt());

        addNeuron(props);

        line = in.readLine();
        lineCount += 1;
    }
    QTextStream(stdout) << "[*] Completed reading " << mPropsMap.size() << " neurons.\n";
}