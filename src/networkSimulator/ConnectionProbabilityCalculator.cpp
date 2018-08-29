#include "ConnectionProbabilityCalculator.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DSparseField.h"
#include "CIS3DSparseVectorSet.h"
#include "Distribution.h"
#include "SparseFieldCalculator.h"
#include "UtilIO.h"
#include "Typedefs.h"
#include <QFile>
#include <QIODevice>
#include <QTextStream>
#include <iomanip>
#include <mutex>
#include <omp.h>

/*
    Constructor.
    @param featureProvider The features of the neuron selections.
*/
ConnectionProbabilityCalculator::ConnectionProbabilityCalculator(
    FeatureProvider& featureProvider)
    : mFeatureProvider(featureProvider)
{
    mNumPre = mFeatureProvider.getNumPre();
    mNumPost = mFeatureProvider.getNumPost();
}

/*
    Calculates the connection probability for the specified
    rule parameters.
    @param The connectivity rule parameters.
    @return The connection probability.
*/
double
ConnectionProbabilityCalculator::calculate(QVector<float> parameters)
{
    // qDebug() << "[*] Start simulation.";

    SparseField* postAll = mFeatureProvider.getPostAllExc();
    Statistics innervationHistogram;
    SparseFieldCalculator fieldCalculator;
    std::mutex mutex;
    std::mutex mutex2;
    for (int i = 0; i < mNumPre; i++)
    {
        //#pragma omp parallel for schedule(dynamic)
        for (int j = 0; j < mNumPost; j++)
        {
            mutex.lock();
            SparseField* pre = mFeatureProvider.getPre(i);
            SparseField* post = mFeatureProvider.getPostExc(j);
            int multiplicity = mFeatureProvider.getPreMultiplicity(i);
            mutex.unlock();
            float innervation = fieldCalculator.calculatePetersRule(
                *pre, *post, *postAll, parameters[0], parameters[1], parameters[2], parameters[3]);
            mutex2.lock();
            for (int k = 0; k < multiplicity; k++)
            {
                innervationHistogram.addSample(innervation);
            }
            mutex2.unlock();
        }
    }

    // qDebug() << "[*] Finish simulation.";

    return calculateProbability(innervationHistogram.getMean());
}

double
ConnectionProbabilityCalculator::calculateSynapse(QVector<float> parameters,
                                                  bool matrix)
{
    float eps = 0.000001;
    float b0 = parameters[0];
    float b1 = parameters[1];
    float b2 = parameters[2];
    float b3 = parameters[3];

    std::map<int, float> postAllField =
        mFeatureProvider.getPostAllExc()->getModifiedCopy(b3, eps);
    std::vector<std::map<int, float> > preFields;
    for (int i = 0; i < mNumPre; i++)
    {
        preFields.push_back(mFeatureProvider.getPre(i)->getModifiedCopy(b1, eps));
    }

    std::vector<std::map<int, float> > postFields;
    for (int i = 0; i < mNumPost; i++)
    {
        postFields.push_back(
            mFeatureProvider.getPostExc(i)->getModifiedCopy(b2, eps));
    }

    std::vector<int> empty(mNumPost, 0);
    std::vector<std::vector<int> > synapses(mNumPre, empty);

#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < mNumPre; i++)
    {
        for (int j = 0; j < mNumPost; j++)
        {
            // qDebug() << i << j;
            Distribution dist;
            for (auto itPre = preFields[i].begin(); itPre != preFields[i].end();
                 ++itPre)
            {
                auto itPost = postFields[j].find(itPre->first);
                auto itPostAll = postAllField.find(itPre->first);
                if (itPost != postFields[j].end() && itPostAll != postAllField.end())
                {
                    float innervation =
                        exp(b0 + itPre->second + itPost->second + itPostAll->second);
                    // qDebug() << innervation;
                    synapses[i][j] = synapses[i][j] + dist.drawSynapseCount(innervation);
                    // qDebug() << synapses[i][j];
                }
            }
        }
    }

    Statistics connectionProbabilities;
    for (int i = 0; i < mNumPre; i++)
    {
        int realizedConnections = 0;
        for (int j = 0; j < mNumPost; j++)
        {
            realizedConnections += synapses[i][j] > 0 ? 1 : 0;
        }
        double probability = (double)realizedConnections / (double)mNumPost;
        connectionProbabilities.addSample(probability);
    }

    if (matrix)
    {
        QString filename;
        filename.sprintf("synapses_%+06.3f_%+06.3f_%+06.3f_%+06.3f", parameters[0], parameters[1], parameters[2], parameters[3]);
        filename = QDir("matrices").filePath(filename);
        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly))
        {
            const QString msg =
                QString("Cannot open file %1 for writing.").arg(filename);
            throw std::runtime_error(qPrintable(msg));
        }
        QTextStream out(&file);
        for (int i = 0; i < mNumPre; i++)
        {
            for (int j = 0; j < mNumPost; j++)
            {
                out << synapses[i][j];
                if (j + 1 < mNumPost)
                {
                    out << " ";
                }
            }
            if (i + 1 < mNumPre)
            {
                out << "\n";
            }
        }
    }

    return connectionProbabilities.getMean();
}

double
ConnectionProbabilityCalculator::distributeSynapses(QVector<float> parameters)
{
    float b0 = parameters[0];
    float b1 = parameters[1];
    float b2 = parameters[2];
    float b3 = parameters[3];

    qDebug() << QString("Start distributing synapses [%1,%2,%3,%4].").arg(b0).arg(b1).arg(b2).arg(b3);

    std::map<int, std::map<int, float> > neuron_pre;
    std::map<int, std::map<int, float> > neuron_postExc;
    std::map<int, std::map<int, float> > neuron_postInh;
    std::map<int, float> voxel_postAllExc;
    std::map<int, float> voxel_postAllInh;
    std::map<int, int> neuron_funct;
    std::map<int, std::set<int> > voxel_neuronsPre;
    std::map<int, std::set<int> > voxel_neuronsPostExc;
    std::map<int, std::set<int> > voxel_neuronsPostInh;
    std::vector<int> voxel;

    mFeatureProvider.load(neuron_pre, b1,
                          neuron_postExc, b2,
                          neuron_postInh,
                          voxel_postAllExc, b3,
                          voxel_postAllInh,
                          neuron_funct,
                          voxel_neuronsPre,
                          voxel_neuronsPostExc,
                          voxel_neuronsPostInh);

    std::vector<int> preIndices;
    //preIndices.push_back(284568);

    for(auto it = neuron_pre.begin(); it != neuron_pre.end(); ++it){
        preIndices.push_back(it->first);
    }


    std::vector<int> postIndices;
    //postIndices.push_back(341887);

    for(auto it = neuron_postExc.begin(); it != neuron_postExc.end(); ++it){
        postIndices.push_back(it->first);
    }

    std::vector<int> empty(postIndices.size(), 0);
    std::vector<std::vector<int> > contacts(preIndices.size(), empty);

    Distribution poisson;
    #pragma omp parallel for schedule(dynamic)
    for(int i=0; i<preIndices.size(); i++){
            int preId = preIndices[i];
            for(int j=0; j<postIndices.size(); j++){
                    int postId = postIndices[j];
                    //qDebug() << i << j << preId << postId;
                    if(preId != postId){
                        for(auto pre = neuron_pre[preId].begin(); pre != neuron_pre[preId].end(); ++pre){
                            if(neuron_postExc[postId].find(pre->first) != neuron_postExc[postId].end()){

                                float preVal = pre->second;
                                float postVal = neuron_postExc[postId][pre->first];
                                float postAllVal = voxel_postAllExc[pre->first];
                                float arg = b0 + preVal + postVal + postAllVal;
                                //qDebug() << k++ << arg << preVal << postVal << postAllVal;
                                int synapses = 0;
                                if(arg >= -7 && arg <= 7){
                                    float mu = exp(arg);
                                    synapses = poisson.drawSynapseCount(mu);
                                } else if (arg > 7){
                                    synapses = 1000;
                                }
                                if(synapses > 0){
                                    contacts[i][j] = synapses;
                                    break;
                                }
                            }
                        }

                    }
                }
            }
    Statistics connectionProbabilities;
    for (int i = 0; i < preIndices.size(); i++)
    {
        int realizedConnections = 0;
        for (int j = 0; j < postIndices.size(); j++)
        {
            realizedConnections += contacts[i][j] > 0 ? 1 : 0;
        }
        double probability = (double)realizedConnections / (double)postIndices.size();
        connectionProbabilities.addSample(probability);
    }

    double connProb = connectionProbabilities.getMean();
    qDebug() << QString("Distributed synapses [%1,%2,%3,%4]. Connection prob.: %5").arg(b0).arg(b1).arg(b2).arg(b3).arg(connProb);
    return connProb;
}



double
ConnectionProbabilityCalculator::calculateProbability(double innervationMean)
{
    return 1 - exp(-1 * innervationMean);
}
