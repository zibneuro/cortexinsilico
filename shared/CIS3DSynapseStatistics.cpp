#include "CIS3DSynapseStatistics.h"

SynapseStatistics::SynapseStatistics() :
    mTotalNumberOfSynapses(0)
{
    mPreNeuronIds.insert(CIS3D::APICAL, QSet<int>());
    mPreNeuronIds.insert(CIS3D::DEND,   QSet<int>());
    mPreNeuronIds.insert(CIS3D::SOMA,   QSet<int>());

    mDistStats.insert(CIS3D::APICAL, Statistics());
    mDistStats.insert(CIS3D::DEND,   Statistics());
    mDistStats.insert(CIS3D::SOMA,   Statistics());

    mNumberOfSynapses.insert(CIS3D::APICAL, 0);
    mNumberOfSynapses.insert(CIS3D::DEND,   0);
    mNumberOfSynapses.insert(CIS3D::SOMA,   0);
}


void SynapseStatistics::addSynapse(const CIS3D::Synapse& syn) {

   if ((syn.structure != CIS3D::SOMA) &&
       (syn.structure != CIS3D::APICAL) &&
       (syn.structure != CIS3D::DEND))
   {
        const QString msg = QString("Error creating statistics: invalid structure %1").arg(CIS3D::getStructureName(syn.structure));
        throw std::runtime_error(qPrintable(msg));
   }

    mTotalNumberOfSynapses += 1;
    mAllDistStats.addSample(syn.distanceToSomaPost);
    mAllPreNeuronIds.insert(syn.preNeuronId);

    mNumberOfSynapses[syn.structure] += 1;
    mDistStats[syn.structure].addSample(syn.distanceToSomaPost);
    mPreNeuronIds[syn.structure].insert(syn.preNeuronId);

}


unsigned int SynapseStatistics::getNumberOfSynapses() const
{
    return mTotalNumberOfSynapses;
}


unsigned int SynapseStatistics::getNumberOfSynapses(CIS3D::Structure structure) const
{
    return mNumberOfSynapses.value(structure);
}


const Statistics SynapseStatistics::getDistanceToSomaStatistics() const
{
    return mAllDistStats;
}


const Statistics SynapseStatistics::getDistanceToSomaStatistics(CIS3D::Structure structure) const
{
    return mDistStats.value(structure);
}


int SynapseStatistics::getNumberOfConnectedNeurons() const
{
    return mAllPreNeuronIds.size();
}


int SynapseStatistics::getNumberOfConnectedNeurons(CIS3D::Structure structure) const
{
    return mPreNeuronIds.value(structure).size();
}


