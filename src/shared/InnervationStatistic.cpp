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
                                           FormulaCalculator& calculator,
                                           QueryHandler* handler,
                                           const float innervationBinSize,
                                           const float connProbBinSize)
    : NetworkStatistic(networkProps, calculator, handler)
{
    innervationHisto = Histogram(innervationBinSize);
    connProbHisto = Histogram(connProbBinSize);
    numPreNeurons = 0;
    numPostNeurons = 0;
    numPreNeuronsUnique = 0;
}


/**
    Performs the actual computation based on the specified neurons.
    @param selection The selected neurons.
*/
void
InnervationStatistic::doCalculate(const NeuronSelection& selection)
{
    std::map<int, int> preIds; // unique ID, multiplicity
    std::map<int, float> postInnervation;

    CIS3D::Structure postTarget = selection.getPostTarget(1);

    this->numPostNeurons = selection.SelectionB().size();
    this->numPreNeurons = selection.SelectionA().size();
    this->mNumConnections = (long long)(this->numPreNeurons) * (long long)(this->numPostNeurons);

    for (int i = 0; i < selection.SelectionA().size(); ++i)
    {
        const int preId = selection.SelectionA()[i];
        const int mappedPreId = mNetwork.axonRedundancyMap.getNeuronIdToUse(preId);
        if (preIds.find(mappedPreId) == preIds.end())
        {
            preIds[mappedPreId] = 1;
        }
        else
        {
            preIds[mappedPreId] += 1;
        }
    }

    for (int i = 0; i < selection.SelectionB().size(); ++i)
    {
        int postId = selection.SelectionB()[i];
        postInnervation[postId] = 0;
    }

    for (auto itPre = preIds.begin(); itPre != preIds.end(); itPre++)
    {
        if (mAborted)
        {
            return;
        }

        float currentPreInnervation = 0;

        for (int j = 0; j < selection.SelectionB().size(); ++j)
        {
            const int postId = selection.SelectionB()[j];

            if (selection.getBandA(itPre->first) != selection.getBandB(postId))
            {
                continue;
            }

            const float innervation = mInnervationMatrix->getValue(itPre->first, postId, postTarget);
            currentPreInnervation += innervation;
            float connProb = mCalculator.calculateConnectionProbability(innervation);

            for (int k = 0; k < itPre->second; k++)
            {
                this->innervationHisto.addValue(innervation);
                this->connProbHisto.addValue(connProb);
                this->innervation.addSample(innervation);
                this->connProb.addSample(connProb);
                postInnervation[postId] += innervation;
            }
        }

        for (int k = 0; k < itPre->second; k++)
        {
            this->innervationPerPre.addSample(currentPreInnervation);
        }

        this->innervationPerPost = Statistics();
        for (auto it = postInnervation.begin(); it != postInnervation.end(); it++)
        {
            this->innervationPerPost.addSample(it->second);
        }

        mConnectionsDone += (long long)(selection.SelectionB().size()) * (long long)(itPre->second);
        reportUpdate();
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
    //out << "Number of unique presynaptic neurons:" << sep << numPreNeuronsUnique << "\n";
    out << "Number of postsynaptic neurons:" << sep << numPostNeurons << "\n";
    out << "Number of neuron pairs:" << sep << innervationHisto.getNumberOfValues() << "\n";
    out << "Number of non-overlapping neuron pairs:" << sep << innervationHisto.getNumberOfZeros() << "\n";
    out << "\n";

    out << "Dense structural overlap" << sep
        << "Average" << sep << innervation.getMean() << sep
        << "StDev" << sep << innervation.getStandardDeviation() << sep
        << "Min" << sep << innervation.getMinimum() << sep
        << "Max" << sep << innervation.getMaximum() << "\n";

    out << "Connection probability" << sep
        << "Average" << sep << connProb.getMean() << sep
        << "StDev" << sep << connProb.getStandardDeviation() << sep
        << "Min" << sep << connProb.getMinimum() << sep
        << "Max" << sep << connProb.getMaximum() << "\n";

    /*
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
    */
    out << "Dense structural overlap per presynaptic neuron" << sep
        << "Average" << sep << innervationPerPre.getMean() << sep
        << "StDev" << sep << innervationPerPre.getStandardDeviation() << sep
        << "Min" << sep << innervationPerPre.getMinimum() << sep
        << "Max" << sep << innervationPerPre.getMaximum() << "\n";
    /*
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
    */
    out << "Dense structural overlap per postsynaptic neuron" << sep
        << "Average" << sep << innervationPerPost.getMean() << sep
        << "StDev" << sep << innervationPerPost.getStandardDeviation() << sep
        << "Min" << sep << innervationPerPost.getMinimum() << sep
        << "Max" << sep << innervationPerPost.getMaximum() << "\n";

    /*
    out << "Convergence" << sep
        << "Average" << sep << convergence.getMean() << sep
        << "StDev" << sep << convergence.getStandardDeviation() << sep
        << "Min" << sep << convergence.getMinimum() << sep
        << "Max" << sep << convergence.getMaximum() << "\n";
    */
    out << "\n";

    out << "Dense structural overlap histogram\n";
    out << "Number of non-zero values:" << sep << innervationHisto.getNumberOfValues() << "\n";
    out << "Number of zero values:" << sep << innervationHisto.getNumberOfZeros() << "\n";
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
    out << "Number of non-zero values:" << sep << connProbHisto.getNumberOfValues() << "\n";
    out << "Number of zero values:" << sep << connProbHisto.getNumberOfZeros() << "\n";
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
