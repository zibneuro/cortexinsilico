#include "InDegreeStatistic.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DSparseVectorSet.h"
#include "InnervationStatistic.h"
#include "Typedefs.h"
#include "Util.h"
#include <QChar>
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QJsonArray>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <algorithm>
#include <ctime>
#include <math.h>
#include <random>
#include <stdexcept>

InDegreeStatistic::InDegreeStatistic(const NetworkProps &networkProps,
                                     int sampleSize, int sampleSeed,
                                     FormulaCalculator &calculator,
                                     QueryHandler *handler)
    : NetworkStatistic(networkProps, calculator, handler),
      mSampleSize(sampleSize), mSampleSeed(sampleSeed) {}

void InDegreeStatistic::checkInput(const NeuronSelection &selection) {
  if (selection.SelectionA().size() == 0) {
    const QString msg = QString("In-Degree selection A empty");
    throw std::runtime_error(qPrintable(msg));
  }
  if (selection.SelectionB().size() == 0) {
    const QString msg = QString("In-Degree selection B empty");
    throw std::runtime_error(qPrintable(msg));
  }
  if (selection.SelectionC().size() == 0) {
    const QString msg = QString("In-Degree selection C empty");
    throw std::runtime_error(qPrintable(msg));
  }
}

QList<int> InDegreeStatistic::samplePostIds(QList<int> selectionC) {
  if (selectionC.size() <= mSampleSize) {
    mSampleSize = selectionC.size();
    return selectionC;
  } else {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(selectionC.begin(), selectionC.end(), g);
    QList<int> postIds;
    for (int i = 0; i < mSampleSize; i++) {
      postIds.append(selectionC[i]);
    }
    return postIds;
  }
}

void InDegreeStatistic::doCalculate(const NeuronSelection &selection) {
  checkInput(selection);
  QList<int> postIds = samplePostIds(selection.SelectionC());

  for (int i = 0; i < postIds.size(); i++) {
    mPostNeuronId.push_back(postIds[i]);
    mValuesAC.push_back(0);
    mValuesBC.push_back(0);
  }
  const IdList preIdListA = selection.SelectionA();
  const IdList preIdListB = selection.SelectionB();
  int nPre = std::max(preIdListA.size(), preIdListB.size());

  this->mNumConnections = nPre;

  for (int i = 0; i < nPre; i++) {

    for (unsigned int j = 0; j < mPostNeuronId.size(); j++) {
      int postId = mPostNeuronId[j];
      CIS3D::SliceBand postSliceBand = selection.getBandC(postId);

      if (i < preIdListA.size()) {
        const int preId = preIdListA[i];
        if (selection.getBandA(preId) == postSliceBand) {
          const int mappedPreId =
              mNetwork.axonRedundancyMap.getNeuronIdToUse(preId);
          const float innervation = mInnervationMatrix->getValue(
              mappedPreId, postId, selection.getPostTarget(2));
          mValuesAC[j] += (double)innervation;
        }
      }
      if (i < preIdListB.size()) {
        const int preId = preIdListB[i];
        if (selection.getBandB(preId) == postSliceBand) {
          const int mappedPreId =
              mNetwork.axonRedundancyMap.getNeuronIdToUse(preId);
          const float innervation = mInnervationMatrix->getValue(
              mappedPreId, postId, selection.getPostTarget(2));
          mValuesBC[j] += (double)innervation;
        }
      }
    }

    mConnectionsDone++;
    if (mConnectionsDone % 50 == 0) {
      calculateStatistics();
      reportUpdate();
    }
  }

  calculateStatistics();
  reportComplete();
}

void InDegreeStatistic::calculateStatistics() {
  mStatisticsAC = Statistics();
  mStatisticsBC = Statistics();
  for (unsigned int j = 0; j < mPostNeuronId.size(); j++) {
    mStatisticsAC.addSample(mValuesAC[j]);
    mStatisticsBC.addSample(mValuesBC[j]);
  }
  calculateCorrelation();
}

void InDegreeStatistic::calculateCorrelation() {
  if (mValuesAC.size() < 2) {
    mCorrelation = 0;
    return;
  }

  double stdAC = mStatisticsAC.getStandardDeviation();
  double stdBC = mStatisticsBC.getStandardDeviation();
  double eps = 0.0001;
  if (std::abs(stdAC) < eps || std::abs(stdBC) < eps) {
    mCorrelation = 0;
    return;
  }

  double sum = 0;
  double meanAC = calculateMean(mValuesAC);
  double meanBC = calculateMean(mValuesBC);
  for (unsigned long i = 0; i < mValuesAC.size(); i++) {
    sum += (mValuesAC[i] - meanAC) * (mValuesBC[i] - meanBC);
  }
  sum /= mValuesAC.size();
  mCorrelation = sum / (stdAC * stdBC);
}

double InDegreeStatistic::calculateMean(std::vector<double> &values) {
  double sum = 0;
  for (auto it = values.begin(); it != values.end(); ++it) {
    sum += *it;
  }
  return sum / values.size();
}

void InDegreeStatistic::doCreateJson(QJsonObject &obj) const {
  obj.insert("sampleSize", mSampleSize);
  obj.insert("innervationStatisticsAC",
             Util::createJsonStatistic(mStatisticsAC));
  obj.insert("innervationStatisticsBC",
             Util::createJsonStatistic(mStatisticsBC));
  obj.insert("innervationValuesAC", Util::createJsonArray(mValuesAC));
  obj.insert("innervationValuesBC", Util::createJsonArray(mValuesBC));
  obj.insert("correlation", mCorrelation);
}

void InDegreeStatistic::doCreateCSV(QTextStream &out, const QChar sep) const {
  out << "Number of postsynaptic neuron samples (from selection C):" << sep
      << mSampleSize << "\n\n";

  out << "Dense structural overlap A->C" << sep << mStatisticsAC.getMean()
      << sep << "StDev" << sep << mStatisticsAC.getStandardDeviation() << sep
      << "Min" << sep << mStatisticsAC.getMinimum() << sep << "Max" << sep
      << mStatisticsAC.getMaximum() << "\n";
  out << "Dense structural overlap B->C" << sep << mStatisticsBC.getMean()
      << sep << "StDev" << sep << mStatisticsBC.getStandardDeviation() << sep
      << "Min" << sep << mStatisticsBC.getMinimum() << sep << "Max" << sep
      << mStatisticsBC.getMaximum() << "\n";

  out << "Correlation" << sep << mCorrelation;
  out << "\n";
  writeDiagram(out);
}

void InDegreeStatistic::writeDiagram(QTextStream &out) const {
  out << "Correlation diagram\n";
  out << "postNeuronID overlap_A->C overlap_B->C\n";
  for (unsigned int i = 0; i < mPostNeuronId.size(); i++) {
    out << mPostNeuronId[i] << " " << mValuesAC[i] << " " << mValuesBC[i] << "\n";
  }
};