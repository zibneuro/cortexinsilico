#include "InnervationQueryHandler.h"

InnervationQueryHandler::InnervationQueryHandler()
    : QueryHandler()
{
}

void
InnervationQueryHandler::doProcessQuery()
{
    InnervationStatistic innervation(mNetwork, mCalculator, this);
    innervation.calculate(mSelection);
};

QString
InnervationQueryHandler::getResultKey()
{
    return "innervationResult";
}