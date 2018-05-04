#pragma once

#include <CIS3DConstantsHelpers.h>
#include <CIS3DStatistics.h>
#include <QSet>
#include <QMap>

/**
    Collects explicit synapse locations and computes statistics about these
    synapses.
*/
class SynapseStatistics {
    public:

        /**
            Constructor.
        */
        SynapseStatistics();

        /**
            Adds synapse to the collection. The synapse is associated with one
            of the following structures: Soma, Basal, Apical.
            @param syn The sample synapse to add.
            @throws runtime_error if the associated structure is invalid.
        */
        void addSynapse(const CIS3D::Synapse& syn);

        /**
            Returns the number of sample synapses.
            @return The number of samples.
        */
        unsigned int getNumberOfSynapses() const;

        /**
            Returns the number of sample synapses for the specified struture.
            @param structure The associated structure.
            @return The number of samples.
        */
        unsigned int getNumberOfSynapses(CIS3D::Structure structure) const;

        /**
            Returns standard statistics about the distance to soma for all
            synapses.
            @return The distance to soma statistics.
        */
        const Statistics getDistanceToSomaStatistics() const;

        /**
            Returns standard statistics about the distance to soma for all
            synapses with the specified structure.
            @param The associated structure.
            @return The distance to soma statistics.
        */
        const Statistics getDistanceToSomaStatistics(CIS3D::Structure structure) const;

        /**
            Returns the number of connected presynaptic neurons for all synapses.
            @return The number of connected neurons.
        */
        int getNumberOfConnectedNeurons() const;

        /**
            Returns the number of connected presynaptic neurons for all synapses
            with the specified structure.
            @param The associated structure.
            @return The number of connected neurons.
        */
        int getNumberOfConnectedNeurons(CIS3D::Structure structure) const;

    private:
        QSet<int>    mAllPreNeuronIds;
        QMap<CIS3D::Structure, QSet<int>> mPreNeuronIds;

        Statistics   mAllDistStats;
        QMap<CIS3D::Structure, Statistics> mDistStats;

        unsigned int mTotalNumberOfSynapses;
        QMap<CIS3D::Structure, unsigned int> mNumberOfSynapses;
};
