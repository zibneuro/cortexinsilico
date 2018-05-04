#ifndef SELECTIONQUERYHANDLER_H
#define SELECTIONQUERYHANDLER_H

#include "CIS3DNetworkProps.h"
#include "QueryHelpers.h"
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QtNetwork/QNetworkAccessManager>

class SelectionQueryHandler : public QObject {

    Q_OBJECT

public:
    SelectionQueryHandler(QObject *parent = 0);

    void process(const QString &selectionQueryId, const QJsonObject &config);

signals:
    void completedProcessing();
    void exitWithError(int);

private slots:
    void replyGetQueryFinished(QNetworkReply *reply);
    void replyPutResultFinished(QNetworkReply *reply);

private:
    QString mQueryId;
    QJsonObject mConfig;
    QString mDataRoot;
    QNetworkAccessManager mNetworkManager;
    NetworkProps mNetwork;
    QString mLoginUrl;
    QString mLogoutUrl;
    QString mQueryUrl;
    AuthInfo mAuthInfo;

    void logoutAndExit(const int exitCode);
};

#endif // SELECTIONQUERYHANDLER_H
