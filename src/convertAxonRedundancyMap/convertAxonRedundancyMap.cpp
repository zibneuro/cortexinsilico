/*
    This tool converts a AxonRedundancyMap into a csv-File    

    ./convertAxonRedundancyMap  <inputFile> <outputFile>
*/

#include <QDebug>
#include <QProcess>
#include <QtCore>

#include "CIS3DAxonRedundancyMap.h"

void printUsage() {
    qDebug() << "Usage: ./convertAxonRedundancyMap <inputFile> <outputFile>";
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printUsage();
        return 1;
    }

    const QString inputFile = argv[1];
    const QString outputFile = argv[2];

    AxonRedundancyMap redundancyMap;
    redundancyMap.loadBinary(inputFile);
    redundancyMap.saveCSV(outputFile);

    return 0;
}
