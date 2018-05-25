#ifndef QUERYHELPERS_H
#define QUERYHELPERS_H

#include "CIS3DNeurons.h"
#include "CIS3DNetworkProps.h"
#include "CIS3DStatistics.h"
#include "Histogram.h"
#include "NetworkStatistic.h"
#include "Typedefs.h"
#include <QJsonArray>
#include <QNetworkAccessManager>


struct AuthInfo {
    QString userId;
    QString authToken;
};


namespace QueryHelpers {

    AuthInfo login(const QString url,
                   const QString& username,
                   const QString& password,
                   QNetworkAccessManager& networkManager);

    void logout(const QString& url,
                const AuthInfo& authInfo,
                QNetworkAccessManager& networkManager);

    int uploadToS3(const QString& key,
                   const QString& filename,
                   const QJsonObject& config);

    QString getDatasetPath(const QString& datasetShortName,
                           const QJsonObject& config);

    QString getPrimaryDatasetRoot(const QJsonObject& config);

    QJsonArray getDatasetsAsJson(const QJsonObject& config);
}

#endif // QUERYHELPERS_H
