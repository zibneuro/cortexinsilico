#include "FeatureProvider.h"
#include <omp.h>
#include <QChar>
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <mutex>
#include "CIS3DConstantsHelpers.h"
#include "FeatureReader.h"
#include "Typedefs.h"

FeatureProvider::FeatureProvider() {}

FeatureProvider::~FeatureProvider(){
}

void FeatureProvider::preprocess(NetworkProps& networkProps, NeuronSelection& selection) {
    QDir modelDataDir = CIS3D::getModelDataDir(networkProps.dataRoot);

    QString postAllExcFile;
    QList<QString> postExcFiles;
    QList<QString> preFiles;
    QList<int> preMultiplicities;

    // PostAllExc
    postAllExcFile = CIS3D::getPSTAllFullPath(modelDataDir, CIS3D::EXCITATORY);

    // Pre
    QMap<int, int> multiplicity;
    for (int i = 0; i < selection.Presynaptic().size(); i++) {
        int neuronId = selection.Presynaptic()[i];
        int mappedId = networkProps.axonRedundancyMap.getNeuronIdToUse(neuronId);
        QMap<int, int>::const_iterator it = multiplicity.find(mappedId);
        if (it == multiplicity.end()) {
            multiplicity.insert(mappedId, 1);
        } else {
            multiplicity.insert(mappedId, multiplicity[mappedId] + 1);
        }
    }
    QList<int> uniquePreIds = multiplicity.keys();
    qSort(uniquePreIds);
    for (int i = 0; i < uniquePreIds.size(); i++) {
        int neuronId = uniquePreIds[i];
        preMultiplicities.push_back(multiplicity[neuronId]);
        int cellTypeId = networkProps.neurons.getCellTypeId(neuronId);
        QString cellType = networkProps.cellTypes.getName(cellTypeId);
        preFiles.push_back(CIS3D::getBoutonsFileFullPath(modelDataDir, cellType, neuronId));
    }

    // Post
    for (int i = 0; i < selection.Postsynaptic().size(); i++) {
        int neuronId = selection.Postsynaptic()[i];
        int cellTypeId = networkProps.neurons.getCellTypeId(neuronId);
        QString cellType = networkProps.cellTypes.getName(cellTypeId);
        int regionId = networkProps.neurons.getRegionId(neuronId);
        QString region = networkProps.regions.getName(regionId);

        postExcFiles.push_back(
            CIS3D::getPSTFileFullPath(modelDataDir, region, cellType, neuronId, CIS3D::EXCITATORY));
    }

    // Write to init file
    QFile csv(mInitFileName);
    if (!csv.open(QIODevice::WriteOnly)) {
        const QString msg = QString("Cannot open file %1 for writing.").arg(mInitFileName);
        throw std::runtime_error(qPrintable(msg));
    }
    const QChar sep(',');
    QTextStream out(&csv);
    out << -2 << sep << postAllExcFile << "\n";
    for (int i = 0; i < preFiles.size(); i++) {
        out << preMultiplicities[i] << sep << preFiles[i] << "\n";
    }
    for (int i = 0; i < postExcFiles.size(); i++) {
        out << -1 << sep << postExcFiles[i] << "\n";
    }

    qDebug() << "[*] Preprocessed selection (#preUnique, #postExc, initFileName)" << preFiles.size()
             << postExcFiles.size() << mInitFileName;
}

void FeatureProvider::init() {
    QFile file(mInitFileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString msg =
            QString("Error reading features file. Could not open file %1").arg(mInitFileName);
        throw std::runtime_error(qPrintable(msg));
    }
    const QChar sep = ',';
    QTextStream in(&file);

    QString line = in.readLine();
    if (line.isNull()) {
        const QString msg = QString("Error reading features file %1. No content.").arg(mInitFileName);
        throw std::runtime_error(qPrintable(msg));
    }

    // PostAllExc
    QStringList parts = line.split(sep);
    mPostAllExc = SparseField::load(parts[1]);

    // Pre & Post
    line = in.readLine();
    while (!line.isNull()) {
        QStringList parts = line.split(sep);
        int mult = parts[0].toInt();        
        SparseField* field = SparseField::load(parts[1]);
        if(mult > 0){
            mPreMultiplicity.push_back(mult);            
            mPre.push_back(field);
        } else {
            mPostExc.push_back(field);
        }
        line = in.readLine();
    }

    //qDebug() << mPre.size() << mPostExc.size();
}

int FeatureProvider::getNumPre() { return mPre.size(); }

int FeatureProvider::getNumPost() { return mPostExc.size(); }

SparseField* FeatureProvider::getPre(int neuronId) { return mPre[neuronId]; }

SparseField* FeatureProvider::getPostExc(int neuronId) { return mPostExc[neuronId]; }

SparseField* FeatureProvider::getPostAllExc() { return mPostAllExc; }

int FeatureProvider::getPreMultiplicity(int neuronId) { return mPreMultiplicity[neuronId]; }