#ifndef BOUNDINGBOXES_H
#define BOUNDINGBOXES_H

#include "CIS3DVec3.h"
#include <QMap>

/**
    A spatial datastructure to speed up intersection queries.
*/
class BoundingBox {

public:
    /**
        Constructor.
        Lower and upper corner initialized with zero.
    */
    BoundingBox();

    /**
        Constructor.
        @param minCorner Lower corner.
        @param maxCorner Upper corner.
    */
    BoundingBox(const Vec3f& minCorner, const Vec3f& maxCorner);

    /**
        Retrieve lower corner.
        @return The lower corner.
    */
    Vec3f getMin() const;

    /**
        Retrieve upper corner.
        @return The upper corner.
    */
    Vec3f getMax() const;

    /**
        Sets the lower corner.
        @param The lower corner.
    */
    void setMin(const Vec3f& minCorner);

    /**
        Sets the upper corner.
        @param The upper corner.
    */
    void setMax(const Vec3f& maxCorner);

    /**
        Checks whether the bounding box covers a positive range in at least
        one dimension. That is, if the upper corner is larger than the lower
        corner in at least one dimension.
        @return true if bounding box covers a positive range.
    */
    bool isEmpty() const;

    /**
        Checks whether the specified bounding box intersects with this
        bounding box.
        @param other The other bounding box.
        @return true if bounding box intersects.
    */
    bool intersects(const BoundingBox& other) const;

    /**
        Prints the parameters of the bounding box to stdout.
    */
    void print() const;

private:
    Vec3f mMin;
    Vec3f mMax;

};

/**
    A NeuronBox holds the individual bounding boxes associated with the
    morphological features of a neuron.
*/
struct NeuronBox {
    NeuronBox() :
        id(-1)
    {}

    /**
        The ID of the neuron.
    */
    int id;

    /**
        The bounding box covering the dendtritic tree.
    */
    BoundingBox dendBox;

    /**
        The bounding box covering the axonal tree.
    */
    BoundingBox axonBox;
};

/**
    This class serves as a container for the NeuronBoxes
    associated with a collection of neurons.
*/
class BoundingBoxes {

public:
    /**
        Constructor.
    */
    BoundingBoxes();

    /**
        Constructor.
        Loads the data from the specified csv-file.
        @param csvFile The data file.
    */
    BoundingBoxes(const QString& csvFile);

    /**
        Retrieves the bounding box of the axonal tree.
        @param id ID of the neuron.
        @return The Bounding Box.
        @throws runtime_error if the the neuron ID is not found.
    */
    BoundingBox getAxonBox(const int id) const;

    /**
        Retrieves the bounding box of the dendritic tree.
        @param id ID of the neuron.
        @return The Bounding Box.
        @throws runtime_error if the the neuron ID is not found.
    */
    BoundingBox getDendriteBox(const int id) const;

    /**
        Adds a NeuronBox to the collection.
        @param box The NeuronBox.
        @throws runtime_error if the the neuron ID already exists.
    */
    void addNeuronBox(const NeuronBox& box);

    /**
        Saves the collection to the specified csv-file.
        @fileName The csv-file.
        @throws runtime_error in case of failure.
    */
    void saveCSV(const QString& fileName) const;

    /**
        Loads the collection from the specified csv-file.
        @fileName The csv-file.
        @throws runtime_error in case of failure.
    */
    void loadCSV(const QString& fileName);

private:

    typedef QMap<int, NeuronBox> BoxMap;
    BoxMap mBoxMap;

};

#endif // BOUNDINGBOXES_H
