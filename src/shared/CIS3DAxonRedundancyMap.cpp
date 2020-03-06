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

void AxonRedundancyMap::updateMapping(const int neuronId, const int desiredMappedId) {
    const int currentMappedId = mMap[neuronId];
    const int newMappedId = mMap[desiredMappedId];
    qDebug() << "updateMapping" << currentMappedId << newMappedId;
    mMap[neuronId] = newMappedId;
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


void AxonRedundancyMap::loadCSV(QString fileName){
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
        QStringList parts = line.split(',');
        mMap[parts[0].toInt()] = parts[1].toInt();
        line = in.readLine();
    }
}

bool AxonRedundancyMap::isUnique(int neuronId){
    int mappedId = getNeuronIdToUse(neuronId);
    return mappedId == neuronId;
}