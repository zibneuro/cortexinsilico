#include "NeuronSelection.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include "Util.h"
#include "UtilIO.h"
#include "Columns.h"

/**
  Empty constructor.
*/
NeuronSelection::NeuronSelection()
{
    mPostTarget.push_back(CIS3D::DEND);
    mPostTarget.push_back(CIS3D::DEND);
    mPostTarget.push_back(CIS3D::DEND);
};

/**
    Constructor for a second order network statistic, containing pre-
    and postsynaptic neuron selections.
    @param presynaptic The presynaptic neuron IDs.
    @param postsynaptic The postsynaptic neuron IDs.
*/
NeuronSelection::NeuronSelection(const IdList& presynaptic, const IdList& postsynaptic)
    : mPresynaptic(presynaptic)
    , mPostsynaptic(postsynaptic)
{
    mPostTarget.push_back(CIS3D::DEND);
    mPostTarget.push_back(CIS3D::DEND);
    mPostTarget.push_back(CIS3D::DEND);
};

IdList
NeuronSelection::filterTissueDepth(const NetworkProps& networkProps,
                                   IdList& neuronIds,
                                   double sliceRef,
                                   double low,
                                   double high,
                                   CIS3D::SliceBand band)
{
    if (sliceRef == -9999)
    {
        return neuronIds;
    }

    IdList pruned;

    for (auto it = neuronIds.begin(); it != neuronIds.end(); ++it)
    {
        Vec3f soma = networkProps.neurons.getSomaPosition(*it);
        double somaX = (double)soma.getX();
        bool first;
        bool second;
        inSliceRange(somaX, sliceRef, low, high, first, second);
        if (band == CIS3D::SliceBand::FIRST && first)
        {
            pruned.push_back(*it);
        }
        else if (band == CIS3D::SliceBand::SECOND && second)
        {
            pruned.push_back(*it);
        }
        else if (band == CIS3D::SliceBand::BOTH && (first || second))
        {
            pruned.push_back(*it);
        }
    }
    return pruned;
}

/**
    Determines a innervation statistic selection from a specification file.
    @param spec The spec file with the filter definition.
    @param networkProps the model data of the network.
    @param samplingFactor Sampling rate for selection (default: 1 = take all)
    @param seed The random seed for the sampling (-1, to use random seed)
*/
void
NeuronSelection::setInnervationSelection(const QJsonObject& spec, const NetworkProps& networkProps, int samplingFactor, int seed)
{
    mPresynaptic.clear();
    mPostsynaptic.clear();
    IdList pre = UtilIO::getPreSynapticNeurons(spec, networkProps);
    IdList post = UtilIO::getPostSynapticNeuronIds(spec, networkProps);
    if (samplingFactor == 1)
    {
        mPresynaptic.append(pre);
        mPostsynaptic.append(post);
    }
    else
    {
        RandomGenerator randomGenerator(seed);
        randomGenerator.shuffleList(pre);
        randomGenerator.shuffleList(post);
        for (int i = 0; i < pre.size(); i += samplingFactor)
        {
            mPresynaptic.append(pre[i]);
        }
        for (int i = 0; i < post.size(); i += samplingFactor)
        {
            mPostsynaptic.append(post[i]);
        }
    }
}

/**
    Determines a triplet motif statistic selection from a specification file.
    @param spec The spec file with the filter definition.
    @param networkProps the model data of the network.
*/
void
NeuronSelection::setTripletSelection(const QJsonObject& spec,
                                     const NetworkProps& networkProps)
{
    QJsonObject tmpSpec;

    tmpSpec["NEURON_REGIONS"] = spec["MOTIF_A_REGIONS"];
    tmpSpec["NEURON_CELLTYPES"] = spec["MOTIF_A_CELLTYPES"];
    tmpSpec["NEURON_IDS"] = spec["MOTIF_A_IDS"];

    mMotifA.clear();
    mMotifA.append(UtilIO::getNeuronIds(tmpSpec, networkProps));

    tmpSpec["NEURON_REGIONS"] = spec["MOTIF_B_REGIONS"];
    tmpSpec["NEURON_CELLTYPES"] = spec["MOTIF_B_CELLTYPES"];
    tmpSpec["NEURON_IDS"] = spec["MOTIF_B_IDS"];
    mMotifB.clear();
    mMotifB.append(UtilIO::getNeuronIds(tmpSpec, networkProps));

    tmpSpec["NEURON_REGIONS"] = spec["MOTIF_C_REGIONS"];
    tmpSpec["NEURON_CELLTYPES"] = spec["MOTIF_C_CELLTYPES"];
    tmpSpec["NEURON_IDS"] = spec["MOTIF_C_IDS"];
    mMotifC.clear();
    mMotifC.append(UtilIO::getNeuronIds(tmpSpec, networkProps));
}

/**
    Determines a triplet motif statistic selection from selection strings.
    @param motifASelString The first selection string.
    @param motifBSelString The second selection string.
    @param motifASelString The third selection string.
    @param networkProps the model data of the network.
*/
void
NeuronSelection::setTripletSelection(const QString motifASelString,
                                     const QString motifBSelString,
                                     const QString motifCSelString,
                                     const NetworkProps& networkProps)
{
    mMotifA.clear();
    mMotifA.append(getSelectedNeurons(motifASelString, networkProps));
    mMotifB.clear();
    mMotifB.append(getSelectedNeurons(motifBSelString, networkProps));
    mMotifC.clear();
    mMotifC.append(getSelectedNeurons(motifCSelString, networkProps));
}

void
NeuronSelection::setInDegreeSelection(const QString selAString,
                                      const QString selBString,
                                      const QString selCString,
                                      const NetworkProps& networkProps)
{
    mMotifA.clear();
    mMotifA.append(getSelectedNeurons(selAString, networkProps, CIS3D::PRESYNAPTIC));
    mMotifB.clear();
    mMotifB.append(getSelectedNeurons(selBString, networkProps, CIS3D::PRESYNAPTIC));
    mMotifC.clear();
    mMotifC.append(getSelectedNeurons(selCString, networkProps, CIS3D::POSTSYNAPTIC));
}

/**
    Determines neuron IDs based on a selection string;
    @param selectionString The selection string.
    @param networkProps The model data of the network.
    @return A list of neuron IDs.
*/
IdList
NeuronSelection::getSelectedNeurons(const QString selectionString,
                                    const NetworkProps& networkProps,
                                    CIS3D::SynapticSide synapticSide)
{
    QJsonDocument doc = QJsonDocument::fromJson(selectionString.toLocal8Bit());
    QJsonArray arr = doc.array();
    SelectionFilter filter = Util::getSelectionFilterFromJson(arr, networkProps, synapticSide);
    Util::correctVPMSelectionFilter(filter, networkProps);
    Util::correctInterneuronSelectionFilter(filter, networkProps);
    return networkProps.neurons.getFilteredNeuronIds(filter);
}

/**
  Returns the presynaptic subselection.
*/
IdList
NeuronSelection::Presynaptic() const
{
    return mPresynaptic;
}

/**
  Returns the postynaptic subselection.
*/
IdList
NeuronSelection::Postsynaptic() const
{
    return mPostsynaptic;
}

/**
  Returns the first neuron subselection for motif statistics.
*/
IdList
NeuronSelection::MotifA() const
{
    return mMotifA;
}

/**
  Returns the second neuron subselection for motif statistics.
*/
IdList
NeuronSelection::MotifB() const
{
    return mMotifB;
}

/**
  Returns the third neuron subselection for motif statistics.
*/
IdList
NeuronSelection::MotifC() const
{
    return mMotifC;
}

CIS3D::SliceBand
NeuronSelection::getMotifABand(int id) const
{
    auto it = mMotifABand.find(id);
    if (it != mMotifABand.end())
    {
        return it->second;
    }
    else
    {
        return CIS3D::SliceBand::FIRST;
    }
}

CIS3D::SliceBand
NeuronSelection::getMotifBBand(int id) const
{
    auto it = mMotifBBand.find(id);
    if (it != mMotifBBand.end())
    {
        return it->second;
    }
    else
    {
        return CIS3D::SliceBand::FIRST;
    }
}

CIS3D::SliceBand
NeuronSelection::getMotifCBand(int id) const
{
    auto it = mMotifCBand.find(id);
    if (it != mMotifCBand.end())
    {
        return it->second;
    }
    else
    {
        return CIS3D::SliceBand::FIRST;
    }
}

CIS3D::SliceBand
NeuronSelection::getPresynapticBand(int id) const
{
    auto it = mPresynapticBand.find(id);
    if (it != mPresynapticBand.end())
    {
        return it->second;
    }
    else
    {
        return CIS3D::SliceBand::FIRST;
    }
}

CIS3D::SliceBand
NeuronSelection::getPostsynapticBand(int id) const
{
    auto it = mPostsynapticBand.find(id);
    if (it != mPostsynapticBand.end())
    {
        return it->second;
    }
    else
    {
        return CIS3D::SliceBand::FIRST;
    }
}

/*
    Prints the number of selected neurons for motif statistics.
*/
void
NeuronSelection::printMotifStats()
{
    qDebug() << "[*] Number of selected neurons (motif A,B,C):" << mMotifA.size() << mMotifB.size()
             << mMotifC.size();
}

void
NeuronSelection::setBBox(QVector<float> min, QVector<float> max)
{
    mBBoxMin = min;
    mBBoxMax = max;
}

QVector<float>
NeuronSelection::getBBoxMin()
{
    return mBBoxMin;
}

QVector<float>
NeuronSelection::getBBoxMax()
{
    return mBBoxMax;
}

void
NeuronSelection::setPiaSomaDistance(QVector<float> rangePre, QVector<float> rangePost, const NetworkProps& networkProps)
{
    //qDebug() << rangePre << rangePost;
    mPiaSomaDistancePre = rangePre;
    mPiaSomaDistancePost = rangePost;
    filterPiaSoma(mPresynaptic, rangePre, networkProps);
    filterPiaSoma(mPostsynaptic, rangePost, networkProps);
}

void
NeuronSelection::setFullModel(const NetworkProps& networkProps, bool uniquePre)
{
    mPresynaptic.clear();
    mPostsynaptic.clear();

    SelectionFilter preFilter;
    preFilter.synapticSide = CIS3D::PRESYNAPTIC;
    IdList preNeurons = networkProps.neurons.getFilteredNeuronIds(preFilter);
    for (int i = 0; i < preNeurons.size(); i++)
    {
        int cellTypeId = networkProps.neurons.getCellTypeId(preNeurons[i]);
        if (networkProps.cellTypes.isExcitatory(cellTypeId))
        {
            mPresynaptic.append(preNeurons[i]);
        }
    }
    if (uniquePre)
    {
        filterUniquePre(networkProps);
    }

    SelectionFilter postFilter;
    postFilter.synapticSide = CIS3D::POSTSYNAPTIC;
    IdList postNeurons = networkProps.neurons.getFilteredNeuronIds(postFilter);
    for (int i = 0; i < postNeurons.size(); i++)
    {
        int cellTypeId = networkProps.neurons.getCellTypeId(postNeurons[i]);
        if (networkProps.cellTypes.isExcitatory(cellTypeId))
        {
            mPostsynaptic.append(postNeurons[i]);
        }
    }
}

void
NeuronSelection::filterPiaSoma(IdList& neuronIds, QVector<float> range, const NetworkProps& networkProps)
{
    Columns columns;
    if (range.size() == 2)
    {
        qDebug() << "[*] Filter pia soma distance" << range;
        IdList pruned;
        for (auto it = neuronIds.begin(); it != neuronIds.end(); ++it)
        {
            int regionId = networkProps.neurons.getRegionId(*it);
            QString regionName = networkProps.regions.getName(regionId);
            Vec3f soma = networkProps.neurons.getSomaPosition(*it);
            if (columns.inRange(regionName, soma, range))
            {
                pruned.push_back(*it);
            }
        }
        neuronIds = pruned;
    }
}

void
applyTissueDepthFilter(IdList& selectionIds,
                       std::map<int, CIS3D::SliceBand>& selectionBand,
                       const NetworkProps& networkProps,
                       double sliceRef,
                       double low,
                       double high,
                       QString mode)
{
    if (mode == "twoSided")
    {
        selectionIds = NeuronSelection::filterTissueDepth(networkProps,
                                                          selectionIds,
                                                          sliceRef,
                                                          low,
                                                          high,
                                                          CIS3D::SliceBand::BOTH);
        IdList first = NeuronSelection::filterTissueDepth(networkProps,
                                                          selectionIds,
                                                          sliceRef,
                                                          low,
                                                          high,
                                                          CIS3D::SliceBand::FIRST);
        for (auto it = first.begin(); it != first.end(); it++)
        {
            selectionBand[*it] = CIS3D::SliceBand::FIRST;
        }

        IdList second = NeuronSelection::filterTissueDepth(networkProps,
                                                           selectionIds,
                                                           sliceRef,
                                                           low,
                                                           high,
                                                           CIS3D::SliceBand::SECOND);

        for (auto it = second.begin(); it != second.end(); it++)
        {
            selectionBand[*it] = CIS3D::SliceBand::SECOND;
        }
    }
    else
    {
        selectionIds = NeuronSelection::filterTissueDepth(networkProps,
                                                          selectionIds,
                                                          sliceRef,
                                                          low,
                                                          high,
                                                          CIS3D::SliceBand::FIRST);
        for (auto it = selectionIds.begin(); it != selectionIds.end(); it++)
        {
            selectionBand[*it] = CIS3D::SliceBand::FIRST;
        }
    }
}

void
NeuronSelection::filterTripletSlice(const NetworkProps& networkProps,
                                    double sliceRef,
                                    double tissueLowMotifA,
                                    double tissueHighMotifA,
                                    QString tissueModeMotifA,
                                    double tissueLowMotifB,
                                    double tissueHighMotifB,
                                    QString tissueModeMotifB,
                                    double tissueLowMotifC,
                                    double tissueHighMotifC,
                                    QString tissueModeMotifC)
{
    applyTissueDepthFilter(mMotifA, mMotifABand, networkProps, sliceRef, tissueLowMotifA, tissueHighMotifA, tissueModeMotifA);
    applyTissueDepthFilter(mMotifB, mMotifBBand, networkProps, sliceRef, tissueLowMotifB, tissueHighMotifB, tissueModeMotifB);
    applyTissueDepthFilter(mMotifC, mMotifCBand, networkProps, sliceRef, tissueLowMotifC, tissueHighMotifC, tissueModeMotifC);
}

void
NeuronSelection::filterInnervationSlice(const NetworkProps& networkProps,
                                        double sliceRef,
                                        double tissueLowPre,
                                        double tissueHighPre,
                                        QString tissueModePre,
                                        double tissueLowPost,
                                        double tissueHighPost,
                                        QString tissueModePost)
{
    applyTissueDepthFilter(mPresynaptic, mPresynapticBand, networkProps, sliceRef, tissueLowPre, tissueHighPre, tissueModePre);
    applyTissueDepthFilter(mPostsynaptic, mPostsynapticBand, networkProps, sliceRef, tissueLowPost, tissueHighPost, tissueModePost);
}

void
NeuronSelection::setSelectionFromQuery(const QJsonObject& spec, const NetworkProps& networkProps)
{

}

QVector<float>
NeuronSelection::getPiaSomaDistancePre()
{
    return mPiaSomaDistancePre;
}

QVector<float>
NeuronSelection::getPiaSomaDistancePost()
{
    return mPiaSomaDistancePost;
}

void
NeuronSelection::sampleDown(int maxSize, int seed)
{
    RandomGenerator randomGenerator(seed);
    mPresynaptic = getDownsampled(mPresynaptic, maxSize, randomGenerator);
    mPostsynaptic = getDownsampled(mPostsynaptic, maxSize, randomGenerator);
    mMotifA = getDownsampled(mMotifA, maxSize, randomGenerator);
    mMotifB = getDownsampled(mMotifB, maxSize, randomGenerator);
    mMotifC = getDownsampled(mMotifC, maxSize, randomGenerator);
}

void
NeuronSelection::sampleDownFactor(int samplingFactor, int seed)
{
    RandomGenerator randomGenerator(seed);
    mPresynaptic = getDownsampledFactor(mPresynaptic, samplingFactor, randomGenerator);
    mPostsynaptic = getDownsampledFactor(mPostsynaptic, samplingFactor, randomGenerator);
    mMotifA = getDownsampledFactor(mMotifA, samplingFactor, randomGenerator);
    mMotifB = getDownsampledFactor(mMotifB, samplingFactor, randomGenerator);
    mMotifC = getDownsampledFactor(mMotifC, samplingFactor, randomGenerator);
}

IdList
NeuronSelection::filterPreOrBoth(const NetworkProps& networkProps, IdList ids)
{
    IdList pruned;
    for (auto it = ids.begin(); it != ids.end(); it++)
    {
        if (networkProps.neurons.getSynapticSide(*it) != CIS3D::POSTSYNAPTIC)
        {
            pruned.append(*it);
        }
    }
    return pruned;
}

void
NeuronSelection::setPostTarget(CIS3D::Structure selectionA, CIS3D::Structure selectionB, CIS3D::Structure selectionC)
{
    mPostTarget.clear();
    mPostTarget.push_back(selectionA);
    mPostTarget.push_back(selectionB);
    mPostTarget.push_back(selectionC);
}

CIS3D::Structure
NeuronSelection::getPostTarget(int selectionIndex) const
{
    return mPostTarget[selectionIndex];
}

void
NeuronSelection::filterUniquePre(const NetworkProps& networkProps)
{
    IdList pruned;
    for (int i = 0; i < mPresynaptic.size(); i++)
    {
        int id = mPresynaptic[i];
        int mappedId = networkProps.axonRedundancyMap.getNeuronIdToUse(id);
        if (!pruned.contains(mappedId))
        {
            pruned.append(mappedId);
        }
    }
    mPresynaptic = pruned;
}

IdList
NeuronSelection::getDownsampled(IdList& original, int maxSize, RandomGenerator& randomGenerator)
{
    if (original.size() <= maxSize)
    {
        return original;
    }
    else
    {
        randomGenerator.shuffleList(original);
        IdList pruned;
        for (int i = 0; i < maxSize; i++)
        {
            pruned.append(original[i]);
        }
        return pruned;
    }
}

IdList
NeuronSelection::getDownsampledFactor(IdList& original, int factor, RandomGenerator& randomGenerator)
{
    if (factor == -1)
    {
        return original;
    }
    std::sort(original.begin(), original.end());
    randomGenerator.shuffleList(original);
    IdList pruned;
    for (int i = 0; i < original.size(); i += factor)
    {
        pruned.append(original[i]);
    }
    return pruned;
}

bool
NeuronSelection::inSliceBand(double somaX, double min, double max)
{
    return somaX >= min && somaX <= max;
}

void
NeuronSelection::inSliceRange(double somaX, double sliceRef, double low, double high, bool& first, bool& second)
{
    double sliceWidth = 300;
    first = inSliceBand(somaX, sliceRef + low, sliceRef + high);
    second = inSliceBand(somaX, sliceRef + sliceWidth - high, sliceRef + sliceWidth - low);
}
