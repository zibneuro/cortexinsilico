#pragma once

#include <QString>
#include <QMap>

/**
    This class represents the hierachy of brain regions that are part of the
    model.
*/
class Regions {

public:
    /**
        Constructor. Initializes default model of rat vS1.
    */
    Regions();

    /**
        Returns the name of the specified brain region.
        @param id The ID of the region.
        @throws runtime_error if ID is not found.
    */
    QString getName(const int id) const;

    /**
        Returns the ID of the specified brain region.
        @param name The name of the region.
        @throws runtime_error if name is not found.
    */
    int getId(const QString& name) const;

    /**
        Returns the IDs of all brain regions.
        @param id The IDs.
    */
    QList<int> getAllRegionIds() const;

    /**
        Determines whether the specified region is contained in the specified
        subregion.
        @param id The ID of the region to check.
        @param subtreeRoot The ID of the subregion.
        @return True if the region is contained in the subregion.
    */
    bool isInSubtree(const int regionId, const int subtreeRoot) const;

    /**
        Saves the region to file.
        @param fileName The name of the file.
        @throws runtime_error if saving the file fails.
    */
    void saveCSV(const QString& fileName) const;

    /**
        Loads the region from file.
        @param fileName The name of the file.
        @throws runtime_error if loading or parsing the file fails.
    */
    void loadCSV(const QString& fileName);

private:
    struct RegionProperties {
        RegionProperties() :
            id(-1), name(""), parentId(-1) {}
        RegionProperties(int regionId, QString regionName, int regionParentId) :
            id(regionId), name(regionName), parentId(regionParentId) {}

        int id;
        QString name;
        int parentId;
    };

    typedef QMap<int, RegionProperties> PropsMap;
    PropsMap mPropsMap;

};
