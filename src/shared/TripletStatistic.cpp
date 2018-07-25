#include "TripletStatistic.h"
#include <math.h>
#include <QChar>
#include <QDebug>
#include <QFile>
#include <QIODevice>
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
*/
TripletStatistic::TripletStatistic(const NetworkProps& networkProps, int sampleSize)
    : NetworkStatistic(networkProps), mSampleSize(sampleSize) {}

/**
        Checks whether the neuron selectio is valid.
        @throws runtime_error if selection is ncot valid.
*/
void TripletStatistic::checkInput(const NeuronSelection& selection) {
    if (selection.MotifA().size() == 0) {
        const QString msg = QString("Motif selection A empty");
        throw std::runtime_error(qPrintable(msg));
    }
    if (selection.MotifB().size() == 0) {
        const QString msg = QString("Motif selection B empty");
        throw std::runtime_error(qPrintable(msg));
    }
    if (selection.MotifC().size() == 0) {
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
QList<CellTriplet> TripletStatistic::drawTriplets(const NeuronSelection& selection) {
    QList<CellTriplet> triplets;

    qDebug() << "[*] Selecting " << mSampleSize << " random neuron triplets.";

    std::srand(std::time(NULL));

    const unsigned int NMAX1 = selection.MotifA().size();
    const unsigned int NMAX2 = selection.MotifB().size();
    const unsigned int NMAX3 = selection.MotifC().size();

    std::list<std::vector<unsigned int> > usedTriplets;
    while (triplets.size() < mSampleSize) {
        unsigned int index1, index2, index3;

        // Cell1 randomly drawn from Seleciton1
        index1 = std::rand() % NMAX1;
        int neuron1 = selection.MotifA()[index1];

        // Cell2 randomly drawn from Selection2
        index2 = std::rand() % NMAX2;
        int neuron2 = selection.MotifA()[index2];

        // Cell3 randomly drawn from Selection3
        index3 = std::rand() % NMAX3;
        int neuron3 = selection.MotifA()[index3];

        // If drawn CellIDs are identical, draw again
        if (neuron1 == neuron2 || neuron1 == neuron3 || neuron2 == neuron3) {
            continue;
        }

        CellTriplet newTriplet(neuron1, neuron2, neuron3);
        triplets.append(newTriplet);
    }
    return triplets;
}

/**
        Sets the innervation values in the specified triplets.
        @param triplets The uninitialized triplets.
*/
void TripletStatistic::setInnervation(QList<CellTriplet>& triplets) {
    for (int i = 0; i < triplets.size(); i++) {
        triplets[i].setInnervation(mConnectome);
    }
}

/**
      Initializes empty statistics.
*/
void TripletStatistic::initializeStatistics() {
    const int nMotifs = 16;
    for (int i = 0; i < nMotifs; i++) {
        Statistics stat;
        mMotifProbabilities.append(stat);
        Statistics stat2;
        mMotifExpectedProbabilities.append(stat);
    }
}

/**
    Performs the actual computation based on the specified neurons.
    @param selection The selected neurons.
*/
void TripletStatistic::doCalculate(const NeuronSelection& selection) {
    initializeStatistics();
    checkInput(selection);
    QList<CellTriplet> triplets = drawTriplets(selection);
    setInnervation(triplets);
    MotifCombinations combinations;
    std::map<unsigned int, std::list<TripletMotif*> > motifs =
        combinations.initializeNonRedundantTripletMotifs();
    qDebug() << "[*] Computing motif probabilities based on random selection of" << triplets.size()
             << "neuron triplets.";
    computeProbabilities(triplets, motifs);
    std::vector<std::vector<double> > avgInnervation =
        TripletStatistic::getAverageInnervation(triplets);
    computeExpectedProbabilities(avgInnervation, motifs);
    deleteMotifCombinations(motifs);
    writeResult();
}

/**
        Determines the average innervation values between the three neuron
        subselections using the randomly drawn triplets for sampling.
        @param triplets The randomly drawn triplets.
        @return The average innervation values.
*/
std::vector<std::vector<double> > TripletStatistic::getAverageInnervation(
    QList<CellTriplet>& triplets) {
    std::vector<std::vector<double> > avgInnervation;
    for (int i = 0; i < 3; i++) {
        std::vector<double> emptyRow;
        avgInnervation.push_back(emptyRow);
        for (int j = 0; j < 3; j++) {
            if (i == j) {
                avgInnervation[i].push_back(0.0);
            } else {
                Statistics stat;
                for (int k = 0; k < triplets.size(); k++) {
                    stat.addSample(triplets[k].innervation[i][j]);
                }
                avgInnervation[i].push_back(stat.getMean());
                //qDebug() << stat.getMean();
            }
        }
    }
    return avgInnervation;
}

/**
        Computes the occurrence probability of the specified motifs based
        on a random selection of neuron triplets.
        @param triplets The random selection of neurons.
        @param tripletMotifs The motif combinations.
*/
void TripletStatistic::computeProbabilities(
    QList<CellTriplet>& triplets, std::map<unsigned int, std::list<TripletMotif*> > tripletMotifs) {
    std::list<CellTriplet*>::const_iterator tripletsIt;

    for (int i = 0; i < triplets.size(); i++) {
        CellTriplet currentTriplet = triplets[i];

        // Go through all 16 main motifs
        for (unsigned int j = 0; j < tripletMotifs.size(); j++) {
            // Go through all possible configurations of the current motif and sum up probabilities
            std::list<TripletMotif*> motifList = tripletMotifs[j];
            std::list<TripletMotif*>::const_iterator motifListIt;
            double motifProb = 0;

            for (motifListIt = motifList.begin(); motifListIt != motifList.end(); ++motifListIt) {
                TripletMotif* currentMotif = *motifListIt;
                motifProb += currentMotif->computeOccurrenceProbability(currentTriplet.innervation);
            }
            mMotifProbabilities[j].addSample(motifProb);
        }
    }
}

/**
        Computes the expected occurrence probability of each motifs based
        on the average connection probability between the neuron subselections.
        @param avgInnervation The average connection probabilties.
        @param tripletMotifs The motif combinations.
    */
void TripletStatistic::computeExpectedProbabilities(
    std::vector<std::vector<double> > avgInnervation,
    std::map<unsigned int, std::list<TripletMotif*> > tripletMotifs) {

    // Go through all 16 main motifs
    for (unsigned int j = 0; j < tripletMotifs.size(); j++) {
        // Go through all possible configurations of the current motif and sum up probabilities
        std::list<TripletMotif*> motifList = tripletMotifs[j];
        std::list<TripletMotif*>::const_iterator motifListIt;
        double expectedProb = 0;

        for (motifListIt = motifList.begin(); motifListIt != motifList.end(); ++motifListIt) {
            TripletMotif* currentMotif = *motifListIt;
            expectedProb += currentMotif->computeOccurrenceProbability(avgInnervation);
        }
        mMotifExpectedProbabilities[j].addSample(expectedProb);
    }
}

/**
        Deletes the motif combinations.
        @param Map of motif combinations.
*/
void TripletStatistic::deleteMotifCombinations(
    std::map<unsigned int, std::list<TripletMotif*> > tripletMotifs) {
    for (unsigned int ii = 0; ii < tripletMotifs.size(); ++ii) {
        std::list<TripletMotif*> motifList = tripletMotifs[ii];
        std::list<TripletMotif*>::iterator motifListIt;
        for (motifListIt = motifList.begin(); motifListIt != motifList.end(); ++motifListIt) {
            delete *motifListIt;
        }
    }
}

/**
    Adds the result values to a JSON object
    @param obj JSON object to which the values are appended
*/
void TripletStatistic::doCreateJson(QJsonObject& /*obj*/) const {
    // Implement when integrating the statistic into the webframework
    // refer to shared/InnervationStatistic.cpp as reference
}

/**
    Writes the result values to file stream (CSV).
    @param out The file stream to which the values are written.
    @param sep The separator between parameter name and value.
*/
void TripletStatistic::doCreateCSV(QTextStream& /*out*/, const QChar /*sep*/) const {
    // Implement when integrating the statistic into the webframework
    // refer to shared/InnervationStatistic.cpp as reference
}

/**
    Writes result to file, for testing purposes.
*/
void TripletStatistic::writeResult() const {
    QString fileName = "triplet.csv";
    QFile csv(fileName);
    if (!csv.open(QIODevice::WriteOnly)) {
        const QString msg = QString("Cannot open file %1 for writing.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }
    const QChar sep(',');
    QTextStream out(&csv);
    out << "motifID" << sep << "probability"  << sep << "expectedProbability"
        << "\n";
    for (int i = 0; i < mMotifProbabilities.size(); i++) {
        int motifID = i + 1;
        double probability = mMotifProbabilities[i].getMean();
        double expectedProbability = mMotifExpectedProbabilities[i].getMean();
        out << motifID << sep << probability << sep << expectedProbability << "\n";
    }
}
