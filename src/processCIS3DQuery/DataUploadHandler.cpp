#include "DataUploadHandler.h"
#include "QueryHelpers.h"
#include "CIS3DCellTypes.h"
#include "CIS3DNeurons.h"
#include "CIS3DRegions.h"
#include "CIS3DConstantsHelpers.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCoreApplication>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <stdexcept>
#include "UtilIO.h"

DataUploadHandler::DataUploadHandler(QObject* parent)
    : QObject(parent)
{
}

QJsonArray
getCellTypesAsJson(const NetworkProps& network)
{
    QJsonArray arr;

    const QList<int> ids = network.cellTypes.getAllCellTypeIds(true);
    for (int i = 0; i < ids.size(); ++i)
    {
        const int id = ids[i];

        const Vec3f dendCol = network.cellTypes.getDendriteColor(id);
        QJsonArray dendColor;
        dendColor << dendCol.getX() << dendCol.getY() << dendCol.getZ();

        const Vec3f axonCol = network.cellTypes.getAxonColor(id);
        QJsonArray axonColor;
        axonColor << axonCol.getX() << axonCol.getY() << axonCol.getZ();

        QJsonObject obj;
        obj.insert("ID", id);
        obj.insert("name", network.cellTypes.getName(id));
        obj.insert("dendriteColor", dendColor);
        obj.insert("axonColor", axonColor);
        arr.append(obj);
    }

    return arr;
}

QJsonArray
getColumnsAsJson(const NetworkProps& network)
{
    QJsonArray arr;

    QList<int> columnRegionIds;
    columnRegionIds.append(network.regions.getId("A1"));
    columnRegionIds.append(network.regions.getId("A2"));
    columnRegionIds.append(network.regions.getId("A3"));
    columnRegionIds.append(network.regions.getId("A4"));
    columnRegionIds.append(network.regions.getId("B1"));
    columnRegionIds.append(network.regions.getId("B2"));
    columnRegionIds.append(network.regions.getId("B3"));
    columnRegionIds.append(network.regions.getId("B4"));
    columnRegionIds.append(network.regions.getId("C1"));
    columnRegionIds.append(network.regions.getId("C2"));
    columnRegionIds.append(network.regions.getId("C3"));
    columnRegionIds.append(network.regions.getId("C4"));
    columnRegionIds.append(network.regions.getId("D1"));
    columnRegionIds.append(network.regions.getId("D2"));
    columnRegionIds.append(network.regions.getId("D3"));
    columnRegionIds.append(network.regions.getId("D4"));
    columnRegionIds.append(network.regions.getId("E1"));
    columnRegionIds.append(network.regions.getId("E2"));
    columnRegionIds.append(network.regions.getId("E3"));
    columnRegionIds.append(network.regions.getId("E4"));
    columnRegionIds.append(network.regions.getId("Alpha"));
    columnRegionIds.append(network.regions.getId("Beta"));
    columnRegionIds.append(network.regions.getId("Gamma"));
    columnRegionIds.append(network.regions.getId("Delta"));

    for (int i = 0; i < columnRegionIds.size(); ++i)
    {
        const int id = columnRegionIds[i];
        QJsonObject obj;
        obj.insert("ID", id);
        obj.insert("name", network.regions.getName(id));
        arr.append(obj);
    }

    return arr;
}

QJsonArray
getRegionsAsJson(const NetworkProps& network)
{
    QJsonArray arr;

    const QList<int> ids = network.regions.getAllRegionIds();
    for (int i = 0; i < ids.size(); ++i)
    {
        const int id = ids[i];
        QJsonObject obj;
        obj.insert("ID", id);
        obj.insert("name", network.regions.getName(id));
        arr.append(obj);
    }

    return arr;
}

QJsonArray
getLaminarLocationsAsJson()
{
    QJsonArray arr;

    QJsonObject l1;
    l1.insert("ID", int(CIS3D::LAYER1));
    l1.insert("name", "I");
    arr.append(l1);

    QJsonObject l2;
    l2.insert("ID", int(CIS3D::LAYER2));
    l2.insert("name", "II");
    arr.append(l2);

    QJsonObject l3;
    l3.insert("ID", int(CIS3D::LAYER3));
    l3.insert("name", "III");
    arr.append(l3);

    QJsonObject l4;
    l4.insert("ID", int(CIS3D::LAYER4));
    l4.insert("name", "IV");
    arr.append(l4);

    QJsonObject l5;
    l5.insert("ID", int(CIS3D::LAYER5));
    l5.insert("name", "V");
    arr.append(l5);

    QJsonObject l6;
    l6.insert("ID", int(CIS3D::LAYER6));
    l6.insert("name", "VI");
    arr.append(l6);

    return arr;
}

QJsonArray
getPostsynapticTargetsAsJson()
{
    QJsonArray arr;

    QJsonObject granularObj;
    granularObj.insert("ID", int(CIS3D::BASAL));
    granularObj.insert("name", "Basal");
    arr.append(granularObj);

    QJsonObject infraObj;
    infraObj.insert("ID", int(CIS3D::APICAL));
    infraObj.insert("name", "Apical");
    arr.append(infraObj);

    return arr;
}

void
DataUploadHandler::uploadNetworkData(const QJsonObject& config)
{
    qDebug() << "UPLOAD NW DATA";

    mConfig = config;

    const QString baseUrl = mConfig["METEOR_URL_CIS3D"].toString();

    const QString loginEndPoint = mConfig["METEOR_LOGIN_ENDPOINT"].toString();
    const QString logoutEndPoint = mConfig["METEOR_LOGOUT_ENDPOINT"].toString();
    const QString cellTypesEndPoint = mConfig["METEOR_CELLTYPES_ENDPOINT"].toString();
    const QString columnsEndPoint = mConfig["METEOR_COLUMNS_ENDPOINT"].toString();
    const QString regionsEndPoint = mConfig["METEOR_REGIONS_ENDPOINT"].toString();
    const QString laminarLocationsEndPoint = mConfig["METEOR_LAMINARLOCATIONS_ENDPOINT"].toString();
    const QString networksEndPoint = mConfig["METEOR_NETWORKS_ENDPOINT"].toString();
    const QString postsynapticTargetsEndpoint = mConfig["METEOR_POSTSYNAPTICTARGETS_ENDPOINT"].toString();

    mLoginUrl = baseUrl + loginEndPoint;
    mLogoutUrl = baseUrl + logoutEndPoint;
    const QString cellTypesUrl = baseUrl + cellTypesEndPoint;
    const QString columnsUrl = baseUrl + columnsEndPoint;
    const QString regionsUrl = baseUrl + regionsEndPoint;
    const QString laminarLocationsUrl = baseUrl + laminarLocationsEndPoint;
    const QString networksUrl = baseUrl + networksEndPoint;
    const QString postsynapticTargetsUrl = baseUrl + postsynapticTargetsEndpoint;

    mDataRoot = QueryHelpers::getPrimaryDatasetRoot(config);
    mNetwork.setDataRoot(mDataRoot);
    mNetwork.loadFilesForQuery();

    mAuthInfo = QueryHelpers::login(mLoginUrl,
                                    mConfig["WORKER_USERNAME"].toString(),
                                    mConfig["WORKER_PASSWORD"].toString(),
                                    mNetworkManager,
                                    mConfig);

    mPendingRequestIds.append("CellTypes");
    mPendingRequestIds.append("Columns");
    mPendingRequestIds.append("Regions");
    mPendingRequestIds.append("LaminarLocations");
    mPendingRequestIds.append("Networks");
    mPendingRequestIds.append("PostsynapticTargets");

    QNetworkRequest ctRequest;
    ctRequest.setUrl(cellTypesUrl);
    ctRequest.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
    ctRequest.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
    ctRequest.setAttribute(QNetworkRequest::User, QVariant("CellTypes"));
    ctRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QueryHelpers::setAuthorization(mConfig, ctRequest);
    QJsonArray ctData = getCellTypesAsJson(mNetwork);
    QJsonDocument ctDoc(ctData);
    QString ctPostData(ctDoc.toJson());

    QNetworkRequest colRequest;
    colRequest.setUrl(columnsUrl);
    colRequest.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
    colRequest.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
    colRequest.setAttribute(QNetworkRequest::User, QVariant("Columns"));
    colRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QueryHelpers::setAuthorization(mConfig, colRequest);
    QJsonArray colData = getColumnsAsJson(mNetwork);
    QJsonDocument colDoc(colData);
    QString colPostData(colDoc.toJson());

    QNetworkRequest regRequest;
    regRequest.setUrl(regionsUrl);
    regRequest.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
    regRequest.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
    regRequest.setAttribute(QNetworkRequest::User, QVariant("Regions"));
    regRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QueryHelpers::setAuthorization(mConfig, regRequest);
    QJsonArray regData = getRegionsAsJson(mNetwork);
    QJsonDocument regDoc(regData);
    QString regPostData(regDoc.toJson());

    QNetworkRequest locRequest;
    locRequest.setUrl(laminarLocationsUrl);
    locRequest.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
    locRequest.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
    locRequest.setAttribute(QNetworkRequest::User, QVariant("LaminarLocations"));
    locRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QueryHelpers::setAuthorization(mConfig, locRequest);
    QJsonArray locData = getLaminarLocationsAsJson();
    QJsonDocument locDoc(locData);
    QString locPostData(locDoc.toJson());

    QNetworkRequest networksRequest;
    networksRequest.setUrl(networksUrl);
    networksRequest.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
    networksRequest.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
    networksRequest.setAttribute(QNetworkRequest::User, QVariant("Networks"));
    networksRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QueryHelpers::setAuthorization(mConfig, networksRequest);
    QJsonArray networksData = QueryHelpers::getDatasetsAsJson(config);
    QJsonDocument networksDoc(networksData);
    QString networksPostData(networksDoc.toJson());

    QNetworkRequest targetsRequest;
    targetsRequest.setUrl(postsynapticTargetsUrl);
    targetsRequest.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
    targetsRequest.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
    targetsRequest.setAttribute(QNetworkRequest::User, QVariant("PostsynapticTargets"));
    targetsRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QueryHelpers::setAuthorization(mConfig, targetsRequest);
    QJsonArray tarData = getPostsynapticTargetsAsJson();
    QJsonDocument targetsDoc(tarData);
    QString targetsPostData(targetsDoc.toJson());

    connect(&mNetworkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyGetQueryFinished(QNetworkReply*)));
    mNetworkManager.put(ctRequest, ctPostData.toLocal8Bit());
    mNetworkManager.put(colRequest, colPostData.toLocal8Bit());
    mNetworkManager.put(regRequest, regPostData.toLocal8Bit());
    mNetworkManager.put(locRequest, locPostData.toLocal8Bit());
    mNetworkManager.put(networksRequest, networksPostData.toLocal8Bit());
    mNetworkManager.put(targetsRequest, targetsPostData.toLocal8Bit());
}

void
DataUploadHandler::replyGetQueryFinished(QNetworkReply* reply)
{
    QNetworkReply::NetworkError error = reply->error();
    const QString requestId = reply->request().attribute(QNetworkRequest::User).toString();
    if (error == QNetworkReply::NoError)
    {
        if (requestId == "CellTypes" ||
            requestId == "Columns" ||
            requestId == "Regions" ||
            requestId == "LaminarLocations" ||
            requestId == "Networks" ||
            requestId == "PostsynapticTargets" ||
            requestId == "Update")
        {
            if(requestId == "Update"){
                qDebug() << "[*] Successful upload of" << mUploadedFile;
            } else {
                qDebug() << "[*] Successful upload of" << requestId;
            }            
            reply->deleteLater();
            mPendingRequestIds.removeOne(requestId);
            if (mPendingRequestIds.isEmpty())
            {
                QueryHelpers::logout(mLogoutUrl,
                                     mAuthInfo,
                                     mNetworkManager,
                                     mConfig);
                emit completedProcessing();
            }
        }
        else
        {
            return;
        }
    }
    else
    {
        qDebug() << "[-] Error uploading data:" << requestId;
        qDebug() << reply->errorString();
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404)
        {
            qDebug() << QString(reply->readAll().replace("\"", ""));
        }
        reply->deleteLater();
        QCoreApplication::exit(1);
    }
}

void
DataUploadHandler::uploadFile(const QJsonObject& config, const QString& queryId, const QString& filename)
{
    mConfig = config;

    const QString baseUrl = mConfig["METEOR_URL_CIS3D"].toString();

    const QString loginEndPoint = mConfig["METEOR_LOGIN_ENDPOINT"].toString();
    const QString logoutEndPoint = mConfig["METEOR_LOGOUT_ENDPOINT"].toString();
    const QString updateEndpoint = mConfig["METEOR_EVALUATIONQUERY_ENDPOINT"].toString();

    mLoginUrl = baseUrl + loginEndPoint;
    mLogoutUrl = baseUrl + logoutEndPoint;
    const QString updateUrl = baseUrl + updateEndpoint + queryId;

    mAuthInfo = QueryHelpers::login(mLoginUrl,
                                    mConfig["WORKER_USERNAME"].toString(),
                                    mConfig["WORKER_PASSWORD"].toString(),
                                    mNetworkManager,
                                    mConfig);

    mPendingRequestIds.append("Update");

    QNetworkRequest updateRequest;
    updateRequest.setUrl(updateUrl);
    updateRequest.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
    updateRequest.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
    updateRequest.setAttribute(QNetworkRequest::User, QVariant("Update"));
    updateRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QueryHelpers::setAuthorization(mConfig, updateRequest);
    mUploadedFile = filename;
    QJsonObject fileData = UtilIO::parseSpecFile(filename);
    QJsonDocument doc(fileData);
    QString updateData(doc.toJson());

    connect(&mNetworkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyGetQueryFinished(QNetworkReply*)));
    mNetworkManager.put(updateRequest, updateData.toLocal8Bit());    
}