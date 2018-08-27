#ifndef INDEGREEQUERYHANDLER_H
#define INDEGREEQUERYHANDLER_H

#include "CIS3DNetworkProps.h"
#include "QueryHelpers.h"
#include "NetworkStatistic.h"
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QtNetwork/QNetworkAccessManager>

class InDegreeQueryHandler : public QObject {

    Q_OBJECT

public:
    InDegreeQueryHandler(QObject *parent = 0);

    void process(const QString &inDegreeQueryId, const QJsonObject &config);

signals:
    void completedProcessing();

private slots:
    void replyGetQueryFinished(QNetworkReply *reply);
    void replyPutResultFinished(QNetworkReply *reply);
    void reportUpdate(NetworkStatistic* stat);
    void reportComplete(NetworkStatistic* stat);

private:
    QString mQueryId;
    QJsonObject mConfig;
    QString mDataRoot;
    QNetworkAccessManager mNetworkManager;
    NetworkProps mNetwork;
    AuthInfo mAuthInfo;
    QString mLoginUrl;
    QString mLogoutUrl;
    QString mQueryUrl;
    QJsonObject mCurrentJsonData;


    void logoutAndExit(const int exitCode);
};

#endif // INDEGREEQUERYHANDLER_H
