#ifndef INDEGREEQUERYHANDLER_H
#define INDEGREEQUERYHANDLER_H

#include "CIS3DNetworkProps.h"
#include "NetworkStatistic.h"
#include "QueryHandler.h"
#include "QueryHelpers.h"
#include <QJsonObject>
#include <QString>

class InDegreeQueryHandler : public QueryHandler {

public:
  InDegreeQueryHandler();

private:
  void doProcessQuery() override;
  QString getResultKey() override;
};

#endif // INDEGREEQUERYHANDLER_H
