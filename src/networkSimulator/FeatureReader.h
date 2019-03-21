#pragma once

#include <QDebug>
#include <QHash>
#include <QPair>
#include <QJsonArray>
#include <random>
#include "CIS3DAxonRedundancyMap.h"
#include "CIS3DBoundingBoxes.h"
#include "CIS3DCellTypes.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DNetworkProps.h"
#include "CIS3DNeurons.h"
#include "CIS3DRegions.h"
#include "CIS3DSparseField.h"
#include "CIS3DSparseVectorSet.h"
#include "CIS3DVec3.h"
#include "Histogram.h"
#include "NeuronSelection.h"
#include "SparseFieldCalculator.h"
#include "SparseVectorCache.h"
#include "Typedefs.h"
#include "Util.h"
#include "UtilIO.h"

/*
    This class reads in a features.csv file.
*/
class FeatureReader {
   public:
    /*
        Loads a features.csv file.

        @param filename The name of the file.
        @return A list of features.
    */
    QList<Feature> load(const QString fileName);
};