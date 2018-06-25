#include "UtilIO.h"
#include "CIS3DSparseField.h"
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QDebug>
#include <QRegularExpression>

/**
    Reads specification file which can contain filter definitions
    for pre- and postsynaptic neurons.
    @param fileName Path to spec file (in JSON format).
    @returns A JSON object with the specifications.
    @throws runtime_error if file cannot be loaded or parsed.
*/
QJsonObject UtilIO::parseSpecFile(const QString& fileName){
    QFile jsonFile(fileName);
    if (!jsonFile.open(QIODevice::ReadOnly)) {
        const QString msg = QString("Cannot open file for reading: %1").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }
    QByteArray data = jsonFile.readAll();
    QJsonParseError error;
    QJsonDocument doc(QJsonDocument::fromJson(data, &error));
    if (error.error != QJsonParseError::NoError) {
        const QString msg = QString("Error parsing JSON file: %1").arg(error.errorString());
        throw std::runtime_error(qPrintable(msg));
    }
    return doc.object();
}

/**
    Retrieves postsynaptic neurons and their properties (e.g., bounding box)
    that meet the filter definition.
    @param spec The spec file with the filter definition.
    @param networkProps the model data of the network.
    @param normalized Whether the PST density of each neuron is normalized by the overall PST density.
    @returns The postsynaptic neurons.
*/
PropsMap UtilIO::getPostSynapticNeurons(const QJsonObject& spec, const NetworkProps& networkProps, bool normalized) {
    const QJsonArray regions   = spec["POST_NEURON_REGIONS"].toArray();
    const QJsonArray cellTypes = spec["POST_NEURON_CELLTYPES"].toArray();
    const QJsonArray neuronIds = spec["POST_NEURON_IDS"].toArray();
    const QString    dataRoot  = spec["DATA_ROOT"].toString();

    QDir rootDir(dataRoot);

    QList<int> selectedNeuronIds = getPostSynapticNeuronIds(spec,networkProps);

    PropsMap neurons;
    neurons.reserve(selectedNeuronIds.size() + int(0.02 * selectedNeuronIds.size())); // Slightly bigger than necessary, see QHash doc.

    qDebug() << "[*] Start reading properties for" << selectedNeuronIds.size() << "postsynaptic neurons";
    for (int i=0; i<selectedNeuronIds.size(); ++i) {
        NeuronProps props;
        props.id = selectedNeuronIds[i];
        props.boundingBox = networkProps.boundingBoxes.getDendriteBox(props.id);
        if (props.boundingBox.isEmpty()) {
            throw std::runtime_error(qPrintable(QString("Empty dendrite bounding box for neuron %1.").arg(props.id)));
        }
        props.cellTypeId = networkProps.neurons.getCellTypeId(props.id);
        props.regionId = networkProps.neurons.getRegionId(props.id);
        props.somaPos = networkProps.neurons.getSomaPosition(props.id);

        const QString ctName = networkProps.cellTypes.getName(props.cellTypeId);
        const QString regionName = networkProps.regions.getName(props.regionId);

        QString excFilePath;
        if(normalized){
            excFilePath = CIS3D::getNormalizedPSTFileFullPath(rootDir, regionName, ctName, props.id, CIS3D::EXCITATORY);
        } else {
            excFilePath = CIS3D::getPSTFileFullPath(rootDir, regionName, ctName, props.id, CIS3D::EXCITATORY);
        }
        props.pstExc = SparseField::load(excFilePath);
        if (!props.pstExc) {
            throw std::runtime_error(qPrintable(QString("No excitatory PST file for neuron %1.").arg(props.id)));
        }

        QString inhFilePath;
        if(normalized) {
            inhFilePath = CIS3D::getNormalizedPSTFileFullPath(rootDir, regionName, ctName, props.id, CIS3D::INHIBITORY);
        } else {
            inhFilePath = CIS3D::getPSTFileFullPath(rootDir, regionName, ctName, props.id, CIS3D::INHIBITORY);
        }
        props.pstInh = SparseField::load(inhFilePath);
        if (!props.pstInh) {
            throw std::runtime_error(qPrintable(QString("No inhibitory PST file for neuron %1.").arg(props.id)));
        }

        neurons.insert(props.id, props);

        if (i % 1000 == 0 && i > 0) {
            qDebug() << "    Read properties for" << i << "neurons";
        }
    }
    qDebug() << "[*] Completed reading properties for" << selectedNeuronIds.size() << "postsynaptic neurons";

    return neurons;
}

/**
    Determines ids of postsynaptic neurons meeting the filter definition
    @param spec The spec file with the filter definition.
    @param networkProps The model data of the network.
    @returns A list of postsynaptic neuron IDs.
    @throws runtime_error if data cannot be loaded.
*/
QList<int> UtilIO::getPostSynapticNeuronIds(const QJsonObject& spec, const NetworkProps& networkProps){
    
    const QJsonArray regions   = spec["POST_NEURON_REGIONS"].toArray();
    const QJsonArray cellTypes = spec["POST_NEURON_CELLTYPES"].toArray();
    const QJsonArray neuronIds = spec["POST_NEURON_IDS"].toArray();
    const QString    dataRoot  = spec["DATA_ROOT"].toString();

    QList<int> selectedCellTypes;
    QList<int> selectedRegions;

    for (int c=0; c<cellTypes.size(); ++c) {
        const QString cellTypeStr = cellTypes[c].toString();
        if (!cellTypeStr.isEmpty()) {
            const int cellTypeId = networkProps.cellTypes.getId(cellTypeStr);
            selectedCellTypes.append(cellTypeId);
        }
    }

    for (int r=0; r<regions.size(); ++r) {
        const QString regionStr = regions[r].toString();
        if (!regionStr.isEmpty()) {
            const int regionId = networkProps.regions.getId(regionStr);
            selectedRegions.append(regionId);
        }
    }

    QDir rootDir(dataRoot);
    if (!rootDir.exists()) {
        throw std::runtime_error("Invalid data root in specification");
    }

    QDir pstDir(rootDir);
    if (!pstDir.cd("NormalizedPSTs")) {
        QString msg = QString("No NormalizedPSTs directory in %1").arg(dataRoot);
        throw std::runtime_error(qPrintable(msg));
    }

    QList<int> selectedNeuronIds = networkProps.neurons.getFilteredNeuronIds(selectedCellTypes,
                                                                             selectedRegions,
                                                                             CIS3D::POSTSYNAPTIC);

    for (int i=0; i<neuronIds.size(); ++i) {
        const int selectedId = neuronIds[i].toInt(-1);
        if (selectedId != -1) {
            selectedNeuronIds.append(selectedId);
        }
    }

    return selectedNeuronIds;
}

/**
    Determines IDs of presynaptic neurons meeting the filter definition.
    @param spec The spec file with the filter definition.
    @param networkProps The model data of the network.
    @returns A list of presynaptic neuron IDs.
    @throws runtime_error if data cannot be loaded.
*/
QList<int> UtilIO::getPreSynapticNeurons(const QJsonObject& spec, const NetworkProps& networkProps) {
    const QJsonArray regions   = spec["PRE_NEURON_REGIONS"].toArray();
    const QJsonArray cellTypes = spec["PRE_NEURON_CELLTYPES"].toArray();
    const QJsonArray neuronIds = spec["PRE_NEURON_IDS"].toArray();
    const QString    dataRoot  = spec["DATA_ROOT"].toString();

    QList<int> selectedCellTypes;
    QList<int> selectedRegions;

    for (int c=0; c<cellTypes.size(); ++c) {
        const QString cellTypeStr = cellTypes[c].toString();
        if (!cellTypeStr.isEmpty()) {
            const int cellTypeId = networkProps.cellTypes.getId(cellTypeStr);
            selectedCellTypes.append(cellTypeId);
        }
    }

    for (int r=0; r<regions.size(); ++r) {
        const QString regionStr = regions[r].toString();
        if (!regionStr.isEmpty()) {
            const int regionId = networkProps.regions.getId(regionStr);
            selectedRegions.append(regionId);
        }
    }

    QDir rootDir(dataRoot);
    if (!rootDir.exists()) {
        throw std::runtime_error("Invalid data root in specification");
    }

    QDir boutonDir(rootDir);
    if (!boutonDir.cd("Boutons")) {
        QString msg = QString("No Boutons directory in %1").arg(dataRoot);
        throw std::runtime_error(qPrintable(msg));
    }

    QList<int> selectedNeuronIds = networkProps.neurons.getFilteredNeuronIds(selectedCellTypes,
                                                                             selectedRegions,
                                                                             CIS3D::PRESYNAPTIC);

    for (int i=0; i<neuronIds.size(); ++i) {
        const int selectedId = neuronIds[i].toInt(-1);
        if (selectedId != -1) {
            selectedNeuronIds.append(selectedId);
        }
    }

    return selectedNeuronIds;
}


int getNeuronIdFromFile(const QString& pattern, const QString& fileName, const int regExGroup) {
    const QRegularExpression rx(pattern);
    QRegularExpressionMatch match = rx.match(fileName);
    if (match.hasMatch()) {
        const QString str = match.captured(regExGroup);
        int id = str.toInt();
        return id;
    }
    const QString errorStr = QString("Cannot find neuron ID in filename %1 (pattern: %2)")
            .arg(fileName).arg(pattern);
    throw std::runtime_error(errorStr.toStdString());
}

/**
    Determines the neuron ID from the specfied presynaptic data file name.
    @param fileName A Boutons_*.dat file name.
    @returns The presynaptic neuron ID.
    @throws runtime_error if ID cannot be extracted from file name.
*/
int UtilIO::getPreNeuronIdFromFile(const QString& fileName) {
    return  getNeuronIdFromFile("Boutons_(\\d+).dat", fileName, 1);
}

/**
    Determines the neuron ID from the specfied postsynaptic data file name.
    @param fileName A PST_inhibitoryPre_*.dat OR PST_excitatoryPre_*.dat file name.
    @returns The postsynaptic neuron ID.
    @throws runtime_error if ID cannot be extracted from file name.
*/
int UtilIO::getPostNeuronIdFromFile(const QString& fileName) {
    return  getNeuronIdFromFile("NormalizedPST_(excitatory|inhibitory)Pre_(\\d+).dat", fileName, 2);
}

/**
    Determines whether the specfied file mame represents excitatory
    or inhibitory postsynaptic data.
    @param file A PST_inhibitoryPre_*.dat OR PST_excitatoryPre_*.dat file name.
    @returns True if the file represents excitatory neurons.
*/
bool UtilIO::isExcitatoryFileName(const QString& fileName) {
    return fileName.contains("excitatory");
}