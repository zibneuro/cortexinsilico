/*
    This tool creates a csv files from the innervation matrix that is strored
    as multiple files in the sparse vector format. Usage:

    ./convertInnervationToCSV  <data-dir> <output-dir> [<tar-output-dir>]

    <data_dir> The directory containing the innervation matrix files.
    <data_dir> The directory to which the csv-files are written.
    <data-output-dir> The directoy to which the compressed csv-files are written.
*/

#include <QDebug>
#include <QProcess>
#include <QtCore>

#include "CIS3DSparseVectorSet.h"

void printErrorAndExit(const std::runtime_error& e) {
    qDebug() << QString(e.what());
    qDebug() << "Aborting.";
    exit(1);
}

void printUsage() {
    qDebug() << "Usage: ./convertInnervationToCSV  <data-dir> <output-dir> [<tar-output-dir>]";
}

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        printUsage();
        return 1;
    }

    const QDir dataDir(argv[1]);
    const QDir innervationDir(dataDir);
    const QDir outputDir(argv[2]);

    const bool createTar = (argc == 4);
    QDir tarOutputDir;
    if (createTar) {
        tarOutputDir = QDir(argv[3]);
    }

    if (!dataDir.exists()) {
        std::runtime_error err("Data directory does not exist");
        printErrorAndExit(err);
    }

    if (!innervationDir.exists()) {
        qDebug() << innervationDir;
        std::runtime_error err("Innervation directory does not exist");
        printErrorAndExit(err);
    }

    if (!outputDir.exists()) {
        qDebug() << "[*] Creating output directory" << argv[2];
        outputDir.mkpath(argv[2]);
    }

    if (outputDir.exists() &&
        outputDir.entryList(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files).size() > 0) {
        std::runtime_error err("Output directory is not empty");
        printErrorAndExit(err);
    }

    if (createTar) {
        if (!tarOutputDir.exists()) {
            qDebug() << "[*] Creating tar output directory" << tarOutputDir.path();
            outputDir.mkpath(tarOutputDir.path());
        }

        if (tarOutputDir.exists() &&
            tarOutputDir.entryList(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files).size() > 0) {
            std::runtime_error err("Tar output directory is not empty");
            printErrorAndExit(err);
        }
    }

    int numInnervationFiles = 0;
    QDirIterator countIt(innervationDir.path(), QStringList() << "*.dat", QDir::Files,
                         QDirIterator::Subdirectories);
    while (countIt.hasNext()) {
        countIt.next();
        numInnervationFiles += 1;
    }

    int counter = 1;
    QDirIterator it(innervationDir.path(), QStringList() << "*.dat", QDir::Files,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        const QTime startTime = QTime::currentTime();
        qDebug() << "[*] Processing file:   " << counter << "/" << numInnervationFiles;

        const QString datFileName = it.next();
        const QString csvFileName = outputDir.filePath(QString::number(counter) + ".csv");

        QFile csv(csvFileName);
        if (!csv.open(QIODevice::WriteOnly | QIODevice::Text)) {
            const QString msg =
                QString("Error saving CSV file. Could not open file %1").arg(csvFileName);
            throw std::runtime_error(qPrintable(msg));
        }
        
        qDebug() << datFileName;
        SparseVectorSet* vectorSet = SparseVectorSet::load(datFileName);
        SparseVectorSet::saveCSV(vectorSet, csvFileName);
        delete vectorSet;

        
        if (createTar) {
            const QString tarFileName =
                tarOutputDir.filePath(outputDir.relativeFilePath(csvFileName + ".tar.gz"));
            qDebug() << "    Creating archive:  " << tarFileName;
            const QString tarDirToCreate = QFileInfo(tarFileName).dir().path();
            tarOutputDir.mkpath(tarDirToCreate);

            const QString cwd = QFileInfo(csvFileName).dir().path();
            QStringList args;
            args.append("czf");
            args.append(tarFileName);
            args.append("-C");
            args.append(cwd);
            args.append(QFileInfo(csvFileName).fileName());
            const int returnCode = QProcess::execute(QString("tar"), args);
            if (returnCode) {
                const QString msg = QString("Error creating tar file. Return code: %1. File: %2")
                                        .arg(returnCode)
                                        .arg(tarFileName);
                throw std::runtime_error(qPrintable(msg));
            }
        }

        counter += 1;

        const QTime endTime = QTime::currentTime();
        const int secs = startTime.secsTo(endTime);
        qDebug() << "    Time:              " << secs / 60 << "Min." << secs % 60 << "Secs.";
    }

    return 0;
}
