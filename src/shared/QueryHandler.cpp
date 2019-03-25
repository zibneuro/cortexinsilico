#include "QueryHandler.h"
#include "EvaluationQueryHandler.h"
#include "Histogram.h"
#include "QueryHelpers.h"
#include "CIS3DCellTypes.h"
#include "CIS3DNeurons.h"
#include "CIS3DRegions.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DSparseVectorSet.h"
#include "InnervationStatistic.h"
#include "Util.h"
#include "UtilIO.h"
#include <QDir>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <stdexcept>
#include "NetworkStatistic.h"

QueryHandler::QueryHandler()
{
}

void
QueryHandler::processQuery(const QJsonObject& config, const QString& queryId, const QJsonObject& query)
{
    mAborted = false;
    mConfig = config;
    mQueryId = queryId;
    mQuery = query;
    mUpdateCount = 0;
    mCompleted = false;
    mAdvancedSettings = Util::getAdvancedSettingsString(query);
    mNetworkNumber = mQuery["mNetworkNumber"].toInt();

    setSelection();

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
    const double percent = double(connectionsDone) * 100.0 / double(numConnections);

    QJsonObject result = stat->createJson("", 0);

    QJsonObject queryStatus;
    queryStatus.insert("statusMessage", "Running");
    queryStatus.insert("progress", percent);

    QJsonObject query = mQuery;
    QString resultKey = getResultKey();

    query.insert("queryStatus", queryStatus);
    query.insert(resultKey, result);

    writeResult(query);
}

void
QueryHandler::reportComplete(NetworkStatistic* stat)
{
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
    mAborted = !mCalculator.init();
    if (mAborted)
    {
        abort("Failed parsing formula.");
    }
}

void
QueryHandler::writeResult(QJsonObject& query)
{
    QString filename = getQueryResultDir() + mQueryId + "_" + QString::number(mUpdateCount);
    UtilIO::writeJson(query, filename);
}

void
QueryHandler::abort(QString message)
{
    qDebug() << "abort" << message;
}

QString
QueryHandler::getQueryResultDir()
{
    QString queryDir = mConfig["QUERY_DIRECTORY"].toString();
    return queryDir + "/" + mQueryId + "/results/";
}

int
QueryHandler::uploadToS3(const QString& key,
                         const QString& filename)
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