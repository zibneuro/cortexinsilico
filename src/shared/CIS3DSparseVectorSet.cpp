#include "CIS3DSparseVectorSet.h"
#include <stdexcept>
#include <stdio.h>
#include <QFile>
#include <QDataStream>
#include <QIODevice>
#include <QDebug>
#include <QDir>

const float SparseVectorSet::DEFAULT_VALUE = 0.0f;

/**
    Constructor.
*/
SparseVectorSet::SparseVectorSet()
{
}

/**
    Adds a new sparse vector with the specified ID.
    @param id The ID of the sparse vector.
    @throws runtime_error if the ID already exists.
*/
void SparseVectorSet::addVector(const int id) {
    if (mVectors.contains(id)) {
        throw std::runtime_error("Id already exists");
    }
    mVectors.insert(id, SparseVector());
}

/**
    Sets a value at the specified IDs.
    @param vectorId The ID of the sparse vector.
    @param entryId The ID of the entry in the sparse vector.
    @throws runtime_error if the vector ID does not exist.
*/
void SparseVectorSet::setValue(const int vectorId, const int entryId, const float value) {
    VectorHash::Iterator it = mVectors.find(vectorId);
    if (it == mVectors.end()) {
        const QString msg = QString("SparseVectorSet::setValue: Vector %1 does not exist").arg(vectorId);
        throw std::runtime_error(qPrintable(msg));
    }
    SparseVector& v = it.value();
    v.insert(entryId, value);
}

/**
    Retrieves a value at the specified IDs.
    @param vectorId The ID of the sparse vector.
    @param entryId The ID of the entry in the sparse vector.
    @return The value if the entry ID exists, DEFAULT_VALUE otherwise.
    @throws runtime_error if the vector ID does not exist.
*/
float SparseVectorSet::getValue(const int vectorId, const int entryId) const {
    VectorHash::ConstIterator it = mVectors.constFind(vectorId);
    if (it == mVectors.constEnd()) {
        const QString msg = QString("SparseVectorSet::getValue: Vector %1 does not exist").arg(vectorId);
        throw std::runtime_error(qPrintable(msg));
    }
    const SparseVector& v = it.value();
    return v.value(entryId, DEFAULT_VALUE);
}

/**
    Returns the number of defined sparse vectors.
    @return The number of vectors.
*/
int SparseVectorSet::getNumberOfVectors() const {
    return mVectors.size();
}

/**
    Returns the number of entries for the specified sparse vector.
    @param vectorId The ID of the sparse vector.
    @return The number of entries.
    @throws runtime_error if the vector ID does not exist.
*/
int SparseVectorSet::getNumberOfEntries(const int vectorId) const {
    VectorHash::ConstIterator it = mVectors.constFind(vectorId);
    if (it == mVectors.constEnd()) {
        const QString msg = QString("SparseVectorSet::getNumberOfEntries: Vector %1 does not exist").arg(vectorId);
        throw std::runtime_error(qPrintable(msg));
    }
    return it.value().size();
}

/**
    Returns the IDs of all sparse vectors.
    @return The IDs.
*/
QList<int> SparseVectorSet::getVectorIds() const
{
    return mVectors.keys();
}

/**
    Returns the entry IDs for the specified sparse vector.
    @param vectorId The ID of the sparse vector.
    @return The entry IDs.
    @throws runtime_error if the vector ID does not exist.
*/
QList<int> SparseVectorSet::getEntryIds(const int vectorId) const
{
    VectorHash::ConstIterator it = mVectors.constFind(vectorId);
    if (it == mVectors.constEnd()) {
        const QString msg = QString("SparseVectorSet::getEntryIds: Vector %1 does not exist").arg(vectorId);
        throw std::runtime_error(qPrintable(msg));
    }
    const SparseVector& v = it.value();
    return v.keys();
}

/**
    Saves a SparseVectorSet to file.
    @param vs The SparseVectorSet.
    @param fileName The name of the file.
    @return 1 if successfull, 0 otherwise.
*/
int SparseVectorSet::save(const SparseVectorSet* vs, const QString &fileName) {
    QFileInfo fi(fileName);
    QDir dir(fileName);
    dir.mkpath(fi.absoluteDir().absolutePath());

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        return 0;
    }

    QDataStream out(&file);
    writeToStream(vs, out);

    return 1;
}

/**
    Loads a SparseVectorSet from file.
    @param fileName The name of the file.
    @return The SparseVectorSet if successfull, 0 otherwise.
    @throws runtime_error if there is a version mismatch.
*/
SparseVectorSet *SparseVectorSet::load(const QString &fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << QString("Cannot open file for reading: %1").arg(fileName);
        return 0;
    }

    QDataStream in(&file);
    SparseVectorSet* vs = readFromStream(in);
    return vs;
}

/**
    Writes a SparseVectorSet to the specified data stream.
    @param out The data stream.
*/
void SparseVectorSet::writeToStream(const SparseVectorSet *fs, QDataStream &out) {
    const QString version = "0.1";
    out << version;
    out << fs->mVectors;
}

/**
    Reads a SparseVectorSet from the specified data stream.
    @param in The data stream.
    @return The SparseVectorSet.
    @throws runtime_error if there is a version mismatch.
*/
SparseVectorSet* SparseVectorSet::readFromStream(QDataStream &in) {
    QString version;
    in >> version;

    if (version != "0.1") {
        throw std::runtime_error("Invalid file format version");
    }

    SparseVectorSet* vs = new SparseVectorSet();
    in >> vs->mVectors;

    return vs;
}
