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
#include "FileHelper.h"
#include "CIS3DStatistics.h"

/**
    Constructor.
    @param networkProps The model data of the network.
    @param innervation BinSize Bin size of the innervation histogram.
    @param conProbBinSize Bin size of the connectionProbability histogram.
*/
InnervationStatistic::InnervationStatistic(const NetworkProps& networkProps,
                                           FormulaCalculator& calculator,
                                           QueryHandler* handler)
    : NetworkStatistic(networkProps, calculator, handler)
{
    innervationHisto = Histogram(Histogram::getBinSize(10));
    connProbHisto = Histogram(Histogram::getBinSize(1));
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

    for (auto itPre = preIds.begin(); itPre != preIds.end(); itPre++) {
        qDebug() << "PRE ID" << itPre->first << itPre->second;
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
InnervationStatistic::doCreateCSV(FileHelper& fileHelper) const
{
    fileHelper.openFile("statistics.csv");
    fileHelper.write(Statistics::getHeaderCsv());
    fileHelper.write(Statistics::getLineSingleValue("presynaptic neurons",numPreNeurons));
    fileHelper.write(Statistics::getLineSingleValue("postsynaptic neurons",numPostNeurons));    
    fileHelper.write(Statistics::getLineSingleValue("neuron pairs",innervationHisto.getNumberOfValues()));
    fileHelper.write(Statistics::getLineSingleValue("neuron pairs without overlap",innervationHisto.getNumberOfZeros()));   
    fileHelper.write(innervation.getLineCsv("dense structural composition (DSC)"));
    fileHelper.write(connProb.getLineCsv("connection probability"));
    fileHelper.write(innervationPerPre.getLineCsv("DSC per presynaptic neuron"));
    fileHelper.write(innervationPerPost.getLineCsv("DSC per postsynaptic neuron"));
    fileHelper.closeFile();

    innervationHisto.writeFile(fileHelper,"histogram_dense_structural_composition.csv");
    connProbHisto.writeFile(fileHelper,"histogram_connection_probability.csv");
}

/**
    Adds the result values to a JSON object
    @param obj: JSON object to which the values are appended
*/
void
InnervationStatistic::doCreateJson(QJsonObject& obj) const
{    
    obj.insert("innervationHisto", innervationHisto.createJson());  
    obj.insert("connectionProbabilityHisto", connProbHisto.createJson());

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
