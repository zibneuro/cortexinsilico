#include "QueryHandler.h"
#include "CIS3DCellTypes.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DNeurons.h"
#include "CIS3DRegions.h"
#include "CIS3DSparseVectorSet.h"
#include "EvaluationQueryHandler.h"
#include "Histogram.h"
#include "InnervationStatistic.h"
#include "NetworkStatistic.h"
#include "QueryHelpers.h"
#include "Util.h"
#include "UtilIO.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <stdexcept>
#include <ctime>

QueryHandler::QueryHandler()
{
    mLastUpdateTime = std::clock();
}

void
QueryHandler::processQuery(const QJsonObject& config,
                           const QString& queryId,
                           const QJsonObject& query)
{
    mAborted = false;
    mConfig = config;
    mQueryId = queryId;
    mQuery = query;
    mUpdateCount = 0;
    mCompleted = false;
    mResultFileHeader = Util::getResultFileHeader(query);
    mNetworkNumber = mQuery["networkNumber"].toInt();
    mNetworkSelection = mQuery["networkSelection"].toObject();
    QString datasetName = Util::getShortName(mNetworkSelection, mNetworkNumber);
    mDataRoot = Util::getDatasetPath(datasetName, mConfig);
    mSampleSettings = mQuery["sampleSelection"].toObject();
    Util::getSampleSettings(mSampleSettings, mNetworkNumber, mSampleNumber, mSampleSeed);

    if (initSelection())
    {
        setSelection();
    }

    if (!mAborted)
    {
        setFormulas();
    }
    if (!mAborted)
    {
        doProcessQuery();
    }
}

void
QueryHandler::reportUpdate(NetworkStatistic* stat)
{
    long long numConnections = stat->getNumConnections();
    long long connectionsDone = stat->getConnectionsDone();
    const double percent =
        double(connectionsDone) * 100.0 / double(numConnections);

    if (percent < 100)
    {
        mUpdateCount++;
        QJsonObject result = stat->createJson("", 0);

        QJsonObject queryStatus;
        queryStatus.insert("statusMessage", "");
        queryStatus.insert("progress", percent);

        QJsonObject query = mQuery;
        QString resultKey = getResultKey();

        query.insert("status", queryStatus);
        query.insert(resultKey, result);

        writeResult(query);
    }
}

void
QueryHandler::reportComplete(NetworkStatistic* stat)
{
    mUpdateCount++;

    QString downloadFile = "results_" + mQueryId + ".csv";
    QString downloadFilePath = stat->createCSVFile(downloadFile, mResultFileHeader, getQueryResultDir());

    QJsonObject result;
    if (uploadToS3(downloadFile, downloadFilePath) == 0)
    {
        const qint64 fileSizeBytes = QFileInfo(downloadFilePath).size();
        result = stat->createJson(downloadFile, fileSizeBytes);
    }
    else
    {
        result = stat->createJson("", 0);
    }

    QJsonObject queryStatus;
    queryStatus.insert("statusMessage", "");
    queryStatus.insert("progress", 100);

    QJsonObject query = mQuery;
    QString resultKey = getResultKey();

    query.insert("status", queryStatus);
    query.insert(resultKey, result);

    writeResult(query);
}

void QueryHandler::doProcessQuery(){};

void
QueryHandler::setSelection()
{
    mSelection.setSelectionFromQuery(mQuery, mNetwork, mConfig);
    QString error;
    if (!mSelection.isValid(mQuery, error))
    {
        abort(error);
    }
}

bool
QueryHandler::initSelection()
{
    return true;
}

void
QueryHandler::setFormulas()
{
    QJsonObject formulaSelection = mQuery["formulaSelection"].toObject();
    QJsonObject formulas;
    if (mNetworkNumber == 1)
    {
        formulas = formulaSelection["formulasNetwork1"].toObject();
    }
    else
    {
        formulas = formulaSelection["formulasNetwork2"].toObject();
    }
    mCalculator = FormulaCalculator(formulas);
    if (!mCalculator.init())
    {
        abort("Failed parsing formula.");
    }
}

void
QueryHandler::updateQuery(QJsonObject& result, double progress)
{
    if (!mAborted)
    {
        mUpdateCount++;
        //qDebug() << progress << result["voxelS3key"];
        QJsonObject query = mQuery;
        query.insert(getResultKey(), result);
        query.insert("status", getStatus(progress));
        writeResult(query);
    }
}

void
QueryHandler::writeResult(QJsonObject& query)
{
    QString queryStatus = mQueryId + "_" + QString::number(mUpdateCount);
    QString filename = getQueryResultDir() + queryStatus;
    UtilIO::writeJson(query, filename);

    QString statusFilename = getQueryStatusDir() + queryStatus;
    QJsonObject status = query["status"].toObject();
    double progress = status["progress"].toDouble();
    if (!stallUpdate(progress))
    {
        UtilIO::writeJson(status, statusFilename);
    }
}

void
QueryHandler::abort(QString message)
{
    mAborted = true;
    QString queryStatus = mQueryId + "_" + QString::number(mUpdateCount);
    QString filename = getQueryResultDir() + queryStatus;
    QJsonObject query = mQuery;
    query.insert("status", getAbortedStatus(message));
    UtilIO::writeJson(query, filename);

    QString statusFilename = getQueryStatusDir() + queryStatus;
    QJsonObject status = query["status"].toObject();
    UtilIO::writeJson(status, statusFilename);
}

QString
QueryHandler::getQueryResultDir()
{
    QString queryDir = mConfig["QUERY_DIRECTORY"].toString();
    return queryDir + "/" + mQueryId + "/results/";
}

QString
QueryHandler::getQueryStatusDir()
{
    QString queryDir = mConfig["QUERY_DIRECTORY"].toString();
    return queryDir + "/status/";
}

int
QueryHandler::uploadToS3(const QString& key, const QString& filename)
{
    const QString program = mConfig["WORKER_PYTHON_BIN"].toString();
    QStringList arguments;
    arguments.append(mConfig["WORKER_S3UPLOAD_SCRIPT"].toString());
    arguments.append("UPLOAD");
    arguments.append(key);
    arguments.append(filename);
    arguments.append(mConfig["AWS_ACCESS_KEY_CIS3D"].toString());
    arguments.append(mConfig["AWS_SECRET_KEY_CIS3D"].toString());
    arguments.append(mConfig["AWS_S3_REGION_CIS3D"].toString());
    arguments.append(mConfig["AWS_S3_BUCKET_CIS3D"].toString());

    return QProcess::execute(program, arguments);
}

QJsonObject
QueryHandler::getCompletedStatus()
{
    QJsonObject status;
    status.insert("progress", 100);
    status.insert("statusMessage", "");
    return status;
}

QJsonObject
QueryHandler::getAbortedStatus(QString message)
{
    QJsonObject status;
    status.insert("progress", -1);
    status.insert("statusMessage", message);
    return status;
}

QJsonObject
QueryHandler::getStatus(double progress)
{
    QJsonObject status;
    status.insert("progress", progress);
    status.insert("statusMessage", "");
    return status;
}

bool
QueryHandler::stallUpdate(double progress)
{
    if (progress == 100)
    {
        return false;
    }
    double currentTime = std::clock();
    double delta = currentTime - mLastUpdateTime;
    if ((delta / (double)CLOCKS_PER_SEC) >= 1.5)
    {
        mLastUpdateTime = currentTime;
        return false;
    }
    else
    {
        return true;
    }
}

