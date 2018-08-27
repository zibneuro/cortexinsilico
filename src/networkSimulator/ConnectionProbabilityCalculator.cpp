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

void
ConnectionProbabilityCalculator::distributeSynapses(QVector<float> parameters)
{

    float b1 = parameters[0];
    float b2 = parameters[1];
    float b3 = parameters[2];
    float b4 = parameters[3];

    qDebug() << QString("Distributing synapses [%1,%2,%3,%4]").arg(b1).arg(b2).arg(b3).arg(b4);

    UtilIO::makeDir("output_synapses");
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

    mFeatureProvider.load(neuron_pre,
                          neuron_postExc,
                          neuron_postInh,
                          voxel_postAllExc,
                          voxel_postAllInh,
                          neuron_funct,
                          voxel_neuronsPre,
                          voxel_neuronsPostExc,
                          voxel_neuronsPostInh);


    for(auto it = voxel_neuronsPre.begin(); it!=voxel_neuronsPre.end(); ++it){
        voxel.push_back(it->first);
    }

    Distribution poisson;
    #pragma omp parallel for schedule(dynamic)
    for(int i=0; i<voxel.size(); i++){
        int voxelId = voxel[i];
        //qDebug() << "Processing voxel (ID):" << voxelId;
        std::vector<Contact> contacts;
        float postAllVal = voxel_postAllExc[voxelId];
        for(auto pre = voxel_neuronsPre[voxelId].begin(); pre != voxel_neuronsPre[voxelId].end(); ++pre){
            for(auto post = voxel_neuronsPostExc[voxelId].begin(); post != voxel_neuronsPostExc[voxelId].end(); ++post){
                if(*pre != *post){
                    float preVal = neuron_pre[*pre][voxelId];
                    float postVal = neuron_postExc[*post][voxelId];
                    float mu = exp(b1 + b2 * preVal + b3 * postVal + b4 * postAllVal);
                    int synapses = poisson.drawSynapseCount(mu);

                    Contact c;
                    c.pre = *pre;
                    c.post = *post;
                    c.preVal = preVal;
                    c.postVal = postVal;
                    c.postAllVal = postAllVal;
                    c.mu = mu;
                    c.count = synapses;
                    contacts.push_back(c);

                }
            }
        }
        QString filename = QString("%1.dat").arg(voxelId);
        filename = QDir("output_synapses").filePath(filename);
        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly))
        {
            const QString msg =
                QString("Cannot open file %1 for writing.").arg(filename);
            throw std::runtime_error(qPrintable(msg));
        }
        QTextStream out(&file);
        out << "presynapticNeuronID" << " " << "postsynapticNeuronID" << " "
            << "boutons" << " "<< "PSTs" << " " << "PSTs_all" << " " << "synapses" << "\n";
        for (unsigned int i = 0; i < contacts.size(); i++)
        {
            out << contacts[i].pre << " " <<
                   contacts[i].post << " " <<
                   contacts[i].preVal << " " <<
                   contacts[i].postVal << " " <<
                   contacts[i].postAllVal << " " <<
                   /*contacts[i].mu << " " <<*/
                   contacts[i].count << "\n";
        }
    }

    qDebug() << "[*] Finished distributing synapses.";

}

double
ConnectionProbabilityCalculator::calculateProbability(double innervationMean)
{
    return 1 - exp(-1 * innervationMean);
}
