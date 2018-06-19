#include "FeatureReader.h"
#include <QBitArray>
#include <QFile>
#include <QStringList>
#include <QTextStream>

/*
    Loads a features.csv file.

    @param fileName The name of the file.
    @return A list of features.
*/
QList<Feature> FeatureReader::load(const QString fileName) {
    QFile file(fileName);
    QTextStream(stdout) << "[*] Reading features from " << fileName << "\n";

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString msg =
            QString("Error reading features file. Could not open file %1").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    const QChar sep = ',';
    QTextStream in(&file);

    int lineCount = 1;
    QString line = in.readLine();
    if (line.isNull()) {
        const QString msg = QString("Error reading features file %1. No content.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    QStringList parts = line.split(sep);
    if (parts.size() != 14 || parts[0] != "voxelID" || parts[1] != "voxelX" || parts[2] != "voxelY" ||
        parts[3] != "voxelZ" || parts[4] != "neuronID" || parts[5] != "morphologicalCellType" ||
        parts[6] != "functionalCellType" || parts[7] != "region" || parts[8] != "synapticSide" ||
        parts[9] != "pre" || parts[10] != "postExc" || parts[11] != "postAllExc" || parts[12] != "postInh" ||
        parts[13] != "postAllInh") {
        const QString msg =
            QString("Error reading features file %1. Invalid header columns.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    line = in.readLine();
    lineCount += 1;

    QList<Feature> features;

    while (!line.isNull()) {
        parts = line.split(sep);
        if (parts.size() != 14) {
            const QString msg =
                QString("Error reading features file %1. Invalid columns.").arg(fileName);
            throw std::runtime_error(qPrintable(msg));
        }

        Feature feature;
        feature.voxelID = parts[0].toInt();
        feature.voxelX = parts[1].toInt();
        feature.voxelY = parts[2].toInt();
        feature.voxelZ = parts[3].toInt();
        feature.neuronID = parts[4].toInt();
        feature.morphologicalCellType = parts[5];
        feature.functionalCellType = parts[6];
        feature.region = parts[7];
        feature.synapticSide = parts[8].toInt();
        feature.pre = parts[9].toFloat();
        feature.postExc = parts[10].toFloat();
        feature.postAllExc = parts[11].toFloat();
        feature.postInh = parts[12].toFloat();
        feature.postAllInh = parts[13].toFloat();        
        features.append(feature);

        line = in.readLine();
        lineCount += 1;
    }

    return features;
}