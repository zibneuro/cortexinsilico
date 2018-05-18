#include "CIS3DSynapseStatistics.h"

/**
    Constructor.
*/
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

/**
    Adds synapse to the collection. The synapse is associated with one
    of the following structures: Soma, Basal, Apical.
    @param syn The sample synapse to add.
    @throws runtime_error if the associated structure is invalid.
*/
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

/**
    Returns the number of sample synapses.
    @return The number of samples.
*/
unsigned int SynapseStatistics::getNumberOfSynapses() const
{
    return mTotalNumberOfSynapses;
}

/**
    Returns the number of sample synapses for the specified struture.
    @param structure The associated structure.
    @return The number of samples.
*/
unsigned int SynapseStatistics::getNumberOfSynapses(CIS3D::Structure structure) const
{
    return mNumberOfSynapses.value(structure);
}

/**
    Returns standard statistics about the distance to soma for all
    synapses.
    @return The distance to soma statistics.
*/
const Statistics SynapseStatistics::getDistanceToSomaStatistics() const
{
    return mAllDistStats;
}

/**
    Returns standard statistics about the distance to soma for all
    synapses with the specified structure.
    @param The associated structure.
    @return The distance to soma statistics.
*/
const Statistics SynapseStatistics::getDistanceToSomaStatistics(CIS3D::Structure structure) const
{
    return mDistStats.value(structure);
}

/**
    Returns the number of connected presynaptic neurons for all synapses.
    @return The number of connected neurons.
*/
int SynapseStatistics::getNumberOfConnectedNeurons() const
{
    return mAllPreNeuronIds.size();
}

/**
    Returns the number of connected presynaptic neurons for all synapses
    with the specified structure.
    @param The associated structure.
    @return The number of connected neurons.
*/
int SynapseStatistics::getNumberOfConnectedNeurons(CIS3D::Structure structure) const
{
    return mPreNeuronIds.value(structure).size();
}
