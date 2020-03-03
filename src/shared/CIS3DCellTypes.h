#pragma once

#include "CIS3DVec3.h"
#include <QString>
#include <QMap>

/**
    Manages the meta information about the various cell types in
    the network.
*/
class CellTypes {

public:
    
    /**
        Checks whether the specified cell type exists.
        @param id The ID of the cell type.
        @return true if the cell type exists.
    */
    bool exists(const int id) const;

    /**
        Determines whether the specified cell type is excitatory.
        @param id The ID of the cell type.
        @return true if the cell type is excitatory.
        @throws runtime_error if the ID does not exist.
    */
    bool isExcitatory(const int id) const;

    /**
        Retrieves name of the specified cell type.
        @param id The ID of the cell type.
        @return The name of the cell type.
        @throws runtime_error if the ID does not exist.
    */
    QString getName(const int id) const;

    /**
        Retrieves dendrite color of the specified cell type.
        @param id The ID of the cell type.
        @return rgb-components of the color.
        @throws runtime_error if the ID does not exist.
    */
    Vec3f getDendriteColor(const int id) const;

    /**
        Retrieves axon color of the specified cell type.
        @param id The ID of the cell type.
        @return rgb-components of the color.
        @throws runtime_error if the ID does not exist.
    */
    Vec3f getAxonColor(const int id) const;

    /**
        Retrieves ID of the specified cell type.
        @param name The name of the cell type.
        @return ID of the cell type.
        @throws runtime_error if the name does not exist.
    */
    int getId(const QString& name) const;

    /**
        Retrieves all existing cell type IDs.
        @return The IDs.
    */
    QList<int> getAllCellTypeIds(bool excitatory = false) const;

    /**
        Loads cell types from file.
        @param The name of the file.
        @throws runtime_error if file could not be loaded or parsed.
    */
    void loadCSV(const QString& fileName);

private:
    struct Properties {
        Properties() :
            name(""),
            isExcitatory(false),
            colorDendR(0.0f),
            colorDendG(0.0f),
            colorDendB(0.0f),
            colorAxonR(0.0f),
            colorAxonG(0.0f),
            colorAxonB(0.0f)
        {}

        QString name;
        bool isExcitatory;
        float colorDendR;
        float colorDendG;
        float colorDendB;
        float colorAxonR;
        float colorAxonG;
        float colorAxonB;
    };

    typedef QMap<int, Properties> PropsMap;
    PropsMap mPropsMap;

};
