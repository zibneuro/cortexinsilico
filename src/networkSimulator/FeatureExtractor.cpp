#include "FeatureExtractor.h"

/*
    Constructor.

    @param props The complete model data.
*/
FeatureExtractor::FeatureExtractor(NetworkProps& props) : mNetworkProps(props) {}

/*
    Extracts the model features from the specified subvolume according
    to the specified filters and writes the results to file. Creates
    the files features.csv, neurons.csv, voxels.csv.

    @param origin Origin of the subvolume (in spatial coordinates).
    @param dimensions Number of voxels in each direction from the origin.
    @param cellTypes The cell types filter.
    @param regions The regions filter.
    @param neuronIds The neuron IDs filter (overrules all others, if set).
    @param samplingFactor The neuron sampling factor, 1: take all, 2: take half, etc.
*/
void FeatureExtractor::extract(QVector<float> origin, QVector<int> dimensions,
                               QSet<QString> cellTypes, QSet<QString> regions, QSet<int> neuronIds,
                               int samplingFactor) {
    correctOrigin(origin);
    qDebug() << "[*] Pre-filtering neurons.";
    QList<int> neurons = determineNeurons(origin, dimensions, cellTypes, regions, neuronIds);
    qDebug() << "[*] Extracting features.";
    QList<Feature> features = extractFeatures(neurons, origin, dimensions, samplingFactor);
    qDebug() << "[*] Writing extracted features to file: features.csv.";
    writeFeatures("features.csv", features);
    qDebug() << "[*] Writing extracted features to file: featuresSpatial.csv.";
    writeFeaturesSpatial("featuresSpatial.csv", features, origin);
    qDebug() << "[*] Writing extracted neurons to file: neurons.csv.";
    writeNeurons("neurons.csv", features, origin, dimensions);
    qDebug() << "[*] Writing extracted voxels to file: voxels.csv.";
    writeVoxels("voxels.csv", features);
}

/*
    Determines all neurons that meet the specified properties.

    @param origin Origin of the subvolume (in spatial coordinates).
    @param dimensions Number of voxels in each direction from the origin.
    @param cellTypes The cell types filter.
    @param regions The regions filter.
    @param neuronIds The neuron IDs filter.
*/
QList<int> FeatureExtractor::determineNeurons(QVector<float> origin, QVector<int> dimensions,
                                              QSet<QString> cellTypes, QSet<QString> regions,
                                              QSet<int> neuronIds) {
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

                if (pre != NULL && pre->inRange(position)) {
                    Vec3i voxelLocation = pre->getVoxelContainingPoint(position);
                    voxel.pre = pre->getFieldValue(voxelLocation);
                } else {
                    voxel.pre = 0;
                }

                if (postExc != NULL && postExc->inRange(position)) {
                    Vec3i voxelLocation = postExc->getVoxelContainingPoint(position);
                    voxel.postExc = postExc->getFieldValue(voxelLocation);
                } else {
                    voxel.postExc = 0;
                }

                if (postAllExc != NULL && postAllExc->inRange(position)) {
                    Vec3i voxelLocation = postAllExc->getVoxelContainingPoint(position);
                    voxel.postAllExc = postAllExc->getFieldValue(voxelLocation);
                } else {
                    voxel.postAllExc = 0;
                }

                if (postInh != NULL && postInh->inRange(position)) {
                    Vec3i voxelLocation = postInh->getVoxelContainingPoint(position);
                    voxel.postInh = postInh->getFieldValue(voxelLocation);
                } else {
                    voxel.postInh = 0;
                }

                if (postAllInh != NULL && postAllInh->inRange(position)) {
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

    int nonEmptyNeurons = 0;
    for (int i = 0; i < neurons.size(); i++) {
        int neuronId = neurons[i];
        int cellTypeId = mNetworkProps.neurons.getCellTypeId(neuronId);
        QString cellType = mNetworkProps.cellTypes.getName(cellTypeId);
        QString functionalCellType =
            mNetworkProps.cellTypes.isExcitatory(cellTypeId) ? "exc" : "inh";
        int regionId = mNetworkProps.neurons.getRegionId(neuronId);
        QString region = mNetworkProps.regions.getName(regionId);
        int synapticSide = mNetworkProps.neurons.getSynapticSide(neuronId);

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

        QList<Voxel> voxels =
            determineVoxels(boutons, postExc, postAllExc, postInh, postAllInh, origin, dimensions);

        if (boutons != NULL) {
            delete boutons;
        }
        if (postExc != NULL) {
            delete postExc;
        }
        if (postInh != NULL) {
            delete postInh;
        }

        if (voxels.size() > 0) {
            nonEmptyNeurons++;
            if (nonEmptyNeurons % samplingFactor == 0) {
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
        }

        if (i % 1000 == 0) {
            qDebug() << "   Processed neuron " << i + 1 << "of" << neurons.size()
                     << "- Number of features" << features.size();
        }
    }

    delete postAllExc;
    delete postAllInh;

    qSort(features.begin(), features.end(), lessThan);
    return features;
}

/*
    Writes the features into a csv file.

    @param fileName The name of the file.
    @param features A list with the features.
*/
void FeatureExtractor::writeFeatures(QString fileName, QList<Feature>& features) {
    QFile csv(fileName);
    if (!csv.open(QIODevice::WriteOnly)) {
        const QString msg = QString("Cannot open file %1 for writing.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
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
    Writes the features into a csv file using spatial coordinates.

    @param fileName The name of the file.
    @param origin The corrected origin.
    @param features A list with the features.
*/
void FeatureExtractor::writeFeaturesSpatial(QString fileName, QList<Feature>& features,
                                            QVector<float> origin) {
    QFile csv(fileName);
    if (!csv.open(QIODevice::WriteOnly)) {
        const QString msg = QString("Cannot open file %1 for writing.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }
    const QChar sep(',');
    QTextStream out(&csv);
    out << "x" << sep << "y" << sep << "z" << sep << "neuronID" << sep << "morphologicalCellType"
        << sep << "functionalCellType" << sep << "region" << sep << "synapticSide" << sep << "pre"
        << sep << "postExc" << sep << "postAllExc" << sep << "postInh" << sep << "postAllInh"
        << "\n";
    float voxelSize = 50;
    for (int i = 0; i < features.size(); i++) {
        Feature feature = features[i];
        float x = origin[0] + (feature.voxelX - 1) * voxelSize;
        float y = origin[1] + (feature.voxelY - 1) * voxelSize;
        float z = origin[2] + (feature.voxelZ - 1) * voxelSize;
        out << x << sep << y << sep << z << sep << feature.neuronID << sep
            << feature.morphologicalCellType << sep << feature.functionalCellType << sep
            << feature.region << sep << feature.synapticSide << sep << feature.pre << sep
            << feature.postExc << sep << feature.postAllExc << sep << feature.postInh << sep
            << feature.postAllInh << "\n";
    }
}

/*
    Writes properties of the extracted neurons into a csv file.

    @param fileName The name of the file.
    @param features A list with the features.
    @param origin Origin of the subvolume (in spatial coordinates).
    @param dimensions Number of voxels in each direction from the origin.
*/
void FeatureExtractor::writeNeurons(QString fileName, QList<Feature>& features,
                                    QVector<float> origin, QVector<int> dimensions) {
    QFile csv(fileName);
    if (!csv.open(QIODevice::WriteOnly)) {
        const QString msg = QString("Cannot open file %1 for writing.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }
    const QChar sep(',');
    QTextStream out(&csv);
    out << "neuronID" << sep << "somaX" << sep << "somaY" << sep << "somaZ" << sep
        << "somaWithinSubcube" << sep << "morphologicalCellType" << sep << "functionalCellType"
        << sep << "region" << sep << "laminarLocation" << sep << "synapticSide"
        << "\n";
    QSet<int> neuronIdSet;
    for (int i = 0; i < features.size(); i++) {
        neuronIdSet.insert(features[i].neuronID);
    }
    QList<int> neuronIds = neuronIdSet.toList();
    qSort(neuronIds);
    for (int i = 0; i < neuronIds.size(); i++) {
        int neuronId = neuronIds[i];
        Vec3f somaPosition = mNetworkProps.neurons.getSomaPosition(neuronId);
        QString somaWithin =
            somaWithinSubcube(somaPosition, origin, dimensions) ? "inside" : "outside";
        int cellTypeId = mNetworkProps.neurons.getCellTypeId(neuronId);
        QString cellType = mNetworkProps.cellTypes.getName(cellTypeId);
        QString functionalCellType =
            mNetworkProps.cellTypes.isExcitatory(cellTypeId) ? "exc" : "inh";
        int regionId = mNetworkProps.neurons.getRegionId(neuronId);
        QString region = mNetworkProps.regions.getName(regionId);
        QString laminarLocation =
            CIS3D::getLaminarLocation(mNetworkProps.neurons.getLaminarLocation(neuronId));
        QString synapticSide =
            CIS3D::getSynapticSide(mNetworkProps.neurons.getSynapticSide(neuronId));
        out << neuronId << sep << somaPosition[0] << sep << somaPosition[1] << sep
            << somaPosition[2] << sep << somaWithin << sep << cellType << sep << functionalCellType
            << sep << region << sep << laminarLocation << sep << synapticSide << "\n";
    }
}

/*
    Writes properties of the extracted voxels into a csv file.

    @param fileName The name of the file.
    @param features The extracted features.
*/
void FeatureExtractor::writeVoxels(QString fileName, QList<Feature>& features) {
    QFile csv(fileName);
    if (!csv.open(QIODevice::WriteOnly)) {
        const QString msg = QString("Cannot open file %1 for writing.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }
    const QChar sep(',');
    QTextStream out(&csv);
    out << "voxelID" << sep << "voxelX" << sep << "voxelY" << sep << "voxelZ" << sep << "preNeurons"
        << sep << "postNeurons" << sep << "allNeurons"
        << "\n";
    if (features.size() == 0) {
        return;
    }
    int voxelId = features[0].voxelID;
    QSet<int> preNeurons;
    QSet<int> postNeurons;
    QSet<int> allNeurons;
    for (int i = 0; i < features.size(); i++) {
        if (voxelId == features[i].voxelID) {
            allNeurons.insert(features[i].neuronID);
            if (features[i].pre != 0) {
                preNeurons.insert(features[i].neuronID);
            }
            if (features[i].postExc != 0 || features[i].postInh != 0) {
                postNeurons.insert(features[i].neuronID);
            }
        }
        if (voxelId != features[i].voxelID || i + 1 == features.size()) {
            int idx = i + 1 == features.size() ? i : i - 1;
            out << features[idx].voxelID << sep << features[idx].voxelX << sep
                << features[idx].voxelY << sep << features[idx].voxelZ << sep << preNeurons.size()
                << sep << postNeurons.size() << sep << allNeurons.size() << "\n";
            preNeurons.clear();
            postNeurons.clear();
            allNeurons.clear();
            voxelId = features[i].voxelID;
        }
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

    @param origin The origin of the subcube.
    @param dimensions The number of voxels in each dimension.
    @return The intersecting neurons.
*/
QList<int> FeatureExtractor::determineIntersectingNeurons(QVector<float> origin,
                                                          QVector<int> dimensions) {
    const float voxelSize = 50;
    Vec3f shiftedOrigin(origin[0], origin[1], origin[2]);
    Vec3f pointMax(origin[0] + dimensions[0] * voxelSize, origin[1] + dimensions[1] * voxelSize,
                   origin[2] + dimensions[2] * voxelSize);
    BoundingBox subcube(shiftedOrigin, pointMax);

    QList<int> intersectingNeurons;
    SelectionFilter emptyFilter;
    QList<int> allNeurons = mNetworkProps.neurons.getFilteredNeuronIds(emptyFilter);

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
    Determines whether the soma is located within the specified subcube.

    @param somaPosition Spatial location of the soma.
    @param origin The origin of the subcube.
    @param dimensions The number of voxels in each dimension.
    @return True, if the soma is located within the subcube.
*/
bool FeatureExtractor::somaWithinSubcube(Vec3f somaPosition, QVector<float> origin,
                                         QVector<int> dimensions) {
    bool within = true;
    const float voxelSize = 50;
    for (int i = 0; i < 3; i++) {
        float low = origin[i];
        float high = origin[i] + dimensions[i] * voxelSize;
        within = within && (somaPosition[i] >= low && somaPosition[i] <= high);
    }
    return within;
}

/*
    Corrects the user specified origin such that it lies in the
    centre of the voxel (according to the grid exported from Amira).
    @param origin The user specified origin.
*/
void FeatureExtractor::correctOrigin(QVector<float>& origin) {
    QDir modelDataDir = CIS3D::getModelDataDir(mNetworkProps.dataRoot);
    const QString pstAllFileExc = CIS3D::getPSTAllFullPath(modelDataDir, CIS3D::EXCITATORY);
    SparseField* postAllExc = SparseField::load(pstAllFileExc);
    Vec3f origin3f(origin[0], origin[1], origin[2]);
    Vec3i voxelLocation = postAllExc->getVoxelContainingPoint(origin3f);
    Vec3f referenceOrigin = postAllExc->getOrigin();
    Vec3f voxelSize = postAllExc->getVoxelSize();
    for (int i = 0; i < 3; i++) {
        origin[i] = referenceOrigin[i] + voxelLocation[i] * voxelSize[i] + 0.5 * voxelSize[i];
    }
    delete postAllExc;
}