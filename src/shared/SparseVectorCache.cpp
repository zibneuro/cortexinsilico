#include "SparseVectorCache.h"

/**
    Registers a new entry.
    @param name The filename.
    @param vectorSet The SparseVectorSet.
*/
void SparseVectorCache::add(const QString name, SparseVectorSet* vectorSet){
    mCache.insert(name,vectorSet);
}

/**
    Determines whether a certain entry is contained.
    @param name The filename.
    @return true if the entry exists.
*/
bool SparseVectorCache::contains(const QString name){
    return mCache.contains(name);
}

/**
    Clears all entries in the cache and deletes the
    SparseVectorSets.
*/
void SparseVectorCache::clear(){
    QList<SparseVectorSet*> values = mCache.values();
    for(int i=0; i<values.size(); i++){
        SparseVectorSet* s = values[i];
        delete s;
    }
}

/**
    Retrieves a SparseVectorSet from the cache.
    @param name The filename.
    @return The SparseVectorSet.
    @throws runtime_error if the entry does not exist.
*/
SparseVectorSet* SparseVectorCache::get(const QString name){
    if(mCache.contains(name)){
        return mCache[name];
    } else {
        const QString msg = QString("SparseVectorSet %1 not cached.").arg(name);
        throw std::runtime_error(qPrintable(msg));
    }
}
