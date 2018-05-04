#pragma once

#include <CIS3DVec3.h>
#include <CIS3DSparseField.h>
#include <CIS3DConstantsHelpers.h>
#include <QList>
#include <QString>
#include <QMap>

/**
    Represents a 3D point that is associated with a diameter.
*/
struct MorphologyPoint {
    MorphologyPoint() :
        diameter(-1.0f),
        distanceToSoma(-1.0f),
        sectionX(-1.0f) {}

    Vec3f coords;
    float diameter;
    float distanceToSoma;
    float sectionX;
};

/**
    A collection of morphology points that together represent a
    substructure of the neuron.
*/
struct Section {
    Section();
    Section(const QString& n);

    QList<MorphologyPoint> points;
    QString                name;
    QString                parentName;
    float                  parentX;
    CIS3D::Structure       structure;
    int                    sectionID;

    void print() const;
};

/**
    The result of a Section being intersected by the regular voxel grid.
    Thus, one segment is associated with one voxel.
*/
struct Segment {
    float length;
    float area;
    MorphologyPoint p0;
    MorphologyPoint p1;
    CIS3D::Structure structure;
    Vec3i voxel;
    int sectionID;
};

/**
    A list of segments.
*/
typedef QList<Segment> SegmentList;

/**
    Maps Section names to Sections.
*/
typedef QMap<QString, Section> SectionMap;


/**
    Contains the geometric features of the neuron.
*/
class Morphology {

public:
    /**
        Constructor.
    */
    Morphology();

    /**
        Constructor.
        @param hocFile The file to parse.
        @throws runtime_error if file could not be parsed.
    */
    Morphology(const QString& hocFile);

    /**
        Parses a hoc-file.
        @param hocFile The file to parse.
        @throws runtime_error if file could not be parsed.
    */
    void parseHoc(const QString& hocFile);

    /**
        Determines segments by intersecting the morphological features with the
        voxel grid.
        @param The voxel grid.
        @return The segments.
    */
    SegmentList getGridSegments(const SparseField& grid) const;

    /**
        Determines bounding box containing all morphological features of the
        neuron.
        @return The bounding box.
    */
    BoundingBox getBoundingBox() const;

    /**
        Determines the number of different sections that are part of the
        morphology.
        @return The number of sections.
    */
    int getNumberOfSections() const;

    /**
        Determines name of the specified section.
        @param sectionID The id of the section.
        @return The name of the section.
        @throws runtime_error if section ID is unknown.
    */
    QString getSectionName(const int sectionID) const;

    /**
        Retrieves the structure associated with the specified section.
        @param sectionID The ID of the section.
        @return The structure.
        @throws runtime_error if section ID is unknown.
    */
    CIS3D::Structure getSectionStructure(const int sectionID) const;


private:
    SectionMap mSections;
};
