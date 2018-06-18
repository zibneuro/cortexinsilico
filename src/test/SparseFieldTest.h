#include <QTest>

class SparseFieldTest : public QObject {
    Q_OBJECT

   private slots:
    void returnsZeroForNonExistingPositionInExistingField();
    void multiplication();
    void sum();
    void multiplicationAndSumOfUnequalFieldDims();
    void testSaveLoadStream();
};
