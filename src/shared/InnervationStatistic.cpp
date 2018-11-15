#include "InnervationStatistic.h"
#include "Util.h"
#include "Typedefs.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DSparseVectorSet.h"
#include <QTextStream>
#include <QDebug>
#include <QTime>
#include <math.h>
#include <iostream>

/**
    Constructor.
    @param networkProps The model data of the network.
    @param innervation BinSize Bin size of the innervation histogram.
    @param conProbBinSize Bin size of the connectionProbability histogram.
*/
InnervationStatistic::InnervationStatistic(const NetworkProps& networkProps,
                                           const float innervationBinSize,
                                           const float connProbBinSize)
    : NetworkStatistic(networkProps)
{
    innervationHisto = Histogram(innervationBinSize);
    connProbHisto = Histogram(connProbBinSize);
    numPreNeurons = 0;
    numPostNeurons = 0;
    numPreNeuronsUnique = 0;
    mExpression = "1-exp(-x)";
}

/**
    Constructor.
    @param networkProps The model data of the network.
    @param innervation BinSize Bin size of the innervation histogram.
    @param conProbBinSize Bin size of the connectionProbability histogram.
    @param cache Cache of preloaded innervation values.
*/
InnervationStatistic::InnervationStatistic(const NetworkProps& networkProps,
                                           const SparseVectorCache& cache,
                                           const float innervationBinSize,
                                           const float connProbBinSize)
    : NetworkStatistic(networkProps, cache)
{
    innervationHisto = Histogram(innervationBinSize);
    connProbHisto = Histogram(connProbBinSize);
    numPreNeurons = 0;
    numPostNeurons = 0;
    numPreNeuronsUnique = 0;
    mExpression = "1-exp(-x)";
}

void
InnervationStatistic::setExpression(QString expression)
{
    mExpression = expression.toStdString();
}

/**
    Performs the actual computation based on the specified neurons.
    @param selection The selected neurons.
*/
void
InnervationStatistic::doCalculate(const NeuronSelection& selection)
{
    QHash<int, float> preNeuronInnervationSum;
    QHash<int, float> preNeuronConnProbSum;
    QHash<int, float> preNeuronInnervationSumUnique;
    QHash<int, float> preNeuronConnProbSumUnique;

    float x;
    symbol_table_t symbol_table;
    symbol_table.add_variable("x", x);
    symbol_table.add_constants();
    expression_t expression;
    expression.register_symbol_table(symbol_table);
    parser_t parser;
    parser.compile(mExpression, expression);
    std::cout << "[*] Connection probability formula " << mExpression << std::endl;

    this->numPreNeurons = selection.Presynaptic().size();
    this->numPreNeuronsUnique = 0;
    for (int pre = 0; pre < selection.Presynaptic().size(); ++pre)
    {
        const int preId = selection.Presynaptic()[pre];
        const int mappedPreId = mNetwork.axonRedundancyMap.getNeuronIdToUse(preId);
        preNeuronInnervationSum.insert(preId, 0.0f);
        preNeuronConnProbSum.insert(preId, 0.0f);

        if (preId == mappedPreId)
        {
            this->numPreNeuronsUnique += 1;
            preNeuronInnervationSumUnique.insert(mappedPreId, 0.0f);
            preNeuronConnProbSumUnique.insert(mappedPreId, 0.0f);
        }
    }

    this->mNumConnections = (long long)(this->numPreNeurons) * (long long)(selection.Postsynaptic().size());

    const IdsPerCellTypeRegion idsPerCellTypeRegion = Util::sortByCellTypeRegionIDs(selection.Postsynaptic(), mNetwork);

    for (IdsPerCellTypeRegion::ConstIterator it = idsPerCellTypeRegion.constBegin(); it != idsPerCellTypeRegion.constEnd(); ++it)
    {
        if (mAborted)
        {
            return;
        }

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
            const QTime loadStart = QTime::currentTime();
            vectorSet = SparseVectorSet::load(innervationFile);
            const QTime loadEnd = QTime::currentTime();
            fromCache = false;
            qDebug() << "[*] Loading" << innervationFile;
            qDebug() << "Time load:      " << loadStart.secsTo(loadEnd) << "sec.";
        }

        const IdList preIdList = selection.Presynaptic();

        const QTime processStart = QTime::currentTime();
        for (int post = 0; post < ids.size(); ++post)
        {
            const int postId = ids[post];

            float postNeuronInnervationSum = 0.0f;
            float postNeuronConnProbSum = 0.0f;
            float postNeuronInnervationSumUnique = 0.0f;
            float postNeuronConnProbSumUnique = 0.0f;

            for (int pre = 0; pre < preIdList.size(); ++pre)
            {
                const int preId = preIdList[pre];

                if(selection.getPostsynapticBand(preId) != selection.getPostsynapticBand(preId)){
                    continue;
                }

                const int mappedPreId = mNetwork.axonRedundancyMap.getNeuronIdToUse(preId);
                const float innervation = vectorSet->getValue(postId, mappedPreId);
                x = innervation;
                //const float connProb = float(1.0 - exp(-1.0 * innervation));
                float connProb = expression.value();
                if(connProb != connProb){ // check nan
                    connProb = 0;
                }
                connProb = connProb < 0 ? 0 : connProb;
                connProb = connProb > 1 ? 1 : connProb;                
                this->innervationHisto.addValue(innervation);
                this->connProbHisto.addValue(connProb);

                this->innervation.addSample(innervation);
                this->connProb.addSample(connProb);

                preNeuronInnervationSum[preId] += innervation;
                preNeuronConnProbSum[preId] += connProb;
                postNeuronInnervationSum += innervation;
                postNeuronConnProbSum += connProb;

                if (preId == mappedPreId)
                {
                    this->innervationUnique.addSample(innervation);
                    this->connProbUnique.addSample(connProb);

                    preNeuronInnervationSumUnique[mappedPreId] += innervation;
                    preNeuronConnProbSumUnique[mappedPreId] += connProb;
                    postNeuronInnervationSumUnique += innervation;
                    postNeuronConnProbSumUnique += connProb;
                }
            }
            this->numPostNeurons += 1;

            this->innervationPerPost.addSample(postNeuronInnervationSum);
            this->convergence.addSample(postNeuronConnProbSum / this->numPreNeurons);
        }
        const QTime processEnd = QTime::currentTime();

        const QTime statisticsStart = QTime::currentTime();
        this->innervationPerPre = Statistics();
        this->innervationPerPreUnique = Statistics();
        this->divergence = Statistics();
        this->divergenceUnique = Statistics();

        for (QHash<int, float>::ConstIterator it = preNeuronInnervationSum.constBegin(); it != preNeuronInnervationSum.constEnd(); ++it)
        {
            this->innervationPerPre.addSample(it.value());
        }
        for (QHash<int, float>::ConstIterator it = preNeuronInnervationSumUnique.constBegin(); it != preNeuronInnervationSumUnique.constEnd(); ++it)
        {
            this->innervationPerPreUnique.addSample(it.value());
        }
        for (QHash<int, float>::ConstIterator it = preNeuronConnProbSum.constBegin(); it != preNeuronConnProbSum.constEnd(); ++it)
        {
            this->divergence.addSample(it.value() / this->numPostNeurons);
        }
        for (QHash<int, float>::ConstIterator it = preNeuronConnProbSumUnique.constBegin(); it != preNeuronConnProbSumUnique.constEnd(); ++it)
        {
            this->divergenceUnique.addSample(it.value() / this->numPostNeurons);
        }

        mConnectionsDone += (long long)(ids.size()) * (long long)(selection.Presynaptic().size());
        const QTime statisticsEnd = QTime::currentTime();

        const QTime reportStart = QTime::currentTime();
        reportUpdate();
        const QTime reportEnd = QTime::currentTime();

        if (!fromCache)
        {
            delete vectorSet;
        }

        qDebug() << "Time process:   " << processStart.secsTo(processEnd)
                 << "     ("
                 << "Pre:" << preIdList.size() << "Post:" << ids.size() << ")";
        qDebug() << "Time statistics:" << statisticsStart.secsTo(statisticsEnd);
        qDebug() << "Time report:    " << reportStart.secsTo(reportEnd);
    }
    reportComplete();
}

/**
    Writes the result values to file stream (CSV).
    @param out The file stream to which the values are written.
    @param sep The separator between parameter name and value.
*/
void
InnervationStatistic::doCreateCSV(QTextStream& out, const QChar sep) const
{
    out << "Number of presynaptic neurons:" << sep << numPreNeurons << "\n";
    out << "Number of unique presynaptic neurons:" << sep << numPreNeuronsUnique << "\n";
    out << "Number of postsynaptic neurons:" << sep << numPostNeurons << "\n";
    out << "\n";

    out << "Innervation" << sep
        << "Average" << sep << innervation.getMean() << sep
        << "StDev" << sep << innervation.getStandardDeviation() << sep
        << "Min" << sep << innervation.getMinimum() << sep
        << "Max" << sep << innervation.getMaximum() << "\n";

    out << "Connection probability" << sep
        << "Average" << sep << connProb.getMean() << sep
        << "StDev" << sep << connProb.getStandardDeviation() << sep
        << "Min" << sep << connProb.getMinimum() << sep
        << "Max" << sep << connProb.getMaximum() << "\n";

    out << "Innervation unique" << sep
        << "Average" << sep << innervationUnique.getMean() << sep
        << "StDev" << sep << innervationUnique.getStandardDeviation() << sep
        << "Min" << sep << innervationUnique.getMinimum() << sep
        << "Max" << sep << innervationUnique.getMaximum() << "\n";

    out << "Connection probability unique" << sep
        << "Average" << sep << connProbUnique.getMean() << sep
        << "StDev" << sep << connProbUnique.getStandardDeviation() << sep
        << "Min" << sep << connProbUnique.getMinimum() << sep
        << "Max" << sep << connProbUnique.getMaximum() << "\n";

    out << "Innervation per presynaptic neuron" << sep
        << "Average" << sep << innervationPerPre.getMean() << sep
        << "StDev" << sep << innervationPerPre.getStandardDeviation() << sep
        << "Min" << sep << innervationPerPre.getMinimum() << sep
        << "Max" << sep << innervationPerPre.getMaximum() << "\n";

    out << "Divergence" << sep
        << "Average" << sep << divergence.getMean() << sep
        << "StDev" << sep << divergence.getStandardDeviation() << sep
        << "Min" << sep << divergence.getMinimum() << sep
        << "Max" << sep << divergence.getMaximum() << "\n";

    out << "Innervation per presynaptic neuron unique" << sep
        << "Average" << sep << innervationPerPreUnique.getMean() << sep
        << "StDev" << sep << innervationPerPreUnique.getStandardDeviation() << sep
        << "Min" << sep << innervationPerPreUnique.getMinimum() << sep
        << "Max" << sep << innervationPerPreUnique.getMaximum() << "\n";

    out << "Divergence unique" << sep
        << "Average" << sep << divergenceUnique.getMean() << sep
        << "StDev" << sep << divergenceUnique.getStandardDeviation() << sep
        << "Min" << sep << divergenceUnique.getMinimum() << sep
        << "Max" << sep << divergenceUnique.getMaximum() << "\n";

    out << "Innervation per postsynaptic neuron" << sep
        << "Average" << sep << innervationPerPost.getMean() << sep
        << "StDev" << sep << innervationPerPost.getStandardDeviation() << sep
        << "Min" << sep << innervationPerPost.getMinimum() << sep
        << "Max" << sep << innervationPerPost.getMaximum() << "\n";

    out << "Convergence" << sep
        << "Average" << sep << convergence.getMean() << sep
        << "StDev" << sep << convergence.getStandardDeviation() << sep
        << "Min" << sep << convergence.getMinimum() << sep
        << "Max" << sep << convergence.getMaximum() << "\n";

    out << "\n";

    out << "Innervation histogram\n";
    out << "Number of values:" << sep << innervationHisto.getNumberOfValues() << "\n";
    out << "Number of zeros:" << sep << innervationHisto.getNumberOfZeros() << "\n";
    out << "\n";
    out << "Bin" << sep << "Bin range min" << sep << "Bin range max" << sep << "Value"
        << "\n";
    for (int b = 0; b < innervationHisto.getNumberOfBins(); ++b)
    {
        out << b << sep
            << innervationHisto.getBinStart(b) << sep
            << innervationHisto.getBinEnd(b) << sep
            << innervationHisto.getBinValue(b) << "\n";
    }
    out << "\n";

    out << "Connection probability histogram\n";
    out << "Number of values:" << sep << connProbHisto.getNumberOfValues() << "\n";
    out << "Number of zeros:" << sep << connProbHisto.getNumberOfZeros() << "\n";
    out << "\n";
    out << "Bin" << sep << "Bin range min" << sep << "Bin range max" << sep << "Value"
        << "\n";
    for (int b = 0; b < connProbHisto.getNumberOfBins(); ++b)
    {
        out << b << sep
            << connProbHisto.getBinStart(b) << sep
            << connProbHisto.getBinEnd(b) << sep
            << connProbHisto.getBinValue(b) << "\n";
    }
    out << "\n";
}

/**
    Adds the result values to a JSON object
    @param obj: JSON object to which the values are appended
*/
void
InnervationStatistic::doCreateJson(QJsonObject& obj) const
{
    QJsonObject innervationHistogram = Util::createJsonHistogram(innervationHisto);
    obj.insert("innervationHisto", innervationHistogram);

    QJsonObject connProbHistogram = Util::createJsonHistogram(connProbHisto);
    obj.insert("connectionProbabilityHisto", connProbHistogram);

    obj.insert("innervation", Util::createJsonStatistic(innervation));
    obj.insert("connectionProbability", Util::createJsonStatistic(connProb));
    obj.insert("innervationUnique", Util::createJsonStatistic(innervationUnique));
    obj.insert("connectionProbabilityUnique", Util::createJsonStatistic(connProbUnique));

    obj.insert("innervationPerPre", Util::createJsonStatistic(innervationPerPre));
    obj.insert("divergence", Util::createJsonStatistic(divergence));
    obj.insert("innervationPerPreUnique", Util::createJsonStatistic(innervationPerPreUnique));
    obj.insert("divergenceUnique", Util::createJsonStatistic(divergenceUnique));

    obj.insert("innervationPerPost", Util::createJsonStatistic(innervationPerPost));
    obj.insert("convergence", Util::createJsonStatistic(convergence));
}
