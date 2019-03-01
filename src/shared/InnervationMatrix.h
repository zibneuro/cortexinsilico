#ifndef INNERVATIONMATRIX_H
#define INNERVATIONMATRIX_H

#include "CIS3DNetworkProps.h"
#include "RandomGenerator.h"
#include "CIS3DConstantsHelpers.h"

/**
    Represents the innervation matrix. Provides acces to innervation values,
    which are internally stored in multiple files. 
*/
class InnervationMatrix
{
    typedef std::tuple<int, int, CIS3D::Structure> nniKey;

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
    float getValue(int preID, int postID, int selectionIndex, CIS3D::Structure target);
    float getValue(int preID, int postID, CIS3D::Structure target);
    void setOriginalPreIds(QList<int> preIdsA, QList<int> preIdsB, QList<int> preIdsC);

private:
    int getRandomDuplicatedPreId(int selectionIndex);
    void loadFile(int preId, CIS3D::Structure target);

    const NetworkProps& mNetwork;
    unsigned int mCacheLimit;
    QList<int> mOriginalPreIdsA;
    QList<int> mOriginalPreIdsB;
    QList<int> mOriginalPreIdsC;
    RandomGenerator mRandomGenerator;
    std::map<nniKey, float> mCache;
};

#endif // INNERVATIONMATRIX_H