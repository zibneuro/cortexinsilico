#include "CIS3DAxonRedundancyMap.h"
#include <QString>
#include <QFile>
#include <QIODevice>
#include <QDataStream>
#include <QTextStream>
#include <QDebug>
#include <stdexcept>

void AxonRedundancyMap::add(const int neuronId, const int neuronIdToUse) {
   if (mMap.contains(neuronId)) {
       QString msg = QString("Error adding neuron to AxonRedundancyMap: neuronId %1 already exists.").arg(neuronId);
       throw std::runtime_error(qPrintable(msg));
   }
   mMap.insert(neuronId, neuronIdToUse);
}


int AxonRedundancyMap::getNeuronIdToUse(const int neuronId) const {
   if (!mMap.contains(neuronId)) {
       QString msg = QString("Error in AxonRedundancyMap: neuronId %1 does not exist.").arg(neuronId);
       throw std::runtime_error(qPrintable(msg));
   }
   return mMap.value(neuronId);
}


int AxonRedundancyMap::saveBinary(const QString &fileName) const {
    QFile redundancyMapFile(fileName);
    if (!redundancyMapFile.open(QIODevice::WriteOnly)) {
        throw std::runtime_error("Cannot save axon morphology redundancy file");
    }
    QDataStream stream(&redundancyMapFile);
    stream << mMap;
    return 1;
}


int AxonRedundancyMap::loadBinary(const QString &fileName) {
    QFile redundancyMapFile(fileName);
    QTextStream(stdout) << "[*] Reading axon redundancy map from " << fileName << "\n";

    if (!redundancyMapFile.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Cannot open axon morphology redundancy file");
    }
    QDataStream stream(&redundancyMapFile);
    stream >> mMap;

    QTextStream(stdout) << "[*] Completed reading " <<  mMap.size() << " entries.\n";

    return 1;
}
