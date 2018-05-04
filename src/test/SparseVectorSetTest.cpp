#include "SparseVectorSetTest.h"
#include "CIS3DSparseVectorSet.h"
#include <QTest>
#include <QDebug>
#include <stdexcept>
#include <QScopedPointer>
#include <QDataStream>
#include <QBuffer>


//QTEST_APPLESS_MAIN(SparseVectorSetTest)

void SparseVectorSetTest::testAddingVector() {
    SparseVectorSet vectorSet;
    vectorSet.addVector(54);
    vectorSet.addVector(0);
    QVERIFY(2 == vectorSet.getNumberOfVectors());
}


void SparseVectorSetTest::testSettingVectorValues() {
    SparseVectorSet vectorSet;
    const int v = 34;
    vectorSet.addVector(v);
    QVERIFY(1 == vectorSet.getNumberOfVectors());

    const int i = 58;
    const float vi = 32;

    vectorSet.setValue(v, i, vi);
    const int nonexisting = 1;
    QVERIFY(vi == vectorSet.getValue(v, i));
    QVERIFY_EXCEPTION_THROWN(vectorSet.setValue(nonexisting, i, vi), std::runtime_error);
}


void SparseVectorSetTest::getValueOfNonExistingEntryInExistingVectorReturnsZero() {
    SparseVectorSet vectorSet;
    const int v = 34;
    const int i = 0;
    const int nonexisting = 1;
    const float value = 80.0f;
    vectorSet.addVector(v);
    vectorSet.setValue(v, i, value);
    QVERIFY(0.0 == vectorSet.getValue(v, nonexisting));
}


void SparseVectorSetTest::getValueOfNonExistingVectorIdThrows() {
    SparseVectorSet vectorSet;
    const int v = 34;
    const int i = 0;
    vectorSet.addVector(v);
    vectorSet.setValue(v, i, 80.0f);
    QVERIFY_EXCEPTION_THROWN(vectorSet.getValue(0, i), std::runtime_error);
}


void SparseVectorSetTest::testSaveLoadStream() {
    SparseVectorSet vectorSet;
    const int v1 = 1;
    const int v2 = 2;
    const int i1 = 48;
    const int i2 = 34;
    const int i3 = 593;

    const float v1i1 = 1.0f;
    const float v2i2 = 9.0f;
    const float v2i3 = 99.0f;

    vectorSet.addVector(v1);
    vectorSet.addVector(v2);
    vectorSet.setValue(v1, i1, v1i1);
    vectorSet.setValue(v2, i2, v2i2);
    vectorSet.setValue(v2, i3, v2i3);

    QBuffer buffer;

    buffer.open(QIODevice::WriteOnly);
    QDataStream outStream(&buffer);
    SparseVectorSet::writeToStream(&vectorSet, outStream);
    buffer.close();

    QDataStream inStream(&buffer);
    buffer.open(QIODevice::ReadOnly);
    QScopedPointer<SparseVectorSet> reread(SparseVectorSet::readFromStream(inStream));
    buffer.close();

    QCOMPARE(false, reread.isNull());
    QCOMPARE(2,  reread->getNumberOfVectors());
    QCOMPARE(1, reread->getNumberOfEntries(v1));
    QCOMPARE(2, reread->getNumberOfEntries(v2));

    QCOMPARE(v1i1, reread->getValue(v1, i1));
    QCOMPARE(v2i2, reread->getValue(v2, i2));
    QCOMPARE(v2i3, reread->getValue(v2, i3));
}
