#include "FeatureProvider.h"
#include "CIS3DConstantsHelpers.h"
#include "FeatureReader.h"
#include "Typedefs.h"
#include "UtilIO.h"
#include <QChar>
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <mutex>
#include <omp.h>

FeatureProvider::FeatureProvider()
{
}

FeatureProvider::~FeatureProvider()
{
}

void
FeatureProvider::preprocess(NetworkProps& networkProps,
                            NeuronSelection& selection,
                            bool duplicity)
{
    QDir modelDataDir = CIS3D::getModelDataDir(networkProps.dataRoot);

    QString postAllExcFile;
    QList<QString> postExcFiles;
    QList<QString> preFiles;
    QList<int> preMultiplicities;

    // PostAllExc
    postAllExcFile = CIS3D::getPSTAllFullPath(modelDataDir, CIS3D::EXCITATORY);

    // Pre
    if (duplicity)
    {
        QMap<int, int> multiplicity;
        for (int i = 0; i < selection.Presynaptic().size(); i++)
        {
            int neuronId = selection.Presynaptic()[i];
            int cellTypeId = networkProps.neurons.getCellTypeId(neuronId);
            if (!networkProps.cellTypes.isExcitatory(cellTypeId))
            {
                const QString msg =
                    QString("Presynaptic inhibitory neurons not implemented yet.");
                throw std::runtime_error(qPrintable(msg));
            }
            int mappedId = networkProps.axonRedundancyMap.getNeuronIdToUse(neuronId);
            QMap<int, int>::const_iterator it = multiplicity.find(mappedId);
            if (it == multiplicity.end())
            {
                multiplicity.insert(mappedId, 1);
            }
            else
            {
                multiplicity.insert(mappedId, multiplicity[mappedId] + 1);
            }
            if (selection.Postsynaptic().contains(neuronId))
            {
                multiplicity.insert(mappedId, multiplicity[mappedId] - 1);
            }
        }
        QList<int> uniquePreIds = multiplicity.keys();
        qSort(uniquePreIds);
        for (int i = 0; i < uniquePreIds.size(); i++)
        {
            int neuronId = uniquePreIds[i];
            preMultiplicities.push_back(multiplicity[neuronId]);
            int cellTypeId = networkProps.neurons.getCellTypeId(neuronId);
            QString cellType = networkProps.cellTypes.getName(cellTypeId);
            preFiles.push_back(
                CIS3D::getBoutonsFileFullPath(modelDataDir, cellType, neuronId));
        }
    }
    else
    {
        for (int i = 0; i < selection.Presynaptic().size(); i++)
        {
            int neuronId = selection.Presynaptic()[i];
            int mappedId = networkProps.axonRedundancyMap.getNeuronIdToUse(neuronId);
            int cellTypeId = networkProps.neurons.getCellTypeId(mappedId);
            QString cellType = networkProps.cellTypes.getName(cellTypeId);
            if (!networkProps.cellTypes.isExcitatory(cellTypeId))
            {
                const QString msg =
                    QString("Presynaptic inhibitory neurons not implemented yet.");
                throw std::runtime_error(qPrintable(msg));
            }
            preFiles.push_back(
                CIS3D::getBoutonsFileFullPath(modelDataDir, cellType, mappedId));
            preMultiplicities.push_back(1);
        }
    }

    // Post
    for (int i = 0; i < selection.Postsynaptic().size(); i++)
    {
        int neuronId = selection.Postsynaptic()[i];
        int cellTypeId = networkProps.neurons.getCellTypeId(neuronId);
        QString cellType = networkProps.cellTypes.getName(cellTypeId);
        int regionId = networkProps.neurons.getRegionId(neuronId);
        QString region = networkProps.regions.getName(regionId);

        postExcFiles.push_back(CIS3D::getPSTFileFullPath(
            modelDataDir, region, cellType, neuronId, CIS3D::EXCITATORY));
    }

    // Write to init file
    QFile csv(mInitFileName);
    if (!csv.open(QIODevice::WriteOnly))
    {
        const QString msg =
            QString("Cannot open file %1 for writing.").arg(mInitFileName);
        throw std::runtime_error(qPrintable(msg));
    }
    const QChar sep(',');
    QTextStream out(&csv);
    out << -2 << sep << postAllExcFile << "\n";
    for (int i = 0; i < preFiles.size(); i++)
    {
        out << preMultiplicities[i] << sep << preFiles[i] << "\n";
    }
    for (int i = 0; i < postExcFiles.size(); i++)
    {
        out << -1 << sep << postExcFiles[i] << "\n";
    }

    qDebug() << "[*] Preprocessed selection (#preUnique, #postExc, initFileName)"
             << preFiles.size() << postExcFiles.size() << mInitFileName;
}

void
FeatureProvider::preprocessFeatures(NetworkProps& networkProps,
                                    NeuronSelection& selection,
                                    double eps)
{
    // ########### INIT FIELDS ###########
    QDir modelDataDir = CIS3D::getModelDataDir(networkProps.dataRoot);
    std::set<int> voxel;
    std::map<int, int> neuron_funct;
    std::map<int, int> neuron_morph;
    std::map<int, int> neuron_regio;    
    std::map<int, std::map<int,float> > neuron_pre;
    std::map<int, std::map<int,float> > neuron_postExc;
    std::map<int, std::map<int,float> > neuron_postInh;
    std::map<int, float> voxel_postAllExc;
    std::map<int, float> voxel_postAllInh;

    // ########### POST ALL EXC ###########
    QString postAllExcFile =
        CIS3D::getPSTAllFullPath(modelDataDir, CIS3D::EXCITATORY);
    SparseField* postAllExcField = SparseField::load(postAllExcFile);
    mGridOrigin = postAllExcField->getOrigin();
    mGridDimensions = postAllExcField->getDimensions();
    voxel_postAllExc = postAllExcField->getModifiedCopy(1, eps);

    // ########### POST ALL INH ###########
    QString postAllInhFile =
        CIS3D::getPSTAllFullPath(modelDataDir, CIS3D::INHIBITORY);
    SparseField* postAllInhField = SparseField::load(postAllInhFile);
    assertGrid(postAllInhField);
    voxel_postAllInh = postAllInhField->getModifiedCopy(1, eps);
    
    // ########### PRE ###########
    for (int i = 0; i < selection.Presynaptic().size(); i++)
    {
        int neuronId = selection.Presynaptic()[i];

        int cellTypeId = networkProps.neurons.getCellTypeId(neuronId);
        neuron_morph[neuronId] = cellTypeId;
        QString cellType = networkProps.cellTypes.getName(cellTypeId);
        int funcType = networkProps.cellTypes.isExcitatory(cellTypeId) ? 0 : 1;
        neuron_funct[neuronId] = funcType;
        int regionId = networkProps.neurons.getRegionId(neuronId);
        neuron_regio[neuronId] = regionId; 

        int mappedId = networkProps.axonRedundancyMap.getNeuronIdToUse(neuronId);
        QString filePath = CIS3D::getBoutonsFileFullPath(modelDataDir, cellType, mappedId);
        SparseField* preField = SparseField::load(filePath);
        assertGrid(preField);

        neuron_pre[neuronId] = preField->getModifiedCopy(1, eps);
    }

    // ########### POST ###########
    for (int i = 0; i < selection.Postsynaptic().size(); i++)
    {
        int neuronId = selection.Postsynaptic()[i];

        int cellTypeId = networkProps.neurons.getCellTypeId(neuronId);
        neuron_morph[neuronId] = cellTypeId;
        QString cellType = networkProps.cellTypes.getName(cellTypeId);
        int regionId = networkProps.neurons.getRegionId(neuronId);
        neuron_regio[neuronId] = regionId;
        QString region = networkProps.regions.getName(regionId);
        int funcType = networkProps.cellTypes.isExcitatory(cellTypeId) ? 0 : 1;
        neuron_funct[neuronId] = funcType;

        QString filePathExc = CIS3D::getPSTFileFullPath(modelDataDir, region, cellType, neuronId, CIS3D::EXCITATORY);
        SparseField* postFieldExc = SparseField::load(filePathExc);
        assertGrid(postFieldExc);        
        neuron_postExc[neuronId] = postFieldExc->getModifiedCopy(1, eps);

        QString filePathInh = CIS3D::getPSTFileFullPath(modelDataDir, region, cellType, neuronId, CIS3D::INHIBITORY);
        SparseField* postFieldInh = SparseField::load(filePathInh);
        assertGrid(postFieldInh);
        neuron_postInh[neuronId] = postFieldInh->getModifiedCopy(1, eps);        
    }

    // ########### PRUNE ###########
    QVector<float> bbMin = selection.getBBoxMin();
    QVector<float> bbMax = selection.getBBoxMax();
    for(auto it = voxel_postAllExc.begin();it!=voxel_postAllExc.end();){
        if(inRange(postAllExcField,bbMin,bbMax,it->first)){
            ++it;
        } else {
            voxel_postAllExc.erase(it++);
        }
    }

    // ########### WRITE TO FILE ###########
    UtilIO::makeDir("features_meta");
    UtilIO::makeDir("features_pre");    
    UtilIO::makeDir("features_postExc");
    UtilIO::makeDir("features_postInh");
    UtilIO::makeDir("features_postAll");    

    writeMapFloat(voxel_postAllExc, "features_postAll", "voxel_postAllExc");
    writeMapFloat(voxel_postAllInh, "features_postAll", "voxel_postAllInh");
  
    writeMapInt(neuron_funct, "features_meta", "neuron_funct");
    writeMapInt(neuron_morph, "features_meta", "neuron_morph");
    writeMapInt(neuron_regio, "features_meta", "neuron_regio");
    
}

void
FeatureProvider::init()
{
    QFile file(mInitFileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        const QString msg =
            QString("Error reading features file. Could not open file %1")
                .arg(mInitFileName);
        throw std::runtime_error(qPrintable(msg));
    }
    const QChar sep = ',';
    QTextStream in(&file);

    QString line = in.readLine();
    if (line.isNull())
    {
        const QString msg = QString("Error reading features file %1. No content.")
                                .arg(mInitFileName);
        throw std::runtime_error(qPrintable(msg));
    }

    // PostAllExc
    QStringList parts = line.split(sep);
    mPostAllExc = SparseField::load(parts[1]);

    // Pre & Post
    line = in.readLine();
    while (!line.isNull())
    {
        QStringList parts = line.split(sep);
        int mult = parts[0].toInt();
        SparseField* field = SparseField::load(parts[1]);
        if (mult > 0)
        {
            mPreMultiplicity.push_back(mult);
            mPre.push_back(field);
        }
        else
        {
            mPostExc.push_back(field);
        }
        line = in.readLine();
    }

    // qDebug() << mPre.size() << mPostExc.size();
}

int
FeatureProvider::getNumPre()
{
    return mPre.size();
}

int
FeatureProvider::getNumPost()
{
    return mPostExc.size();
}

SparseField*
FeatureProvider::getPre(int neuronId)
{
    return mPre[neuronId];
}

SparseField*
FeatureProvider::getPostExc(int neuronId)
{
    return mPostExc[neuronId];
}

SparseField*
FeatureProvider::getPostAllExc()
{
    return mPostAllExc;
}

int
FeatureProvider::getPreMultiplicity(int neuronId)
{
    return mPreMultiplicity[neuronId];
}

void
FeatureProvider::assertGrid(SparseField* field)
{
    if (!field->getOrigin().equals(mGridOrigin) || field->getDimensions() != mGridDimensions)
    {
        qDebug() << mGridOrigin.getX() << mGridOrigin.getY() << mGridOrigin.getZ()
                 << mGridDimensions.getX() << mGridDimensions.getY() << mGridDimensions.getZ();
        field->getCoordinates().print();
        throw std::runtime_error("grid spacing");
    }
}

void
FeatureProvider::writeMapFloat(std::map<int, float>& mapping, QString folder, QString fileName)
{
    qDebug() << "[*] Writing file " << fileName;
    if (folder != "")
    {
        fileName = QDir(folder).filePath(fileName);
    }
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        const QString msg =
            QString("Cannot open file %1 for writing.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }
    QTextStream stream(&file);

    for (auto it = mapping.begin(); it != mapping.end(); ++it)
    {
        stream << it->first << " " << it->second << "\n";
    }
}

void
FeatureProvider::writeMapInt(std::map<int, int>& mapping, QString folder, QString fileName)
{
    qDebug() << "[*] Writing file " << fileName;
    if (folder != "")
    {
        fileName = QDir(folder).filePath(fileName);
    }
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        const QString msg =
            QString("Cannot open file %1 for writing.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }
    QTextStream stream(&file);

    for (auto it = mapping.begin(); it != mapping.end(); ++it)
    {
        stream << it->first << " " << it->second << "\n";
    }
}

void
FeatureProvider::writeVoxels(SparseField* postAllExc, std::set<int>& voxelIds, QString folder, QString fileName)
{
    qDebug() << "[*] Writing file " << fileName;
    if (folder != "")
    {
        fileName = QDir(folder).filePath(fileName);
    }
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        const QString msg =
            QString("Cannot open file %1 for writing.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }
    QTextStream stream(&file);
    for (auto it = voxelIds.begin(); it != voxelIds.end(); ++it)
    {
        Vec3f position = postAllExc->getSpatialLocation(*it);
        stream << *it << " " << position[0] << " " << position[1] << " " << position[2] << " "
               << "\n";
    }
}

void
FeatureProvider::registerVoxelIds(std::set<int>& voxelIds, std::map<int, float>& field)
{
    for (auto it = field.begin(); it != field.end(); ++it)
    {
        voxelIds.insert(it->first);
    }
}

bool FeatureProvider::inRange(SparseField* postAllExc, QVector<float>& bbMin, QVector<float>& bbMax, int voxelId){
    Vec3f pos = postAllExc->getSpatialLocation(voxelId);
    return pos[0] >= bbMin[0] && pos[1] >= bbMin[1] && pos[2] >= bbMin[2] 
        && pos[0] <= bbMax[0] && pos[1] <= bbMax[1] && pos[2] <= bbMax[2];
}