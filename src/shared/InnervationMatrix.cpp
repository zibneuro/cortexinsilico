#include "InnervationMatrix.h"
#include <QDebug>
#include <QDir>
#include <QTextStream>
#include "CIS3DConstantsHelpers.h"
#include "Util.h"
#include <random>
#include <algorithm>

CacheEntry::CacheEntry(int preId)
    : mPreId(preId)
{
    mHits = 0;
}

void
CacheEntry::load(QString filePath)
{
    QFile innervationFile(filePath);
    if (innervationFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&innervationFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            line = line.trimmed();
            QStringList parts = line.split(" ");
            int postId = parts[0].toInt();
            float innervation = parts[1].toFloat();
            mInnervation[postId] = innervation;
        }
    }
    else
    {
        const QString msg =
            QString("Error reading innervation file. Could not open file %1").arg(filePath);
        throw std::runtime_error(qPrintable(msg));
    }
}

float
CacheEntry::getValue(int postId)
{
    mHits++;
    return mInnervation[postId];
}

int CacheEntry::getPreId(){
    return mPreId;
}

long
CacheEntry::getHits() const
{
    return mHits;
}

/**
    Constructor.
    @param networkProps The model data.
*/
InnervationMatrix::InnervationMatrix(const NetworkProps& networkProps)
    : mNetwork(networkProps)
    , mCacheLimit(100)
    , mRandomGenerator(-1){};

/**
    Destructor.
*/
InnervationMatrix::~InnervationMatrix()
{
    clearCache(mInnervationAll);
    clearCache(mInnervationApical);
    clearCache(mInnervationBasal);
}

/**
    Retrieves innervation between the specified neurons.
    @param pre The presynaptic neuron ID.
    @param post The postsynaptic neuron ID.
    @return The innervation from presynaptic to postsynaptic neuron.
*/
float
InnervationMatrix::getValue(int preId, int postId, int selectionIndex, CIS3D::Structure target)
{
    CIS3D::SynapticSide preSide = mNetwork.neurons.getSynapticSide(preId);
    CIS3D::SynapticSide postSide = mNetwork.neurons.getSynapticSide(postId);
    if (preSide == CIS3D::SynapticSide::POSTSYNAPTIC)
    {
        int mappedId = getRandomDuplicatedPreId(selectionIndex);
        if (mappedId != -1)
        {
            preId = mappedId;
        }
        else
        {
            return 0;
        }
    }
    if (postSide == CIS3D::SynapticSide::PRESYNAPTIC)
    {
        return 0;
    }

    return getValue(preId, postId, target);
}

float
InnervationMatrix::getValue(int preId, int postId, CIS3D::Structure target)
{
    const int mappedPreId = mNetwork.axonRedundancyMap.getNeuronIdToUse(preId);
    CacheEntry* entry = getEntry(mappedPreId, target);
    return entry->getValue(postId);
}

void
InnervationMatrix::setOriginalPreIds(QList<int> preIdsA, QList<int> preIdsB, QList<int> preIdsC)
{
    mOriginalPreIdsA = preIdsA;
    mOriginalPreIdsB = preIdsB;
    mOriginalPreIdsC = preIdsC;
    mRandomGenerator = RandomGenerator(-1);
}

int
InnervationMatrix::getRandomDuplicatedPreId(int selectionIndex)
{
    if (selectionIndex == 0)
    {
        return mRandomGenerator.getRandomEntry(mOriginalPreIdsA);
    }
    else if (selectionIndex == 1)
    {
        return mRandomGenerator.getRandomEntry(mOriginalPreIdsB);
    }
    else
    {
        return mRandomGenerator.getRandomEntry(mOriginalPreIdsC);
    }
}

void
InnervationMatrix::clearCache(std::map<int, CacheEntry*>& cache)
{
    for (auto it = cache.begin(); it != cache.end(); it++)
    {
        delete it->second;
    }
}

CacheEntry*
InnervationMatrix::getEntry(int preId, CIS3D::Structure target)
{
    if (target == CIS3D::DEND)
    {
        return getOrLoad(mInnervationAll, preId, target);
    }
    else if (target == CIS3D::BASAL)
    {
        return getOrLoad(mInnervationBasal, preId, target);
    }
    else if (target == CIS3D::APICAL)
    {
        return getOrLoad(mInnervationApical, preId, target);
    }
    else
    {
        const QString msg =
            QString("Invalid postsynatic target: %1").arg(target);
        throw std::runtime_error(qPrintable(msg));
    }
}

CacheEntry*
InnervationMatrix::getOrLoad(std::map<int, CacheEntry*>& cache, int preId, CIS3D::Structure target)
{
    if (cache.find(preId) != cache.end())
    {
        return cache[preId];
    }
    else
    {
        if (cache.size() >= mCacheLimit)
        {
            pruneCache(cache);
        }

        QString filename = QString("preNeuronID_%1_sum").arg(preId);
        QString folder = Util::getInnervationFolderName(target);
        QString filepath = QDir::cleanPath(mNetwork.dataRoot + QDir::separator() + folder + QDir::separator() + filename);

        CacheEntry* newEntry = new CacheEntry(preId);
        newEntry->load(filepath);
        cache[preId] = newEntry;
        return newEntry;
    }
}

void
InnervationMatrix::pruneCache(std::map<int, CacheEntry*>& cache)
{
    //qDebug() << "Prune" << cache.size();
    std::map<long, int> hitsPreId;
    for(auto it = cache.begin(); it != cache.end(); it++){
        hitsPreId[it->second->getHits()] = it->first;
    }

    int toDelete = hitsPreId.begin()->second;
    //qDebug() << "Delete" << toDelete << hitsPreId.begin()->first;
    delete cache[toDelete];
    cache.erase(toDelete);
    //qDebug() << "After delete" << cache.size();

}