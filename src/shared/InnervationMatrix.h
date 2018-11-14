#ifndef INNERVATIONMATRIX_H
#define INNERVATIONMATRIX_H

#include "CIS3DNetworkProps.h"
#include "SparseVectorCache.h"
#include "RandomGenerator.h"

/**
    Represents the innervation matrix. Provides acces to innervation values,
    which are internally stored in multiple files. 
*/
class InnervationMatrix {
    public:

    /**
        Constructor.
        @param networkProps The model data.
    */
    InnervationMatrix(const NetworkProps& networkProps);

    /**
        Destructor.
    */
    ~InnervationMatrix();

    /**
        Retrieves innervation between the specified neurons.
        @param pre The presynaptic neuron ID.
        @param post The postsynaptic neuron ID.
        @return The innervation from presynaptic to postsynaptic neuron.
    */
    float getValue(int preID, int postID, int selectionIndex);

    int getRandomDuplicatedPreId(int selectionIndex);

    void setOriginalPreIds(QList<int> preIdsA, QList<int> preIdsB, QList<int> preIdsC);

    const NetworkProps& mNetwork;
    SparseVectorCache mCache;
    QList<int> mOriginalPreIdsA;
    QList<int> mOriginalPreIdsB;
    QList<int> mOriginalPreIdsC;
    RandomGenerator mRandomGenerator;
};

#endif // INNERVATIONMATRIX_H