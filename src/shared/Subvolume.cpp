#include "Subvolume.h"
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QTextStream>

void Subvolume::load(QString dataDir, int sid, std::set<int> &whitelistPre, std::set<int> &whitelistPreMapped, std::set<int> &whitelistPost)
{
    SID = sid;
    QDir dataFolder(dataDir);

    presynaptic.clear();
    postsynaptic.clear();
    cellbodies.clear();

    loadPresynaptic(dataFolder.filePath(QString::number(SID) + "_presynaptic.csv"), whitelistPreMapped);
    loadPostsynaptic(dataFolder.filePath(QString::number(SID) + "_postsynaptic.csv"), whitelistPost);
    loadCellbodies(dataFolder.filePath(QString::number(SID) + "_cellbodies.csv"), whitelistPre, whitelistPost);
}

void Subvolume::loadPresynaptic(QString filename, std::set<int> &whitelistPre)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

    QTextStream in(&file);
    QString line = in.readLine();
    line = in.readLine();
    while (!line.isNull())
    {
        QStringList parts = line.split(',');

        int NID = parts[0].toInt();
        if (whitelistPre.find(NID) != whitelistPre.end())
        {
            PropsPresynaptic props;
            props.length = parts[1].toFloat();
            props.boutons = parts[2].toFloat();
            props.branchlets = parts[3].toInt();
            presynaptic[NID] = props;
        }

        line = in.readLine();
    }
}

void Subvolume::loadPostsynaptic(QString filename, std::set<int> &whitelistPost)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

    QTextStream in(&file);
    QString line = in.readLine();
    line = in.readLine();
    while (!line.isNull())
    {
        QStringList parts = line.split(',');

        int NID = parts[0].toInt();
        if (whitelistPost.find(NID) != whitelistPost.end())
        {
            PropsPostsynaptic props;
            props.length = parts[1].toFloat();
            props.pstExc = parts[2].toFloat();
            props.pstInh = parts[3].toFloat();
            props.branchlets = parts[4].toInt();
            postsynaptic[NID] = props;
        }

        line = in.readLine();
    }
}

void Subvolume::loadCellbodies(QString filename, std::set<int> &whitelistPre, std::set<int> &whitelistPost)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
       return;
    }

    QTextStream in(&file);
    QString line = in.readLine();
    line = in.readLine();
    while (!line.isNull())
    {
        QStringList parts = line.split(',');

        int NID = parts[0].toInt();
        if (whitelistPre.find(NID) != whitelistPre.end() || whitelistPost.find(NID) != whitelistPost.end())
        {
            cellbodies.insert(NID);
        }

        line = in.readLine();
    }
}