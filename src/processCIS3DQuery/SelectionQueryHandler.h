#ifndef SELECTIONQUERYHANDLER_H
#define SELECTIONQUERYHANDLER_H

#include "CIS3DNetworkProps.h"
#include "QueryHandler.h"
#include "QueryHelpers.h"
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QtNetwork/QNetworkAccessManager>

class SelectionQueryHandler : public QueryHandler
{
public:
    SelectionQueryHandler();

private:
    void doProcessQuery() override;
    QString getResultKey() override;
};

#endif