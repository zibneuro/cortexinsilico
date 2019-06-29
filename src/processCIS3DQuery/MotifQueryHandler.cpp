#include "MotifQueryHandler.h"
#include "CIS3DCellTypes.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DNeurons.h"
#include "CIS3DRegions.h"
#include "CIS3DSparseVectorSet.h"
#include "Histogram.h"
#include "NeuronSelection.h"
#include "QueryHelpers.h"
#include "TripletStatistic.h"
#include "Util.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <stdexcept>

MotifQueryHandler::MotifQueryHandler() : QueryHandler() {}

void MotifQueryHandler::doProcessQuery() {
  TripletStatistic statistic(mNetwork, mSampleNumber, mSampleSeed, mCalculator, this);  
  statistic.calculate(mSelection);
}

QString MotifQueryHandler::getResultKey() { return "motifResult"; }
