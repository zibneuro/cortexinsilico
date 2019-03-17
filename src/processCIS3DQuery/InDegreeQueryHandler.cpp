#include "InDegreeQueryHandler.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <stdexcept>
#include "CIS3DCellTypes.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DNeurons.h"
#include "CIS3DRegions.h"
#include "CIS3DSparseVectorSet.h"
#include "Histogram.h"
#include "NeuronSelection.h"
#include "QueryHelpers.h"
#include "InDegreeStatistic.h"
#include "Util.h"

InDegreeQueryHandler::InDegreeQueryHandler(QObject* parent)
    : QObject(parent)
{
}

void
InDegreeQueryHandler::process(const QString& inDegreeQueryId, const QJsonObject& config)
{
    mConfig = config;
    mQueryId = inDegreeQueryId;

    const QString baseUrl = mConfig["METEOR_URL_CIS3D"].toString();
    const QString queryEndPoint = mConfig["METEOR_INDEGREEQUERY_ENDPOINT"].toString();
    const QString loginEndPoint = mConfig["METEOR_LOGIN_ENDPOINT"].toString();
    const QString logoutEndPoint = mConfig["METEOR_LOGOUT_ENDPOINT"].toString();

    if (baseUrl.isEmpty())
    {
        throw std::runtime_error("InDegreeQueryHandler: Cannot find METEOR_URL_CIS3D");
    }
    if (queryEndPoint.isEmpty())
    {
        throw std::runtime_error(
            "InDegreeQueryHandler: Cannot find METEOR_INDEGREEQUERY_ENDPOINT");
    }
    if (loginEndPoint.isEmpty())
    {
        throw std::runtime_error("InDegreeQueryHandler: Cannot find METEOR_LOGIN_ENDPOINT");
    }
    if (logoutEndPoint.isEmpty())
    {
        throw std::runtime_error("InDegreeQueryHandler: Cannot find METEOR_LOGOUT_ENDPOINT");
    }

    mQueryUrl = baseUrl + queryEndPoint + mQueryId;
    mLoginUrl = baseUrl + loginEndPoint;
    mLogoutUrl = baseUrl + logoutEndPoint;

    mAuthInfo = QueryHelpers::login(mLoginUrl, mConfig["WORKER_USERNAME"].toString(), mConfig["WORKER_PASSWORD"].toString(), mNetworkManager, mConfig);

    QNetworkRequest request;
    request.setUrl(mQueryUrl);
    request.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
    request.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
    request.setAttribute(QNetworkRequest::User, QVariant("getQueryData"));
    QueryHelpers::setAuthorization(mConfig, request);

    connect(&mNetworkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyGetQueryFinished(QNetworkReply*)));
    mNetworkManager.get(request);
}

void
InDegreeQueryHandler::reportUpdate(NetworkStatistic* stat)
{
    long long numConnections = stat->getNumConnections();
    long long connectionsDone = stat->getConnectionsDone();

    QJsonObject result = stat->createJson("", 0);
    QJsonObject progress;
    progress.insert("completed", connectionsDone);
    progress.insert("total", numConnections);
    const double percent = double(connectionsDone) * 100.0 / double(numConnections);
    progress.insert("percent", percent);
    QJsonObject jobStatus;
    jobStatus.insert("status", "running");
    jobStatus.insert("progress", progress);

    QJsonObject payload;
    payload.insert("result", result);
    payload.insert("jobStatus", jobStatus);

    QNetworkRequest putRequest;
    putRequest.setUrl(mQueryUrl);
    putRequest.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
    putRequest.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
    putRequest.setAttribute(QNetworkRequest::User, QVariant("putIntermediateInDegreeResult"));
    putRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QueryHelpers::setAuthorization(mConfig, putRequest);

    QJsonDocument putDoc(payload);
    QString putData(putDoc.toJson());

    qDebug() << "Posting intermediate result:" << percent << "\\%    (" << connectionsDone << "/"
             << numConnections << ")";
    QEventLoop loop;
    QNetworkReply* reply = mNetworkManager.put(putRequest, putData.toLocal8Bit());
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    QNetworkReply::NetworkError error = reply->error();
    const QString requestId = reply->request().attribute(QNetworkRequest::User).toString();
    if (error != QNetworkReply::NoError)
    {
        qDebug() << "[-] Error putting Evaluation result (queryId" << mQueryId << "):";
        qDebug() << reply->errorString();
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404)
        {
            qDebug() << QString(reply->readAll().replace("\"", ""));
        }
        reply->deleteLater();
        logoutAndExit(1);
        stat->abort();
    }
}

void
InDegreeQueryHandler::reportComplete(NetworkStatistic* stat)
{
    long long numConnections = stat->getNumConnections();

    const QString motifASelId = mCurrentJsonData["inDegreeASelectionId"].toString();
    const QString motifBSelId = mCurrentJsonData["inDegreeBSelectionId"].toString();
    const QString motifCSelId = mCurrentJsonData["inDegreeCSelectionId"].toString();
    const QString key = QString("inDegree_%1_%2_%3_%4.csv")
                            .arg(mQueryId)
                            .arg(motifASelId)
                            .arg(motifBSelId)
                            .arg(motifCSelId);
    const QString motifASelectionText = mCurrentJsonData["inDegreeASelectionFilterAsText"].toString();
    const QString motifBSelectionText = mCurrentJsonData["inDegreeBSelectionFilterAsText"].toString();
    const QString motifCSelectionText = mCurrentJsonData["inDegreeCSelectionFilterAsText"].toString();
    const QString csvfile =
        stat->createCSVFile(key, motifASelectionText, motifBSelectionText, motifCSelectionText, mConfig["WORKER_TMP_DIR"].toString(), mAdvancedSettings);
    const qint64 fileSizeBytes = QFileInfo(csvfile).size();
    if (QueryHelpers::uploadToS3(key, csvfile, mConfig) != 0)
    {
        qDebug() << "Error uploading csv file to S3:" << csvfile;
        logoutAndExit(1);
    }

    QJsonObject result = stat->createJson(key, fileSizeBytes);
    QJsonObject progress;
    progress.insert("completed", numConnections);
    progress.insert("total", numConnections);
    progress.insert("percent", 100);
    QJsonObject jobStatus;
    jobStatus.insert("status", "completed");
    jobStatus.insert("progress", progress);

    QJsonObject payload;
    payload.insert("result", result);
    payload.insert("jobStatus", jobStatus);

    QNetworkRequest putRequest;
    putRequest.setUrl(mQueryUrl);
    putRequest.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
    putRequest.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
    putRequest.setAttribute(QNetworkRequest::User, QVariant("putInDegreeResult"));
    putRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QueryHelpers::setAuthorization(mConfig, putRequest);

    QJsonDocument putDoc(payload);
    QString putData(putDoc.toJson());

    qDebug() << "In degree signal finished";

    connect(&mNetworkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyPutResultFinished(QNetworkReply*)));
    mNetworkManager.put(putRequest, putData.toLocal8Bit());
};

void
InDegreeQueryHandler::replyGetQueryFinished(QNetworkReply* reply)
{
    QNetworkReply::NetworkError error = reply->error();
    if (error == QNetworkReply::NoError &&
        !reply->request().attribute(QNetworkRequest::User).toString().contains("getQueryData"))
    {
        return;
    }
    else if (error == QNetworkReply::NoError)
    {
        const QByteArray content = reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(content);
        mCurrentJsonData = jsonResponse.object().value("data").toObject();
        QJsonObject networkSelection = mCurrentJsonData["networkSelection"].toObject();
        int networkNumber = mCurrentJsonData["networkNumber"].toInt();

        QJsonObject sampleSettings = mCurrentJsonData["sampleSettings"].toObject();
        mAdvancedSettings = Util::getAdvancedSettingsString(mCurrentJsonData);
        reply->deleteLater();

        QJsonObject formulas = mCurrentJsonData["formulas"].toObject();
        FormulaCalculator calculator(formulas);
        calculator.init();

        int numberSamples, randomSeed;
        Util::getSampleSettings(sampleSettings, networkNumber, numberSamples, randomSeed);
        
        NeuronSelection selection;
        selection.setSelectionFromQuery(mCurrentJsonData, mNetwork, mConfig);

        InDegreeStatistic statistic(mNetwork, numberSamples, calculator);
        connect(&statistic, SIGNAL(update(NetworkStatistic*)), this, SLOT(reportUpdate(NetworkStatistic*)));
        connect(&statistic, SIGNAL(complete(NetworkStatistic*)), this, SLOT(reportComplete(NetworkStatistic*)));
        statistic.calculate(selection);
    }
    else
    {
        qDebug() << "[-] Error obtaining InDegreeQuery data:";
        qDebug() << reply->errorString();
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404)
        {
            qDebug() << QString(reply->readAll().replace("\"", ""));
        }
        reply->deleteLater();
        logoutAndExit(1);
    }
}

void
InDegreeQueryHandler::replyPutResultFinished(QNetworkReply* reply)
{
    QNetworkReply::NetworkError error = reply->error();
    const QString requestId = reply->request().attribute(QNetworkRequest::User).toString();
    if (error == QNetworkReply::NoError && !(requestId == "putInDegreeResult"))
    {
        return;
    }
    else if (error == QNetworkReply::NoError)
    {
        qDebug() << "    Completed processing inDegree query" << mQueryId;
        reply->deleteLater();
        logoutAndExit(0);
    }
    else
    {
        qDebug() << "[-] Error putting inDegree result (queryId" << mQueryId << "):";
        qDebug() << reply->errorString();
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404)
        {
            qDebug() << QString(reply->readAll().replace("\"", ""));
        }
        reply->deleteLater();
        logoutAndExit(1);
    }
}

void
InDegreeQueryHandler::logoutAndExit(const int exitCode)
{
    QueryHelpers::logout(mLogoutUrl, mAuthInfo, mNetworkManager, mConfig);

    if (exitCode == 0)
    {
        emit completedProcessing();
    }
    else
    {
        QCoreApplication::exit(exitCode);
    }
}
