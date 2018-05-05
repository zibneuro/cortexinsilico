#include "MyStatistic.h"
#include "Util.h"
#include "Typedefs.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DSparseVectorSet.h"
#include <QDebug>

MyStatistic::MyStatistic(const NetworkProps& networkProps) :
    NetworkStatistic(networkProps){
}


void MyStatistic::doCalculate(const IdList& preNeurons, const IdList& postNeurons){

    // Iterate over regions with postsynaptic neurons that meet the filter criteria
    const IdsPerCellTypeRegion idsPerCellTypeRegion = Util::sortByCellTypeRegionIDs(postNeurons, mNetwork);
    for (IdsPerCellTypeRegion::ConstIterator it = idsPerCellTypeRegion.constBegin(); it != idsPerCellTypeRegion.constEnd(); ++it) {

        // Load file with precomputed innervation data corresponding to the current region
        const CellTypeRegion cellTypeRegion = it.key();
        const QString cellTypeName = mNetwork.cellTypes.getName(cellTypeRegion.first);
        const QString regionName = mNetwork.regions.getName(cellTypeRegion.second);
        QString dataRoot = mNetwork.dataRoot;
        const QString innervationFile = CIS3D::getInnervationPostFileName(dataRoot, regionName, cellTypeName);
        const IdList& ids = it.value();
        SparseVectorSet* vectorSet = SparseVectorSet::load(innervationFile);
        qDebug() << "Loading" << innervationFile;

        // Iterate over postsynaptic neurons in the current region
        for (int post=0; post<ids.size(); ++post) {
            const int postId = ids[post];

            // Iterate over presynaptic neurons (exploiting axon redundancy)
            for (int pre=0; pre<preNeurons.size(); ++pre) {
                const int preId = preNeurons[pre];
                const int mappedPreId = mNetwork.axonRedundancyMap.getNeuronIdToUse(preId);

                // Retrieve the innervation value
                const float innervationValue = vectorSet->getValue(postId, mappedPreId);

                // Perform your analysis with the innervation value.
                // Here: Record as sample to determine overall mean, variance, etc. of innervation values
                mStandardStatistic.addSample(innervationValue);
            }
        }

        // Uncomment, when integrating the statistic into the webframework
        // reportUpdate();
    }

    // Uncomment, when integrating the statistic into the webframework
    // reportComplete();
}


void MyStatistic::doCreateJson(QJsonObject& /*obj*/) const {
    // Implement when integrating the statistic into the webframework
    // refer to shared/InnervationStatistic.cpp as reference
}

void MyStatistic::doCreateCSV(QTextStream& /*out*/, const QChar /*sep*/) const {
    // Implement when integrating the statistic into the webframework
    // refer to shared/InnervationStatistic.cpp as reference
}

Statistics MyStatistic::getResult() const{
    return mStandardStatistic;
}
