/*
    This tool can be used compute statistics offline, outside of the web
    application. Usage:

    ./computeStatistic <spec_file>

    <spec_file> Specification file that contains the data directory path
                and filter definitions.
*/

#include "UtilIO.h"
#include "MyStatistic.h"
#include "CIS3DNetworkProps.h"
#include "CIS3DCellTypes.h"
#include "CIS3DNeurons.h"
#include "CIS3DRegions.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DSparseVectorSet.h"
#include <QString>
#include <QDebug>
#include <QJsonObject>


void printUsage() {
    qDebug() << "Usage: ./computeStatistic <spec_file>";
}


int main(int argc, char* argv[])
{
    if (argc != 2) {
        printUsage();
        return 1;
    }

    // Parse spec file containing data path and filter definition
    const QString specPath = argv[1];
    QJsonObject spec = UtilIO::parseSpecFile(specPath);

    // Load raw data (network properties, precomputed innervation data)
    NetworkProps networkProps;
    networkProps.setDataRoot(spec["DATA_ROOT"].toString());
    networkProps.loadFilesForSynapseComputation();

    // Retrieve pre- and postsynaptic neurons as defined in specFile
    IdList preNeurons = UtilIO::getPreSynapticNeurons(spec, networkProps);
    IdList postNeurons = UtilIO::getPostSynapticNeuronIds(spec, networkProps);

    // Calculate statistic
    MyStatistic myStatistic(networkProps);
    myStatistic.calculate(preNeurons, postNeurons);

    qDebug() << "Innervation mean: " << myStatistic.getResult().getMean();

    return 0;
}
