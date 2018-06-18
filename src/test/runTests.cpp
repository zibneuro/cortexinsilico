#include <QtTest/QtTest>
#include "SparseFieldTest.h"
#include "SparseVectorSetTest.h"

int main() {
    SparseFieldTest sparseFieldTest;
    QTest::qExec(&sparseFieldTest);

    printf("\n");

    SparseVectorSetTest sparseVectorSetTest;
    QTest::qExec(&sparseVectorSetTest);

    return 0;
}
