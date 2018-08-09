/*
    This tool converts a SparseField into a csv-File with
    spatial coordinates. Usage:

    ./convertSparseField  <inputFile> <outputFile>
*/

#include <QDebug>
#include <QProcess>
#include <QtCore>

#include "CIS3DSparseField.h"

void printUsage() {
    qDebug() << "Usage: ./convertSparseField <inputFile> <outputFile>";
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printUsage();
        return 1;
    }

    const QString inputFile = argv[1];
    const QString outputFile = argv[2];

    SparseField* field = SparseField::load(inputFile);
    field->saveCSV(outputFile, true);

    return 0;
}
