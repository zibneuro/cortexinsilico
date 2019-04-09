#ifndef QUERYHANDLER_H
#define QUERYHANDLER_H

#include "CIS3DNetworkProps.h"
#include "CIS3DNeurons.h"
#include "CIS3DStatistics.h"
#include "FormulaCalculator.h"
#include "Histogram.h"
#include "NeuronSelection.h"
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QTextStream>

class NetworkStatistic;

class QueryHandler {
public:
  QueryHandler();

  void processQuery(const QJsonObject &config, const QString &queryId,
                    const QJsonObject &query);
  virtual void reportUpdate(NetworkStatistic *stat);
  virtual void reportComplete(NetworkStatistic *stat);

protected:
  virtual void doProcessQuery();
  void setSelection();
  virtual bool initSelection();
  void setFormulas();
  void updateQuery(QJsonObject &result, double progress);
  void writeResult(QJsonObject &query);
  void abort(QString error);
  virtual QString getResultKey() = 0;
  QString getQueryResultDir();
  QString getQueryStatusDir();
  int uploadToS3(const QString &key, const QString &filename);
  QJsonObject getCompletedStatus();
  QJsonObject getAbortedStatus(QString message);
  QJsonObject getStatus(double progress);
  bool stallUpdate(double progress);
  QString createHeader();

  QJsonObject mConfig;
  QString mQueryId;
  QJsonObject mQuery;
  QString mDataRoot;
  QJsonObject mLatestResult;
  int mUpdateCount;
  bool mCompleted;
  QJsonObject mNetworkSelection;
  int mNetworkNumber;
  QString mResultFileHeader;
  QJsonObject mFormulas;
  QJsonObject mSampleSettings;
  int mSampleNumber;
  int mSampleSeed;
  bool mAborted;
  FormulaCalculator mCalculator;
  NetworkProps mNetwork;
  NeuronSelection mSelection;
  double mLastUpdateTime;
};

#endif // QUERYHANDLER_H
