#pragma once

#include <QHash>

/**
    Internal data representation:
    - Map indices of nonzero columns to SparseVector -> VectorHash
    - Map indices of nonzero rows to values -> SparseVector
*/
typedef QHash<int, float> SparseVector;
typedef QHash<int, SparseVector> VectorHash;

class QDataStream;

/**
    This class implements a memory efficient way to handle a sparse matrix
    of float values. It is used to store the innervation matrix.
*/
class SparseVectorSet {

public:

    /**
        Constructor.
    */
    SparseVectorSet();

    /**
        Adds a new sparse vector with the specified ID.
        @param id The ID of the sparse vector.
        @throws runtime_error if the ID already exists.
    */
    void addVector(const int id);

    /**
        Sets a value at the specified IDs.
        @param vectorId The ID of the sparse vector.
        @param entryId The ID of the entry in the sparse vector.
        @throws runtime_error if the vector ID does not exist.
    */
    void setValue(const int vectorId, const int entryId, const float value);

    /**
        Retrieves a value at the specified IDs.
        @param vectorId The ID of the sparse vector.
        @param entryId The ID of the entry in the sparse vector.
        @return The value if the entry ID exists, DEFAULT_VALUE otherwise.
        @throws runtime_error if the vector ID does not exist.
    */
    float getValue(const int vectorId, const int entryId) const;

    /**
        Returns the number of defined sparse vectors.
        @return The number of vectors.
    */
    int getNumberOfVectors() const;

    /**
        Returns the number of entries for the specified sparse vector.
        @param vectorId The ID of the sparse vector.
        @return The number of entries.
        @throws runtime_error if the vector ID does not exist.
    */
    int getNumberOfEntries(const int vectorId) const;

    /**
        Returns the IDs of all sparse vectors.
        @return The IDs.
    */
    QList<int> getVectorIds() const;

    /**
        Returns the entry IDs for the specified sparse vector.
        @param vectorId The ID of the sparse vector.
        @return The entry IDs.
        @throws runtime_error if the vector ID does not exist.
    */
    QList<int> getEntryIds(const int vectorId) const;

    /**
        Saves a SparseVectorSet to file.
        @param vs The SparseVectorSet.
        @param fileName The name of the file.
        @return 1 if successfull, 0 otherwise.
    */
    static int save(const SparseVectorSet* vs, const QString &fileName);

    /**
        Loads a SparseVectorSet from file.
        @param fileName The name of the file.
        @return The SparseVectorSet if successfull, 0 otherwise.
        @throws runtime_error if there is a version mismatch.
    */
    static SparseVectorSet* load(const QString &fileName);

    /**
        Writes a SparseVectorSet to the specified data stream.
        @param out The data stream.
    */
    static void writeToStream(const SparseVectorSet *fs, QDataStream &out);

    /**
        Reads a SparseVectorSet from the specified data stream.
        @param in The data stream.
        @return The SparseVectorSet.
        @throws runtime_error if there is a version mismatch.
    */
    static SparseVectorSet* readFromStream(QDataStream &in);

private:
    VectorHash mVectors;
    static const float DEFAULT_VALUE;


};
