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
#include <set>
#include <map>
#include <vector>

class SpatialInnervationQueryHandler : public QueryHandler
{

public:
  SpatialInnervationQueryHandler();

private:
  struct DSCEntry
  {
    int postId;
    int subvolumeId;
    float dsc;
  };

  void doProcessQuery() override;

  QString getResultKey() override;

  QJsonObject createJsonResult(const QString &keyView,
                               const qint64 fileSizeBytes1,
                               const QString &keyData,
                               const qint64 fileSizeBytes2, int nVoxels);

  std::vector<DSCEntry> loadDSC(QString filename, std::set<int> &postIds, CIS3D::Structure postTarget);
  void saveDSC(QString filename, std::vector<DSCEntry> &data);
  void writeReadme(QString filename);

  void setFormulas() override;

  Statistics mStatistics;
  QString mTempFolder;
};

#endif // SELECTIONQUERYHANDLER_H
