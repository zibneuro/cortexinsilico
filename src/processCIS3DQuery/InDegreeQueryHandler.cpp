#include "InDegreeQueryHandler.h"
#include "CIS3DCellTypes.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DNeurons.h"
#include "CIS3DRegions.h"
#include "CIS3DSparseVectorSet.h"
#include "Histogram.h"
#include "InDegreeStatistic.h"
#include "NeuronSelection.h"
#include "QueryHelpers.h"
#include "Util.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <stdexcept>

InDegreeQueryHandler::InDegreeQueryHandler() : QueryHandler() {}

void InDegreeQueryHandler::doProcessQuery() {
    InDegreeStatistic stat(mNetwork, mSampleNumber, mSampleSeed, mCalculator, this);
    stat.calculate(mSelection);
}

QString InDegreeQueryHandler::getResultKey() { return "inDegreeResult"; }
