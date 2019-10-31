#include "TripletStatistic.h"
#include <math.h>
#include <QChar>
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QJsonArray>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <ctime>
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
TripletStatistic::TripletStatistic(const NetworkProps &networkProps,
                                   int sampleSize, int sampleSeed,
                                   FormulaCalculator &calculator,
                                   QueryHandler *handler)
    : NetworkStatistic(networkProps, calculator, handler)
{
  mSampleSize = 30;
  mSampleSeed = sampleSeed;
  mOverallSampleSize = sampleSize;
  mOverallCompletedSamples = 0;
  mIterations = (int)std::ceil((double)sampleSize / (double)mSampleSize);
  this->mNumConnections = (long long)mIterations;
  mConnectionsDone = 0;

  mRandomGenerator = RandomGenerator(mSampleSeed);
}

/**
        Checks whether the neuron selectio is valid.
        @throws runtime_error if selection is ncot valid.
*/
void TripletStatistic::checkInput(const NeuronSelection &selection)
{
  if (selection.SelectionA().size() == 0)
  {
    const QString msg = QString("Motif selection A empty");
    throw std::runtime_error(qPrintable(msg));
  }
  if (selection.SelectionB().size() == 0)
  {
    const QString msg = QString("Motif selection B empty");
    throw std::runtime_error(qPrintable(msg));
  }
  if (selection.SelectionC().size() == 0)
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
QList<CellTriplet> TripletStatistic::drawTriplets(
    const NeuronSelection &selection)
{
  QList<CellTriplet> triplets;

  // qDebug() << "[*] Selecting " << mSampleSize << " random neuron triplets.";

  const unsigned int NMAX1 = selection.SelectionA().size();
  const unsigned int NMAX2 = selection.SelectionB().size();
  const unsigned int NMAX3 = selection.SelectionC().size();

  std::list<std::vector<unsigned int>> usedTriplets;

  int maxDraws = 100 * mSampleSize;
  int nDraws = 0;

  while (triplets.size() < mSampleSize && nDraws <= maxDraws)
  {
    nDraws++;
    unsigned int index1, index2, index3;

    // Cell1 randomly drawn from Seleciton1
    index1 = mRandomGenerator.drawNumber(NMAX1 - 1);
    int neuron1 = selection.SelectionA()[index1];

    // Cell2 randomly drawn from Selection2
    index2 = mRandomGenerator.drawNumber(NMAX2 - 1);
    int neuron2 = selection.SelectionB()[index2];

    // Cell3 randomly drawn from Selection3
    index3 = mRandomGenerator.drawNumber(NMAX3 - 1);
    int neuron3 = selection.SelectionC()[index3];

    // If drawn CellIDs are identical, draw again
    if (neuron1 == neuron2 || neuron1 == neuron3 || neuron2 == neuron3)
    {
      continue;
    }

    // If drawn CellIDs are in different slices, draw again
    if ((selection.getBandA(neuron1) != selection.getBandB(neuron2) ||
         selection.getBandA(neuron1) != selection.getBandC(neuron3)))
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
  }
  else
  {
    mSampleSize = triplets.size();
  }

  return triplets;
}

/**
        Sets the innervation values in the specified triplets.
        @param triplets The uninitialized triplets.
*/
void TripletStatistic::setInnervation(QList<CellTriplet> &triplets)
{
  // qDebug() << "[*] Setting innervation values.";
  for (int i = 0; i < triplets.size(); i++)
  {
    triplets[i].setInnervation(mInnervationMatrix, mPostTargets);
  }
}

/**
      Initializes empty statistics.
*/
void TripletStatistic::initializeStatistics()
{
  const int nMotifs = 16;
  for (int i = 0; i < nMotifs; i++)
  {
    Statistics stat;
    mMotifProbabilities.append(stat);
    Statistics stat2;
    mMotifExpectedProbabilities.append(stat);
  }
  mConvergences.clear();
  for (int i = 0; i < 3; i++)
  {
    std::vector<double> empty(3, 0.0);
    mConvergences.push_back(empty);
  }
}

/**
    Performs the actual computation based on the specified neurons.
    @param selection The selected neurons.
*/
void TripletStatistic::doCalculate(const NeuronSelection &selection)
{
  initializeStatistics();
  checkInput(selection);

  mPostTargets.push_back(selection.getPostTarget(0));
  mPostTargets.push_back(selection.getPostTarget(1));
  mPostTargets.push_back(selection.getPostTarget(2));

  // qDebug() << "[*] Initializing motif combinations.";
  MotifCombinations combinations;
  std::map<unsigned int, std::list<TripletMotif *>> motifs =
      combinations.initializeNonRedundantTripletMotifs();

  calculateAverageConvergence(selection);

  while (mOverallCompletedSamples < mOverallSampleSize &&
         mConnectionsDone < mIterations)
  {
    if (mAborted)
    {
      return;
    }

    if (mOverallCompletedSamples + mSampleSize > mOverallSampleSize)
    {
      mSampleSize = mOverallSampleSize - mOverallCompletedSamples;
    }

    // double t0 = std::clock();
    QList<CellTriplet> triplets = drawTriplets(selection);
    mOverallCompletedSamples += triplets.size();
    // double t1 = std::clock();
    setInnervation(triplets);
    // double t2 = std::clock();
    computeProbabilities(triplets, motifs);
    // double t3 = std::clock();
    // double t4 = std::clock();
    // printAverageConvergence();
    computeExpectedProbabilities(motifs);
    // double t5 = std::clock();

    // double dt1 = (t1 - t0) / (double)CLOCKS_PER_SEC;
    // double dt2 = (t2 - t1) / (double)CLOCKS_PER_SEC;
    // double dt3 = (t3 - t2) / (double)CLOCKS_PER_SEC;
    // double dt4 = (t4 - t3) / (double)CLOCKS_PER_SEC;
    // double dt5 = (t5 - t4) / (double)CLOCKS_PER_SEC;
    // qDebug() << "MOTIF TIME" << dt1 << dt2 << dt3 << dt4 << dt5;

    calculateConcentration();

    mConnectionsDone += 1;
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
double TripletStatistic::calculateConnectionProbability(double innervation)
{
  return (double)mCalculator.calculateConnectionProbability((float)innervation);
}

void TripletStatistic::calculateConnectionProbability(const NeuronSelection &selection, IdList &a, IdList &b, int b_idx, IdList &c, int c_idx, double &ab, double &ac)
{

  Statistics stat_ab;
  Statistics stat_ac;

  std::map<int, int> preIds; // unique ID, multiplicity
  std::map<int, float> postInnervation;

  CIS3D::Structure b_postTarget = selection.getPostTarget(b_idx);
  CIS3D::Structure c_postTarget = selection.getPostTarget(c_idx);

  // get pre neurons with multiplicity
  for (int i = 0; i < a.size(); ++i)
  {
    const int preId = a[i];
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

  // iterate over pre neurons
  for (auto itPre = preIds.begin(); itPre != preIds.end(); itPre++)
  {
    // iterate over post neurons b
    for (int j = 0; j < b.size(); ++j)
    {
      const int postId = b[j];
      const float innervation = mInnervationMatrix->getValue(itPre->first, postId, b_postTarget);
      float connProb = mCalculator.calculateConnectionProbability(innervation);
      for (int k = 0; k < itPre->second; k++)
      {
        stat_ab.addSample(connProb);
      }
    }
    // iterate over post neurons c
    for (int j = 0; j < c.size(); ++j)
    {
      const int postId = c[j];
      const float innervation = mInnervationMatrix->getValue(itPre->first, postId, c_postTarget);
      float connProb = mCalculator.calculateConnectionProbability(innervation);
      for (int k = 0; k < itPre->second; k++)
      {
        stat_ac.addSample(connProb);
      }
    }
  }

  ab = stat_ab.getMean();
  ac = stat_ac.getMean();
}

/**
    Calculates the average convergence for by sampling a subset of the
   postsynaptic neurons in each group.
    @param selection The selected neuron groups.
*/
void TripletStatistic::calculateAverageConvergence(
    const NeuronSelection &selection)
{

  IdList selA = selection.SelectionA(); // 0
  IdList selB = selection.SelectionB(); // 1
  IdList selC = selection.SelectionC(); // 2

  double ab, ac, ba, bc, ca, cb;

  calculateConnectionProbability(selection, selA, selB, 1, selC, 2, ab, ac);
  calculateConnectionProbability(selection, selB, selA, 0, selC, 2, ba, bc);
  calculateConnectionProbability(selection, selC, selA, 0, selB, 1, ca, cb);

  mConvergences[0][0] = 0;
  mConvergences[0][1] = ab;
  mConvergences[0][2] = ac;
  mConvergences[1][0] = ba;
  mConvergences[1][1] = 0;
  mConvergences[1][2] = bc;
  mConvergences[2][0] = ca;
  mConvergences[2][1] = cb;
  mConvergences[2][2] = 0;

  qDebug() << "TRIPLET CONN PROB";
  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      qDebug() << i << " " << j << " " << mConvergences[i][j];
    }
  }
}

/**
        Computes the occurrence probability of the specified motifs based
        on a random selection of neuron triplets.
        @param triplets The random selection of neurons.
        @param tripletMotifs The motif combinations.
*/
void TripletStatistic::computeProbabilities(
    QList<CellTriplet> &triplets,
    std::map<unsigned int, std::list<TripletMotif *>> tripletMotifs)
{
  std::list<CellTriplet *>::const_iterator tripletsIt;

  for (int i = 0; i < triplets.size(); i++)
  {
    CellTriplet currentTriplet = triplets[i];

    // Go through all 16 main motifs
    for (unsigned int j = 0; j < tripletMotifs.size(); j++)
    {
      // Go through all possible configurations of the current motif and sum up
      // probabilities
      std::list<TripletMotif *> motifList = tripletMotifs[j];
      std::list<TripletMotif *>::const_iterator motifListIt;
      double motifProb = 0;

      for (motifListIt = motifList.begin(); motifListIt != motifList.end();
           ++motifListIt)
      {
        TripletMotif *currentMotif = *motifListIt;
        motifProb += currentMotif->computeOccurrenceProbability(
            currentTriplet.innervation, this);
      }
      mMotifProbabilities[j].addSample(motifProb);
    }
  }
}

/**
    Computes the expected occurrence probability of each motif based
    on the average convergence between the neuron subselections.
    @param tripletMotifs The motif combinations.
*/
void TripletStatistic::computeExpectedProbabilities(
    std::map<unsigned int, std::list<TripletMotif *>> tripletMotifs)
{
  /*
  std::vector<std::vector<double> > avgConvergence;
  for (int i = 0; i < 3; i++) {
    std::vector<double> emptyRow;
    avgConvergence.push_back(emptyRow);
    for (int j = 0; j < 3; j++) {
      avgConvergence[i].push_back(mConvergences[i][j]);
    }
  }*/
  mMotifExpectedProbabilities.clear();
  // Go through all 16 main motifs
  for (unsigned int j = 0; j < tripletMotifs.size(); j++)
  {
    // Go through all possible configurations of the current motif and sum up
    // probabilities
    std::list<TripletMotif *> motifList = tripletMotifs[j];
    std::list<TripletMotif *>::const_iterator motifListIt;
    double expectedProb = 0;

    for (motifListIt = motifList.begin(); motifListIt != motifList.end();
         ++motifListIt)
    {
      TripletMotif *currentMotif = *motifListIt;
      expectedProb +=
          currentMotif->computeOccurrenceProbabilityGivenInputProbability(
              mConvergences);
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
void TripletStatistic::deleteMotifCombinations(
    std::map<unsigned int, std::list<TripletMotif *>> tripletMotifs)
{
  for (unsigned int ii = 0; ii < tripletMotifs.size(); ++ii)
  {
    std::list<TripletMotif *> motifList = tripletMotifs[ii];
    std::list<TripletMotif *>::iterator motifListIt;
    for (motifListIt = motifList.begin(); motifListIt != motifList.end();
         ++motifListIt)
    {
      delete *motifListIt;
    }
  }
}

/**
    Adds the result values to a JSON object
    @param obj JSON object to which the values are appended
*/
void TripletStatistic::doCreateJson(QJsonObject &obj) const
{
  obj["sampleSize"] = mOverallCompletedSamples;

  std::vector<int> permutation = getMotifPermutation();
  QJsonArray probabilities;
  QJsonArray expectedProbabilities;
  QJsonArray probabilityDeviations;
  QJsonArray concentrations;
  QJsonArray expectedConcentrations;
  QJsonArray concentrationDeviations;

  for (int i = 0; i < 15; i++)
  {
    int dataIndex = permutation[i] - 1;
    probabilities.push_back(
        Util::createJsonStatistic(mMotifProbabilities[dataIndex]));
    expectedProbabilities.push_back(
        mMotifExpectedProbabilities[dataIndex].getMean());
    probabilityDeviations.push_back(getDeviation(dataIndex));
    concentrations.push_back(mConcentrations[i]);
    expectedConcentrations.push_back(mExpectedConcentrations[i]);
    concentrationDeviations.push_back(getConcentrationDeviation(i));
  }

  obj.insert("probabilities", probabilities);
  obj.insert("expectedProbabilities", expectedProbabilities);
  obj.insert("probabilityDeviations", probabilityDeviations);
  obj.insert("concentrations", concentrations);
  obj.insert("expectedConcentrations", expectedConcentrations);
  obj.insert("concentrationDeviations", concentrationDeviations);
}

/**
    Writes the result values to file stream (CSV).
    @param out The file stream to which the values are written.
    @param sep The separator between parameter name and value.
*/
void TripletStatistic::doCreateCSV(FileHelper &fileHelper) const
{
  fileHelper.openFile("statistics.csv");
  fileHelper.write("motif,probability,probability std,probability min,probability max,expected probability (random network),deviation to expected probability,concentration,expected concentration (random network),deviation to expected concentration\n");
  std::vector<int> permutation = getMotifPermutation();
  for (int i = 0; i < mMotifProbabilities.size() - 1; i++)
  {
    int dataIndex = permutation[i] - 1;
    fileHelper.write(QString::number(i + 1) 
        + "," + QString::number(mMotifProbabilities[dataIndex].getMean())
        + "," + QString::number(mMotifProbabilities[dataIndex].getStandardDeviation())
        + "," + QString::number(mMotifProbabilities[dataIndex].getMinimum())
        + "," + QString::number(mMotifProbabilities[dataIndex].getMaximum())
        + "," + QString::number(mMotifExpectedProbabilities[dataIndex].getMean())
        + "," + QString::number(getDeviation(dataIndex))
        + "," + QString::number(mConcentrations[i])
        + "," + QString::number(mExpectedConcentrations[i])
        + "," + QString::number(getConcentrationDeviation(i)) + "\n");
  }
  fileHelper.closeFile();
}

/**
    Writes result to file, for testing purposes.
*/
void TripletStatistic::writeResult() const
{
  QString fileName = "triplet.csv";
  QFile csv(fileName);
  if (!csv.open(QIODevice::WriteOnly))
  {
    const QString msg =
        QString("Cannot open file %1 for writing.").arg(fileName);
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
double TripletStatistic::getDeviation(int motif) const
{
  double d = mMotifProbabilities[motif].getMean();
  double dRef = mMotifExpectedProbabilities[motif].getMean();
  double deviation = getNumericDeviation(d, dRef);
  return deviation;
}

double TripletStatistic::getNumericDeviation(double observed,
                                             double expected) const
{
  /*
  double deviation;
  double d = observed;
  double dRef = expected;
  if (Util::almostEqual(dRef, 0, 0.0001)) {
    deviation = Util::almostEqual(d, 0, 0.0001) ? 0 : 1;
  } else {
    deviation = (d - dRef) / dRef;
  }
  return deviation;
  */
  if (expected == 0)
  {
    return 0;
  }
  else
  {
    return observed / expected;
  }
}

/**
    Prints the average convergence values between the subselections.
*/
void TripletStatistic::printAverageConvergence()
{
  qDebug() << "====== CONVERGENCE ======";
  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      qDebug() << i << j << mConvergences[i][j];
    }
  }
}

/**
    Permutation of motif IDs, as obtained by ordering the motifs by:
    1) Number of connected edges
    2) Degree of recurrence (number of bidirectional connections)
*/
std::vector<int> TripletStatistic::getMotifPermutation() const
{
  std::vector<int> permutation; // [1 3 5 6 4 7 2 8 10 11 12 13 9 14 15 16]
  permutation.push_back(1);
  permutation.push_back(3);
  permutation.push_back(5);
  permutation.push_back(6);
  permutation.push_back(4);
  permutation.push_back(7);
  permutation.push_back(2);
  permutation.push_back(8);
  permutation.push_back(10);
  permutation.push_back(11);
  permutation.push_back(12);
  permutation.push_back(13);
  permutation.push_back(9);
  permutation.push_back(14);
  permutation.push_back(15);
  permutation.push_back(16);
  return permutation;
}

void TripletStatistic::calculateConcentration()
{
  // Get reordered probabilities (observed and expected)
  std::vector<int> permutation = getMotifPermutation();
  std::vector<double> probabilities;
  std::vector<double> expectedProbabilities;

  for (int i = 0; i < 15; i++)
  {
    int dataIndex = permutation[i] - 1;
    probabilities.push_back(mMotifProbabilities[dataIndex].getMean());
    expectedProbabilities.push_back(
        mMotifExpectedProbabilities[dataIndex].getMean());
  }

  // Calculate group probabilities (observed and expected)
  std::vector<double> groupProbability(3, 0.0);
  std::vector<double> expectedGroupProbability(3, 0.0);

  for (int i = 0; i < 15; i++)
  {
    int groupIndex = getGroupIndex(i);
    groupProbability[groupIndex] += probabilities[i];
    expectedGroupProbability[groupIndex] += expectedProbabilities[i];
  }

  // Calculate concentrations (observed and expected)
  mConcentrations = std::vector<double>(15, 0.0);
  mExpectedConcentrations = std::vector<double>(15, 0.0);

  for (int i = 0; i < 15; i++)
  {
    int groupIndex = getGroupIndex(i);
    double concentration =
        getNumericConcentration(probabilities[i], groupProbability[groupIndex]);
    mConcentrations[i] = (concentration);
    double expectedConcentration = getNumericConcentration(
        expectedProbabilities[i], expectedGroupProbability[groupIndex]);
    mExpectedConcentrations[i] = (expectedConcentration);
  }
}

double TripletStatistic::getConcentrationDeviation(int motif) const
{
  double observed = mConcentrations[motif];
  double expected = mExpectedConcentrations[motif];
  if (expected == 0)
  {
    return 0;
  }
  else
  {
    return observed / expected;
  };
};

int TripletStatistic::getGroupIndex(const int motif) const
{
  if (motif < 7)
  {
    return 0;
  }
  else if (motif < 13)
  {
    return 1;
  }
  else
  {
    return 2;
  }
};

double TripletStatistic::getNumericConcentration(double probability,
                                                 double groupProbability)
{
  if (groupProbability == 0)
  {
    return 0;
  }
  else
  {
    return probability / groupProbability;
  }
}
