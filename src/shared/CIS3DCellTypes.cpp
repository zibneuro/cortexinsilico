#include "CIS3DCellTypes.h"
#include <stdexcept>
#include <QFile>
#include <QChar>
#include <QTextStream>
#include <QIODevice>
#include <QStringList>
#include <QDebug>

const QString FIELD_ID = "id";
const QString FIELD_NAME = "name";
const QString FIELD_ISEXCITATORY = "excitatory";
const QString FIELD_COLOR_DEND_R = "ColorDendriteR";
const QString FIELD_COLOR_DEND_G = "ColorDendriteG";
const QString FIELD_COLOR_DEND_B = "ColorDendriteB";
const QString FIELD_COLOR_AXON_R = "ColorAxonR";
const QString FIELD_COLOR_AXON_G = "ColorAxonG";
const QString FIELD_COLOR_AXON_B = "ColorAxonB";


/**
    Checks whether the specified cell type exists.
    @param id The ID of the cell type.
    @return true if the cell type exists.
*/
bool CellTypes::exists(const int id) const {
    return mPropsMap.contains(id);
}

/**
    Determines whether the specified cell type is excitatory.
    @param id The ID of the cell type.
    @return true if the cell type is excitatory.
    @throws runtime_error if the ID does not exist.
*/
bool CellTypes::isExcitatory(const int id) const {
    if (!mPropsMap.contains(id)) {
        throw std::runtime_error("Cell type id does not exist");
    }
    return mPropsMap.value(id).isExcitatory;
}

/**
    Retrieves name of the specified cell type.
    @param id The ID of the cell type.
    @return The name of the cell type.
    @throws runtime_error if the ID does not exist.
*/
QString CellTypes::getName(const int id) const
{
    QMap<int, Properties>::ConstIterator it = mPropsMap.find(id);
    if (it == mPropsMap.constEnd()) {
        const QString msg = QString("Cell type id %1 does not exist").arg(id);
        throw std::runtime_error(qPrintable(msg));
    }
    return it.value().name;
}

/**
    Retrieves dendrite color of the specified cell type.
    @param id The ID of the cell type.
    @return rgb-components of the color.
    @throws runtime_error if the ID does not exist.
*/
Vec3f CellTypes::getDendriteColor(const int id) const {
    QMap<int, Properties>::ConstIterator it = mPropsMap.find(id);
    if (it == mPropsMap.constEnd()) {
        const QString msg = QString("Cell type id %1 does not exist").arg(id);
        throw std::runtime_error(qPrintable(msg));
    }

    Vec3f col;
    col.setX(it.value().colorDendR);
    col.setY(it.value().colorDendG);
    col.setZ(it.value().colorDendB);
    return col;
}

/**
    Retrieves axon color of the specified cell type.
    @param id The ID of the cell type.
    @return rgb-components of the color.
    @throws runtime_error if the ID does not exist.
*/
Vec3f CellTypes::getAxonColor(const int id) const {
    QMap<int, Properties>::ConstIterator it = mPropsMap.find(id);
    if (it == mPropsMap.constEnd()) {
        const QString msg = QString("Cell type id %1 does not exist").arg(id);
        throw std::runtime_error(qPrintable(msg));
    }

    Vec3f col;
    col.setX(it.value().colorAxonR);
    col.setY(it.value().colorAxonG);
    col.setZ(it.value().colorAxonB);
    return col;
}

/**
    Retrieves ID of the specified cell type.
    @param name The name of the cell type.
    @return ID of the cell type.
    @throws runtime_error if the name does not exist.
*/
int CellTypes::getId(const QString& name) const {
    for (QMap<int, Properties>::ConstIterator it = mPropsMap.constBegin(); it != mPropsMap.constEnd(); ++it) {
        if (it.value().name == name) {
            return it.key();
        }
    }
    const QString msg = QString("Cell type name %1 does not exist").arg(name);
    throw std::runtime_error(qPrintable(msg));
}

/**
    Retrieves all existing cell type IDs.
    @return The IDs.
*/
QList<int> CellTypes::getAllCellTypeIds(bool excitatory) const {
    if(!excitatory){
        return mPropsMap.keys();
    } else {
        QList<int> tmp = mPropsMap.keys();
        QList<int> result;
        for(int i=0; i<tmp.size(); i++){
            if(mPropsMap[tmp[i]].isExcitatory){
                result.push_back(tmp[i]);
            }
        }
        return result;
    }
    

}

/**
    Loads cell types from file.
    @param The name of the file.
    @throws runtime_error if file could not be loaded or parsed.
*/
void CellTypes::loadCSV(const QString &fileName)
{
    QFile file(fileName);
    QTextStream(stdout) << "[*] Reading celltypes from " << fileName << "\n";

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString msg = QString("Error reading celltypes file. Could not open file %1").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    const QChar sep = ',';
    QTextStream in(&file);

    int lineCount = 1;
    QString line = in.readLine();
    if (line.isNull()) {
        const QString msg = QString("Error reading celltypes file %1. No content.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    QStringList parts = line.split(sep);
    if (parts.size() != 3 ||
        parts[ 0] != FIELD_ID ||
        parts[ 1] != FIELD_NAME ||
        parts[ 2] != FIELD_ISEXCITATORY)
    {
        const QString msg = QString("Error reading celltypes file %1. Invalid column headers.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    line = in.readLine();
    lineCount += 1;

    while (!line.isNull()) {
        parts = line.split(sep);
        if (parts.size() != 3) {
            const QString msg = QString("Error reading celltypes file %1. Invalid columns.").arg(fileName);
            throw std::runtime_error(qPrintable(msg));
        }

        const int id =  parts[0].toInt();

        Properties props;
        props.name = parts[1];
        props.isExcitatory = (parts[2].toInt() == 1);

        mPropsMap.insert(id, props);

        line = in.readLine();
        lineCount += 1;
    }
    QTextStream(stdout) << "[*] Completed reading " <<  mPropsMap.size() << " celltypes.\n";
}
