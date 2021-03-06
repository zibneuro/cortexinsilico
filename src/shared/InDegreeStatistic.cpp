#include "InDegreeStatistic.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DSparseVectorSet.h"
#include "CIS3DStatistics.h"
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
#include <stdexcept>
#include "RandomGenerator.h"

InDegreeStatistic::InDegreeStatistic(const NetworkProps &networkProps,
                                     int sampleSize, int sampleSeed, bool sampleEnabled,
                                     FormulaCalculator &calculator,
                                     QueryHandler *handler)
    : NetworkStatistic(networkProps, calculator, handler),
      mSampleSize(sampleSize), mSampleSeed(sampleSeed), mSampleEnabled(sampleEnabled) {}

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
  if (selectionC.size() <= mSampleSize || !mSampleEnabled) {
      mSampleSize = selectionC.size();
    return selectionC;
  } else {    
    RandomGenerator generator(mSampleSeed);
    return generator.getSample(selectionC, mSampleSize);
  }
}

void InDegreeStatistic::doCalculate(const NeuronSelection &selection) {
  checkInput(selection);
  QList<int> postIds = samplePostIds(selection.SelectionC());
  for (int i = 0; i < postIds.size(); i++) {
    mPostNeuronId.push_back(postIds[i]);
    mValuesAC.push_back(0);
    mValuesBC.push_back(0);
    std::vector<double> a, b;
    mACProbFlat.push_back(a);
    mBCProbFlat.push_back(b);
    mValuesACProb.push_back(0);
    mValuesBCProb.push_back(0);
    mMappingSliceRBC[postIds[i]] = selection.getRBCId(postIds[i]);
  }
  const IdList preIdListA = selection.SelectionA();
  const IdList preIdListB = selection.SelectionB();
  int nPre = std::max(preIdListA.size(), preIdListB.size());

  this->mNumConnections = nPre;

  std::set<int> uniquePreA;
  std::set<int> uniquePreB;

  for (int i = 0; i < nPre; i++) {
    for (unsigned int j = 0; j < mPostNeuronId.size(); j++) {
      int postId = mPostNeuronId[j];
      CIS3D::SliceBand postSliceBand = selection.getBandC(postId);

      if (i < preIdListA.size()) {
        const int preId = preIdListA[i];
        if (selection.getBandA(preId) == postSliceBand) {
          const int mappedPreId =
              mNetwork.axonRedundancyMap.getNeuronIdToUse(preId);
          uniquePreA.insert(mappedPreId);
          const float innervation = mInnervationMatrix->getValue(
              mappedPreId, postId, selection.getPostTarget(2));
          mValuesAC[j] += static_cast<double>(innervation);
          mACProbFlat[j].push_back(static_cast<double>(mCalculator.calculateConnectionProbability(innervation)));
        }
      }
      if (i < preIdListB.size()) {
        const int preId = preIdListB[i];
        if (selection.getBandB(preId) == postSliceBand) {
          const int mappedPreId =
              mNetwork.axonRedundancyMap.getNeuronIdToUse(preId);
          uniquePreB.insert(mappedPreId);
          const float innervation = mInnervationMatrix->getValue(
              mappedPreId, postId, selection.getPostTarget(2));
          mValuesBC[j] += static_cast<double>(innervation);
          mBCProbFlat[j].push_back(static_cast<double>(mCalculator.calculateConnectionProbability(innervation)));
        }
      }
    }

    mConnectionsDone++;
    if (mConnectionsDone % 50 == 0) {
      calculateStatistics();
      reportUpdate();
    }
  }

  qDebug() << "UNIQUE PRE A";
  for(auto it = uniquePreA.begin(); it != uniquePreA.end(); it++){
    qDebug() << *it;
  }

  qDebug() << "UNIQUE PRE B";
  for(auto it = uniquePreB.begin(); it != uniquePreB.end(); it++){
    qDebug() << *it;
  }


  calculateStatistics();
  reportComplete();
}

void InDegreeStatistic::calculateStatistics() {

  for (unsigned int j = 0; j < mPostNeuronId.size(); j++) {
    mValuesACProb[j] = calculateMean(mACProbFlat[j]);
    mValuesBCProb[j] = calculateMean(mBCProbFlat[j]);
  }

  mStatisticsAC = Statistics();
  mStatisticsBC = Statistics();
  mStatisticsACProb = Statistics();
  mStatisticsBCProb = Statistics();
  for (unsigned int j = 0; j < mPostNeuronId.size(); j++) {
    mStatisticsAC.addSample(mValuesAC[j]);
    mStatisticsBC.addSample(mValuesBC[j]);
    mStatisticsACProb.addSample(mValuesACProb[j]);
    mStatisticsBCProb.addSample(mValuesBCProb[j]);
  }
  double stdAC = mStatisticsAC.getStandardDeviation();
  double stdBC = mStatisticsBC.getStandardDeviation();
  double stdACProb = mStatisticsACProb.getStandardDeviation();
  double stdBCProb = mStatisticsBCProb.getStandardDeviation();

  mCorrelation = calculateCorrelation(mValuesAC, mValuesBC, stdAC, stdBC);
  mCorrelationProb = calculateCorrelation(mValuesACProb, mValuesBCProb, stdACProb, stdBCProb);
}

double InDegreeStatistic::calculateCorrelation(std::vector<double>& valuesAC, std::vector<double>& valuesBC,  double stdAC, double stdBC) {
  if (valuesAC.size() < 2) {
    return 0;
  }  

  double eps = 0.0001;
  if (std::abs(stdAC) < eps || std::abs(stdBC) < eps) {
      double meanAC = calculateMean(valuesAC);
      double meanBC = calculateMean(valuesBC);
      if(std::abs(meanAC - meanBC) < eps){
          return 1;
      } else {
          return 0;
      }
  }

  double sum = 0;
  double meanAC = calculateMean(valuesAC);
  double meanBC = calculateMean(valuesBC);
  for (unsigned long i = 0; i < valuesAC.size(); i++) {
    sum += (valuesAC[i] - meanAC) * (valuesBC[i] - meanBC);
  }
  sum /= valuesAC.size();
  return sum / (stdAC * stdBC);
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
  obj.insert("probabilityStatisticsAC",
             Util::createJsonStatistic(mStatisticsACProb));
  obj.insert("probabilityStatisticsBC",
             Util::createJsonStatistic(mStatisticsBCProb));
  obj.insert("innervationValuesAC", Util::createJsonArray(mValuesAC));
  obj.insert("innervationValuesBC", Util::createJsonArray(mValuesBC));
  obj.insert("probabilityValuesAC", Util::createJsonArray(mValuesACProb));
  obj.insert("probabilityValuesBC", Util::createJsonArray(mValuesBCProb));
  obj.insert("correlation", mCorrelation);
  obj.insert("correlationProbability", mCorrelationProb);
}

void InDegreeStatistic::doCreateCSV(FileHelper& fileHelper) const {
  fileHelper.openFile("statistics.csv");
  fileHelper.write(Statistics::getHeaderCsv());
  fileHelper.write(Statistics::getLineSingleValue("sample size (from C)",mSampleSize));
  fileHelper.write(mStatisticsAC.getLineCsv("DSO (summed A to C)"));
  fileHelper.write(mStatisticsBC.getLineCsv("DSO (summed B to C)"));
  fileHelper.write(Statistics::getLineSingleValue("corelation (summed DSO)",mCorrelation));
  fileHelper.write(Statistics::getLineSingleValue("corelation (avg. connection probability)",mCorrelationProb));
  fileHelper.closeFile();

  writeDiagramOverlap(fileHelper);
  writeDiagramProbability(fileHelper);
}

void InDegreeStatistic::writeDiagramOverlap(FileHelper& fileHelper) const {
    fileHelper.openFile("scatterPlot_DSO.csv");    
    fileHelper.write("postNeuronID,summedDSO_A->C,summedDSO_B->C\n");
    for (unsigned int i = 0; i < mPostNeuronId.size(); i++) {
        auto it = mMappingSliceRBC.find(mPostNeuronId[i]);
        fileHelper.write(QString::number(it->second)  + "," + QString::number(mValuesAC[i]) + "," + QString::number(mValuesBC[i]) + "\n");
    }
    fileHelper.closeFile();
};

void InDegreeStatistic::writeDiagramProbability(FileHelper& fileHelper) const {
    fileHelper.openFile("scatterPlot_connectionProbability.csv");
    fileHelper.write("postNeuronID,avgConnectionProbability_A->C,avgConnectionProbability_B->C\n");
    for (unsigned int i = 0; i < mPostNeuronId.size(); i++) {
        auto it = mMappingSliceRBC.find(mPostNeuronId[i]);
        fileHelper.write(QString::number(it->second)  + "," + QString::number(mValuesACProb[i]) + "," + QString::number(mValuesBCProb[i]) + "\n");        
    }
    fileHelper.closeFile();
};
