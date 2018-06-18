#include "MyStatistic.h"
#include <QDebug>
#include "CIS3DConstantsHelpers.h"
#include "CIS3DSparseVectorSet.h"
#include "Typedefs.h"
#include "Util.h"

MyStatistic::MyStatistic(const NetworkProps& networkProps) : NetworkStatistic(networkProps) {}

/**
    Performs the actual computation based on the specified neurons.
    @param selection The selected neurons.
*/
void MyStatistic::doCalculate(const NeuronSelection& selection) {
    // Iterate over regions with postsynaptic neurons that meet the filter criteria
    const IdsPerCellTypeRegion idsPerCellTypeRegion =
        Util::sortByCellTypeRegionIDs(selection.Postsynaptic(), mNetwork);
    for (IdsPerCellTypeRegion::ConstIterator it = idsPerCellTypeRegion.constBegin();
         it != idsPerCellTypeRegion.constEnd(); ++it) {
        // Load file with precomputed innervation data corresponding to the current region
        const CellTypeRegion cellTypeRegion = it.key();
        const QString cellTypeName = mNetwork.cellTypes.getName(cellTypeRegion.first);
        const QString regionName = mNetwork.regions.getName(cellTypeRegion.second);
        QString dataRoot = mNetwork.dataRoot;
        const QString innervationFile =
            CIS3D::getInnervationPostFileName(dataRoot, regionName, cellTypeName);
        const IdList& ids = it.value();
        SparseVectorSet* vectorSet = SparseVectorSet::load(innervationFile);
        qDebug() << "Loading" << innervationFile;

        // Iterate over postsynaptic neurons in the current region
        for (int post = 0; post < ids.size(); ++post) {
            const int postId = ids[post];

            // Iterate over presynaptic neurons (exploiting axon redundancy)
            for (int pre = 0; pre < selection.Presynaptic().size(); ++pre) {
                const int preId = selection.Presynaptic()[pre];
                const int mappedPreId = mNetwork.axonRedundancyMap.getNeuronIdToUse(preId);

                // Retrieve the innervation value
                const float innervationValue = vectorSet->getValue(postId, mappedPreId);

                // Perform your analysis with the innervation value.
                // Here: Record as sample to determine overall mean, variance, etc. of innervation
                // values
                mStandardStatistic.addSample(innervationValue);
            }
        }

        // Uncomment, when integrating the statistic into the webframework
        // reportUpdate();
    }

    // Uncomment, when integrating the statistic into the webframework
    // reportComplete();
}

/**
    Adds the result values to a JSON object
    @param obj JSON object to which the values are appended
*/
void MyStatistic::doCreateJson(QJsonObject& /*obj*/) const {
    // Implement when integrating the statistic into the webframework
    // refer to shared/InnervationStatistic.cpp as reference
}

/**
    Writes the result values to file stream (CSV).
    @param out The file stream to which the values are written.
    @param sep The separator between parameter name and value.
*/
void MyStatistic::doCreateCSV(QTextStream& /*out*/, const QChar /*sep*/) const {
    // Implement when integrating the statistic into the webframework
    // refer to shared/InnervationStatistic.cpp as reference
}

/**
    Returns internal result, for testing purposes.
    @return The statistic.
*/
Statistics MyStatistic::getResult() const { return mStandardStatistic; }
