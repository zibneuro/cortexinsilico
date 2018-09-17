#include "NetworkDataUploadHandler.h"
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


NetworkDataUploadHandler::NetworkDataUploadHandler(QObject* parent)
    : QObject(parent)
{
}


QJsonArray getCellTypesAsJson(const NetworkProps& network) {
    QJsonArray arr;

    const QList<int> ids = network.cellTypes.getAllCellTypeIds(true);
    for (int i=0; i<ids.size(); ++i) {
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


QJsonArray getColumnsAsJson(const NetworkProps& network) {
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

    for (int i=0; i<columnRegionIds.size(); ++i) {
        const int id = columnRegionIds[i];
        QJsonObject obj;
        obj.insert("ID", id);
        obj.insert("name", network.regions.getName(id));
        arr.append(obj);
    }

    return arr;
}


QJsonArray getRegionsAsJson(const NetworkProps& network) {
    QJsonArray arr;

    const QList<int> ids = network.regions.getAllRegionIds();
    for (int i=0; i<ids.size(); ++i) {
        const int id = ids[i];
        QJsonObject obj;
        obj.insert("ID", id);
        obj.insert("name", network.regions.getName(id));
        arr.append(obj);
    }

    return arr;
}


QJsonArray getLaminarLocationsAsJson() {
    QJsonArray arr;

    QJsonObject supraObj;
    supraObj.insert("ID", int(CIS3D::SUPRAGRANULAR));
    supraObj.insert("name", "Supragranular");
    arr.append(supraObj);

    QJsonObject granularObj;
    granularObj.insert("ID", int(CIS3D::GRANULAR));
    granularObj.insert("name", "Granular");
    arr.append(granularObj);

    QJsonObject infraObj;
    infraObj.insert("ID", int(CIS3D::INFRAGRANULAR));
    infraObj.insert("name", "Infragranular");
    arr.append(infraObj);

    return arr;
}


void NetworkDataUploadHandler::process(const QJsonObject& config)
{
    mConfig = config;

    const QString baseUrl = mConfig["METEOR_URL_CIS3D"].toString();

    const QString loginEndPoint = mConfig["METEOR_LOGIN_ENDPOINT"].toString();
    const QString logoutEndPoint = mConfig["METEOR_LOGOUT_ENDPOINT"].toString();
    const QString cellTypesEndPoint = mConfig["METEOR_CELLTYPES_ENDPOINT"].toString();
    const QString columnsEndPoint = mConfig["METEOR_COLUMNS_ENDPOINT"].toString();
    const QString regionsEndPoint = mConfig["METEOR_REGIONS_ENDPOINT"].toString();
    const QString laminarLocationsEndPoint = mConfig["METEOR_LAMINARLOCATIONS_ENDPOINT"].toString();
    const QString networksEndPoint = mConfig["METEOR_NETWORKS_ENDPOINT"].toString();

    mLoginUrl  = baseUrl + loginEndPoint;
    mLogoutUrl = baseUrl + logoutEndPoint;
    const QString cellTypesUrl = baseUrl + cellTypesEndPoint;
    const QString columnsUrl = baseUrl + columnsEndPoint;
    const QString regionsUrl = baseUrl + regionsEndPoint;
    const QString laminarLocationsUrl = baseUrl + laminarLocationsEndPoint;
    const QString networksUrl = baseUrl + networksEndPoint;

    mDataRoot = QueryHelpers::getPrimaryDatasetRoot(config);
    mNetwork.setDataRoot(mDataRoot);
    mNetwork.loadFilesForQuery();

    mAuthInfo = QueryHelpers::login(mLoginUrl,
                                    mConfig["WORKER_USERNAME"].toString(),
                                    mConfig["WORKER_PASSWORD"].toString(),
                                    mNetworkManager);

    mPendingRequestIds.append("CellTypes");
    mPendingRequestIds.append("Columns");
    mPendingRequestIds.append("Regions");
    mPendingRequestIds.append("LaminarLocations");
    mPendingRequestIds.append("Networks");

    QNetworkRequest ctRequest;
    ctRequest.setUrl(cellTypesUrl);
    ctRequest.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
    ctRequest.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
    ctRequest.setAttribute(QNetworkRequest::User, QVariant("CellTypes"));
    ctRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonArray ctData = getCellTypesAsJson(mNetwork);
    QJsonDocument ctDoc(ctData);
    QString ctPostData(ctDoc.toJson());

    QNetworkRequest colRequest;
    colRequest.setUrl(columnsUrl);
    colRequest.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
    colRequest.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
    colRequest.setAttribute(QNetworkRequest::User, QVariant("Columns"));
    colRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonArray colData = getColumnsAsJson(mNetwork);
    QJsonDocument colDoc(colData);
    QString colPostData(colDoc.toJson());

    QNetworkRequest regRequest;
    regRequest.setUrl(regionsUrl);
    regRequest.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
    regRequest.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
    regRequest.setAttribute(QNetworkRequest::User, QVariant("Regions"));
    regRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonArray regData = getRegionsAsJson(mNetwork);
    QJsonDocument regDoc(regData);
    QString regPostData(regDoc.toJson());

    QNetworkRequest locRequest;
    locRequest.setUrl(laminarLocationsUrl);
    locRequest.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
    locRequest.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
    locRequest.setAttribute(QNetworkRequest::User, QVariant("LaminarLocations"));
    locRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonArray locData = getLaminarLocationsAsJson();
    QJsonDocument locDoc(locData);
    QString locPostData(locDoc.toJson());

    QNetworkRequest networksRequest;
    networksRequest.setUrl(networksUrl);
    networksRequest.setRawHeader(QByteArray("X-User-Id"), mAuthInfo.userId.toLocal8Bit());
    networksRequest.setRawHeader(QByteArray("X-Auth-Token"), mAuthInfo.authToken.toLocal8Bit());
    networksRequest.setAttribute(QNetworkRequest::User, QVariant("Networks"));
    networksRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonArray networksData = QueryHelpers::getDatasetsAsJson(config);
    QJsonDocument networksDoc(networksData);
    QString networksPostData(networksDoc.toJson());

    connect(&mNetworkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyGetQueryFinished(QNetworkReply*)));
    mNetworkManager.put(ctRequest,  ctPostData.toLocal8Bit());
    mNetworkManager.put(colRequest, colPostData.toLocal8Bit());
    mNetworkManager.put(regRequest, regPostData.toLocal8Bit());
    mNetworkManager.put(locRequest, locPostData.toLocal8Bit());
    mNetworkManager.put(networksRequest, networksPostData.toLocal8Bit());
}


void NetworkDataUploadHandler::replyGetQueryFinished(QNetworkReply* reply) {
    QNetworkReply::NetworkError error = reply->error();
    const QString requestId = reply->request().attribute(QNetworkRequest::User).toString();
    if (error == QNetworkReply::NoError) {
        if (requestId == "CellTypes" ||
            requestId == "Columns" ||
            requestId == "Regions" ||
            requestId == "LaminarLocations" ||
            requestId == "Networks")
        {
            qDebug() << "[*] Successful upload of" << requestId;
            reply->deleteLater();
            mPendingRequestIds.removeOne(requestId);
            if (mPendingRequestIds.isEmpty()) {
                QueryHelpers::logout(mLogoutUrl,
                                     mAuthInfo,
                                     mNetworkManager);
                emit completedProcessing();
            }
        }
        else {
            return;
        }
    }
    else {
        qDebug() << "[-] Error uploading data:" << requestId;
        qDebug() << reply->errorString();
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404) {
            qDebug() << QString(reply->readAll().replace("\"", ""));
        }
        reply->deleteLater();
        QCoreApplication::exit(1);
    }
}
