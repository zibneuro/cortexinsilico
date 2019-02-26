#include "MotifQueryHandler.h"
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
#include "TripletStatistic.h"
#include "Util.h"

MotifQueryHandler::MotifQueryHandler(QObject* parent)
    : QObject(parent)
{
}

void
MotifQueryHandler::process(const QString& motifQueryId, const QJsonObject& config)
{
    mConfig = config;
    mQueryId = motifQueryId;

    const QString baseUrl = mConfig["METEOR_URL_CIS3D"].toString();
    const QString queryEndPoint = mConfig["METEOR_MOTIFQUERY_ENDPOINT"].toString();
    const QString loginEndPoint = mConfig["METEOR_LOGIN_ENDPOINT"].toString();
    const QString logoutEndPoint = mConfig["METEOR_LOGOUT_ENDPOINT"].toString();

    if (baseUrl.isEmpty())
    {
        throw std::runtime_error("MotifQueryHandler: Cannot find METEOR_URL_CIS3D");
    }
    if (queryEndPoint.isEmpty())
    {
        throw std::runtime_error(
            "MotifQueryHandler: Cannot find METEOR_MOTIFQUERY_ENDPOINT");
    }
    if (loginEndPoint.isEmpty())
    {
        throw std::runtime_error("MotifQueryHandler: Cannot find METEOR_LOGIN_ENDPOINT");
    }
    if (logoutEndPoint.isEmpty())
    {
        throw std::runtime_error("MotifQueryHandler: Cannot find METEOR_LOGOUT_ENDPOINT");
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
MotifQueryHandler::reportUpdate(NetworkStatistic* stat)
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
    putRequest.setAttribute(QNetworkRequest::User, QVariant("putIntermediateMotifResult"));
    putRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QueryHelpers::setAuthorization(mConfig, putRequest);

    QJsonDocument putDoc(payload);
    QString putData(putDoc.toJson());

    qDebug() << "Posting intermediate result:" << percent << "\%    (" << connectionsDone << "/"
             << numConnections << ")";
    QEventLoop loop;
    QNetworkReply* reply = mNetworkManager.put(putRequest, putData.toLocal8Bit());
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
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
MotifQueryHandler::reportComplete(NetworkStatistic* stat)
{
    long long numConnections = stat->getNumConnections();

    const QString motifASelId = mCurrentJsonData["motifASelectionId"].toString();
    const QString motifBSelId = mCurrentJsonData["motifBSelectionId"].toString();
    const QString motifCSelId = mCurrentJsonData["motifCSelectionId"].toString();
    const QString key = QString("motif_%1_%2_%3_%4.csv")
                            .arg(mQueryId)
                            .arg(motifASelId)
                            .arg(motifBSelId)
                            .arg(motifCSelId);
    const QString motifASelectionText = mCurrentJsonData["motifASelectionFilterAsText"].toString();
    const QString motifBSelectionText = mCurrentJsonData["motifBSelectionFilterAsText"].toString();
    const QString motifCSelectionText = mCurrentJsonData["motifCSelectionFilterAsText"].toString();
    const QString csvfile =
        stat->createCSVFile(key,
                            motifASelectionText,
                            motifBSelectionText,
                            motifCSelectionText,
                            mConfig["WORKER_TMP_DIR"].toString(),
                            mAdvancedSettings);
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
    putRequest.setAttribute(QNetworkRequest::User, QVariant("putMotifResult"));
    putRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QueryHelpers::setAuthorization(mConfig, putRequest);

    QJsonDocument putDoc(payload);
    QString putData(putDoc.toJson());

    connect(&mNetworkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyPutResultFinished(QNetworkReply*)));
    mNetworkManager.put(putRequest, putData.toLocal8Bit());
};

void
MotifQueryHandler::replyGetQueryFinished(QNetworkReply* reply)
{
    QNetworkReply::NetworkError error = reply->error();
    if (error == QNetworkReply::NoError &&
        !reply->request().attribute(QNetworkRequest::User).toString().contains("getQueryData"))
    {
        return;
    }
    else if (error == QNetworkReply::NoError)
    {
        qDebug() << "[*] Starting computation of motif query";

        const QByteArray content = reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(content);
        QJsonObject jsonData = jsonResponse.object().value("data").toObject();
        mCurrentJsonData = jsonData;
        reply->deleteLater();

        const QString datasetShortName = jsonData["network"].toString();
        mDataRoot = QueryHelpers::getDatasetPath(datasetShortName, mConfig);
        qDebug() << "    Loading network data:" << datasetShortName << "Path: " << mDataRoot;
        mNetwork.setDataRoot(mDataRoot);
        mNetwork.loadFilesForQuery();

        QString motifASelString = jsonData["motifASelectionFilter"].toString();
        QString motifBSelString = jsonData["motifBSelectionFilter"].toString();
        QString motifCSelString = jsonData["motifCSelectionFilter"].toString();

        // EXTRACT SLICE PARAMETERS
        const double tissueLowMotifA = jsonData["tissueLowMotifA"].toDouble();
        const double tissueHighMotifA = jsonData["tissueHighMotifA"].toDouble();
        const QString tissueModeMotifA = jsonData["tissueModeMotifA"].toString();
        const double tissueLowMotifB = jsonData["tissueLowMotifB"].toDouble();
        const double tissueHighMotifB = jsonData["tissueHighMotifB"].toDouble();
        const QString tissueModeMotifB = jsonData["tissueModeMotifB"].toString();
        const double tissueLowMotifC = jsonData["tissueLowMotifC"].toDouble();
        const double tissueHighMotifC = jsonData["tissueHighMotifC"].toDouble();
        const QString tissueModeMotifC = jsonData["tissueModeMotifC"].toString();
        const double sliceRef = jsonData["sliceRef"].toDouble();
        const bool isSlice = sliceRef != -9999;

        // EXTRACT FORMULA
        QJsonObject formulas = jsonData["formulas"].toObject();
        FormulaCalculator calculator(formulas);
        calculator.init();

        mAdvancedSettings = Util::getAdvancedSettingsString(jsonData);

        NeuronSelection selection;
        qDebug() << "[*] Determining triplet selection:" << motifASelString << motifBSelString
                 << motifCSelString;
        selection.setTripletSelection(motifASelString, motifBSelString, motifCSelString, mNetwork);
        //selection.printMotifStats();

        if (isSlice)
        {
            selection.filterTripletSlice(mNetwork,
                                         sliceRef,
                                         tissueLowMotifA,
                                         tissueHighMotifA,
                                         tissueModeMotifA,
                                         tissueLowMotifB,
                                         tissueHighMotifB,
                                         tissueModeMotifB,
                                         tissueLowMotifC,
                                         tissueHighMotifC,
                                         tissueModeMotifC);
        }
        selection.printMotifStats();

        TripletStatistic statistic(mNetwork, 50, 200, calculator);

        if (isSlice)
        {
            statistic.setOriginalPreIds(NeuronSelection::filterPreOrBoth(mNetwork, selection.MotifA()),
                                        NeuronSelection::filterPreOrBoth(mNetwork, selection.MotifB()),
                                        NeuronSelection::filterPreOrBoth(mNetwork, selection.MotifC()));
        }

        connect(&statistic, SIGNAL(update(NetworkStatistic*)), this, SLOT(reportUpdate(NetworkStatistic*)));
        connect(&statistic, SIGNAL(complete(NetworkStatistic*)), this, SLOT(reportComplete(NetworkStatistic*)));
        statistic.calculate(selection);
    }
    else
    {
        qDebug() << "[-] Error obtaining MotifQuery data:";
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
MotifQueryHandler::replyPutResultFinished(QNetworkReply* reply)
{
    QNetworkReply::NetworkError error = reply->error();
    const QString requestId = reply->request().attribute(QNetworkRequest::User).toString();
    if (error == QNetworkReply::NoError && !(requestId == "putMotifResult"))
    {
        return;
    }
    else if (error == QNetworkReply::NoError)
    {
        qDebug() << "    Completed processing motif query" << mQueryId;
        reply->deleteLater();
        logoutAndExit(0);
    }
    else
    {
        qDebug() << "[-] Error putting Motif result (queryId" << mQueryId << "):";
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
MotifQueryHandler::logoutAndExit(const int exitCode)
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
