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
#include <cmath>

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
Calculator::calculateBatch(std::vector<QVector<float> > parametersBatch, QString mode, std::map<QString, float>& propsFloat, std::map<QString, bool>& propsBoolean)
{
    //double zeroTime = std::clock();
    bool parallelLoop = false; //!mRandomGenerator.hasUserSeed();
    float maxInnervation = propsFloat["MAX_INNERVATION"];
    float maxInnervationLog = maxInnervation == 0 ? -100 : log(maxInnervation);
    float boutonPSTRatio = propsFloat["BOUTON_PST_RATIO"] * log(10);
    float innervationCutoff = propsFloat["ABSOLUTE_INNERVATION_CUTOFF"];
    float innervationCutoffLog = innervationCutoff == 0 ? -100 : log(innervationCutoff);
    bool checkMaxInnervation = propsBoolean["APPLY_CONSTRAINT_MAX_INNERVATION"];
    bool checkBoutonPstRatio = propsBoolean["APPLY_CONSTRAINT_BOUTON_PST_RATIO"];
    bool checkPstProbability = propsBoolean["APPLY_CONSTRAINT_PST_PROBABILITY"];
    bool discardSingle = propsBoolean["DISCARD_SINGLE"];
    bool writeRaw = propsBoolean["WRITE_RAW_DATA"];

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
        std::vector<long> constraintCount;
        constraintCount.push_back(0); // total number of checks, i.e. number of all neuron pairs in all voxels
        constraintCount.push_back(0); // number of violated bouton vs. PST magnitude bounds
        constraintCount.push_back(0); // number of violated probability bounds
        constraintCount.push_back(0); // number of violated max innervation bounds
        constraintCount.push_back(0); // number of violated bounds (any)

        int runIndex = mRunIndices[run];
        //qDebug() << "BATCH" << runIndex << parametersBatch.size();
        QVector<float> parameters = parametersBatch[run];

        // ###################### SET PARAMETERS ######################

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

        std::vector<RawDataItem> rawData;

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
                                float postNormVal = postVal - postAllVal; // ln(a/b) = ln(a)-ln(b)
                                float arg;
                                float boundArg1;
                                float boundArg2;

                                if (mode == "h0_intercept_pre_pst_pstAll" || mode == "h0_pre_pst_pstAll")
                                {
                                    arg = b0 + b1 * preVal + b2 * postVal + b3 * postAllVal;
                                    boundArg1 = b1 * preVal - (b2 * postVal + b3 * postAllVal);
                                    boundArg2 = b2 * postVal + b3 * postAllVal;
                                }
                                else
                                {
                                    arg = b0 + b1 * preVal + b2 * postNormVal;
                                    boundArg1 = b1 * preVal - b2 * postNormVal;
                                    boundArg2 = b2 * postNormVal;
                                }

                                //qDebug() << preVal << postVal << postAllVal << postNormVal << arg;

                                bool orderOfMagnitudeBoundViolated = checkBoutonPstRatio && (boundArg1 < boutonPSTRatio);
                                bool probabilityConstraintViolated = checkPstProbability && (boundArg2 > 0);
                                bool innervationBoundViolated = checkMaxInnervation && (arg > maxInnervationLog);

                                bool anyViolated = updateConstraintCount(constraintCount,
                                                                         orderOfMagnitudeBoundViolated,
                                                                         probabilityConstraintViolated,
                                                                         innervationBoundViolated);

                                arg = std::min(arg, innervationCutoffLog);
                                float mu = exp(arg);
                                int synapses = mRandomGenerator.drawPoisson(mu);

                                if (writeRaw)
                                {
                                    RawDataItem rawDataItem;
                                    rawDataItem.beta0 = b0;
                                    rawDataItem.beta1 = b1;
                                    rawDataItem.beta2 = b2;
                                    if (mode == "h0_intercept_pre_pst_pstAll" || mode == "h0_pre_pst_pstAll")
                                    {
                                        rawDataItem.beta3 = b3;
                                    }
                                    else
                                    {
                                        rawDataItem.beta3 = -999;
                                    }
                                    rawDataItem.voxelId = pre->first;
                                    rawDataItem.preId = preId;
                                    rawDataItem.postId = postId;
                                    rawDataItem.pre = preVal;
                                    rawDataItem.post = postVal;
                                    rawDataItem.postAll = postAllVal;
                                    rawDataItem.postNorm = postNormVal;
                                    rawDataItem.innervation = mu;
                                    rawDataItem.synapses = synapses;
                                    rawDataItem.magnitudeBound = orderOfMagnitudeBoundViolated;
                                    rawDataItem.probabilityBound = probabilityConstraintViolated;
                                    rawDataItem.innervationBound = innervationBoundViolated;
                                    rawData.push_back(rawDataItem);
                                }

                                if (!(discardSingle && anyViolated))
                                {
                                    innervation[i][j] += mu;
                                    if (synapses > 0)
                                    {
                                        if (mode == "h0_intercept_pre_pst_pstAll")
                                        {
                                            sufficientStat[0] += synapses;
                                            sufficientStat[1] += preVal * synapses;
                                            sufficientStat[2] += postVal * synapses;
                                            sufficientStat[3] += postAllVal * synapses;
                                        }
                                        else if (mode == "h0_pre_pst_pstAll")
                                        {
                                            sufficientStat[0] += preVal * synapses;
                                            sufficientStat[1] += postVal * synapses;
                                            sufficientStat[2] += postAllVal * synapses;
                                        }
                                        else if (mode == "h0_intercept_pre_pstNorm")
                                        {
                                            sufficientStat[0] += synapses;
                                            sufficientStat[1] += preVal * synapses;
                                            sufficientStat[2] += postNormVal * synapses;
                                        }
                                        else
                                        {
                                            sufficientStat[0] += preVal * synapses;
                                            sufficientStat[1] += postNormVal * synapses;
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
        // qDebug() << constraintCount;

        writeSynapseMatrix(runIndex, contacts);
        writeInnervationMatrix(runIndex, innervation);
        writeStatistics(runIndex,
                        connProbSynapse.getMean(),
                        connProbInnervation.getMean(),
                        sufficientStat,
                        constraintCount);

        if (writeRaw)
        {
            writeRawData(runIndex, rawData);
        }

        //double outputTime = std::clock();

        // ###################### WRITE CONSOLE ######################

        /*
        outputTime = (outputTime - statisticsTime) / (double)CLOCKS_PER_SEC;
        statisticsTime = (statisticsTime - loopTime) / (double)CLOCKS_PER_SEC;
        loopTime = (loopTime - featureLoadTime) / (double)CLOCKS_PER_SEC;
        featureLoadTime = (featureLoadTime - zeroTime) / (double)CLOCKS_PER_SEC;
        QString timeCost = QString("Features %1, Loop %2, Statistics %3, Output %4").arg(featureLoadTime).arg(loopTime).arg(statisticsTime).arg(outputTime);
        */

        qDebug() << "RUN: " << runIndex << paramString << QString("Connection prob.: %1").arg(connProbInnervation.getMean());
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
                            std::vector<long> constraintCount)
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
    out << "\"CONSTRAINT_NUMBER_CHECKS\":" << constraintCount[0] << ",";
    out << "\"CONSTRAINT_MAGNITUDE_VIOLATED\":" << constraintCount[1] << ",";
    out << "\"CONSTRAINT_PROBABILITY_VIOLATED\":" << constraintCount[2] << ",";
    out << "\"CONSTRAINT_INNERVATION_VIOLATED\":" << constraintCount[3] << ",";
    out << "\"CONSTRAINT_ANY_VIOLATED\":" << constraintCount[4];
    out << "}";
}

bool
Calculator::updateConstraintCount(std::vector<long>& count,
                                  bool magnitudeBoundViolated,
                                  bool probabilityBoundViolated,
                                  bool maxInnervationBoundViolated)
{
    count[0] += 1;
    if (magnitudeBoundViolated)
    {
        count[1] += 1;
    }
    if (probabilityBoundViolated)
    {
        count[2] += 1;
    }
    if (maxInnervationBoundViolated)
    {
        count[3] += 1;
    }
    bool anyViolated = magnitudeBoundViolated || probabilityBoundViolated || maxInnervationBoundViolated;
    if (anyViolated)
    {
        count[4] += 1;
    }
    return anyViolated;
}

void
Calculator::writeRawData(int runIndex, std::vector<RawDataItem>& rawData)
{
    QString fileName = QString("rawData_%1.dat").arg(runIndex);
    QString filePath = QDir("output").filePath(fileName);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        const QString msg =
            QString("Cannot open file %1 for writing.").arg(filePath);
        throw std::runtime_error(qPrintable(msg));
    }
    QTextStream out(&file);

    out << "beta0"
        << " beta1"
        << " beta2"
        << " beta3"
        << " voxelId"
        << " preId"
        << " postId"
        << " pre"
        << " post"
        << " postAll"
        << " postNorm"
        << " innervation"
        << " synapses"
        << " magnitudeBoundViolated"
        << " probabilityBoundViolated"
        << " innervationBoundViolated"
        << " discarded"
        << "\n";

    for (unsigned int i = 0; i < rawData.size(); i++)
    {
        RawDataItem r = rawData[i];
        out << r.beta0 << " "
            << r.beta1 << " "
            << r.beta2 << " "
            << r.beta3 << " "
            << r.voxelId << " "
            << r.preId << " "
            << r.postId << " "
            << r.pre << " "
            << r.post << " "
            << r.postAll << " "
            << r.postNorm << " "
            << r.innervation << " "
            << r.synapses << " "
            << r.magnitudeBound << " "
            << r.probabilityBound << " "
            << r.innervationBound << " "
            << r.discarded << "\n";
    }
}
