#include <QTest>


class SparseVectorSetTest: public QObject
{

    Q_OBJECT

private slots:
    void testAddingVector();
    void testSettingVectorValues();
    void getValueOfNonExistingEntryInExistingVectorReturnsZero();
    void getValueOfNonExistingVectorIdThrows();
    void testSaveLoadStream();
};

