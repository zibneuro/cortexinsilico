#include "InnervationMatrix.h"
#include <QDebug>
#include "CIS3DConstantsHelpers.h"

/**
    Constructor.
    @param networkProps The model data.
*/
InnervationMatrix::InnervationMatrix(const NetworkProps& networkProps)
    : mNetwork(networkProps)
    , mRandomGenerator(-1){};

/**
    Destructor.
*/
InnervationMatrix::~InnervationMatrix()
{
    mCache.clear();
}

/**
    Retrieves innervation between the specified neurons.
    @param pre The presynaptic neuron ID.
    @param post The postsynaptic neuron ID.
    @return The innervation from presynaptic to postsynaptic neuron.
*/
float
InnervationMatrix::getValue(int preId, int postId, int selectionIndex)
{
    CIS3D::SynapticSide preSide = mNetwork.neurons.getSynapticSide(preId);
    CIS3D::SynapticSide postSide = mNetwork.neurons.getSynapticSide(postId);
    if (preSide == CIS3D::SynapticSide::POSTSYNAPTIC)
    {
        int mappedId = getRandomDuplicatedPreId(selectionIndex);
        if (mappedId != -1)
        {
            preId = mappedId;
        }
        else
        {
            return 0;
        }
    }
    if (postSide == CIS3D::SynapticSide::PRESYNAPTIC)
    {
        return 0;
    }

    int cellTypeId = mNetwork.neurons.getCellTypeId(postId);
    int regionId = mNetwork.neurons.getRegionId(postId);
    const QString cellTypeName = mNetwork.cellTypes.getName(cellTypeId);
    const QString regionName = mNetwork.regions.getName(regionId);
    const QDir innervationDir = CIS3D::getInnervationDataDir(mNetwork.dataRoot);
    const QString innervationFile =
        CIS3D::getInnervationPostFileName(innervationDir, regionName, cellTypeName);

    SparseVectorSet* vectorSet;
    if (mCache.contains(innervationFile))
    {
        vectorSet = mCache.get(innervationFile);
    }
    else
    {
        vectorSet = SparseVectorSet::load(innervationFile);
        mCache.add(innervationFile, vectorSet);
    }

    const int mappedPreId = mNetwork.axonRedundancyMap.getNeuronIdToUse(preId);
    float innervation = vectorSet->getValue(postId, mappedPreId);
    return innervation;
}

int
InnervationMatrix::getRandomDuplicatedPreId(int selectionIndex)
{
    if (selectionIndex == 0)
    {
        return mRandomGenerator.getRandomEntry(mOriginalPreIdsA);
    }
    else if (selectionIndex == 1)
    {
        return mRandomGenerator.getRandomEntry(mOriginalPreIdsB);
    }
    else
    {
        return mRandomGenerator.getRandomEntry(mOriginalPreIdsC);
    }
}

void
InnervationMatrix::setOriginalPreIds(QList<int> preIdsA, QList<int> preIdsB, QList<int> preIdsC)
{
    mOriginalPreIdsA = preIdsA;
    mOriginalPreIdsB = preIdsB;
    mOriginalPreIdsC = preIdsC;
    mRandomGenerator = RandomGenerator(-1);
}