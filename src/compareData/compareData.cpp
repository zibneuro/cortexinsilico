/*
    This tool is used for testing purposes to compare innveration values.
    Usage:

    ./compareData <sparseVectorSet_ref> <sparseVectorSet_cmp> <epsilon>"

    <sparseVectorSet_ref> InnervationPost directory with the reference values.
    <sparseVectorSet_cmp> InnervationPost directory with the values to be
        compared against the reference values.
    <epsilon> Maximum permissible deviation.
*/

#include <QDebug>
#include <QSet>
#include <QList>
#include <QString>
#include "CIS3DSparseVectorSet.h"


void printUsage() {
    qDebug() << "Usage: ./compareData <sparseVectorSet_ref> <sparseVectorSet_cmp> <epsilon>";
}

// Checks whether two lists of neuron IDs have the same entries.
bool sameEntries(const QList<int> a, const QList<int> b){
    QSet<int> aSet = QSet<int>::fromList(a);
    const QSet<int> bSet = QSet<int>::fromList(b);
    aSet.intersect(bSet);
    return (a.count() == b.count()) && (aSet.count() == a.count());
}

// Checks whether the deviation between a and b is below eps.
bool areIdentical(const float a, const float b, const float eps){\
    return std::fabs(a-b) < eps;
}


int main(int argc, char* argv[])
{
    if (argc != 4) {
        printUsage();
        return 1;
    }

    const QString refFile = argv[1];
    const QString cmpFile = argv[2];
    const float eps = atof(argv[3]);

    SparseVectorSet* vectorSetRef = SparseVectorSet::load(refFile);
    SparseVectorSet* vectorSetCmp = SparseVectorSet::load(cmpFile);

    const QList<int> vectorIdsRef = vectorSetRef->getVectorIds();
    const QList<int> vectorIdsCmp = vectorSetCmp->getVectorIds();

    if(!sameEntries(vectorIdsRef,vectorIdsCmp)){
        qDebug() << "different vector IDs";
        return 1;
    }

    for (QList<int>::ConstIterator postIt=vectorIdsRef.constBegin(); postIt!=vectorIdsRef.constEnd(); ++postIt) {
        const int vectorId = *postIt;

        const QList<int> entryIdsRef = vectorSetRef->getEntryIds(vectorId);
        const QList<int> entryIdsCmp = vectorSetCmp->getEntryIds(vectorId);

        if(!sameEntries(entryIdsRef,entryIdsCmp)){
            qDebug() << "different entry IDs for vector " << vectorId;
            return 1;
        }

        for (QList<int>::ConstIterator entryIt=entryIdsRef.constBegin(); entryIt!=entryIdsRef.constEnd(); ++entryIt) {
            const int entryId = *entryIt;

            const float valueRef = vectorSetRef->getValue(vectorId,entryId);
            const float valueCmp = vectorSetCmp->getValue(vectorId,entryId);

            if(!areIdentical(valueRef,valueCmp,eps)){
                qDebug() << "different value for vector ID " << vectorId << " at entry ID " << entryId;
                qDebug() << "value: " << valueRef << " reference value: " << valueCmp;
                qDebug() << valueRef - valueCmp;
                return 1;
            }
        }
    }

    qDebug() << "identical";
    return 0;
}
