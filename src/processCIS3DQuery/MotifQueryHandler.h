#ifndef MOTIFQUERYHANDLER_H
#define MOTIFQUERYHANDLER_H

#include "CIS3DNetworkProps.h"
#include "NetworkStatistic.h"
#include "QueryHelpers.h"
#include <QJsonObject>
#include <QObject>
#include <QString>
#include "QueryHandler.h"

class MotifQueryHandler : public QueryHandler {

public:
  MotifQueryHandler();

private:
  void doProcessQuery() override;
  QString getResultKey() override;
};

#endif // MOTIFQUERYHANDLER_H
