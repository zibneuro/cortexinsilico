/*
    This tool can be used compute statistics offline, outside of the web
    application. Usage:

    ./computeStatistic <type> <spec_file>

    <type>      Type of statistic: INNERVATION | MOTIF

    <spec_file> Specification file that contains the data directory path
                and neuron filter definitions.
*/

#include <QDebug>
#include <QJsonObject>
#include <QString>
#include "CIS3DCellTypes.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DNetworkProps.h"
#include "CIS3DNeurons.h"
#include "CIS3DRegions.h"
#include "CIS3DSparseVectorSet.h"
#include "MyStatistic.h"
#include "TripletStatistic.h"
#include "UtilIO.h"


/*
    Reads the SAMPLE_SIZE property from the specification file.

    @param spec The specification file as json object.
    @return The SAMPLE_SIZE as int, 100 by default.
*/
int extractSampleSize(const QJsonObject spec) {    
    if (spec["SAMPLE_SIZE"] != QJsonValue::Undefined) {
        return spec["SAMPLE_SIZE"].toInt();        
    } else {
        return 100;
    }
}


void printUsage() {
    qDebug() << "Usage: ./computeStatistic <type> <spec_file>";
    qDebug();
    qDebug() << "<type>         Type of statistic: INNERVATION | MOTIF";
    qDebug() << "<spec_file>    Specifcation file.";
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printUsage();
        return 1;
    }

    // Read input parameters and spec file.
    const QString statType = argv[1];
    const QString specPath = argv[2];
    QJsonObject spec = UtilIO::parseSpecFile(specPath);

    // Load raw data (network properties, precomputed innervation data)
    NetworkProps networkProps;
    networkProps.setDataRoot(spec["DATA_ROOT"].toString());
    networkProps.loadFilesForSynapseComputation();

    // Calculate statistic
    NeuronSelection selection;    
    if (statType == "INNERVATION") {
        selection.setInnervationSelection(spec, networkProps);
        MyStatistic myStatistic(networkProps);
        myStatistic.calculate(selection);
        qDebug() << "Innervation mean: " << myStatistic.getResult().getMean();
    } else if (statType == "MOTIF") {
        NeuronSelection selection;
        selection.setTripletSelection(spec, networkProps);
        int sampleSize = extractSampleSize(spec);
        TripletStatistic statistic(networkProps, sampleSize, 1);
        statistic.calculate(selection);
    } else {
        printUsage();
        return 1;
    }

    return 0;
}
