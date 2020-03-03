#ifndef INNERVATIONMATRIX_H
#define INNERVATIONMATRIX_H

#include "CIS3DNetworkProps.h"
#include "RandomGenerator.h"
#include "CIS3DConstantsHelpers.h"
#include <set>
#include <map>

class CacheEntry
{
public:
    CacheEntry(int preId);
    void load(QString filePath, CIS3D::Structure target);
    float getValue(int postId, unsigned long long currentHit);
    int getPreId();
    unsigned long long getHits() const;

private:
    int mPreId;
    std::map<int, float> mInnervation;
    unsigned long long mHits;
};

/**
    Represents the innervation matrix. Provides acces to innervation values,
    which are internally stored in multiple files. 
*/
class InnervationMatrix
{

public:
    /**
        Constructor.
        @param networkProps The model data.
    */
    InnervationMatrix(const NetworkProps& networkProps);

    /**
        Destructor.
    */
    ~InnervationMatrix();

    /**
        Retrieves innervation between the specified neurons.
        @param pre The presynaptic neuron ID.
        @param post The postsynaptic neuron ID.
        @return The innervation from presynaptic to postsynaptic neuron.
    */
    float getValue(int preID, int postID, CIS3D::Structure target);

    void clearCache(std::map<int, CacheEntry*>& cache);
    CacheEntry* getEntry(int preId, CIS3D::Structure target);
    CacheEntry* getOrLoad(std::map<int, CacheEntry*>& cache, int preId, CIS3D::Structure target);
    void pruneCache(std::map<int, CacheEntry*>& cache);

private:    

    const NetworkProps& mNetwork;
    unsigned int mCacheLimit;
    QList<int> mOriginalPreIdsA;
    QList<int> mOriginalPreIdsB;
    QList<int> mOriginalPreIdsC;
    RandomGenerator mRandomGenerator;
    std::map<int, CacheEntry*> mInnervationAll;
    std::map<int, CacheEntry*> mInnervationBasal;
    std::map<int, CacheEntry*> mInnervationApical;
    std::set<int> mPreIds;
    unsigned long long mCurrentHit;
};

#endif // INNERVATIONMATRIX_H
