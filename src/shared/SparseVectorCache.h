#ifndef SPARSEVECTORCACHE_H
#define SPARSEVECTORCACHE_H

#include <QString>
#include <QHash>
#include "CIS3DSparseVectorSet.h"

/**
    This class implements a simple filename-based cache such that
    SparseVectorSets do not have to be reloaded from disk.
*/
class SparseVectorCache {
public:
    /**
        Registers a new entry.
        @param name The filename.
        @param vectorSet The SparseVectorSet.
    */
    void add(const QString name, SparseVectorSet* vectorSet);

    /**
        Determines whether a certain entry is contained.
        @param name The filename.
        @return true if the entry exists.
    */
    bool contains(const QString name);

    /**
        Clears all entries in the cache and deletes the
        SparseVectorSets.
    */
    void clear();

    /**
        Retrieves a SparseVectorSet from the cache.
        @param name The filename.
        @return The SparseVectorSet.
        @throws runtime_error if the entry does not exist.
    */
    SparseVectorSet* get(const QString name);

private:
    QHash<QString, SparseVectorSet*> mCache;
};



#endif // SPARSEVECTORCACHE_H
