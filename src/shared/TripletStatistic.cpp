#include "TripletStatistic.h"
#include <math.h>
#include <ctime>
#include <QChar>
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QJsonArray>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <stdexcept>
#include "CIS3DConstantsHelpers.h"
#include "CIS3DSparseVectorSet.h"
#include "InnervationStatistic.h"
#include "MotifCombinations.h"
#include "Typedefs.h"
#include "Util.h"

/**
    Constructor.
    @param networkProps The model data of the network.
    @param sampleSize The number of triplets to draw.
    @param iterations The number of iterations.
*/
TripletStatistic::TripletStatistic(const NetworkProps& networkProps, int sampleSize, int iterations)
    : NetworkStatistic(networkProps)
    , mSampleSize(sampleSize)
    , mIterations(iterations)
{
    this->mNumConnections = (long long)mIterations;
}

/**
        Checks whether the neuron selectio is valid.
        @throws runtime_error if selection is ncot valid.
*/
void
TripletStatistic::checkInput(const NeuronSelection& selection)
{
    if (selection.MotifA().size() == 0)
    {
        const QString msg = QString("Motif selection A empty");
        throw std::runtime_error(qPrintable(msg));
    }
    if (selection.MotifB().size() == 0)
    {
        const QString msg = QString("Motif selection B empty");
        throw std::runtime_error(qPrintable(msg));
    }
    if (selection.MotifC().size() == 0)
    {
        const QString msg = QString("Motif selection C empty");
        throw std::runtime_error(qPrintable(msg));
    }
}

/**
        Randomly select triplets of the specified neuron type.
        @param selection The neuron selection.
        @param nrOfTriples The number of triplets to draw.
        @return List of triplets.
*/
QList<CellTriplet>
TripletStatistic::drawTriplets(const NeuronSelection& selection)
{
    QList<CellTriplet> triplets;

    qDebug() << "[*] Selecting " << mSampleSize << " random neuron triplets.";

    std::srand(std::time(NULL));

    const unsigned int NMAX1 = selection.MotifA().size();
    const unsigned int NMAX2 = selection.MotifB().size();
    const unsigned int NMAX3 = selection.MotifC().size();

    std::list<std::vector<unsigned int> > usedTriplets;

    int maxDraws = 100 * mSampleSize;
    int nDraws = 0;

    while (triplets.size() < mSampleSize && nDraws <= maxDraws)
    {
        nDraws++;
        unsigned int index1, index2, index3;

        // Cell1 randomly drawn from Seleciton1
        index1 = std::rand() % NMAX1;
        int neuron1 = selection.MotifA()[index1];

        // Cell2 randomly drawn from Selection2
        index2 = std::rand() % NMAX2;
        int neuron2 = selection.MotifB()[index2];

        // Cell3 randomly drawn from Selection3
        index3 = std::rand() % NMAX3;
        int neuron3 = selection.MotifC()[index3];

        // If drawn CellIDs are identical, draw again
        if (neuron1 == neuron2 || neuron1 == neuron3 || neuron2 == neuron3)
        {
            continue;
        }

        // If drawn CellIDs are in different slices, draw again
        if (
            (selection.getMotifABand(neuron1) != selection.getMotifBBand(neuron2) ||
             selection.getMotifABand(neuron1) != selection.getMotifCBand(neuron3)))
        {
            continue;
        }

        CellTriplet newTriplet(neuron1, neuron2, neuron3);
        triplets.append(newTriplet);
    }

    if (triplets.size() == 0)
    {
        const QString msg = QString("Drawing triplets failed");
        throw std::runtime_error(qPrintable(msg));
    } else {
        mSampleSize = triplets.size();
    }

    return triplets;
}

/**
        Sets the innervation values in the specified triplets.
        @param triplets The uninitialized triplets.
*/
void
TripletStatistic::setInnervation(QList<CellTriplet>& triplets)
{
    qDebug() << "[*] Setting innervation values.";
    for (int i = 0; i < triplets.size(); i++)
    {
        triplets[i].setInnervation(mConnectome);
    }
}

/**
      Initializes empty statistics.
*/
void
TripletStatistic::initializeStatistics()
{
    const int nMotifs = 16;
    for (int i = 0; i < nMotifs; i++)
    {
        Statistics stat;
        mMotifProbabilities.append(stat);
        Statistics stat2;
        mMotifExpectedProbabilities.append(stat);
    }
    for (int i = 0; i < 3; i++)
    {
        std::vector<Statistics> emptyRow;
        mConvergences.push_back(emptyRow);
        for (int j = 0; j < 3; j++)
        {
            Statistics stat;
            mConvergences[i].push_back(stat);
        }
    }
}

/**
    Performs the actual computation based on the specified neurons.
    @param selection The selected neurons.
*/
void
TripletStatistic::doCalculate(const NeuronSelection& selection)
{
    initializeStatistics();
    checkInput(selection);

    qDebug() << "[*] Initializing motif combinations.";
    MotifCombinations combinations;
    std::map<unsigned int, std::list<TripletMotif*> > motifs =
        combinations.initializeNonRedundantTripletMotifs();

    for (int i = 0; i < mIterations; i++)
    {
        if (mAborted)
        {
            return;
        }

        QList<CellTriplet> triplets = drawTriplets(selection);
        setInnervation(triplets);
        computeProbabilities(triplets, motifs);
        // std::vector<std::vector<double> > avgConvergence =
        // TripletStatistic::getAverageConvergence(triplets);
        // computeExpectedProbabilities(avgConvergence, motifs);
        calculateAverageConvergence(selection);
        printAverageConvergence();
        computeExpectedProbabilities(motifs);
        mConnectionsDone = i + 1;
        reportUpdate();
    }

    deleteMotifCombinations(motifs);
    reportComplete();
}

/*
    Calculates the connection probabiltiy for the specfied innervation.
    @param innervation The innervation.
    @return The connection probability.
*/
double
TripletStatistic::calculateConnectionProbability(double innervation)
{
    return 1 - exp(-1 * innervation);
}

/**
    Calculates the convergence to the specified postsynaptic neuron.
    @param presynapticNeurons The IDs of the presynaptic neurons.
    @param postsynapticNeuronId The ID of the postsynaptic neuron.
    @return  The convergence to the postsynaptic neuron.
*/
double
TripletStatistic::calculateConvergence(IdList& presynapticNeurons,
                                       int postsynapticNeuronId,
                                       int selectionIndex)
{
    int presynapticSamplingFactor = 1;
    Statistics connectionProbability;
    for (int i = 0; i < presynapticNeurons.size(); i += presynapticSamplingFactor)
    {
        int preId = presynapticNeurons[i];
        double innervation = mConnectome->getValue(preId, postsynapticNeuronId, selectionIndex);
        double probability = calculateConnectionProbability(innervation);
        connectionProbability.addSample(probability);
    }
    return connectionProbability.getMean();
}

/**
    Calculates the average convergence for by sampling a subset of the postsynaptic
    neurons in each group.
    @param selection The selected neuron groups.
*/
void
TripletStatistic::calculateAverageConvergence(const NeuronSelection& selection)
{
    int preLimit = 3000;
    std::vector<IdList> pre;
    pre.push_back(drawRandomlyExceeds(selection.MotifA(), preLimit));
    pre.push_back(drawRandomlyExceeds(selection.MotifB(), preLimit));
    pre.push_back(drawRandomlyExceeds(selection.MotifC(), preLimit));
    std::vector<IdList> post;
    post.push_back(drawRandomly(selection.MotifA(), mSampleSize));
    post.push_back(drawRandomly(selection.MotifB(), mSampleSize));
    post.push_back(drawRandomly(selection.MotifC(), mSampleSize));
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            if (i != j)
            {
                for (int k = 0; k < post[j].size(); k++)
                {
                    /*
                    if (mConvergences[i][j].hasConverged(0.0001)) {
                        break;
                    } else {
                    */
                    mConvergences[i][j].addSample(calculateConvergence(pre[i], post[j][k], i));
                    //}
                }
            }
        }
    }
}

/**
    Calculates the average convergence to the specified postsynaptic neurons.
    @param presynapticNeurons The IDs of the presynaptic neurons.
    @param postsynapticNeuronId The IDs of the postsynaptic neurons.
    @return  The average convergence to the postsynaptic neurons.
*/
double
TripletStatistic::calculateAverageConvergence(IdList& presynapticNeurons,
                                              IdList& postsynapticNeurons)
{
    int postsynapticSamplingFactor = 1;
    Statistics convergence;
    for (int i = 0; i < postsynapticNeurons.size(); i += postsynapticSamplingFactor)
    {
        convergence.addSample(calculateConvergence(presynapticNeurons, postsynapticNeurons[i], i));
    }
    return convergence.getMean();
}

/**
    Determines the average convergence values between the three neuron
    subselections using the randomly drawn triplets for sampling.
    @param triplets The randomly drawn triplets.
    @return The average convergence values.
*/
std::vector<std::vector<double> >
TripletStatistic::getAverageConvergence(
    // Retrieve sample of neuron selections based on triplets.
    QList<CellTriplet>& triplets)
{
    IdList motifA;
    IdList motifB;
    IdList motifC;
    for (int k = 0; k < triplets.size(); k++)
    {
        motifA.push_back(triplets[k].preCellIndex[0]);
        motifB.push_back(triplets[k].preCellIndex[1]);
        motifC.push_back(triplets[k].preCellIndex[2]);
    }
    std::vector<IdList> selections;
    selections.push_back(motifA);
    selections.push_back(motifB);
    selections.push_back(motifC);

    // Compute average convergence for all 6 cases.
    std::vector<std::vector<double> > avgConvergence;
    for (int i = 0; i < 3; i++)
    {
        std::vector<double> emptyRow;
        avgConvergence.push_back(emptyRow);
        for (int j = 0; j < 3; j++)
        {
            if (i == j)
            {
                avgConvergence[i].push_back(0.0);
            }
            else
            {
                double convergence = calculateAverageConvergence(selections[i], selections[j]);
                qDebug() << "[*] Convergence " << i << j << convergence;
                avgConvergence[i].push_back(convergence);
            }
        }
    }
    return avgConvergence;
}

/**
        Computes the occurrence probability of the specified motifs based
        on a random selection of neuron triplets.
        @param triplets The random selection of neurons.
        @param tripletMotifs The motif combinations.
*/
void
TripletStatistic::computeProbabilities(
    QList<CellTriplet>& triplets, std::map<unsigned int, std::list<TripletMotif*> > tripletMotifs)
{
    std::list<CellTriplet*>::const_iterator tripletsIt;

    for (int i = 0; i < triplets.size(); i++)
    {
        CellTriplet currentTriplet = triplets[i];

        // Go through all 16 main motifs
        for (unsigned int j = 0; j < tripletMotifs.size(); j++)
        {
            // Go through all possible configurations of the current motif and sum up
            // probabilities
            std::list<TripletMotif*> motifList = tripletMotifs[j];
            std::list<TripletMotif*>::const_iterator motifListIt;
            double motifProb = 0;

            for (motifListIt = motifList.begin(); motifListIt != motifList.end(); ++motifListIt)
            {
                TripletMotif* currentMotif = *motifListIt;
                motifProb += currentMotif->computeOccurrenceProbability(currentTriplet.innervation);
            }
            mMotifProbabilities[j].addSample(motifProb);
        }
    }
}

/**
        Computes the expected occurrence probability of each motif based
        on the average connection probability between the neuron subselections.
        @param avgInnervation The average connection probabilties.
        @param tripletMotifs The motif combinations.
*/
void
TripletStatistic::computeExpectedProbabilities(
    std::vector<std::vector<double> > avgInnervation,
    std::map<unsigned int, std::list<TripletMotif*> > tripletMotifs)
{
    // Go through all 16 main motifs
    for (unsigned int j = 0; j < tripletMotifs.size(); j++)
    {
        // Go through all possible configurations of the current motif and sum up probabilities
        std::list<TripletMotif*> motifList = tripletMotifs[j];
        std::list<TripletMotif*>::const_iterator motifListIt;
        double expectedProb = 0;

        for (motifListIt = motifList.begin(); motifListIt != motifList.end(); ++motifListIt)
        {
            TripletMotif* currentMotif = *motifListIt;
            expectedProb += currentMotif->computeOccurrenceProbability(avgInnervation);
        }
        mMotifExpectedProbabilities[j].addSample(expectedProb);
    }
}

/**
    Computes the expected occurrence probability of each motif based
    on the average convergence between the neuron subselections.
    @param tripletMotifs The motif combinations.
*/
void
TripletStatistic::computeExpectedProbabilities(
    std::map<unsigned int, std::list<TripletMotif*> > tripletMotifs)
{
    std::vector<std::vector<double> > avgConvergence;
    for (int i = 0; i < 3; i++)
    {
        std::vector<double> emptyRow;
        avgConvergence.push_back(emptyRow);
        for (int j = 0; j < 3; j++)
        {
            avgConvergence[i].push_back(mConvergences[i][j].getMean());
        }
    }
    mMotifExpectedProbabilities.clear();
    // Go through all 16 main motifs
    for (unsigned int j = 0; j < tripletMotifs.size(); j++)
    {
        // Go through all possible configurations of the current motif and sum up probabilities
        std::list<TripletMotif*> motifList = tripletMotifs[j];
        std::list<TripletMotif*>::const_iterator motifListIt;
        double expectedProb = 0;

        for (motifListIt = motifList.begin(); motifListIt != motifList.end(); ++motifListIt)
        {
            TripletMotif* currentMotif = *motifListIt;
            expectedProb +=
                currentMotif->computeOccurrenceProbabilityGivenInputProbability(avgConvergence);
        }
        Statistics stat;
        stat.addSample(expectedProb);
        mMotifExpectedProbabilities.push_back(stat);
    }
}

/**
        Deletes the motif combinations.
        @param Map of motif combinations.
*/
void
TripletStatistic::deleteMotifCombinations(
    std::map<unsigned int, std::list<TripletMotif*> > tripletMotifs)
{
    for (unsigned int ii = 0; ii < tripletMotifs.size(); ++ii)
    {
        std::list<TripletMotif*> motifList = tripletMotifs[ii];
        std::list<TripletMotif*>::iterator motifListIt;
        for (motifListIt = motifList.begin(); motifListIt != motifList.end(); ++motifListIt)
        {
            delete *motifListIt;
        }
    }
}

/**
    Adds the result values to a JSON object
    @param obj JSON object to which the values are appended
*/
void
TripletStatistic::doCreateJson(QJsonObject& obj) const
{
    obj["sampleSize"] = mSampleSize * mConnectionsDone;

    for (int i = 0; i < 16; i++)
    {
        const QString key = QString("motif%1").arg(i + 1);
        const QString keyRef = QString("motif%1Ref").arg(i + 1);
        obj.insert(key, Util::createJsonStatistic(mMotifProbabilities[i]));
        obj.insert(keyRef, Util::createJsonStatistic(mMotifExpectedProbabilities[i]));
    }
}

/**
    Writes the result values to file stream (CSV).
    @param out The file stream to which the values are written.
    @param sep The separator between parameter name and value.
*/
void
TripletStatistic::doCreateCSV(QTextStream& out, const QChar sep) const
{
    out << "Number of triplet samples:" << sep << mSampleSize * mConnectionsDone << "\n\n";

    for (int i = 0; i < mMotifProbabilities.size(); i++)
    {
        out << QString("Motif %1").arg(i + 1) << sep << "Probability" << sep
            << mMotifProbabilities[i].getMean() << sep << "StDev" << sep
            << mMotifProbabilities[i].getStandardDeviation() << sep << "Min" << sep
            << mMotifProbabilities[i].getMinimum() << sep << "Max" << sep
            << mMotifProbabilities[i].getMaximum() << sep << "Motif deviation" << sep
            << getDeviation(i) << "\n";
    }
}

/**
    Writes result to file, for testing purposes.
*/
void
TripletStatistic::writeResult() const
{
    QString fileName = "triplet.csv";
    QFile csv(fileName);
    if (!csv.open(QIODevice::WriteOnly))
    {
        const QString msg = QString("Cannot open file %1 for writing.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }
    const QChar sep(',');
    QTextStream out(&csv);
    out << "motifID" << sep << "probability" << sep << "expectedProbability"
        << "\n";
    for (int i = 0; i < mMotifProbabilities.size(); i++)
    {
        int motifID = i + 1;
        double probability = mMotifProbabilities[i].getMean();
        double expectedProbability = mMotifExpectedProbabilities[i].getMean();
        out << motifID << sep << probability << sep << expectedProbability << "\n";
    }
}

/**
    Determines the deviation for the specified motif.
    @param motif The number of the motif.
    @return The deviation.
*/
double
TripletStatistic::getDeviation(int motif) const
{
    double deviation;
    double d = mMotifProbabilities[motif].getMean();
    double dRef = mMotifExpectedProbabilities[motif].getMean();
    if (Util::almostEqual(dRef, 0, 0.0001))
    {
        deviation = Util::almostEqual(d, 0, 0.0001) ? 0 : 1;
    }
    else
    {
        deviation = (d - dRef) / dRef;
    }
    return deviation;
}

/**
    Draws a random selection of neuron IDs.
    @param neuronIds The complete ID list.
    @param number The number of neurons to draw.
    @return The selected IDs.
*/
IdList
TripletStatistic::drawRandomly(const IdList& neuronIds, int number)
{
    IdList selection;
    std::srand(std::time(NULL));
    const unsigned int NMAX = neuronIds.size();
    for (int i = 0; i < number; i++)
    {
        unsigned int index = std::rand() % NMAX;
        selection.push_back(neuronIds[index]);
    }
    return selection;
}

/**
    Draws a random selection of neuron IDs, if the number of elements
    in the specified list exceeds a certain limit.
    @param neuronIds The complete ID list.
    @param limit The permissible number of elements.
    @return The selected IDs.
*/
IdList
TripletStatistic::drawRandomlyExceeds(const IdList& neuronIds, int limit)
{
    if (neuronIds.size() <= limit)
    {
        return neuronIds;
    }
    else
    {
        return drawRandomly(neuronIds, limit);
    }
}

/**
    Prints the average convergence values between the subselections.
*/
void
TripletStatistic::printAverageConvergence()
{
    qDebug() << "====== CONVERGENCE ======";
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            qDebug() << i << j << mConvergences[i][j].getMean();
        }
    }
}
