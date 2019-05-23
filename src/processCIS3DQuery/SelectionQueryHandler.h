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

    struct CellProps {
        QString cellType;
        QString region;
        int numNeurons;
        bool operator<(const CellProps& a) const{
            if(cellType != a.cellType){
                return cellType < a.cellType;
            } else {
                return region < a.region;
            }
        }
    };

private:

    void doProcessQuery() override;
    QString getResultKey() override;
};

#endif
