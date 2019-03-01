#include "InnervationMatrix.h"
#include <QDebug>
#include <QDir>
#include <QTextStream>
#include "CIS3DConstantsHelpers.h"
#include "Util.h"

/**
    Constructor.
    @param networkProps The model data.
*/
InnervationMatrix::InnervationMatrix(const NetworkProps& networkProps)
    : mNetwork(networkProps)    
    , mCacheLimit(50000000)
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
InnervationMatrix::getValue(int preId, int postId, int selectionIndex, CIS3D::Structure target)
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

    return getValue(preId, postId, target);
}

float
InnervationMatrix::getValue(int preId, int postId, CIS3D::Structure target)
{
    const int mappedPreId = mNetwork.axonRedundancyMap.getNeuronIdToUse(preId);
    nniKey k(mappedPreId, postId, target);
    auto it = mCache.find(k);
    if (it == mCache.end())
    {
        loadFile(mappedPreId, target);
        it = mCache.find(k);
    }
    return it->second;

}

void
InnervationMatrix::setOriginalPreIds(QList<int> preIdsA, QList<int> preIdsB, QList<int> preIdsC)
{
    mOriginalPreIdsA = preIdsA;
    mOriginalPreIdsB = preIdsB;
    mOriginalPreIdsC = preIdsC;
    mRandomGenerator = RandomGenerator(-1);
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
InnervationMatrix::loadFile(int preId, CIS3D::Structure target)
{
    if (mCache.size() > mCacheLimit)
    {
        mCache.clear();
    }

    QString filename = QString("preNeuronID_%1_sum").arg(preId);
    QString folder = Util::getInnervationFolderName(target);
    QString filepath = QDir::cleanPath(mNetwork.dataRoot + QDir::separator() + folder + QDir::separator() + filename);

    QFile innervationFile(filepath);
    if (innervationFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&innervationFile);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            line = line.trimmed();
            QStringList parts = line.split(" ");
            int postId = parts[0].toInt();
            float innervation = parts[1].toFloat();
            nniKey k(preId, postId, target);
            mCache[k] = innervation;
        }
    }
    else
    {
        const QString msg =
            QString("Error reading innervation file. Could not open file %1").arg(filepath);
        throw std::runtime_error(qPrintable(msg));
    }
}