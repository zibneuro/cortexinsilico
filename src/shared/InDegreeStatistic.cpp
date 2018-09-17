#include "InDegreeStatistic.h"
#include <math.h>
#include <QChar>
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QJsonArray>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <ctime>
#include <stdexcept>
#include "CIS3DConstantsHelpers.h"
#include "CIS3DSparseVectorSet.h"
#include "InnervationStatistic.h"
#include "Typedefs.h"
#include "Util.h"

InDegreeStatistic::InDegreeStatistic(const NetworkProps& networkProps,
                                     int sampleSize)
    : NetworkStatistic(networkProps)
    , mSampleSize(sampleSize)
{
    this->mNumConnections = mSampleSize;
}

void
InDegreeStatistic::checkInput(const NeuronSelection& selection)
{
    if (selection.MotifA().size() == 0)
    {
        const QString msg = QString("In-Degree selection A empty");
        throw std::runtime_error(qPrintable(msg));
    }
    if (selection.MotifB().size() == 0)
    {
        const QString msg = QString("In-Degree selection B empty");
        throw std::runtime_error(qPrintable(msg));
    }
    if (selection.MotifC().size() == 0)
    {
        const QString msg = QString("In-Degree selection C empty");
        throw std::runtime_error(qPrintable(msg));
    }
}

QList<int>
InDegreeStatistic::samplePostIds(QList<int> selectionC)
{
    if (selectionC.size() <= mSampleSize)
    {
        mSampleSize = selectionC.size();
        return selectionC;
    }
    else
    {
        std::random_shuffle(selectionC.begin(), selectionC.end());
        QList<int> postIds;
        for (int i = 0; i < mSampleSize; i++)
        {
            postIds.append(selectionC[i]);
        }
        return postIds;
    }
}

void
InDegreeStatistic::doCalculate(const NeuronSelection& selection)
{
    checkInput(selection);
    QList<int> postIds = samplePostIds(selection.MotifC());

    const IdsPerCellTypeRegion idsPerCellTypeRegion = Util::sortByCellTypeRegionIDs(postIds, mNetwork);
    for (IdsPerCellTypeRegion::ConstIterator it = idsPerCellTypeRegion.constBegin(); it != idsPerCellTypeRegion.constEnd(); ++it)
    {
        const CellTypeRegion cellTypeRegion = it.key();
        const QString cellTypeName = mNetwork.cellTypes.getName(cellTypeRegion.first);
        const QString regionName = mNetwork.regions.getName(cellTypeRegion.second);
        const QDir innervationDir = CIS3D::getInnervationDataDir(mNetwork.dataRoot);
        const QString innervationFile = CIS3D::getInnervationPostFileName(innervationDir, regionName, cellTypeName);

        const IdList& ids = it.value();
        SparseVectorSet* vectorSet;
        bool fromCache;
        if (mCache.contains(innervationFile))
        {
            vectorSet = mCache.get(innervationFile);
            fromCache = true;
        }
        else
        {
            vectorSet = SparseVectorSet::load(innervationFile);
            fromCache = false;
            qDebug() << "[*] Loading" << innervationFile;
        }

        const IdList preIdListA = selection.MotifA();
        const IdList preIdListB = selection.MotifB();

        for (int post = 0; post < ids.size(); ++post)
        {
            if (mAborted)
            {
                return;
            }

            const int postId = ids[post];

            Statistics perPostAC;
            Statistics perPostBC;

            for (int pre = 0; pre < preIdListA.size(); ++pre)
            {
                const int preId = preIdListA[pre];
                const int mappedPreId = mNetwork.axonRedundancyMap.getNeuronIdToUse(preId);
                const float innervation = vectorSet->getValue(postId, mappedPreId);
                perPostAC.addSample(double(innervation));
            }
            for (int pre = 0; pre < preIdListB.size(); ++pre)
            {
                const int preId = preIdListB[pre];
                const int mappedPreId = mNetwork.axonRedundancyMap.getNeuronIdToUse(preId);
                const float innervation = vectorSet->getValue(postId, mappedPreId);
                perPostBC.addSample(double(innervation));
            }

            mStatisticsAC.addSample(perPostAC.getSum());
            mValuesAC.push_back(perPostAC.getSum());
            mStatisticsBC.addSample(perPostBC.getSum());
            mValuesBC.push_back(perPostBC.getSum());

            mConnectionsDone++;
            calculateCorrelation();
            if (mConnectionsDone % 20 == 0)
            {
                reportUpdate();
            }
        }

        if (!fromCache)
        {
            delete vectorSet;
        }
    }
    reportComplete();
}

void
InDegreeStatistic::calculateCorrelation()
{
    if (mValuesAC.size() < 2)
    {
        mCorrelation = 0;
        return;
    }

    double stdAC = mStatisticsAC.getStandardDeviation();
    double stdBC = mStatisticsBC.getStandardDeviation();
    double eps = 0.0001;
    if (std::abs(stdAC) < eps || std::abs(stdBC) < eps)
    {
        mCorrelation = 0;
        return;
    }

    double sum = 0;
    double meanAC = calculateMean(mValuesAC);
    double meanBC = calculateMean(mValuesBC);
    for (unsigned long i = 0; i < mValuesAC.size(); i++)
    {
        sum += (mValuesAC[i] - meanAC) * (mValuesBC[i] - meanBC);
    }
    sum /= mValuesAC.size();
    mCorrelation = sum / (stdAC * stdBC);
}

double
InDegreeStatistic::calculateMean(std::vector<double>& values)
{
    double sum = 0;
    for (auto it = values.begin(); it != values.end(); ++it)
    {
        sum += *it;
    }
    return sum / values.size();
}

void
InDegreeStatistic::doCreateJson(QJsonObject& obj) const
{
    obj["sampleSize"] = mSampleSize;
    obj.insert("innervationStatisticsAC", Util::createJsonStatistic(mStatisticsAC));
    obj.insert("innervationStatisticsBC", Util::createJsonStatistic(mStatisticsBC));
    obj.insert("innervationValuesAC", Util::createJsonArray(mValuesAC));
    obj.insert("innervationValuesBC", Util::createJsonArray(mValuesBC));
    obj.insert("correlation", mCorrelation);
}

void
InDegreeStatistic::doCreateCSV(QTextStream& out, const QChar sep) const
{
    out << "Number of samples (from selection C):" << sep << mSampleSize
        << "\n\n";

    out << "Innervation A->C" << sep << mStatisticsAC.getMean() << sep << "StDev" << sep
        << mStatisticsAC.getStandardDeviation() << sep << "Min" << sep
        << mStatisticsAC.getMinimum() << sep << "Max" << sep
        << mStatisticsAC.getMaximum() << "\n";
    out << "Innervation B->C" << sep << mStatisticsBC.getMean() << sep << "StDev" << sep
        << mStatisticsBC.getStandardDeviation() << sep << "Min" << sep
        << mStatisticsBC.getMinimum() << sep << "Max" << sep
        << mStatisticsBC.getMaximum() << "\n";

    out << "Correlation" << sep << mCorrelation;
}
