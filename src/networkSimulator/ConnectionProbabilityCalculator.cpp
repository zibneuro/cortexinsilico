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

void
ConnectionProbabilityCalculator::calculate(QVector<float> parameters, bool addIntercept)
{
    float b0, b1, b2, b3;

    if (addIntercept)
    {
        b0 = parameters[0];
        b1 = parameters[1];
        b2 = parameters[2];
        b3 = parameters[3];
    }
    else
    {
        b0 = 0;
        b1 = parameters[0];
        b2 = parameters[1];
        b3 = parameters[2];
    }

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

    mFeatureProvider.load(neuron_pre, 1, neuron_postExc, 1, neuron_postInh, voxel_postAllExc, 1, voxel_postAllInh, neuron_funct, voxel_neuronsPre, voxel_neuronsPostExc, voxel_neuronsPostInh);

    std::vector<int> preIndices;
    for (auto it = neuron_pre.begin(); it != neuron_pre.end(); ++it)
    {
        preIndices.push_back(it->first);
    }

    std::vector<int> postIndices;
    for (auto it = neuron_postExc.begin(); it != neuron_postExc.end(); ++it)
    {
        postIndices.push_back(it->first);
    }

    std::vector<int> empty(postIndices.size(), 0);
    std::vector<float> emptyFloat(postIndices.size(), 0);
    std::vector<std::vector<int> > contacts(preIndices.size(), empty);
    std::vector<std::vector<float> > innervation(preIndices.size(), emptyFloat);

    std::vector<double> sufficientStat;
    sufficientStat.push_back(0);
    sufficientStat.push_back(0);
    sufficientStat.push_back(0);
    sufficientStat.push_back(0);

    Distribution poisson;
#pragma omp parallel for schedule(dynamic)
    for (unsigned int i = 0; i < preIndices.size(); i++)
    {
        int preId = preIndices[i];
        for (unsigned int j = 0; j < postIndices.size(); j++)
        {
            int postId = postIndices[j];
            //qDebug() << i << j << preId << postId;
            if (preId != postId)
            {
                for (auto pre = neuron_pre[preId].begin(); pre != neuron_pre[preId].end(); ++pre)
                {
                    if (neuron_postExc[postId].find(pre->first) != neuron_postExc[postId].end())
                    {
                        float preVal = pre->second;
                        int maxSynapses = 1000;
                        float postVal = neuron_postExc[postId][pre->first];
                        float postAllVal = voxel_postAllExc[pre->first];
                        float arg = b0 + b1 * preVal + b2 * postVal + b3 * postAllVal;
                        //qDebug() << k++ << arg << preVal << postVal << postAllVal;
                        int synapses = 0;
                        if (arg >= -7 && arg <= 7)
                        {
                            float mu = exp(arg);
                            innervation[i][j] += mu;
                            synapses = std::min(maxSynapses, poisson.drawSynapseCount(mu));
                        }
                        else if (arg > 7)
                        {
                            innervation[i][j] += exp(7);
                            synapses = maxSynapses;
                        }
                        if (synapses > 0)
                        {
                            sufficientStat[0] += synapses;
                            sufficientStat[1] += preVal * synapses;
                            sufficientStat[2] += postVal * synapses;
                            sufficientStat[3] += postAllVal * synapses;
                            contacts[i][j] += synapses;
                        }
                    }
                }
            }
        }
    }

    Statistics connectionProbabilities;
    for (unsigned int i = 0; i < preIndices.size(); i++)
    {
        int realizedConnections = 0;
        for (unsigned int j = 0; j < postIndices.size(); j++)
        {
            realizedConnections += contacts[i][j] > 0 ? 1 : 0;
        }
        double probability = (double)realizedConnections / (double)postIndices.size();
        connectionProbabilities.addSample(probability);
    }
    double connProb = connectionProbabilities.getMean();

    writeSynapseMatrix(contacts);
    writeInnervationMatrix(innervation);

    if (!addIntercept)
    {
        sufficientStat.erase(sufficientStat.begin());
    }

    writeStatistics(connProb, sufficientStat);

    qDebug() << QString("Distributed synapses [%1,%2,%3,%4]. Connection prob.: %5").arg(b0).arg(b1).arg(b2).arg(b3).arg(connProb);
}

double
ConnectionProbabilityCalculator::calculateProbability(double innervationMean)
{
    return 1 - exp(-1 * innervationMean);
}

void
ConnectionProbabilityCalculator::writeSynapseMatrix(std::vector<std::vector<int> >& contacts)
{
    QString filename = "output_synapseMatrix";
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
    {
        const QString msg =
            QString("Cannot open file %1 for writing.").arg(filename);
        throw std::runtime_error(qPrintable(msg));
    }
    QTextStream out(&file);
    int numPre = contacts.size();
    int numPost = contacts[0].size();
    for (int i = 0; i < numPre; i++)
    {
        for (int j = 0; j < numPost; j++)
        {
            out << contacts[i][j];
            if (j + 1 < numPost)
            {
                out << " ";
            }
        }
        if (i + 1 < numPre)
        {
            out << "\n";
        }
    }
}

void
ConnectionProbabilityCalculator::writeInnervationMatrix(std::vector<std::vector<float> >& innervation)
{
    QString filename = "output_innervationMatrix";
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
    {
        const QString msg =
            QString("Cannot open file %1 for writing.").arg(filename);
        throw std::runtime_error(qPrintable(msg));
    }
    QTextStream out(&file);
    int numPre = innervation.size();
    int numPost = innervation[0].size();
    for (int i = 0; i < numPre; i++)
    {
        for (int j = 0; j < numPost; j++)
        {
            out << innervation[i][j];
            if (j + 1 < numPost)
            {
                out << " ";
            }
        }
        if (i + 1 < numPre)
        {
            out << "\n";
        }
    }
}

void
ConnectionProbabilityCalculator::writeStatistics(double connectionProbability, std::vector<double> sufficientStat)
{
    QString fileName = "output.json";
    QFile json(fileName);
    if (!json.open(QIODevice::WriteOnly))
    {
        const QString msg =
            QString("Cannot open file %1 for writing.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    QTextStream out(&json);
    out << "{\"CONNECTION_PROBABILITY\":" << connectionProbability << ",";
    out << "\"SUFFICIENT_STATISTIC\":[";
    out << sufficientStat[0];
    out << "," << sufficientStat[1];
    out << "," << sufficientStat[2];
    if (sufficientStat.size() == 4)
    {
        out << "," << sufficientStat[3];
    }
    out << "]}";
}