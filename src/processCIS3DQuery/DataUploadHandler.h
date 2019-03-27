#ifndef DATAUPLOADHANDLER_H
#define DATAUPLOADYHANDLER_H

#include "CIS3DNetworkProps.h"
#include "QueryHelpers.h"
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QtNetwork/QNetworkAccessManager>



class DataUploadHandler : public QObject {

    Q_OBJECT

public:
    DataUploadHandler(QObject* parent = 0);

    void uploadNetworkData(const QJsonObject& config);
    void uploadFile(const QJsonObject& config, const QString& queryId, const QString& filename);

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
    QString               mUploadedFile;

};


#endif // DATAUPLOADHANDLER_H
