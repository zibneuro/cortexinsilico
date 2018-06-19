#include "SynapseWriter.h"
#include <QChar>
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <stdexcept>

/*
    Writes a synapses.csv file.

    @param filename The name of the file.
    @param A list of synapses.
    @throws runtime_error if the file cannot be written.
*/
void SynapseWriter::write(QString fileName, QList<Synapse> synapses) {

    QTextStream(stdout) << "[*] Writing synapses to " << fileName << "\n";

    qSort(synapses.begin(), synapses.end(), lessThan);

    QFile csv(fileName);
    if (!csv.open(QIODevice::WriteOnly)) {
        const QString msg = QString("Cannot open file %1 for writing.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }
    const QChar sep(',');
    QTextStream out(&csv);
    out << "voxelID" << sep << "voxelX" << sep << "voxelY" << sep << "voxelZ" << sep
        << "presynapticNeuronID" << sep << "postsynapticNeuronID" << sep << "pre" << sep << "post"
        << sep << "postAll" << sep << "count"
        << "\n";
    for (int i = 0; i < synapses.size(); i++) {
        Synapse synapse = synapses[i];
        out << synapse.voxelId << sep << synapse.voxelX << sep << synapse.voxelY << sep
            << synapse.voxelZ << sep << synapse.preNeuronId << sep << synapse.postNeuronId << sep
            << synapse.pre << sep << synapse.post << sep << synapse.postAll << sep << synapse.count
            << "\n";
    }
}

/*
    Comparator for two synapses. Synapse is considered smaller than other synapse, if voxel ID
    is lower. If the voxel IDs are identical the comparison is based on the pre- and then the 
    postsynaptic neuron ID.

    @param a First synapse.
    @param b Second synapse.
    @return True, if the first synapse is smaller than the second synapse.
*/
bool SynapseWriter::lessThan(Synapse& a, Synapse& b){
    
    if (a.voxelId == b.voxelId) {
        if(a.preNeuronId == b.preNeuronId){
            return a.postNeuronId < b.postNeuronId;
        } else {
            return a.preNeuronId < b.preNeuronId;
        };
    } else {
        return a.voxelId < b.voxelId;
    }


}