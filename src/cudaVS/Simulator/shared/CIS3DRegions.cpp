#include "CIS3DRegions.h"
#include <QChar>
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <stdexcept>

/**
    Constructor. Initializes default model of rat vS1.
*/
Regions::Regions() {
    mPropsMap.insert(0, RegionProperties(0, "Brain", -1));
    mPropsMap.insert(1, RegionProperties(1, "Neocortex", 0));
    mPropsMap.insert(2, RegionProperties(2, "Thalamus", 0));
    mPropsMap.insert(3, RegionProperties(3, "S1", 1));
    mPropsMap.insert(4, RegionProperties(4, "S1_Surrounding", 1));
    mPropsMap.insert(5, RegionProperties(5, "S1_Septum", 3));
    mPropsMap.insert(6, RegionProperties(6, "Alpha", 3));
    mPropsMap.insert(7, RegionProperties(7, "A1", 3));
    mPropsMap.insert(8, RegionProperties(8, "A2", 3));
    mPropsMap.insert(9, RegionProperties(9, "A3", 3));
    mPropsMap.insert(10, RegionProperties(10, "A4", 3));
    mPropsMap.insert(11, RegionProperties(11, "Beta", 3));
    mPropsMap.insert(12, RegionProperties(12, "B1", 3));
    mPropsMap.insert(13, RegionProperties(13, "B2", 3));
    mPropsMap.insert(14, RegionProperties(14, "B3", 3));
    mPropsMap.insert(15, RegionProperties(15, "B4", 3));
    mPropsMap.insert(16, RegionProperties(16, "Gamma", 3));
    mPropsMap.insert(17, RegionProperties(17, "C1", 3));
    mPropsMap.insert(18, RegionProperties(18, "C2", 3));
    mPropsMap.insert(19, RegionProperties(19, "C3", 3));
    mPropsMap.insert(20, RegionProperties(20, "C4", 3));
    mPropsMap.insert(21, RegionProperties(21, "Delta", 3));
    mPropsMap.insert(22, RegionProperties(22, "D1", 3));
    mPropsMap.insert(23, RegionProperties(23, "D2", 3));
    mPropsMap.insert(24, RegionProperties(24, "D3", 3));
    mPropsMap.insert(25, RegionProperties(25, "D4", 3));
    mPropsMap.insert(26, RegionProperties(26, "E1", 3));
    mPropsMap.insert(27, RegionProperties(27, "E2", 3));
    mPropsMap.insert(28, RegionProperties(28, "E3", 3));
    mPropsMap.insert(29, RegionProperties(29, "E4", 3));
    mPropsMap.insert(30, RegionProperties(30, "Alpha_Barreloid", 2));
    mPropsMap.insert(31, RegionProperties(31, "A1_Barreloid", 2));
    mPropsMap.insert(32, RegionProperties(32, "A2_Barreloid", 2));
    mPropsMap.insert(33, RegionProperties(33, "A3_Barreloid", 2));
    mPropsMap.insert(34, RegionProperties(34, "A4_Barreloid", 2));
    mPropsMap.insert(35, RegionProperties(35, "Beta_Barreloid", 2));
    mPropsMap.insert(36, RegionProperties(36, "B1_Barreloid", 2));
    mPropsMap.insert(37, RegionProperties(37, "B2_Barreloid", 2));
    mPropsMap.insert(38, RegionProperties(38, "B3_Barreloid", 2));
    mPropsMap.insert(39, RegionProperties(39, "B4_Barreloid", 2));
    mPropsMap.insert(40, RegionProperties(40, "Gamma_Barreloid", 2));
    mPropsMap.insert(41, RegionProperties(41, "C1_Barreloid", 2));
    mPropsMap.insert(42, RegionProperties(42, "C2_Barreloid", 2));
    mPropsMap.insert(43, RegionProperties(43, "C3_Barreloid", 2));
    mPropsMap.insert(44, RegionProperties(44, "C4_Barreloid", 2));
    mPropsMap.insert(45, RegionProperties(45, "Delta_Barreloid", 2));
    mPropsMap.insert(46, RegionProperties(46, "D1_Barreloid", 2));
    mPropsMap.insert(47, RegionProperties(47, "D2_Barreloid", 2));
    mPropsMap.insert(48, RegionProperties(48, "D3_Barreloid", 2));
    mPropsMap.insert(49, RegionProperties(49, "D4_Barreloid", 2));
    mPropsMap.insert(50, RegionProperties(50, "E1_Barreloid", 2));
    mPropsMap.insert(51, RegionProperties(51, "E2_Barreloid", 2));
    mPropsMap.insert(52, RegionProperties(52, "E3_Barreloid", 2));
    mPropsMap.insert(53, RegionProperties(53, "E4_Barreloid", 2));

    mPropsMap.insert(54, RegionProperties(54, "S1_Surrounding_Alpha", 4));
    mPropsMap.insert(55, RegionProperties(55, "S1_Surrounding_A1", 4));
    mPropsMap.insert(56, RegionProperties(56, "S1_Surrounding_A2", 4));
    mPropsMap.insert(57, RegionProperties(57, "S1_Surrounding_A3", 4));
    mPropsMap.insert(58, RegionProperties(58, "S1_Surrounding_A4", 4));
    mPropsMap.insert(59, RegionProperties(59, "S1_Surrounding_Beta", 4));
    mPropsMap.insert(60, RegionProperties(60, "S1_Surrounding_B1", 4));
    mPropsMap.insert(61, RegionProperties(61, "S1_Surrounding_B2", 4));
    mPropsMap.insert(62, RegionProperties(62, "S1_Surrounding_B3", 4));
    mPropsMap.insert(63, RegionProperties(63, "S1_Surrounding_B4", 4));
    mPropsMap.insert(64, RegionProperties(64, "S1_Surrounding_Gamma", 4));
    mPropsMap.insert(65, RegionProperties(65, "S1_Surrounding_C1", 4));
    mPropsMap.insert(66, RegionProperties(66, "S1_Surrounding_C2", 4));
    mPropsMap.insert(67, RegionProperties(67, "S1_Surrounding_C3", 4));
    mPropsMap.insert(68, RegionProperties(68, "S1_Surrounding_C4", 4));
    mPropsMap.insert(69, RegionProperties(69, "S1_Surrounding_Delta", 4));
    mPropsMap.insert(70, RegionProperties(70, "S1_Surrounding_D1", 4));
    mPropsMap.insert(71, RegionProperties(71, "S1_Surrounding_D2", 4));
    mPropsMap.insert(72, RegionProperties(72, "S1_Surrounding_D3", 4));
    mPropsMap.insert(73, RegionProperties(73, "S1_Surrounding_D4", 4));
    mPropsMap.insert(74, RegionProperties(74, "S1_Surrounding_E1", 4));
    mPropsMap.insert(75, RegionProperties(75, "S1_Surrounding_E2", 4));
    mPropsMap.insert(76, RegionProperties(76, "S1_Surrounding_E3", 4));
    mPropsMap.insert(77, RegionProperties(77, "S1_Surrounding_E4", 4));
    mPropsMap.insert(78, RegionProperties(78, "S1_Surrounding_F1", 4));
    mPropsMap.insert(79, RegionProperties(79, "S1_Surrounding_F2", 4));
    mPropsMap.insert(80, RegionProperties(80, "S1_Surrounding_F3", 4));
    mPropsMap.insert(81, RegionProperties(81, "S1_Surrounding_F4", 4));

    mPropsMap.insert(82, RegionProperties(82, "S1_Septum_Alpha", 5));
    mPropsMap.insert(83, RegionProperties(83, "S1_Septum_A1", 5));
    mPropsMap.insert(84, RegionProperties(84, "S1_Septum_A2", 5));
    mPropsMap.insert(85, RegionProperties(85, "S1_Septum_A3", 5));
    mPropsMap.insert(86, RegionProperties(86, "S1_Septum_A4", 5));
    mPropsMap.insert(87, RegionProperties(87, "S1_Septum_Beta", 5));
    mPropsMap.insert(88, RegionProperties(88, "S1_Septum_B1", 5));
    mPropsMap.insert(89, RegionProperties(89, "S1_Septum_B2", 5));
    mPropsMap.insert(90, RegionProperties(90, "S1_Septum_B3", 5));
    mPropsMap.insert(91, RegionProperties(91, "S1_Septum_B4", 5));
    mPropsMap.insert(92, RegionProperties(92, "S1_Septum_Gamma", 5));
    mPropsMap.insert(93, RegionProperties(93, "S1_Septum_C1", 5));
    mPropsMap.insert(94, RegionProperties(94, "S1_Septum_C2", 5));
    mPropsMap.insert(95, RegionProperties(95, "S1_Septum_C3", 5));
    mPropsMap.insert(96, RegionProperties(96, "S1_Septum_C4", 5));
    mPropsMap.insert(97, RegionProperties(97, "S1_Septum_Delta", 5));
    mPropsMap.insert(98, RegionProperties(98, "S1_Septum_D1", 5));
    mPropsMap.insert(99, RegionProperties(99, "S1_Septum_D2", 5));
    mPropsMap.insert(100, RegionProperties(100, "S1_Septum_D3", 5));
    mPropsMap.insert(101, RegionProperties(101, "S1_Septum_D4", 5));
    mPropsMap.insert(102, RegionProperties(102, "S1_Septum_E1", 5));
    mPropsMap.insert(103, RegionProperties(103, "S1_Septum_E2", 5));
    mPropsMap.insert(104, RegionProperties(104, "S1_Septum_E3", 5));
    mPropsMap.insert(105, RegionProperties(105, "S1_Septum_E4", 5));
    mPropsMap.insert(106, RegionProperties(106, "S1_Septum_F1", 5));
    mPropsMap.insert(107, RegionProperties(107, "S1_Septum_F2", 5));
    mPropsMap.insert(108, RegionProperties(108, "S1_Septum_F3", 5));
    mPropsMap.insert(109, RegionProperties(109, "S1_Septum_F4", 5));
}

/**
    Returns the name of the specified brain region.
    @param id The ID of the region.
    @throws runtime_error if ID is not found.
*/
QString Regions::getName(const int id) const {
    if (!mPropsMap.contains(id)) {
        const QString msg = QString("Region with ID %1 does not exist").arg(id);
        throw std::runtime_error(qPrintable(msg));
    }
    return mPropsMap.value(id).name;
}

/**
    Returns the ID of the specified brain region.
    @param name The name of the region.
    @throws runtime_error if name is not found.
*/
int Regions::getId(const QString& name) const {
    for (PropsMap::ConstIterator it = mPropsMap.begin(); it != mPropsMap.end(); ++it) {
        const RegionProperties& props = it.value();
        if (props.name == name) {
            return props.id;
        }
    }
    const QString msg = QString("Region with name %1 does not exist").arg(name);
    throw std::runtime_error(qPrintable(msg));
}

/**
    Checks whether the specified name exists.
    @param name The name of the cell type.
    @return True, if the name exists.
*/
bool Regions::nameExists(const QString& name) const {
    for (PropsMap::ConstIterator it = mPropsMap.begin(); it != mPropsMap.end(); ++it) {
        const RegionProperties& props = it.value();
        if (props.name == name) {
            return true;
        }
    }
    return false;
}

/**
    Returns the IDs of all brain regions.
    @param id The IDs.
*/
QList<int> Regions::getAllRegionIds() const { return mPropsMap.keys(); }

/**
    Determines whether the specified region is contained in the specified
    subregion.
    @param id The ID of the region to check.
    @param subtreeRoot The ID of the subregion.
    @return True if the region is contained in the subregion.
*/
bool Regions::isInSubtree(const int regionId, const int subtreeRoot) const {
    PropsMap::ConstIterator it = mPropsMap.find(regionId);
    if (it == mPropsMap.end()) {
        const QString msg = QString("Invalid region id: %1").arg(regionId);
        throw std::runtime_error(qPrintable(msg));
    }
    if (regionId == subtreeRoot) {
        return true;
    } else if (it.value().parentId == -1) {
        return false;
    } else {
        return isInSubtree(it.value().parentId, subtreeRoot);
    }
}

/**
    Saves the region to file.
    @param fileName The name of the file.
    @throws runtime_error if saving the file fails.
*/
void Regions::saveCSV(const QString& fileName) const {
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        const QString msg =
            QString("Error saving regions file. Could not open file %1").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    QTextStream out(&file);
    const QChar sep = ',';

    out << "ID" << sep << "Name" << sep << "ParentId"
        << "\n";

    for (PropsMap::ConstIterator it = mPropsMap.begin(); it != mPropsMap.end(); ++it) {
        const RegionProperties& props = it.value();
        out << props.id << sep << props.name << sep << props.parentId << "\n";
    }
}

/**
    Loads the region from file.
    @param fileName The name of the file.
    @throws runtime_error if loading or parsing the file fails.
*/
void Regions::loadCSV(const QString& fileName) {
    QFile file(fileName);
    QTextStream(stdout) << "[*] Reading regions from " << fileName << "\n";

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString msg =
            QString("Error reading regions file. Could not open file %1").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    const QChar sep = ',';
    QTextStream in(&file);

    int lineCount = 1;
    QString line = in.readLine();
    if (line.isNull()) {
        const QString msg = QString("Error reading regions file %1. No content.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    QStringList parts = line.split(sep);
    if (parts.size() != 3 || parts[0] != "ID" || parts[1] != "Name" || parts[2] != "ParentId") {
        const QString msg =
            QString("Error reading regions file %1. Invalid column headers.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    line = in.readLine();
    lineCount += 1;

    while (!line.isNull()) {
        parts = line.split(sep);
        if (parts.size() != 3) {
            const QString msg =
                QString("Error reading regions file %1. Invalid columns.").arg(fileName);
            throw std::runtime_error(qPrintable(msg));
        }

        RegionProperties props;
        props.id = parts[0].toInt();
        props.name = parts[1];
        props.parentId = parts[2].toInt();

        mPropsMap.insert(props.id, props);

        line = in.readLine();
        lineCount += 1;
    }
    QTextStream(stdout) << "[*] Completed reading " << mPropsMap.size() << " regions.\n";
}
