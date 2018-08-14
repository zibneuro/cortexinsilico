#include "FeatureProvider.h"
#include <QDebug>
#include <QList>
#include "CIS3DConstantsHelpers.h"
#include "FeatureReader.h"
#include "Typedefs.h"
#include <omp.h>
#include <mutex>

FeatureProvider::FeatureProvider(NetworkProps& networkProps) : mNetworkProps(networkProps) {}

void FeatureProvider::init(NeuronSelection& selection) {
    mSelection = selection;
    QDir modelDataDir = CIS3D::getModelDataDir(mNetworkProps.dataRoot);

    const QString pstAllFileExc = CIS3D::getPSTAllFullPath(modelDataDir, CIS3D::EXCITATORY);
    mPostsynapticAllExc = SparseField::load(pstAllFileExc);
    /*
    const QString pstAllFileInh = CIS3D::getPSTAllFullPath(modelDataDir, CIS3D::INHIBITORY);
    mPostsynapticAllInh = SparseField::load(pstAllFileInh);
    */
    std::mutex mutex;
    
    for (int i = 0; i < mSelection.Presynaptic().size(); i++) {
        int neuronId = mSelection.Presynaptic()[i];

        int cellTypeId = mNetworkProps.neurons.getCellTypeId(neuronId);
        QString cellType = mNetworkProps.cellTypes.getName(cellTypeId);
        int regionId = mNetworkProps.neurons.getRegionId(neuronId);
        QString region = mNetworkProps.regions.getName(regionId);
        // int synapticSide = mNetworkProps.neurons.getSynapticSide(neuronId);

        int mappedId = mNetworkProps.axonRedundancyMap.getNeuronIdToUse(neuronId);
        QMap<int, int>::const_iterator it = mPresynapticMultiplicity.find(mappedId);
        if (it == mPresynapticMultiplicity.end()) {
            const QString filePathPre =
                CIS3D::getBoutonsFileFullPath(modelDataDir, cellType, mappedId);
            mPresynaptic.insert(mappedId, SparseField::load(filePathPre));
            mPresynapticMultiplicity.insert(mappedId, 1);
        } else {
            mPresynapticMultiplicity.insert(mappedId, mPresynapticMultiplicity[mappedId] + 1);
        }
    }
    
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < mSelection.Postsynaptic().size(); i++) {
        int neuronId = mSelection.Postsynaptic()[i];

        int cellTypeId = mNetworkProps.neurons.getCellTypeId(neuronId);
        QString cellType = mNetworkProps.cellTypes.getName(cellTypeId);
        int regionId = mNetworkProps.neurons.getRegionId(neuronId);
        QString region = mNetworkProps.regions.getName(regionId);

        const QString filePathExc =
            CIS3D::getPSTFileFullPath(modelDataDir, region, cellType, neuronId, CIS3D::EXCITATORY);
        mutex.lock();
        mPostsynapticExc.insert(neuronId, SparseField::load(filePathExc));
        mutex.unlock();
        /*
        const QString filePathInh = CIS3D::getPSTFileFullPath(modelDataDir, region, cellType,
                                                              neuronId, CIS3D::INHIBITORY);
        mPostsynapticInh.insert(neuronId, SparseField::load(filePathInh));
        */
    }

    qDebug() << "[*] Loaded sparse fields (pre, postExc, postInh)" << mPresynaptic.size()
             << mPostsynapticExc.size() << mPostsynapticInh.size();
}

NeuronSelection FeatureProvider::getSelection() { return mSelection; }

QList<int> FeatureProvider::getUniquePresynaptic() { return mPresynaptic.uniqueKeys(); }

SparseField* FeatureProvider::getPre(int neuronId) { return mPresynaptic[neuronId]; }

SparseField* FeatureProvider::getPostExc(int neuronId) { return mPostsynapticExc[neuronId]; }

SparseField* FeatureProvider::getPostInh(int neuronId) { return mPostsynapticInh[neuronId]; }

SparseField* FeatureProvider::getPostAllExc() { return mPostsynapticAllExc; }

SparseField* FeatureProvider::getPostAllInh() { return mPostsynapticAllInh; }

int FeatureProvider::getPresynapticMultiplicity(int neuronId) {
    return mPresynapticMultiplicity[neuronId];
}