#include "Calculator.h"
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
#include <ctime>

/*
    Constructor.
    @param featureProvider The features of the neuron selections.
    @param randomGenerator The random generator for synapse counts.
    @param runIndices The postfixes of the output files.
*/
Calculator::Calculator(
    FeatureProvider& featureProvider, RandomGenerator& randomGenerator, std::vector<int> runIndices)
    : mFeatureProvider(featureProvider)
    , mRandomGenerator(randomGenerator)
    , mRunIndices(runIndices)
{
}

void
Calculator::calculateBatch(std::vector<QVector<float> > parametersBatch, double maxInnervation, QString mode, double boutonPSTRatio, bool checkProb, double maxFailedRatio)
{
    //double zeroTime = std::clock();
    bool parallelLoop = !mRandomGenerator.hasUserSeed();
    bool checkBounds = boutonPSTRatio != -1 || checkProb;
    bool checkOrderMagnitude = boutonPSTRatio != -1;

    // ###################### LOAD FEATURES ######################

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

    //double featureLoadTime = std::clock();

    for (unsigned int run = 0; run < mRunIndices.size(); run++)
    {
        int failedBounds = 0;
        int validBounds = 0;
        bool failed = false;
        int runIndex = mRunIndices[run];
        //qDebug() << "BATCH" << runIndex << parametersBatch.size();
        QVector<float> parameters = parametersBatch[run];

        // ###################### SET PARAMETERS ######################

        float maxInnervationLog = log(maxInnervation);
        float b0 = 0;
        float b1 = 0;
        float b2 = 0;
        float b3 = 0;
        QString paramString;

        if (mode == "h0_intercept_pre_pst_pstAll")
        {
            b0 = parameters[0];
            b1 = parameters[1];
            b2 = parameters[2];
            b3 = parameters[3];
            paramString = QString("Simulating h0_intercept_pre_pst_pstAll [%1,%2,%3,%4].").arg(b0).arg(b1).arg(b2).arg(b3);
        }
        else if (mode == "h0_pre_pst_pstAll")
        {
            b0 = 0;
            b1 = parameters[0];
            b2 = parameters[1];
            b3 = parameters[2];
            paramString = QString("Simulating h0_pre_pst_pstAll [%1,%2,%3].").arg(b1).arg(b2).arg(b3);
        }
        else if (mode == "h0_intercept_pre_pstNorm")
        {
            b0 = parameters[0];
            b1 = parameters[1];
            b2 = parameters[2];
            paramString = QString("Simulating h0_intercept_pre_pstNorm [%1,%2,%3].").arg(b0).arg(b1).arg(b2);
        }
        else if (mode == "h0_pre_pstNorm")
        {
            b0 = 0;
            b1 = parameters[0];
            b2 = parameters[1];
            paramString = QString("Simulating h0_intercept_pre_pstNorm [%1,%2].").arg(b1).arg(b2);
        }
        else
        {
            const QString msg =
                QString("Invalid simulation mode: %1").arg(mode);
            throw std::runtime_error(qPrintable(msg));
        }

        // ###################### INIT OUTPUT FIELDS ######################

        std::vector<int> empty(postIndices.size(), 0);
        std::vector<float> emptyFloat(postIndices.size(), 0);
        std::vector<std::vector<int> > contacts(preIndices.size(), empty);
        std::vector<std::vector<float> > innervation(preIndices.size(), emptyFloat);

        std::vector<double> sufficientStat;
        for (int i = 0; i < parameters.size(); i++)
        {
            sufficientStat.push_back(0);
        }

        Statistics connProbSynapse;
        Statistics connProbInnervation;

        // ###################### LOOP OVER NEURONS ######################

#pragma omp parallel if (parallelLoop)
        {
#pragma omp for
            for (unsigned int i = 0; i < preIndices.size(); i++)
            {
                int preId = preIndices[i];
                for (unsigned int j = 0; j < postIndices.size(); j++)
                {
                    int postId = postIndices[j];
                    if (preId != postId)
                    {
                        for (auto pre = neuron_pre[preId].begin(); pre != neuron_pre[preId].end(); ++pre)
                        {
                            if (neuron_postExc[postId].find(pre->first) != neuron_postExc[postId].end())
                            {
                                float preVal = pre->second;
                                float postVal = neuron_postExc[postId][pre->first];
                                float postAllVal = voxel_postAllExc[pre->first];

                                if (mode == "h0_intercept_pre_pst_pstAll" || mode == "h0_pre_pst_pstAll")
                                {
                                    float arg = b0 + b1 * preVal + b2 * postVal + b3 * postAllVal;
                                    if (checkBounds)
                                    {
                                        float boundArg1 = b1 * preVal - b2 * postVal - b3 * postAllVal;
                                        //boundArg1 = boundArg1 < 0 ? -boundArg1 : boundArg1;
                                        float boundArg2 = b2 * postVal + b3 * postAllVal;

                                        bool orderOfMagnitudeBoundViolated = checkOrderMagnitude && (boundArg1 < boutonPSTRatio);
                                        bool probabilityConstraintViolated = checkProb && (boundArg2 > 0);

                                        if (orderOfMagnitudeBoundViolated || probabilityConstraintViolated)
                                        {
                                            failedBounds++;
                                        }
                                        else
                                        {
                                            validBounds++;
                                        }
                                        //qDebug() << preVal << postNorm << boundArg << boundsViolated;
                                    }
                                    arg = std::min(maxInnervationLog, arg);
                                    int synapses = 0;
                                    if (arg >= -7)
                                    {
                                        float mu = exp(arg);
                                        innervation[i][j] += mu;
                                        synapses = mRandomGenerator.drawPoisson(mu);
                                        if (synapses > 0)
                                        {
                                            if (mode == "h0_intercept_pre_pst_pstAll")
                                            {
                                                sufficientStat[0] += synapses;
                                                sufficientStat[1] += preVal * synapses;
                                                sufficientStat[2] += postVal * synapses;
                                                sufficientStat[3] += postAllVal * synapses;
                                            }
                                            else
                                            {
                                                sufficientStat[0] += preVal * synapses;
                                                sufficientStat[1] += postVal * synapses;
                                                sufficientStat[2] += postAllVal * synapses;
                                            }
                                            contacts[i][j] += synapses;
                                        }
                                    }
                                }
                                else if (mode == "h0_intercept_pre_pstNorm" || mode == "h0_pre_pstNorm")
                                {
                                    float postNorm = postVal - postAllVal; // ln(a/b) = ln(a)-ln(b)
                                    float arg = b0 + b1 * preVal + b2 * postNorm;
                                    if (checkBounds)
                                    {
                                        float boundArg1 = b1 * preVal - b2 * postNorm;
                                        //boundArg1 = boundArg1 < 0 ? -boundArg1 : boundArg1;
                                        float boundArg2 = b2 * postNorm;

                                        bool orderOfMagnitudeBoundViolated = checkOrderMagnitude && (boundArg1 < boutonPSTRatio);
                                        bool probabilityConstraintViolated = checkProb && (boundArg2 > 0);

                                        if (orderOfMagnitudeBoundViolated || probabilityConstraintViolated)
                                        {
                                            failedBounds++;
                                        }
                                        else
                                        {
                                            validBounds++;
                                        }
                                        //qDebug() << preVal << postNorm << boundArg << boundsViolated;
                                    }
                                    arg = std::min(maxInnervationLog, arg);
                                    int synapses = 0;
                                    if (arg >= -7)
                                    {
                                        float mu = exp(arg);
                                        innervation[i][j] += mu;
                                        synapses = mRandomGenerator.drawPoisson(mu);
                                        if (synapses > 0)
                                        {
                                            if (mode == "h0_intercept_pre_pstNorm")
                                            {
                                                sufficientStat[0] += synapses;
                                                sufficientStat[1] += preVal * synapses;
                                                sufficientStat[2] += postNorm * synapses;
                                            }
                                            else
                                            {
                                                sufficientStat[0] += preVal * synapses;
                                                sufficientStat[1] += postNorm * synapses;
                                            }
                                            contacts[i][j] += synapses;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        //double loopTime = std::clock();

        // ###################### DETERMINE STATISTICS ######################

        for (unsigned int i = 0; i < preIndices.size(); i++)
        {
            int realizedConnections = 0;
            for (unsigned int j = 0; j < postIndices.size(); j++)
            {
                connProbInnervation.addSample(calculateProbability(innervation[i][j]));
                realizedConnections += contacts[i][j] > 0 ? 1 : 0;
            }
            double probability = (double)realizedConnections / (double)postIndices.size();
            connProbSynapse.addSample(probability);
        }

        //double statisticsTime = std::clock();

        // ###################### WRITE OUTPUT ######################
        int total = failedBounds + validBounds;
        if (total != 0)
        {
            failed = ((double)failedBounds / (double)(total)) > maxFailedRatio;
        }

        writeSynapseMatrix(runIndex, contacts);
        writeInnervationMatrix(runIndex, innervation);
        writeStatistics(runIndex, connProbSynapse.getMean(), connProbInnervation.getMean(), sufficientStat, failed);

        //double outputTime = std::clock();

        // ###################### WRITE CONSOLE ######################

        /*
    outputTime = (outputTime - statisticsTime) / (double)CLOCKS_PER_SEC;
    statisticsTime = (statisticsTime - loopTime) / (double)CLOCKS_PER_SEC;
    loopTime = (loopTime - featureLoadTime) / (double)CLOCKS_PER_SEC;
    featureLoadTime = (featureLoadTime - zeroTime) / (double)CLOCKS_PER_SEC;
    QString timeCost = QString("Features %1, Loop %2, Statistics %3, Output %4").arg(featureLoadTime).arg(loopTime).arg(statisticsTime).arg(outputTime);
    */
        QString boundString = "";
        if (checkBounds && failed)
        {
            boundString = "FAILED";
        }
        else if (checkBounds && !failed)
        {
            boundString = "OK";
        }

        qDebug() << "RUN: " << runIndex << paramString << QString("Connection prob.: %1").arg(connProbInnervation.getMean()) << boundString;
        //qDebug() << runIndex << "Time" << timeCost;
    }
}

void
Calculator::calculateSpatial(std::map<int, std::map<int, float> >& neuron_pre, std::map<int, std::map<int, float> >& neuron_postExc, QString innervationDir)
{    
    UtilIO::makeDir(innervationDir);

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

    // ###################### INIT OUTPUT FIELDS ######################

    /*
    std::vector<int> empty(postIndices.size(), 0);
    std::vector<float> emptyFloat(postIndices.size(), 0);
    std::vector<std::vector<int> > contacts(preIndices.size(), empty);
    std::vector<std::vector<float> > innervation(preIndices.size(), emptyFloat);

    Statistics connProbSynapse;
    Statistics connProbInnervation;

    qDebug() << "loop" << preIndices.size() << postIndices.size();
    */
    // ###################### LOOP OVER NEURONS ######################

    std::set<QString> fileNames;
    /*
    std::map<int, float> innervationSum;
    for (auto it = voxelIds.begin(); it != voxelIds.end(); ++it)
    {
        innervationSum[*it] = 0;
    }
    */
#pragma omp parallel
    {
#pragma omp for
        for (unsigned int i = 0; i < preIndices.size(); i++)
        {
            qDebug() << i;
            std::map<int, std::map<int, float> > innervationPerPre;
            int preId = preIndices[i];
            for (unsigned int j = 0; j < postIndices.size(); j++)
            {
                int postId = postIndices[j];
                if (preId != postId)
                {
                    std::map<int, float> innervationPerVoxel;
                    for (auto pre = neuron_pre[preId].begin(); pre != neuron_pre[preId].end(); ++pre)
                    {
                        if (neuron_postExc[postId].find(pre->first) != neuron_postExc[postId].end())
                        {
                            float preVal = pre->second;
                            float postVal = neuron_postExc[postId][pre->first];
                            float arg = preVal * postVal;
                            innervationPerVoxel[pre->first] = arg;
                            //innervationSum[pre->first] += arg;
                        }
                    }
                    innervationPerPre[postId] = innervationPerVoxel;
                }
            }
            QString fileName = QDir(innervationDir).filePath("preNeuronID_" + QString::number(preId));
            fileNames.insert(fileName);
            QFile file(fileName);
            if (!file.open(QIODevice::WriteOnly))
            {
                const QString msg =
                    QString("Cannot open file %1 for writing.").arg(fileName);
                throw std::runtime_error(qPrintable(msg));
            }
            QTextStream stream(&file);

            for (auto itPost = innervationPerPre.begin(); itPost != innervationPerPre.end(); ++itPost)
            {
                if (itPost->second.begin() != itPost->second.end())
                {
                    stream << "postNeuronID"
                           << " " << itPost->first << "\n";
                }
                for (auto itVoxel = itPost->second.begin(); itVoxel != itPost->second.end(); ++itVoxel)
                {
                    stream << itVoxel->first << " " << itVoxel->second << "\n";
                }
            }
        }
    }
}

double
Calculator::calculateProbability(double innervationMean)
{
    if (innervationMean < 0)
    {
        return 0;
    }
    return 1 - exp(-1 * innervationMean);
}

void
Calculator::writeSynapseMatrix(int runIndex, std::vector<std::vector<int> >& contacts)
{
    QString fileName = QString("synapseMatrix_%1.dat").arg(runIndex);
    QString filePath = QDir("output").filePath(fileName);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        const QString msg =
            QString("Cannot open file %1 for writing.").arg(filePath);
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
Calculator::writeInnervationMatrix(int runIndex, std::vector<std::vector<float> >& innervation)
{
    QString fileName = QString("innervationMatrix_%1.dat").arg(runIndex);
    QString filePath = QDir("output").filePath(fileName);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        const QString msg =
            QString("Cannot open file %1 for writing.").arg(filePath);
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
Calculator::writeStatistics(int runIndex,
                            double connectionProbability,
                            double connectionProbabilityInnervation,
                            std::vector<double> sufficientStat,
                            bool failed)
{
    QString fileName = QString("statistics_%1.json").arg(runIndex);
    QString filePath = QDir("output").filePath(fileName);

    QFile json(filePath);
    if (!json.open(QIODevice::WriteOnly))
    {
        const QString msg =
            QString("Cannot open file %1 for writing.").arg(filePath);
        throw std::runtime_error(qPrintable(msg));
    }

    QTextStream out(&json);
    out << "{\"CONNECTION_PROBABILITY_SYNAPSE\":" << connectionProbability << ",";
    out << "\"CONNECTION_PROBABILITY_INNERVATION\":" << connectionProbabilityInnervation << ",";
    out << "\"SUFFICIENT_STATISTIC\":[";
    out << sufficientStat[0];
    out << "," << sufficientStat[1];
    if (sufficientStat.size() >= 3)
    {
        out << "," << sufficientStat[2];
    }
    if (sufficientStat.size() == 4)
    {
        out << "," << sufficientStat[3];
    }
    out << "],";
    if (failed)
    {
        out << "\"STATUS\": \"FAILED\"";
    }
    else
    {
        out << "\"STATUS\": \"OK\"";
    }
    out << "}";
}
