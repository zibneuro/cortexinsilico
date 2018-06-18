#include "SparseFieldTest.h"
#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QScopedPointer>
#include <QTest>
#include <stdexcept>
#include "CIS3DSparseField.h"

void SparseFieldTest::returnsZeroForNonExistingPositionInExistingField() {
    SparseField field(Vec3i(1, 2, 3));
    QVERIFY(qFuzzyIsNull(field.getFieldValue(Vec3i(1, 1, 1))));
}

void SparseFieldTest::multiplication() {
    SparseField::Locations positions1;
    positions1.push_back(Vec3i(0, 1, 0));
    positions1.push_back(Vec3i(1, 1, 0));

    SparseField::Locations positions2;
    positions2.push_back(Vec3i(1, 0, 0));
    positions2.push_back(Vec3i(1, 1, 0));

    SparseField::Field field1;
    field1.push_back(2.0f);
    field1.push_back(2.0f);

    SparseField::Field field2;
    field2.push_back(3.0f);
    field2.push_back(3.0f);

    Vec3f origin(0.0f), voxelSize(1.0f);

    const SparseField sparseField1(Vec3i(2, 2, 1), positions1, field1, origin, voxelSize);
    const SparseField sparseField2(Vec3i(2, 2, 1), positions2, field2, origin, voxelSize);

    const SparseField multiplied = multiply(sparseField1, sparseField2);

    // (0,0,0): X * X = X
    // (0,1,0): 2 * X = X
    // (1,0,0): X * 3 = X
    // (1,1,0): 2 * 3 = 6

    QVERIFY(qFuzzyIsNull(multiplied.getFieldValue(Vec3i(0, 0, 0))));
    QVERIFY(qFuzzyIsNull(multiplied.getFieldValue(Vec3i(0, 1, 0))));
    QVERIFY(qFuzzyIsNull(multiplied.getFieldValue(Vec3i(1, 0, 0))));
    QCOMPARE(6.0f, multiplied.getFieldValue(Vec3i(1, 1, 0)));
    QVERIFY(false == multiplied.hasFieldValue(Vec3i(0, 0, 0)));
    QVERIFY(false == multiplied.hasFieldValue(Vec3i(0, 1, 0)));
    QVERIFY(false == multiplied.hasFieldValue(Vec3i(1, 0, 0)));
}

void SparseFieldTest::sum() {
    SparseField::Locations positions;
    positions.push_back(Vec3i(0, 1, 0));
    positions.push_back(Vec3i(1, 1, 0));

    SparseField::Field field;
    field.push_back(1.0f);
    field.push_back(2.0f);

    Vec3f origin(0.0f), voxelSize(0.0f);

    const SparseField sparseField(Vec3i(2, 2, 1), positions, field, origin, voxelSize);

    QCOMPARE(3.0f, sparseField.getFieldSum());
}

void SparseFieldTest::multiplicationAndSumOfUnequalFieldDims() {
    SparseField::Locations positions1;
    positions1.push_back(Vec3i(1, 0, 0));
    positions1.push_back(Vec3i(1, 1, 0));

    SparseField::Locations positions2;
    positions2.push_back(Vec3i(0, 0, 0));
    positions2.push_back(Vec3i(0, 1, 0));

    SparseField::Field field1;
    field1.push_back(2.0f);
    field1.push_back(2.0f);

    SparseField::Field field2;
    field2.push_back(3.0f);
    field2.push_back(3.0f);

    const Vec3i dims1(2, 2, 1);
    const Vec3i dims2(3, 2, 1);
    const Vec3f origin1(0.0f, 0.0f, 0.0f);
    const Vec3f origin2(1.0f, -1.0f, 0.0f);
    const Vec3f voxelSize(1.0f);

    const SparseField sparseField1(dims1, positions1, field1, origin1, voxelSize);
    const SparseField sparseField2(dims2, positions2, field2, origin2, voxelSize);

    const SparseField multiplied = multiply(sparseField1, sparseField2);

    // (1,0,0): 2 * 3 = 6
    QVERIFY(dims1 == multiplied.getDimensions());
    QVERIFY(multiplied.getOrigin().equals(origin1));
    QVERIFY(multiplied.getVoxelSize().equals(voxelSize));

    QCOMPARE(6.0f, multiplied.getFieldValue(Vec3i(1, 0, 0)));
    QVERIFY(false == multiplied.hasFieldValue(Vec3i(0, 0, 0)));
    QVERIFY(false == multiplied.hasFieldValue(Vec3i(0, 1, 0)));
    QVERIFY(false == multiplied.hasFieldValue(Vec3i(1, 1, 0)));

    const SparseField sum = sparseField1 + sparseField2;
    QVERIFY(Vec3i(4, 3, 1) == sum.getDimensions());
    QVERIFY(sum.getOrigin().equals(Vec3f(origin1.getX(), origin2.getY(), origin1.getZ())));
    QVERIFY(sum.getVoxelSize().equals(voxelSize));

    QCOMPARE(10.0f, sum.getFieldSum());
    QCOMPARE(3.0f, sum.getFieldValue(Vec3i(1, 0, 0)));
    QCOMPARE(5.0f, sum.getFieldValue(Vec3i(1, 1, 0)));
    QCOMPARE(2.0f, sum.getFieldValue(Vec3i(1, 2, 0)));
}

void SparseFieldTest::testSaveLoadStream() {
    SparseField::Locations locations;
    locations.push_back(Vec3i(0, 1, 0));
    locations.push_back(Vec3i(1, 1, 0));

    SparseField::Field field1;
    field1.push_back(1.0f);
    field1.push_back(2.0f);

    const Vec3f origin(1.0, 2.0f, 3.0f);
    const Vec3f voxelSize(4.0, 5.0f, 6.0f);

    SparseField field(Vec3i(2, 2, 1), locations, field1, origin, voxelSize);

    QBuffer buffer;

    buffer.open(QIODevice::WriteOnly);
    QDataStream outStream(&buffer);
    SparseField::writeToStream(&field, outStream);
    buffer.close();

    QDataStream inStream(&buffer);
    buffer.open(QIODevice::ReadOnly);
    QScopedPointer<SparseField> reread(SparseField::readFromStream(inStream));
    buffer.close();

    QVERIFY(false == reread.isNull());

    QVERIFY(qFuzzyIsNull(reread->getFieldValue(Vec3i(0, 0, 0))));
    QCOMPARE(1.0f, reread->getFieldValue(Vec3i(0, 1, 0)));
    QCOMPARE(2.0f, reread->getFieldValue(Vec3i(1, 1, 0)));
    QCOMPARE(3.0f, reread->getFieldSum());
}
