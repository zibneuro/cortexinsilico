#pragma once

#include "CIS3DConstantsHelpers.h"
#include <QString>
#include <QMap>
#include <QPair>

/**
    This class manages the (experimentally determined) postsynaptic target
    densities.
*/
class PSTDensities {

public:

    /**
        Returns the postynaptic target length density.
        @param postCtName The cell type of the postsynaptic neuron.
        @param strcuture The structural feature of the postsynaptic neuron.
        @param preSynType The neuron type of the presynaptic neuron.
        @return The length density.
    */
    float getLengthDensity(const QString& postCtName,
                           const CIS3D::Structure structure,
                           const CIS3D::NeuronType preSynType) const;

    /**
        Returns the postynaptic target area density.
        @param postCtName The cell type of the postsynaptic neuron.
        @param structure The structural feature of the postsynaptic neuron.
        @param preSynType The neuron type of the presynaptic neuron.
        @return The area density.
    */
    float getAreaDensity(const QString& postCtName,
                         const CIS3D::Structure structure,
                         const CIS3D::NeuronType preSynType) const;

    /**
         Sets the postynaptic target length density.
         @param postCtName The cell type of the postsynaptic neuron.
         @param structure The structural feature of the postsynaptic neuron.
         @param preSynType The neuron type of the presynaptic neuron.
         @param density The length density.
    */
    void setLengthDensity(const QString& postCtName,
                          const CIS3D::Structure structure,
                          const CIS3D::NeuronType preSynType,
                          const float density);

    /**
        Sets the postynaptic target area density.
        @param postCtName The cell type of the postsynaptic neuron.
        @param structure The structural feature of the postsynaptic neuron.
        @param preSynType The neuron type of the presynaptic neuron.
        @param density The area density.
    */
    void setAreaDensity(const QString& postCtName,
                        const CIS3D::Structure structure,
                        const CIS3D::NeuronType preSynType,
                        const float density);

    /**
        Saves the postsynaptic target densities to file.
        @param fileName Name of the file.
        @throws runtime_error if saving file fails.
    */
    void saveCSV(const QString& fileName) const;

    /**
        Loads the postsynaptic target densities from file.
        @param fileName Name of the file.
        @throws runtime_error if loading or parsing the file fails.
    */
    void loadCSV(const QString& fileName);

private:
    struct Density {
        Density() :
        lengthDensity(0.0f),
        areaDensity(0.0f) {}

        float lengthDensity;
        float areaDensity;
    };

    typedef QPair<CIS3D::Structure, CIS3D::NeuronType> DensityKey;
    typedef QMap<DensityKey, Density> CellTypeDensityMap;
    typedef QMap<QString, CellTypeDensityMap> DensityMap;

    DensityMap mDensityMap;
};
