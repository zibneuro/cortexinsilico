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
CacheEntry::load(QString filePath, CIS3D::Structure target)
{
    QFile innervationFile(filePath);
    if (innervationFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&innervationFile);
        in.readLine();
        while (!in.atEnd())
        {
            QString line = in.readLine(); 
            line = line.trimmed();
            QStringList parts = line.split(",");
            int postId = parts[0].toInt();
            float soma = parts[1].toFloat();
            float apical = parts[2].toFloat();
            float basal = parts[3].toFloat();
            if(target ==  CIS3D::DEND){
                mInnervation[postId] = soma + apical + basal;
            } else if (target ==  CIS3D::APICAL) {
                mInnervation[postId] = apical;
            } else if (target ==  CIS3D::BASAL) {
                mInnervation[postId] = basal;
            }
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
CacheEntry::getValue(int postId, unsigned long long currentHit)
{
    mHits = currentHit;
    return mInnervation[postId];
}

int CacheEntry::getPreId(){
    return mPreId;
}

unsigned long long
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
    , mCacheLimit(1000)
    , mRandomGenerator(-1)
    , mCurrentHit(0){};

/**
    Destructor.
*/
InnervationMatrix::~InnervationMatrix()
{
    clearCache(mInnervationAll);
    clearCache(mInnervationApical);
    clearCache(mInnervationBasal);
}


float
InnervationMatrix::getValue(int preId, int postId, CIS3D::Structure target)
{
    mCurrentHit++;
    const int mappedPreId = mNetwork.axonRedundancyMap.getNeuronIdToUse(preId);
    CacheEntry* entry = getEntry(mappedPreId, target);
    return entry->getValue(postId, mCurrentHit);
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

        QString filename = QString("%1_agg_DSC.csv").arg(preId);
        QString filepath = mNetwork.networkRootDir.absoluteFilePath(QDir::cleanPath("DSC/" + filename));

        CacheEntry* newEntry = new CacheEntry(preId);
        newEntry->load(filepath, target);
        cache[preId] = newEntry;
        return newEntry;
    }
}

void
InnervationMatrix::pruneCache(std::map<int, CacheEntry*>& cache)
{
    //qDebug() << "Prune" << cache.size();
    std::map<unsigned long long, int> hitsPreId;
    for(auto it = cache.begin(); it != cache.end(); it++){
        hitsPreId[it->second->getHits()] = it->first;
    }

    int i = 0;
    for(auto it = hitsPreId.begin(); it != hitsPreId.end(); it++){
        if(i > 0.5 * static_cast<float>(cache.size())){
            break;
        }
        int toDelete = it->second;
        delete cache[toDelete];
        cache.erase(toDelete);
        i++;
    }
}
