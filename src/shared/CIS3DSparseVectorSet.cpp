#include "CIS3DSparseVectorSet.h"
#include <stdexcept>
#include <stdio.h>
#include <QFile>
#include <QDataStream>
#include <QIODevice>
#include <QDebug>
#include <QDir>

const float SparseVectorSet::DEFAULT_VALUE = 0.0f;


SparseVectorSet::SparseVectorSet()
{
}


void SparseVectorSet::addVector(const int id) {
    if (mVectors.contains(id)) {
        throw std::runtime_error("Id already exists");
    }
    mVectors.insert(id, SparseVector());
}


void SparseVectorSet::setValue(const int vectorId, const int entryId, const float value) {
    VectorHash::Iterator it = mVectors.find(vectorId);
    if (it == mVectors.end()) {
        const QString msg = QString("SparseVectorSet::setValue: Vector %1 does not exist").arg(vectorId);
        throw std::runtime_error(qPrintable(msg));
    }
    SparseVector& v = it.value();
    v.insert(entryId, value);
}


float SparseVectorSet::getValue(const int vectorId, const int entryId) const {
    VectorHash::ConstIterator it = mVectors.constFind(vectorId);
    if (it == mVectors.constEnd()) {
        const QString msg = QString("SparseVectorSet::getValue: Vector %1 does not exist").arg(vectorId);
        throw std::runtime_error(qPrintable(msg));
    }
    const SparseVector& v = it.value();
    return v.value(entryId, DEFAULT_VALUE);
}


int SparseVectorSet::getNumberOfVectors() const {
    return mVectors.size();
}


int SparseVectorSet::getNumberOfEntries(const int vectorId) const {
    VectorHash::ConstIterator it = mVectors.constFind(vectorId);
    if (it == mVectors.constEnd()) {
        const QString msg = QString("SparseVectorSet::getNumberOfEntries: Vector %1 does not exist").arg(vectorId);
        throw std::runtime_error(qPrintable(msg));
    }
    return it.value().size();
}


QList<int> SparseVectorSet::getVectorIds() const
{
    return mVectors.keys();
}


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


void SparseVectorSet::writeToStream(const SparseVectorSet *fs, QDataStream &out) {
    const QString version = "0.1";
    out << version;
    out << fs->mVectors;
}


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
