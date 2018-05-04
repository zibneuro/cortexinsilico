#include "QueryHelpers.h"
#include "CIS3DConstantsHelpers.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QProcess>


AuthInfo QueryHelpers::login(const QString url,
                             const QString& username,
                             const QString& password,
                             QNetworkAccessManager& networkManager)
{
    QNetworkRequest req;
    req.setUrl(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setAttribute(QNetworkRequest::User, QVariant("Login"));

    QJsonObject jsonData;
    jsonData.insert("user", QJsonValue(username));
    jsonData.insert("password", QJsonValue(password));

    QJsonDocument jsonDoc(jsonData);
    QString postData(jsonDoc.toJson());

    QEventLoop loop;
    QNetworkReply* reply = networkManager.post(req, postData.toLocal8Bit());
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    AuthInfo authInfo;

    QNetworkReply::NetworkError error = reply->error();
    if (error == QNetworkReply::NoError) {
        const QByteArray content = reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(content);

        qDebug() << "[*] Logged in";
        authInfo.userId    = jsonResponse.object().value(QString("data")).toObject().value("userId").toString();
        authInfo.authToken = jsonResponse.object().value(QString("data")).toObject().value("authToken").toString();

        delete reply;
        return authInfo;
    }
    else {
        const QString msg = QString("Error logging in: %1").arg(reply->errorString());
        delete reply;
        throw std::runtime_error(qPrintable(msg));
    }
}


void QueryHelpers::logout(const QString &url, const AuthInfo &authInfo, QNetworkAccessManager &networkManager)
{
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("X-User-Id"), authInfo.userId.toLocal8Bit());
    request.setRawHeader(QByteArray("X-Auth-Token"), authInfo.authToken.toLocal8Bit());
    request.setAttribute(QNetworkRequest::User, QVariant("Logout"));

    QJsonObject jsonData;
    QJsonDocument jsonDoc(jsonData);
    QString postData(jsonDoc.toJson());

    QEventLoop loop;
    QNetworkReply* reply = networkManager.post(request, postData.toLocal8Bit());
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    QNetworkReply::NetworkError error = reply->error();
    if (error == QNetworkReply::NoError) {
        qDebug() << "[*] Logged out";
    }
    else {
        qDebug() << "[-] Error logging out:" << reply->errorString();
    }

    delete reply;
}


int QueryHelpers::uploadToS3(const QString &key,
                              const QString &filename,
                              const QJsonObject& config)
{
    qDebug() << "[*] Uploading file to S3:" << key << filename;
    const QString program = config["WORKER_PYTHON_BIN"].toString();
    QStringList arguments;
    arguments.append(config["WORKER_S3UPLOAD_SCRIPT"].toString());
    arguments.append("UPLOAD");
    arguments.append(key);
    arguments.append(filename);
    arguments.append(config["AWS_ACCESS_KEY_CIS3D"].toString());
    arguments.append(config["AWS_SECRET_KEY_CIS3D"].toString());
    arguments.append(config["AWS_S3_REGION_CIS3D"].toString());
    arguments.append(config["AWS_S3_BUCKET_CIS3D"].toString());

    qDebug() << "    Program:" << program;
    qDebug() << "    Arguments:" << arguments;

    return QProcess::execute(program, arguments);
}
