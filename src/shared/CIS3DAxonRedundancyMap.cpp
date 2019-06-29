#include "CIS3DAxonRedundancyMap.h"
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QIODevice>
#include <QString>
#include <QTextStream>
#include <stdexcept>

/**
    Registers a neuron id and its mapped id.
    @param neuronId The neuron id to register.
    @param neuronIdToUse The id to use for retrieving axon data.
*/
void AxonRedundancyMap::add(const int neuronId, const int neuronIdToUse) {
    if (mMap.contains(neuronId)) {
        QString msg =
            QString("Error adding neuron to AxonRedundancyMap: neuronId %1 already exists.")
                .arg(neuronId);
        throw std::runtime_error(qPrintable(msg));
    }
    mMap.insert(neuronId, neuronIdToUse);    
}

/**
    Retrieves the mapped id from the original id.
    @param neuronId The original neuron id.
    @returns The mapped id.
    @throws runtime_error when id is not registered.
*/
int AxonRedundancyMap::getNeuronIdToUse(const int neuronId) const {
    if (!mMap.contains(neuronId)) {
        QString msg =
            QString("Error in AxonRedundancyMap: neuronId %1 does not exist.").arg(neuronId);
        throw std::runtime_error(qPrintable(msg));
    }
    return mMap.value(neuronId);
}

/**
    Saves the axon redundancy map as a binary file.
    @param fileName Name of the file to write.
    @returns 1 if file was successfully written.
    @throws runtime_error in case of failure.
*/
int AxonRedundancyMap::saveBinary(const QString &fileName) const {
    QFile redundancyMapFile(fileName);
    if (!redundancyMapFile.open(QIODevice::WriteOnly)) {
        throw std::runtime_error("Cannot save axon morphology redundancy file");
    }
    QDataStream stream(&redundancyMapFile);
    stream << mMap;
    return 1;
}

/**
    Saves the axon redundancy map as a csv file.
    @param fileName Name of the file to write.
    @throws runtime_error in case of failure.
*/
void AxonRedundancyMap::saveCSV(const QString &fileName) const {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QString msg = QString("Could not open file %1 for writing").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    const QChar sep(',');
    QTextStream out(&file);
    out << "neuronID" << sep << "mappedNeuronID"
        << "\n";

    QList<int> neuronIds = mMap.keys();
    qSort(neuronIds);
    for(int i=0; i<neuronIds.size(); i++){
        out << neuronIds[i] << sep << mMap[neuronIds[i]] << "\n";
    }
}

/**
    Loads the axon redundancy map from a binary file.
    @param fileName Name of the file to load.
    @returns 1 if file was successfully loaded.
    @throws: runtime_error in case of failure.
*/
int AxonRedundancyMap::loadBinary(const QString &fileName) {
    QString flatFileName = QDir(QFileInfo(fileName).path()).absoluteFilePath("axon_mapping");
    QFileInfo flatFileInfo(flatFileName);
    if(flatFileInfo.exists()){
        loadFlatFile(flatFileName);
        return 1;
    } else {

        QFile redundancyMapFile(fileName);
        QTextStream(stdout) << "[*] Reading axon redundancy map from " << fileName << "\n";

        if (!redundancyMapFile.open(QIODevice::ReadOnly)) {
            throw std::runtime_error("Cannot open axon morphology redundancy file");
        }
        QDataStream stream(&redundancyMapFile);
        stream >> mMap;

        QTextStream(stdout) << "[*] Completed reading " << mMap.size() << " entries.\n";

        return 1;
    }
}

void AxonRedundancyMap::loadFlatFile(QString fileName){
    mMap.clear();
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        const QString msg =
            QString("Error reading file. Could not open file %1").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }
    QTextStream in(&file);
    QString line = in.readLine();
    while (!line.isNull())
    {
        QStringList parts = line.split(' ');
        mMap[parts[0].toInt()] = parts[1].toInt();
        line = in.readLine();
    }
}

bool AxonRedundancyMap::isUnique(int neuronId){
    int mappedId = getNeuronIdToUse(neuronId);
    return mappedId == neuronId;
}
