#ifndef UTILIO_H
#define UTILIO_H

#include <QList>
#include "CIS3DNetworkProps.h"
#include "Typedefs.h"
#include "NeuronSelection.h"
#include <QTextStream>
#include <set>

class QString;
class QJsonObject;

/**
    Utility classes related with file IO.
*/
namespace UtilIO {

/**
    Reads specification file which can contain filter definitions
    for pre- and postsynaptic neurons.
    @param fileName Path to spec file (in JSON format).
    @returns A JSON object with the specifications.
    @throws runtime_error if file cannot be loaded or parsed.
*/
QJsonObject parseSpecFile(const QString& fileName);

void writeJson(QJsonObject& obj, QString& fileName);

/**
    Determines IDs of presynaptic neurons meeting the filter definition.
    @param spec The spec file with the filter definition.
    @param networkProps The model data of the network.
    @returns A list of presynaptic neuron IDs.
    @throws runtime_error if data cannot be loaded.
*/
QList<int> getPreSynapticNeurons(const QJsonObject& spec, const NetworkProps& networkProps);

/**
    Determines IDs of postsynaptic neurons meeting the filter definition
    @param spec The spec file with the filter definition.
    @param networkProps The model data of the network.
    @returns A list of postsynaptic neuron IDs.
    @throws runtime_error if data cannot be loaded.
*/
QList<int> getPostSynapticNeuronIds(const QJsonObject& spec, const NetworkProps& networkProps);

/**
    Retrieves postsynaptic neurons and their properties (e.g., bounding box)
    that meet the filter definition.
    @param spec The spec file with the filter definition.
    @param networkProps the model data of the network.
    @param normalized Whether the PST density of each neuron is normalized by the overall PST
   density.
    @returns The postsynaptic neurons.
*/
PropsMap getPostSynapticNeurons(const QJsonObject& spec, const NetworkProps& networkProps,
                                bool normalized = true);

/**
    Determines IDs of neurons meeting the filter definition (irrespective of pre- or
    postsynaptic type)
    @param spec The spec file with the filter definition.
    @param networkProps The model data of the network.
    @returns A list neuron IDs.
    @throws runtime_error if data cannot be loaded.
*/
QList<int> getNeuronIds(const QJsonObject& spec, const NetworkProps& networkProps);

/**
    Determines the neuron ID from the specfied presynaptic data file name.
    @param fileName A Boutons_*.dat file name.
    @returns The presynaptic neuron ID.
    @throws runtime_error if ID cannot be extracted from file name.
*/
int getPreNeuronIdFromFile(const QString& fileName);

/**
    Determines the neuron ID from the specfied postsynaptic data file name.
    @param fileName A PST_inhibitoryPre_*.dat OR PST_excitatoryPre_*.dat file name.
    @returns The postsynaptic neuron ID.
    @throws runtime_error if ID cannot be extracted from file name.
*/
int getPostNeuronIdFromFile(const QString& fileName);

/**
 * @brief Retrieves explicitly defined voxel IDs from the selection specification.
 * @param spec The selection specification.
 * @return Set of voxel IDs.
 */
std::set<int> getVoxelWhitelist(const QJsonObject& spec);

/**
    Determines whether the specfied file mame represents excitatory
    or inhibitory postsynaptic data.
    @param file A PST_inhibitoryPre_*.dat OR PST_excitatoryPre_*.dat file name.
    @returns True if the file represents excitatory neurons.
*/
bool isExcitatoryFileName(const QString& fileName);

void makeDir(QString dirname);

std::vector<std::vector<double> > readCsv(QString filename, bool ignoreFileNotFound = false);

QString getCsvName(int id);

QString getSubvolumeFileName(QString subvolumeStr);

}  // namespace UtilIO


#endif  // UTILIO_H
