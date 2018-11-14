/*
    This tool is used for testing purposes to compare spatial innveration values.    
*/

#include <QDebug>
#include <QSet>
#include <QList>
#include <QString>
#include <math.h>
#include "CIS3DSparseVectorSet.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DSparseField.h"
#include "CIS3DStatistics.h"
#include "SparseVectorCache.h"
#include "Histogram.h"
#include "Typedefs.h"
#include "UtilIO.h"
#include <QChar>
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QDirIterator>

void
printUsage()
{
    qDebug() << "Usage: ./compareSpatialInnervation <innervationFile> <modelFolder>";
}

int
main(int argc, char* argv[])
{
    if (argc != 3)
    {
        printUsage();
        return 1;
    }

    const QString innervationFile = argv[1];
    const QString modelFolder = argv[2];
    //const float eps = atof(argv[3]);

    QFile file(innervationFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        const QString msg =
            QString("Error reading file. Could not open file %1")
                .arg(innervationFile);
        throw std::runtime_error(qPrintable(msg));
    }
    const QChar sep = ' ';
    QTextStream in(&file);

    QString line = in.readLine();
    std::map<std::pair<int, int>, float> pairs;
    while (!line.isNull())
    {
        QStringList parts = line.split(sep);
        int pre = parts[0].toInt();
        int post = parts[1].toInt();
        float innervation = (float)parts[2].toDouble();
        std::pair<int, int> p(pre, post);
        pairs[p] = innervation;
        line = in.readLine();
    }

    NetworkProps networkProps;
    networkProps.setDataRoot(modelFolder);
    networkProps.loadFilesForSynapseComputation();
    QDir innervationDataDir = CIS3D::getInnervationDataDir(modelFolder);
    SparseVectorCache cache;

    Statistics deviation;
    Histogram hist(0.001);

    qDebug() << "[*] Connected neuron pairs" << pairs.size();
    int count = 0;
    int lastPre = -1;
    for (auto it = pairs.begin(); it != pairs.end(); it++)
    {
        int preId = it->first.first;
        int postId = it->first.second;
        float innervation = it->second;

        if (lastPre != preId)
        {
            lastPre = preId;
            count++;
            qDebug() << count;
        }

        int cellTypeId = networkProps.neurons.getCellTypeId(postId);
        int regionId = networkProps.neurons.getRegionId(postId);
        QString cellTypeName = networkProps.cellTypes.getName(cellTypeId);
        QString regionName = networkProps.regions.getName(regionId);

        int preCellTypeId = networkProps.neurons.getCellTypeId(preId);
        int preRegionId = networkProps.neurons.getRegionId(preId);
        QString preCellTypeName = networkProps.cellTypes.getName(preCellTypeId);
        QString preRegionName = networkProps.regions.getName(preRegionId);

        QString postFile = CIS3D::getInnervationPostFileName(innervationDataDir,
                                                             regionName,
                                                             cellTypeName);
        if (!cache.contains(postFile))
        {
            SparseVectorSet* vectorSet = SparseVectorSet::load(postFile);
            cache.add(postFile, vectorSet);
        }
        SparseVectorSet* vectorSet = cache.get(postFile);
        float refInnervation = vectorSet->getValue(postId, preId);
        deviation.addSample(std::fabs(innervation - refInnervation));
        hist.addValue(std::fabs(innervation - refInnervation));
    }

    qDebug() << deviation.getMean() << deviation.getStandardDeviation() << deviation.getMinimum() << deviation.getMaximum();
    qDebug() << hist.createJson();
    return 0;
}
