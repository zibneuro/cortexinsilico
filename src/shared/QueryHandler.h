#ifndef QUERYHANDLER_H
#define QUERYHANDLER_H

#include "CIS3DNeurons.h"
#include "CIS3DNetworkProps.h"
#include "CIS3DStatistics.h"
#include "Histogram.h"
#include "Typedefs.h"
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QTextStream>
#include <QString>
#include <QObject>
#include "FormulaCalculator.h"
#include "CIS3DNetworkProps.h"
#include "NeuronSelection.h"
#include <QProcess>

class NetworkStatistic;

class QueryHandler
{
public:
    QueryHandler();

    void processQuery(const QJsonObject& config, const QString& queryId, const QJsonObject& query);
    virtual void reportUpdate(NetworkStatistic* stat);
    virtual void reportComplete(NetworkStatistic* stat);

protected:
    virtual void doProcessQuery();
    void setSelection();
    void setFormulas();
    void update();
    void complete();
    void writeResult(QJsonObject& query);
    void abort(QString error);
    virtual QString getResultKey() = 0;
    QString getQueryResultDir();
    QString getQueryStatusDir();
    int uploadToS3(const QString& key,
                   const QString& filename);
    QJsonObject getCompletedStatus();
    QJsonObject getAbortedStatus(QString message);

    QJsonObject mConfig;
    QString mQueryId;
    QJsonObject mQuery;
    QJsonObject mLatestResult;
    int mUpdateCount;
    bool mCompleted;
    int mNetworkNumber;
    QString mAdvancedSettings;
    QJsonObject mFormulas;
    bool mAborted;
    FormulaCalculator mCalculator;
    NetworkProps mNetwork;
    NeuronSelection mSelection;
};

#endif // QUERYHANDLER_H
