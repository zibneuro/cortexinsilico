#include "FeatureExtractor.h"

/*
    Constructor.

    @param props The complete model data.
*/
FeatureExtractor::FeatureExtractor(NetworkProps& props) : mNetworkProps(props) {}

/*
    Extracts the model features from the specified subvolume and
    writes them into a file named features.csv.

    @param origin Origin of the subvolume (in spatial coordinates).
    @param dimensions Number of voxels in each direction from the origin.
*/
void FeatureExtractor::writeFeaturesToFile(QVector<float> origin, QVector<int> dimensions) {
    QList<int> neurons = getNeuronsWithinVolume(origin, dimensions);
    qDebug() << "[*] Extracting features.";
    QList<Feature> features = extractFeatures(neurons, origin, dimensions);
    qDebug() << "[*] Writing model data to file: features.csv.";
    qSort(features.begin(), features.end(), lessThan);
    writeCSV(features);
}

/*
    Determines all neurons within the subvolume (based on soma position)

    @param origin Origin of the voxelcube.
    @param dimensions Number of voxels in each direction.
    @return List of neuron IDs.
*/
QList<int> FeatureExtractor::getNeuronsWithinVolume(QVector<float> origin,
                                                    QVector<int> dimensions) {
    int voxelsize = 50;

    float xLow = origin[0];
    float xHigh = origin[0] + voxelsize * dimensions[0];
    float yLow = origin[1];
    float yHigh = origin[1] + voxelsize * dimensions[1];
    float zLow = origin[2];
    float zHigh = origin[2] + voxelsize * dimensions[2];

    SelectionFilter emptyFilter;
    QList<int> allNeurons = mNetworkProps.neurons.getFilteredNeuronIds(emptyFilter);

    QList<int> neuronsWithinVolume;
    for (int i = 0; i < allNeurons.size(); i++) {
        Vec3f somaPosition = mNetworkProps.neurons.getSomaPosition(i);
        float x = somaPosition.getX();
        float y = somaPosition.getY();
        float z = somaPosition.getZ();
        if (x >= xLow && y <= xHigh && y >= yLow && y <= yHigh && z >= zLow && z <= zHigh) {
            neuronsWithinVolume.append(i);
        }
    }
    return neuronsWithinVolume;
}

/*
    Determines unique ID for the specified voxel.

    @param locationLocal Position of the voxel relative to the origin.
    @param dimensions Number of voxels in each direction.
    @return The ID of the voxel.
*/
int FeatureExtractor::getVoxelId(Vec3i locationLocal, QVector<int> dimensions) {
    int nx = dimensions[0];
    int ny = dimensions[1];
    int ix = locationLocal[0];
    int iy = locationLocal[1];
    int iz = locationLocal[2];
    return iz * ny * nx + iy * nx + ix + 1;
}

/*
    Retrieves the pre- and postsynaptic target counts for one neuron within all voxels.

    @param pre The presynaptic target counts.
    @param postExc The postsynaptic target counts for (presynaptic) excitatory neurons.
    @param postAllExc The overall postsynaptic target counts for (presynaptic) excitatory
    neurons.
    @param postInh The postsynaptic target counts for (presynaptic) inhibitory neurons.
    @param postAllInh The overall postsynaptic target counts for (presynaptic) inhibitory
    neurons.
    @param origin The origin of the voxel grid.
    @param dimensions The number of voxels in each direction.
    @return All voxels that with the pre- and postsynaptic target counts.

*/
QList<Voxel> FeatureExtractor::determineVoxels(SparseField* pre, SparseField* postExc,
                                               SparseField* postAllExc, SparseField* postInh,
                                               SparseField* postAllInh, QVector<float> origin,
                                               QVector<int> dimensions) {
    const float voxelSize = 50;
    QList<Voxel> voxels;
    Vec3f position;
    for (int x = 0; x < dimensions[0]; x++) {
        position.setX(origin[0] + x * voxelSize);
        for (int y = 0; y < dimensions[1]; y++) {
            position.setY(origin[1] + y * voxelSize);
            for (int z = 0; z < dimensions[2]; z++) {
                position.setZ(origin[2] + z * voxelSize);

                Voxel voxel;
                voxel.voxelId = getVoxelId(Vec3i(x, y, z), dimensions);
                voxel.voxelX = x + 1;
                voxel.voxelY = y + 1;
                voxel.voxelZ = z + 1;

                if (pre != NULL) {
                    Vec3i voxelLocation = pre->getVoxelContainingPoint(position);
                    voxel.pre = pre->getFieldValue(voxelLocation);
                } else {
                    voxel.pre = 0;
                }

                if (postExc != NULL) {
                    Vec3i voxelLocation = postExc->getVoxelContainingPoint(position);
                    voxel.postExc = postExc->getFieldValue(voxelLocation);
                } else {
                    voxel.postExc = 0;
                }

                if (postAllExc != NULL) {
                    Vec3i voxelLocation = postAllExc->getVoxelContainingPoint(position);
                    voxel.postAllExc = postAllExc->getFieldValue(voxelLocation);
                } else {
                    voxel.postAllExc = 0;
                }

                if (postInh != NULL) {
                    Vec3i voxelLocation = postInh->getVoxelContainingPoint(position);
                    voxel.postInh = postInh->getFieldValue(voxelLocation);
                } else {
                    voxel.postInh = 0;
                }

                if (postAllInh != NULL) {
                    Vec3i voxelLocation = postAllInh->getVoxelContainingPoint(position);
                    voxel.postAllInh = postAllInh->getFieldValue(voxelLocation);
                } else {
                    voxel.postAllInh = 0;
                }

                voxels.append(voxel);
            }
        }
    }
    return voxels;
}

/*
    Retrieves the features for all neurons and all voxels.

    @param neurons The neurons within the subvolume.
    @param origin Origin of the subvolume.
    @param dimensions Number of voxels in each direction.
    @return A list of features.
*/
QList<Feature> FeatureExtractor::extractFeatures(QList<int> neurons, QVector<float> origin,
                                                 QVector<int> dimensions) {
    QList<Feature> features;

    const QString pstAllFileExc =
        CIS3D::getPSTAllFullPath(mNetworkProps.dataRoot, CIS3D::EXCITATORY);
    SparseField* postAllExc = SparseField::load(pstAllFileExc);
    const QString pstAllFileInh =
        CIS3D::getPSTAllFullPath(mNetworkProps.dataRoot, CIS3D::INHIBITORY);
    SparseField* postAllInh = SparseField::load(pstAllFileInh);

    for (int i = 0; i < neurons.size(); i++) {
        qDebug() << "[*] Processing neuron " << i + 1 << "of" << neurons.size();

        int neuronId = neurons[i];
        int cellTypeId = mNetworkProps.neurons.getCellTypeId(neuronId);
        QString cellType = mNetworkProps.cellTypes.getName(cellTypeId);
        QString functionalCellType = mNetworkProps.cellTypes.isExcitatory(cellTypeId) ? "exc" : "inh";
        int regionId = mNetworkProps.neurons.getRegionId(neuronId);
        QString region = mNetworkProps.regions.getName(regionId);
        int synapticSide = mNetworkProps.neurons.getSynapticSide(neuronId);

        QList<Voxel> voxels;

        SparseField* boutons = NULL;
        SparseField* postExc = NULL;
        SparseField* postInh = NULL;
        if (synapticSide == CIS3D::SynapticSide::PRESYNAPTIC ||
            synapticSide == CIS3D::SynapticSide::BOTH_SIDES) {
            int mappedId = mNetworkProps.axonRedundancyMap.getNeuronIdToUse(neuronId);
            const QString filePath =
                CIS3D::getBoutonsFileFullPath(mNetworkProps.dataRoot, cellType, mappedId);
            boutons = SparseField::load(filePath);
        }
        if (synapticSide == CIS3D::SynapticSide::POSTSYNAPTIC ||
            synapticSide == CIS3D::SynapticSide::BOTH_SIDES) {
            const QString filePathExc = CIS3D::getPSTFileFullPath(
                mNetworkProps.dataRootDir, region, cellType, neuronId, CIS3D::EXCITATORY);
            postExc = SparseField::load(filePathExc);
            const QString filePathInh = CIS3D::getPSTFileFullPath(
                mNetworkProps.dataRootDir, region, cellType, neuronId, CIS3D::INHIBITORY);
            postInh = SparseField::load(filePathInh);
        }

        voxels.append(
            determineVoxels(boutons, postExc, postAllExc, postInh, postAllInh, origin, dimensions));

        if (boutons != NULL) {
            delete boutons;
        }
        if (postExc != NULL) {
            delete postExc;
        }
        if (postInh != NULL) {
            delete postInh;
        }

        for (int j = 0; j < voxels.size(); j++) {
            Feature feature;
            feature.neuronID = neuronId;
            feature.voxelID = voxels[j].voxelId;
            feature.voxelX = voxels[j].voxelX;
            feature.voxelY = voxels[j].voxelY;
            feature.voxelZ = voxels[j].voxelZ;
            feature.morphologicalCellType = cellType;
            feature.functionalCellType = functionalCellType;
            feature.region = region;
            feature.synapticSide = synapticSide;
            feature.pre = voxels[j].pre;
            feature.postExc = voxels[j].postExc;
            feature.postAllExc = voxels[j].postAllExc;
            feature.postInh = voxels[j].postInh;
            feature.postAllInh = voxels[j].postAllInh;
            features.append(feature);
        }
    }

    delete postAllExc;
    delete postAllInh;

    return features;
}

/*
    Writes the features into a file named features.csv

    @param features A list with the features.
*/
void FeatureExtractor::writeCSV(QList<Feature>& features) {
    QFile csv("features.csv");
    if (!csv.open(QIODevice::WriteOnly)) {
        throw std::runtime_error("Cannot open features.csv for saving.");
    }
    const QChar sep(',');
    QTextStream out(&csv);
    out << "voxelID" << sep << "voxelX" << sep << "voxelY" << sep << "voxelZ" << sep << "neuronID"
        << sep << "morphologicalCellType" << sep << "functionalCellType" << sep << "region" << sep
        << "synapticSide" << sep << "pre" << sep << "postExc" << sep << "postAllExc" << sep
        << "postInh" << sep << "postAllInh"
        << "\n";
    for (int i = 0; i < features.size(); i++) {
        Feature feature = features[i];
        out << feature.voxelID << sep << feature.voxelX << sep << feature.voxelY << sep
            << feature.voxelZ << sep << feature.neuronID << sep << feature.morphologicalCellType
            << sep << feature.functionalCellType << sep << feature.region << sep
            << feature.synapticSide << sep << feature.pre << sep << feature.postExc << sep
            << feature.postAllExc << sep << feature.postInh << sep << feature.postAllInh << "\n";
    }
}

/*
    Comparator for two features. Feature is considered smaller than other feature, if voxel ID
    is lower. If the voxel IDs are identical the comparison is based on the neuron ID.

    @param a First feature.
    @param b Second feature.
    @return True, if the first feature is smaller than the second feature.
*/
bool FeatureExtractor::lessThan(Feature& a, Feature& b) {
    if (a.voxelID == b.voxelID) {
        return a.neuronID < b.neuronID;
    } else {
        return a.voxelID < b.voxelID;
    }
}
