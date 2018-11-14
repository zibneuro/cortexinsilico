#ifndef SPATIALINNERVATIONQUERYHANDLER_H
#define SPATIALINNERVATIONQUERYHANDLER_H

#include "CIS3DNetworkProps.h"
#include "CIS3DStatistics.h"
#include "QueryHelpers.h"
#include "FeatureProvider.h"
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QtNetwork/QNetworkAccessManager>

class SpatialInnervationQueryHandler : public QObject
{
    Q_OBJECT

public:
    SpatialInnervationQueryHandler(QObject* parent = 0);

    void process(const QString& selectionQueryId, const QJsonObject& config);

signals:
    void completedProcessing();
    void exitWithError(int);

private slots:
    void replyGetQueryFinished(QNetworkReply* reply);
    void replyPutResultFinished(QNetworkReply* reply);

private:
    QJsonObject
    createJsonResult(
        const QString& keyView,
        const qint64 fileSizeBytes1,
        const QString& keyData,
        const qint64 fileSizeBytes2,
        int nVoxels);

    QString mQueryId;
    QJsonObject mConfig;
    QString mDataRoot;
    QNetworkAccessManager mNetworkManager;
    NetworkProps mNetwork;
    QString mLoginUrl;
    QString mLogoutUrl;
    QString mQueryUrl;
    AuthInfo mAuthInfo;
    QJsonObject mCurrentJsonData;
    Statistics mStatistics;
    QString mTempFolder;
    bool mAborted;

    void logoutAndExit(const int exitCode);
};

#endif // SELECTIONQUERYHANDLER_H
