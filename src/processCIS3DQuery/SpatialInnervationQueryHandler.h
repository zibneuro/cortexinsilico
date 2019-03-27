#ifndef SPATIALINNERVATIONQUERYHANDLER_H
#define SPATIALINNERVATIONQUERYHANDLER_H

#include "CIS3DNetworkProps.h"
#include "CIS3DStatistics.h"
#include "FeatureProvider.h"
#include "QueryHelpers.h"
#include "QueryHandler.h"
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QtNetwork/QNetworkAccessManager>

class SpatialInnervationQueryHandler : public QueryHandler {

public:
  SpatialInnervationQueryHandler();

private:
   void doProcessQuery() override;

  QString getResultKey() override;

  QJsonObject createJsonResult(const QString &keyView,
                               const qint64 fileSizeBytes1,
                               const QString &keyData,
                               const qint64 fileSizeBytes2, int nVoxels);

  Statistics mStatistics;
  QString mTempFolder;
};

#endif // SELECTIONQUERYHANDLER_H
