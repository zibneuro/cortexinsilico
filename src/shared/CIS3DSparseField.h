#pragma once

#include "CIS3DVec3.h"
#include "CIS3DBoundingBoxes.h"
#include <QHash>
#include <QMap>
#include <QVector>

class QDataStream;

/**
    Specifies the general properties of the SparseField: dimensions, origin,
    and spacing of cells.
*/
struct SparseFieldCoordinates {
    /**
        Number of cells in each direction.
    */
    Vec3i dimensions;

    /**
        Point of origin in real world coordinates.
    */
    Vec3f origin;

    /**
        The cell size in each direction.
    */
    Vec3f spacing;

    /**
        Checks equality with another set of SparseFieldCoordinates.
    */
    bool operator==(const SparseFieldCoordinates& other) const {
        return (dimensions == other.dimensions) &&
               ((origin-other.origin).length() < 1.e-4) &&
               ((spacing-other.spacing).length() < 1.e-4);
    }

    /**
        Checks inequality with another set of SparseFieldCoordinates.
    */
    bool operator!=(const SparseFieldCoordinates& other) const {
        return !(*this == other);
    }

    /**
        Determines shift in origin towards other SparseFieldCoordinates.
        @param other The other SparseFieldCoordinates.
        @return The shift in orgin.
        @throws runtime_error if the spacing differs or if the shift is not an
            integer multiple of the voxel spacing.
    */
    Vec3i getOffsetTo(const SparseFieldCoordinates& other) const;

    /**
        Extends this set of SparseFieldCoordinates, such that the other
        SparseFieldCoordinates are contained by shifting the origin and
        increasing the dimensions.
        @throws runtime_error if the spacing differs or if the shift is not an
            integer multiple of the voxel spacing.
    */
    void extendBy(const SparseFieldCoordinates& other);

    /**
        Prints dimensions, origin, and spacing to qDebug().
    */
    void print() const;
};

/**
    Denotes begin and end state of SparseField iterator.
*/
enum IteratorState { ITERATOR_BEGIN, ITERATOR_END };

/**
    An efficient implementation of a 3D scalar field that is discretized
    as a voxel grid. Provides functions to perform basic calculations based on
    the scalar values in the grid cells.
*/
class SparseField {

public:

    /**
        Internal representation: Assign integer indices to all locations
        and values in the grid. Associate location and value via hash
        (hash key: location index, hash value: grid value index).
        Store actual locations and values in separate QVectors.
    */
    typedef int LocationIndexT;
    typedef int ValueIndexT;
    typedef QHash<LocationIndexT, ValueIndexT> LocationIndexToValueIndexMap;
    typedef QVector<float> Field;
    typedef QVector<Vec3i> Locations;

    /**
        Constructor.
        Initializes origin and dimenions to (0,0,0) and voxel size to (1,1,1).
    */
    SparseField();

    /**
        Constructor.
        @param dims The dimensions of the grid.
        @param origin The origin of the grid.
        @param voxelSize The spacing of the grid.
    */
    SparseField(const Vec3i& dims, const Vec3f& origin=Vec3f(), const Vec3f& voxelSize=Vec3f(1.0f));

    /**
        Constructor.
        @param coords The SparseFieldCoordinates specifying origin, dimensions, and spacing.
    */
    SparseField(const SparseFieldCoordinates& coords);

    /**
        Constructor.
        @param dims The dimensions of the grid.
        @param locations The locations.
        @param field The values at the locations.
        @param origin The origin of the grid.
        @param voxelSize The spacing of the grid.
        @throws runtime_error if the number of entries in locations and field differs.
    */
    SparseField(const Vec3i& dims,
                const Locations& locations,
                const Field& field,
                const Vec3f& origin=Vec3f(),
                const Vec3f& voxelSize=Vec3f(1.0f));

    /**
        Returns the dimensions of the grid.
        @return The dimensions.
    */
    Vec3i getDimensions() const;

    /**
        Returns the origin of the grid.
        @return The origin.
    */
    Vec3f getOrigin() const;

    /**
        Returns the spacing of the grid cells.
        @return The voxel size.
    */
    Vec3f getVoxelSize() const;

    /**
        Sets the value at the specified location.
        @param location The location.
        @param value The value.
        @throws runtime_error if the location has currently no value.
    */
    void setFieldValue(const Vec3i& location, const float value);

    /**
        Returns value at the specified location.
        @location The location.
        @return The value at the location if the location is defined
            DEFAULT_FIELD_VALUE otherwise.
    */
    float getFieldValue(const Vec3i& location) const;

    /**
        Copies the locations and the corresponding values contained in the
        grid.
        @param locations Is filled with the locations.
        @param values Is filled with the corresponding values.
    */
    void getFieldValues(Locations& locations, Field& values) const;

    /**
        Determines whether the specified location has a value.
        @param location The location to check.
        @return True if a value is defined at the location.
    */
    bool hasFieldValue(const Vec3i& location) const;

    /**
        Returns the sum of all values in the grid.
        @return The sum.
    */
    float getFieldSum() const;

    /**
        Adds a value to the grid. If the location is already defined, the
        value is added to the current value in this cell.
        @param location The location.
        @param value The value to add.
    */
    void addValue(const Vec3i& location, const float value);

    /**
        Multiplies all values in the grid by the specified factor.
        @param factor The multiplication factor.
    */
    void multiply(const float factor);

    /**
        Creates a new grid by multiplying all values in the current
        grid by the specified factor.
        @param factor The multiplication factor.
    */
    SparseField multiply(const float& factor) const;

    /**
        Returns the grid location for the specified point.
        @param p The point in real world coordinates.
        @return The location in the grid.
        @throws runtime_error if the point is not covered by the grid.
    */
    Vec3i getVoxelContainingPoint(const Vec3f &p) const;

    /**
        Determines bounding box covering the voxel specified by the grid
        location.
        @param v The grid location.
        @return The bounding box.
    */
    BoundingBox getVoxelBox(const Vec3i& v) const;

    /**
        Determines the bounding box covering all defined nonzero grid locations.
        @return The bounding box.
    */
    BoundingBox getNonZeroBox() const;

    /**
        Multiplies the grid values of the first SparseField with the grid values of
        the second SparseField and returns the result in a new SparseField. The
        multiplication is only carried out if the respective location is also defined
        in the second SparseField.
        @param fs1 The first sparse field.
        @param fs2 The second sparse field.
        @return The resulting sparse field.
        @throws runtime_error if the spacing differs or if the shift between
            both fields is not an integer multiple of the voxel spacing.
    */
    friend SparseField multiply(const SparseField& fs1,
                                const SparseField& fs2);

    /**
        Divides the grid values of the first SparseField by the grid values of
        the second SparseField and returns the result in a new SparseField. The
        division is only carried out if the respective location is also defined
        in the second SparseField.
        @param fs1 The first sparse field.
        @param fs2 The second sparse field.
        @return The resulting sparse field.
        @throws runtime_error if the spacing differs or if the shift between
            both fields is not an integer multiple of the voxel spacing.
    */
    friend SparseField divide(const SparseField& fs1,
                              const SparseField& fs2);

    /**
      Creates a union grid of the specified SparseFields. If a location is defined
      in both SparseFields, the resulting location is assigned the sum from both
      SparseFields.
      @param fs1 The first sparse field.
      @param fs2 The second sparse field.
      @return The resulting sparse field.
      @throws runtime_error if the spacing differs or if the shift between
          both fields is not an integer multiple of the voxel spacing.
    */
    friend SparseField operator+(const SparseField& fs1,
                                 const SparseField& fs2);


    /**
        Saves a SparseField to file.
        @param fs The SparseField to save.
        @param fileName The name of the file.
        @return 1 if successful, 0 otherwise.
    */
    static int save(const SparseField* fs, const QString& fileName);

    /**
        Loads a SparseField from file.
        @param fileName The name of the file.
        @return The SparseField if successful, 0 otherwise.
        @throws runtime_error in case of a version mismatch.
    */
    static SparseField* load(const QString& fileName);

    /**
        Writes a SparseField to the specified data stream.
        @param fs The SparseField to write.
        @param out The data stream.
    */
    static void writeToStream(const SparseField* fs, QDataStream& out);

    /**
        Reads a SparseField from the specified data stream.
        @param in The data stream.
        @return The SparseField.
        @throws runtime_error in case of a version mismatch.
    */
    static SparseField* readFromStream(QDataStream& in);

    /**
        Returns the coordinates of the SparseField (origin, dimension, voxel spacing).
        @return The SparseFieldCoordinates.
    */
    SparseFieldCoordinates getCoordinates() const;

    /**
        Iterates over the cells of the SparseField.
    */
    struct ConstIterator {
        ConstIterator(const SparseField& sf, const IteratorState state) :
            mSparseField(sf)
        {
            if (state == ITERATOR_BEGIN) {
                mIt = mSparseField.mIndexMap.constBegin();
                location = mSparseField.getXYZFromIndex(mIt.key());
                value = mSparseField.mField.at(mIt.value());
            }
            else if (state == ITERATOR_END) {
                mIt = mSparseField.mIndexMap.constEnd();
            }
        }

        /**
            Visits next cell.
        */
        void operator++() {
            ++mIt;
            if (mIt != mSparseField.mIndexMap.constEnd()) {
                location = mSparseField.getXYZFromIndex(mIt.key());
                value = mSparseField.mField.at(mIt.value());
            }
        }

        /**
            Checks equality with another iterator.
        */
        bool operator==(const ConstIterator& other) const {
            return mIt == other.mIt;
        }

        /**
            Checks inequality with another iterator.
        */
        bool operator!=(const ConstIterator& other) const {
            return mIt != other.mIt;
        }

        /**
            The current location in the grid.
        */
        Vec3i location;

        /**
            The value at the current location.
        */
        float value;

    private:
        LocationIndexToValueIndexMap::ConstIterator mIt;
        const SparseField& mSparseField;
    };

    ConstIterator constBegin() const;
    ConstIterator constEnd() const;

private:

    Vec3i mDimensions;
    Vec3f mOrigin;
    Vec3f mVoxelSize;

    LocationIndexToValueIndexMap mIndexMap;
    Field mField;

    LocationIndexT getIndexFromXYZ(const Vec3i& pos) const;
    Vec3i getXYZFromIndex(const LocationIndexT& index) const;

    static const float DEFAULT_FIELD_VALUE;
};
