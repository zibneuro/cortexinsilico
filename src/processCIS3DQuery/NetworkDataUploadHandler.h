#ifndef NETWORKDATAUPLOADHANDLER_H
#define NETWORKDATAUPLOADYHANDLER_H

#include "CIS3DNetworkProps.h"
#include "QueryHelpers.h"
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QtNetwork/QNetworkAccessManager>



class NetworkDataUploadHandler : public QObject {

    Q_OBJECT

public:
    NetworkDataUploadHandler(QObject* parent = 0);

    void process(const QJsonObject& config);

signals:
    void completedProcessing();
    void exitWithError(int);

private slots:
    void replyGetQueryFinished(QNetworkReply* reply);

private:
    QJsonObject           mConfig;
    QString               mDataRoot;
    QNetworkAccessManager mNetworkManager;
    NetworkProps          mNetwork;
    AuthInfo              mAuthInfo;
    QString               mLoginUrl;
    QString               mLogoutUrl;
    QStringList           mPendingRequestIds;

};


#endif // NETWORKDATAUPLOADHANDLER_H
