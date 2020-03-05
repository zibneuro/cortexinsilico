#include "QueryHelpers.h"
#include "CIS3DConstantsHelpers.h"
#include <QJsonObject>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDebug>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QProcess>
#include <QDir>
#include "UtilIO.h"

AuthInfo
QueryHelpers::login(const QString url,
                    const QString& username,
                    const QString& password,
                    QNetworkAccessManager& networkManager,
                    const QJsonObject& config)
{
    QNetworkRequest req;
    req.setUrl(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setAttribute(QNetworkRequest::User, QVariant("Login"));
    setAuthorization(config, req);

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
    if (error == QNetworkReply::NoError)
    {
        const QByteArray content = reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(content);

        qDebug() << "[*] Logged in";
        authInfo.userId = jsonResponse.object().value(QString("data")).toObject().value("userId").toString();
        authInfo.authToken = jsonResponse.object().value(QString("data")).toObject().value("authToken").toString();

        delete reply;
        return authInfo;
    }
    else
    {
        const QString msg = QString("Error logging in: %1").arg(reply->errorString());
        delete reply;
        throw std::runtime_error(qPrintable(msg));
    }
}

void
QueryHelpers::logout(const QString& url, const AuthInfo& authInfo, QNetworkAccessManager& networkManager, const QJsonObject& config)
{
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader(QByteArray("X-User-Id"), authInfo.userId.toLocal8Bit());
    request.setRawHeader(QByteArray("X-Auth-Token"), authInfo.authToken.toLocal8Bit());
    setAuthorization(config, request);

    QJsonObject jsonData;
    QJsonDocument jsonDoc(jsonData);
    QString postData(jsonDoc.toJson());

    QEventLoop loop;
    QNetworkReply* reply = networkManager.post(request, postData.toLocal8Bit());
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    QNetworkReply::NetworkError error = reply->error();
    if (error == QNetworkReply::NoError)
    {
        qDebug() << "[*] Logged out";
    }
    else
    {
        qDebug() << "[-] Error logging out:" << reply->errorString();
    }

    delete reply;
}

int
QueryHelpers::uploadToS3(const QString& key,
                         const QString& filename,
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

QString
QueryHelpers::getDatasetPath(const QString& datasetShortName,
                             const QJsonObject& config)
{
    qDebug() << datasetShortName;

    const QJsonValue datasetsJson = config["WORKER_DATASETS_CIS3D"];
    if (!datasetsJson.isArray())
    {
        throw std::runtime_error("QueryHelpers::getDatasetPath: WORKER_DATASETS_CIS3D is not an array");
    }

    const QJsonArray datasetsArray = datasetsJson.toArray();

    for (int i = 0; i < datasetsArray.size(); ++i)
    {
        const QJsonValue& datasetJson = datasetsArray.at(i);

        if (!datasetJson.isObject())
        {
            throw std::runtime_error("QueryHelpers::getDatasetPath: dataset entry is not an object");
        }

        const QJsonObject dataset = datasetJson.toObject();

        if (!dataset.contains(QString("shortName")))
        {
            throw std::runtime_error("QueryHelpers::getDatasetPath: dataset entry has no field 'shortName'");
        }

        const QJsonValue shortNameJson = dataset.value(QString("shortName"));
        if (!shortNameJson.isString())
        {
            throw std::runtime_error("QueryHelpers::getDatasetPath: dataset entry 'shortName' is not a string");
        }

        const QString shortName = shortNameJson.toString();

        if (shortName == datasetShortName)
        {
            if (!dataset.contains(QString("path")))
            {
                throw std::runtime_error("QueryHelpers::getDatasetPath: dataset entry has no field 'path'");
            }

            const QJsonValue pathJson = dataset.value(QString("path"));
            if (!pathJson.isString())
            {
                throw std::runtime_error("QueryHelpers::getDatasetPath: dataset entry 'path' is not a string");
            }

            return pathJson.toString();
        }
    }

    throw std::runtime_error("QueryHelpers::getDatasetPath: no path found for dataset shortName");
}

QString
QueryHelpers::getPrimaryDatasetRoot(const QJsonObject& config)
{
    const QJsonValue datasetsJson = config["WORKER_DATASETS_CIS3D"];
    if (!datasetsJson.isArray())
    {
        throw std::runtime_error("QueryHelpers::getPrimaryDatasetRoot: WORKER_DATASETS_CIS3D is not an array");
    }

    const QJsonArray datasetsArray = datasetsJson.toArray();

    for (int i = 0; i < datasetsArray.size(); ++i)
    {
        const QJsonValue& datasetJson = datasetsArray.at(i);

        if (!datasetJson.isObject())
        {
            throw std::runtime_error("QueryHelpers::getPrimaryDatasetRoot: dataset entry is not an object");
        }

        const QJsonObject dataset = datasetJson.toObject();

        if (!dataset.contains(QString("priority")))
        {
            throw std::runtime_error("QueryHelpers::getPrimaryDatasetRoot: dataset entry has no field 'priority'");
        }

        const QJsonValue priorityJson = dataset.value(QString("priority"));
        if (!priorityJson.isString())
        {
            throw std::runtime_error("QueryHelpers::getPrimaryDatasetRoot: dataset entry 'priority' is not a string");
        }

        const QString priority = priorityJson.toString();

        if (priority == "PRIMARY")
        {
            if (!dataset.contains(QString("path")))
            {
                throw std::runtime_error("QueryHelpers::getPrimaryDatasetRoot: dataset entry has no field 'path'");
            }

            const QJsonValue pathJson = dataset.value(QString("path"));
            if (!pathJson.isString())
            {
                throw std::runtime_error("QueryHelpers::getPrimaryDatasetRoot: dataset entry 'path' is not a string");
            }

            return pathJson.toString();
        }
    }

    throw std::runtime_error("QueryHelpers::getPrimaryDatasetRoot: no path found for primary dataset");
}

QJsonArray
QueryHelpers::getDatasetsAsJson(const QJsonObject& config)
{
    QDir featureDir(config["WORKER_DATA_DIR_CIS3D"].toString());
    QString filepath = featureDir.absoluteFilePath("networks.json");
    const QJsonValue docObject = UtilIO::parseSpecFile(filepath).value("networks");
    
    QJsonArray result;
    const QJsonArray datasetsArray = docObject.toArray();
    qDebug() << "size" << datasetsArray.size();

    for (int i = 0; i < datasetsArray.size(); ++i)
    {
        QJsonObject resultItem;

        const QJsonValue& datasetJson = datasetsArray.at(i);

        if (!datasetJson.isObject())
        {
            throw std::runtime_error("QueryHelpers::getDatasetsAsJson: dataset entry is not an object");
        }
        const QJsonObject dataset = datasetJson.toObject();

        if (!dataset.contains(QString("priority")))
        {
            throw std::runtime_error("QueryHelpers::getDatasetsAsJson: dataset entry has no field 'priority'");
        }
        const QJsonValue priorityJson = dataset.value(QString("priority"));
        if (!priorityJson.isString())
        {
            throw std::runtime_error("QueryHelpers::getDatasetsAsJson: dataset entry 'priority' is not a string");
        }

        if (!dataset.contains(QString("shortName")))
        {
            throw std::runtime_error("QueryHelpers::getDatasetsAsJson: dataset entry has no field 'shortName'");
        }
        const QJsonValue shortnameJson = dataset.value(QString("shortName"));
        if (!shortnameJson.isString())
        {
            throw std::runtime_error("QueryHelpers::getDatasetsAsJson: dataset entry 'shortName' is not a string");
        }

        if (!dataset.contains(QString("longName")))
        {
            throw std::runtime_error("QueryHelpers::getDatasetsAsJson: dataset entry has no field 'longName'");
        }
        const QJsonValue longnameJson = dataset.value(QString("longName"));
        if (!longnameJson.isString())
        {
            throw std::runtime_error("QueryHelpers::getDatasetsAsJson: dataset entry 'longName' is not a string");
        }

        resultItem.insert("priority", priorityJson);
        resultItem.insert("shortName", shortnameJson);
        resultItem.insert("longName", longnameJson);

        result.append(resultItem);
    }

    return result;
}

void
QueryHelpers::setAuthorization(const QJsonObject& config, QNetworkRequest& request)
{
    QString concatenated = config["AUTHORIZATION"].toString();
    QByteArray data = concatenated.toLocal8Bit().toBase64();
    QString headerData = "Basic " + data;
    request.setRawHeader("Authorization", headerData.toLocal8Bit());
}