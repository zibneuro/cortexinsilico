#ifndef INNERVATIONQUERYHANDLER_H
#define INNERVATIONQUERYHANDLER_H

#include "CIS3DNetworkProps.h"
#include "QueryHandler.h"
#include "QueryHelpers.h"
#include "NetworkStatistic.h"
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QtNetwork/QNetworkAccessManager>
#include "InnervationStatistic.h"

class InnervationQueryHandler : public QueryHandler
{
public:
    InnervationQueryHandler();

private:
    void doProcessQuery() override;
    QString getResultKey() override;
};

#endif