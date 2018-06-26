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
    @param cellTypes The cell types filter.
    @param regions The regions filter.
    @param neuronIds The neuron IDs filter.
    @param samplingFactor The sampling factor, 1: take all, 2: take half, etc.
*/
void FeatureExtractor::writeFeaturesToFile(QVector<float> origin, QVector<int> dimensions,
                                           QSet<QString> cellTypes, QSet<QString> regions,
                                           QSet<int> neuronIds, int samplingFactor) {
    qDebug() << "[*] Pruning neurons.";
    QList<int> neurons = determineNeurons(origin, dimensions, cellTypes, regions, neuronIds);
    qDebug() << "[*] Extracting features.";
    QList<Feature> features = extractFeatures(neurons, origin, dimensions,samplingFactor);
    qDebug() << "[*] Writing model data to file: features.csv.";
    writeCSV(features);
}

/*
    Counts the number of neurons in each voxel and writes the results
    to file.

    @param origin Origin of the subvolume (in spatial coordinates).
    @param dimensions Number of voxels in each direction from the origin.
    @param cellTypes The cell types filter.
    @param regions The regions filter.
    @param neuronIds The neuron IDs filter.
    @param samplingFactor The sampling factor, 1: take all, 2: take half, etc.
*/
void FeatureExtractor::writeVoxelStats(QVector<float> /*origin*/, QVector<int> /*dimensions*/,
                                       QSet<QString> /*cellTypes*/, QSet<QString> /*regions*/,
                                       QSet<int> /*neuronIds*/, int /*samplingFactor*/) {
    QDir modelDataDir = CIS3D::getModelDataDir(mNetworkProps.dataRoot);
    const QString pstAllFileExc = CIS3D::getPSTAllFullPath(modelDataDir, CIS3D::EXCITATORY);
    SparseField* postAllExc = SparseField::load(pstAllFileExc);
    Vec3f origin = postAllExc->getOrigin();
    Vec3i dimensions(1, 1, 1);
    Vec3f voxelSize = postAllExc->getVoxelSize();
    delete postAllExc;

    SparseField count(dimensions, origin, voxelSize);

    SelectionFilter emptyFilter;
    QList<int> allNeurons = mNetworkProps.neurons.getFilteredNeuronIds(emptyFilter);
    SparseFieldCalculator calculator;
    // std::random_shuffle(allNeurons.begin(),allNeurons.end());
    for (int i = 0; i < allNeurons.size(); i++) {
        int neuronId = allNeurons[i];
        int synapticSide = mNetworkProps.neurons.getSynapticSide(neuronId);
        int regionId = mNetworkProps.neurons.getRegionId(neuronId);
        QString region = mNetworkProps.regions.getName(regionId);
        int cellTypeId = mNetworkProps.neurons.getCellTypeId(neuronId);
        QString cellType = mNetworkProps.cellTypes.getName(cellTypeId);

        SparseField temp(dimensions, origin, voxelSize);

        SparseField* boutons = NULL;
        SparseField* postExc = NULL;
        SparseField* postInh = NULL;
        if (synapticSide == CIS3D::SynapticSide::PRESYNAPTIC ||
            synapticSide == CIS3D::SynapticSide::BOTH_SIDES) {
            int mappedId = mNetworkProps.axonRedundancyMap.getNeuronIdToUse(neuronId);
            const QString filePath =
                CIS3D::getBoutonsFileFullPath(modelDataDir, cellType, mappedId);
            boutons = SparseField::load(filePath);
        }
        if (synapticSide == CIS3D::SynapticSide::POSTSYNAPTIC ||
            synapticSide == CIS3D::SynapticSide::BOTH_SIDES) {
            const QString filePathExc = CIS3D::getPSTFileFullPath(modelDataDir, region, cellType,
                                                                  neuronId, CIS3D::EXCITATORY);
            postExc = SparseField::load(filePathExc);
            const QString filePathInh = CIS3D::getPSTFileFullPath(modelDataDir, region, cellType,
                                                                  neuronId, CIS3D::INHIBITORY);
            postInh = SparseField::load(filePathInh);
        }

        if (boutons != NULL) {
            temp = temp + *boutons;
            delete boutons;
        }
        if (postExc != NULL) {
            temp = temp + *postExc;
            delete postExc;
        }
        if (postInh != NULL) {
            temp = temp + *postInh;
            delete postInh;
        }

        temp = calculator.binarize(temp);
        count = count + temp;

        if (i % 1000 == 0) {
            qDebug() << "[*] Processed neuron" << i << "of" << allNeurons.size();
        }
    }

    qDebug() << "[*] Writing voxel stats to file voxelStats.csv";
    count.saveCSV("voxelStats.csv");
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
        if (x >= xLow && x <= xHigh && y >= yLow && y <= yHigh && z >= zLow && z <= zHigh) {
            neuronsWithinVolume.append(i);
        }
    }
    return neuronsWithinVolume;
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
                voxel.voxelId = SparseField::getVoxelId(Vec3i(x, y, z), dimensions);
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

                if (voxel.pre != 0 || voxel.postExc != 0 || voxel.postInh != 0) {
                    voxels.append(voxel);
                }
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
    @param samplingFactor The sampling factor, 1: take all, 2: take half, etc.
    @return A list of features.
*/
QList<Feature> FeatureExtractor::extractFeatures(QList<int> neurons, QVector<float> origin,
                                                 QVector<int> dimensions, int samplingFactor) {
    QList<Feature> features;
    QDir modelDataDir = CIS3D::getModelDataDir(mNetworkProps.dataRoot);

    const QString pstAllFileExc = CIS3D::getPSTAllFullPath(modelDataDir, CIS3D::EXCITATORY);
    SparseField* postAllExc = SparseField::load(pstAllFileExc);
    const QString pstAllFileInh = CIS3D::getPSTAllFullPath(modelDataDir, CIS3D::INHIBITORY);
    SparseField* postAllInh = SparseField::load(pstAllFileInh);

    if (samplingFactor > 1) {
        std::random_shuffle(neurons.begin(), neurons.end());
    }

    for (int i = 0; i < neurons.size(); i++) {
        int neuronId = neurons[i];
        int cellTypeId = mNetworkProps.neurons.getCellTypeId(neuronId);
        QString cellType = mNetworkProps.cellTypes.getName(cellTypeId);
        QString functionalCellType =
            mNetworkProps.cellTypes.isExcitatory(cellTypeId) ? "exc" : "inh";
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
                CIS3D::getBoutonsFileFullPath(modelDataDir, cellType, mappedId);
            boutons = SparseField::load(filePath);
        }
        if (synapticSide == CIS3D::SynapticSide::POSTSYNAPTIC ||
            synapticSide == CIS3D::SynapticSide::BOTH_SIDES) {
            const QString filePathExc = CIS3D::getPSTFileFullPath(modelDataDir, region, cellType,
                                                                  neuronId, CIS3D::EXCITATORY);
            postExc = SparseField::load(filePathExc);
            const QString filePathInh = CIS3D::getPSTFileFullPath(modelDataDir, region, cellType,
                                                                  neuronId, CIS3D::INHIBITORY);
            postInh = SparseField::load(filePathInh);
        }

        QList<Voxel> nextValues = determineVoxels(boutons, postExc, postAllExc, postInh, postAllInh, origin, dimensions);

        if (boutons != NULL) {
            delete boutons;
        }
        if (postExc != NULL) {
            delete postExc;
        }
        if (postInh != NULL) {
            delete postInh;
        }

        if ((nextValues.size() > 0) && ((i + 1) % samplingFactor == 0)) {
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

        if (i % 1000 == 0) {
            qDebug() << "[*] Processed neuron " << i + 1 << "of" << neurons.size()
                     << ". Total number of features" << features.size();
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
    qSort(features.begin(), features.end(), lessThan);
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
   Comparator for two features. Feature is considered smaller than other feature, if voxel
   ID is lower. If the voxel IDs are identical, the comparison is based on the neuron ID.

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

/*
    Determines all neurons that have intersecting bounding boxes with the
    specified subcube.

    @param origin The origing of the subcube.
    @param dimensions The number of voxels in each dimension.
    @return The intersecting neurons.
*/
QList<int> FeatureExtractor::determineIntersectingNeurons(QVector<float> origin,
                                                          QVector<int> dimensions) {
    const float voxelSize = 50;
    Vec3f shiftedOrigin(origin[0] - voxelSize, origin[1] - voxelSize, origin[2] - voxelSize);
    Vec3f pointMax(origin[0] + dimensions[0] * voxelSize, origin[1] + dimensions[1] * voxelSize,
                   origin[2] + dimensions[2] * voxelSize);
    BoundingBox subcube(shiftedOrigin, pointMax);

    QList<int> intersectingNeurons;
    SelectionFilter emptyFilter;
    QList<int> allNeurons = mNetworkProps.neurons.getFilteredNeuronIds(emptyFilter);

    qDebug() << allNeurons.size();

    for (int i = 0; i < allNeurons.size(); i++) {
        int neuronId = allNeurons[i];
        bool axonIntersects = false;
        bool dendriteIntersects = false;
        bool isPresynaptic =
            mNetworkProps.neurons.getSynapticSide(neuronId) != CIS3D::SynapticSide::POSTSYNAPTIC;
        bool isPostsynaptic =
            mNetworkProps.neurons.getSynapticSide(neuronId) != CIS3D::SynapticSide::PRESYNAPTIC;

        if (isPresynaptic) {
            axonIntersects = mNetworkProps.boundingBoxes.getAxonBox(neuronId).intersects(subcube);
        }

        if (isPostsynaptic) {
            dendriteIntersects =
                mNetworkProps.boundingBoxes.getDendriteBox(neuronId).intersects(subcube);
        }

        if (axonIntersects || dendriteIntersects) {
            intersectingNeurons.append(neuronId);
        }
    }

    return intersectingNeurons;
}

/*
    Determines all neurons that meet the specified properties.

    @param origin Origin of the subvolume (in spatial coordinates).
    @param dimensions Number of voxels in each direction from the origin.
    @param cellTypes The cell types filter.
    @param regions The regions filter.
    @param neuronIds The neuron IDs filter.
*/
QList<int> FeatureExtractor::determineNeurons(QVector<float> origin, QVector<int> dimensions, QSet<QString> cellTypes,
                            QSet<QString> regions, QSet<int> neuronIds) {
    if (neuronIds.size() > 0) {
        return neuronIds.toList();
    }

    QList<int> neurons = determineIntersectingNeurons(origin, dimensions);
    QList<int> prunedNeurons;
    QList<int> prunedNeurons2;

    if (cellTypes.size() == 0) {
        prunedNeurons.append(neurons);
    } else {
        for (int i = 0; i < neurons.size(); i++) {
            int neuronId = neurons[i];
            int cellTypeId = mNetworkProps.neurons.getCellTypeId(neuronId);
            QString cellType = mNetworkProps.cellTypes.getName(cellTypeId);
            if (cellTypes.contains(cellType)) {
                prunedNeurons.append(neuronId);
            }
        }
    }

    if (regions.size() == 0) {
        prunedNeurons2.append(prunedNeurons);
    } else {
        for (int i = 0; i < prunedNeurons.size(); i++) {
            int neuronId = prunedNeurons[i];
            int regionId = mNetworkProps.neurons.getRegionId(neuronId);
            QString region = mNetworkProps.regions.getName(regionId);
            if (regions.contains(region)) {
                prunedNeurons2.append(neuronId);
            }
        }
    }

    return prunedNeurons2;
}

/*
    Saves the properties of the specified neurons into a csv file.

    @param fileName The name of the file.
    @param neurons The neuron IDs.
*/
void FeatureExtractor::saveNeurons(QString fileName, QList<int> neuronsToSave) {
    Neurons neurons;
    for (int i = 0; i < neuronsToSave.size(); i++) {
        int neuronId = neuronsToSave[i];
        neurons.addNeuron(mNetworkProps.neurons.getNeuronProps(neuronId));
    }
    neurons.saveCSV(fileName);
}
