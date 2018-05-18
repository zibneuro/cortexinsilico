#include "CIS3DBoundingBoxes.h"
#include <stdexcept>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDebug>

/**
    Constructor.
*/
BoundingBoxes::BoundingBoxes()
{
}

/**
    Constructor.
    Loads the data from the specified csv-file.
    @param csvFile The data file.
*/
BoundingBoxes::BoundingBoxes(const QString &csvFile)
{
    loadCSV(csvFile);
}

/**
    Retrieves the bounding box of the axonal tree.
    @param id ID of the neuron.
    @return The Bounding Box.
    @throws runtime_error if the the neuron ID is not found.
*/
BoundingBox BoundingBoxes::getAxonBox(const int id) const {
    if (mBoxMap.contains(id)) {
        return mBoxMap.value(id).axonBox;
    }
    QString msg = QString("Error in getAxonBox: no such Neuron ID %1").arg(id);
    throw std::runtime_error(qPrintable(msg));
}

/**
    Retrieves the bounding box of the dendritic tree.
    @param id ID of the neuron.
    @return The Bounding Box.
    @throws runtime_error if the the neuron ID is not found.
*/
BoundingBox BoundingBoxes::getDendriteBox(const int id) const {
    if (mBoxMap.contains(id)) {
        return mBoxMap.value(id).dendBox;
    }
    QString msg = QString("Error in getDendriteBox: no such Neuron ID %1").arg(id);
    throw std::runtime_error(qPrintable(msg));
}

/**
    Adds a NeuronBox to the collection.
    @param box The NeuronBox.
    @throws runtime_error if the the neuron ID already exists.
*/
void BoundingBoxes::addNeuronBox(const NeuronBox& box)
{
    const int id = box.id;
    if (mBoxMap.contains(id)) {
        throw std::runtime_error("Error in addNeuronBox: Neuron ID exists");
    }
    mBoxMap.insert(id, box);
}

/**
    Saves the collection to the specified csv-file.
    @fileName The csv-file.
    @throws runtime_error in case of failure.
*/
void BoundingBoxes::saveCSV(const QString &fileName) const
{
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        const QString msg = QString("Error saving boundingbox file. Could not open file %1").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    QTextStream out(&file);
    const QChar sep = ',';

    out << "ID" << sep;

    out << "DendMinX" << sep;
    out << "DendMinY" << sep;
    out << "DendMinZ" << sep;

    out << "DendMaxX" << sep;
    out << "DendMaxY" << sep;
    out << "DendMaxZ" << sep;

    out << "AxonMinX" << sep;
    out << "AxonMinY" << sep;
    out << "AxonMinZ" << sep;

    out << "AxonMaxX" << sep;
    out << "AxonMaxY" << sep;
    out << "AxonMaxZ" << "\n";


    for (BoxMap::ConstIterator it = mBoxMap.begin(); it != mBoxMap.end(); ++it) {
        const NeuronBox& box = it.value();
        out << box.id << sep;
        out << box.dendBox.getMin().getX() << sep;
        out << box.dendBox.getMin().getY() << sep;
        out << box.dendBox.getMin().getZ() << sep;

        out << box.dendBox.getMax().getX() << sep;
        out << box.dendBox.getMax().getY() << sep;
        out << box.dendBox.getMax().getZ() << sep;

        out << box.axonBox.getMin().getX() << sep;
        out << box.axonBox.getMin().getY() << sep;
        out << box.axonBox.getMin().getZ() << sep;

        out << box.axonBox.getMax().getX() << sep;
        out << box.axonBox.getMax().getY() << sep;
        out << box.axonBox.getMax().getZ() << "\n";
    }
}

/**
    Loads the collection from the specified csv-file.
    @fileName The csv-file.
    @throws runtime_error in case of failure.
*/
void BoundingBoxes::loadCSV(const QString &fileName)
{
    QFile file(fileName);
    QTextStream(stdout) << "[*] Reading bounding boxes from " << fileName << "\n";

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString msg = QString("Error reading boundingbox file. Could not open file %1").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    const QChar sep = ',';
    QTextStream in(&file);

    int lineCount = 1;
    QString line = in.readLine();
    if (line.isNull()) {
        const QString msg = QString("Error reading boundingbox file %1. No content.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    QStringList parts = line.split(sep);
    if (parts.size() != 13 ||
        parts[ 0] != "ID" ||
        parts[ 1] != "DendMinX" ||
        parts[ 2] != "DendMinY" ||
        parts[ 3] != "DendMinZ" ||
        parts[ 4] != "DendMaxX" ||
        parts[ 5] != "DendMaxY" ||
        parts[ 6] != "DendMaxZ" ||
        parts[ 7] != "AxonMinX" ||
        parts[ 8] != "AxonMinY" ||
        parts[ 9] != "AxonMinZ" ||
        parts[10] != "AxonMaxX" ||
        parts[11] != "AxonMaxY" ||
        parts[12] != "AxonMaxZ")
    {
        const QString msg = QString("Error reading boundingbox file %1. Invalid columns.").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }

    line = in.readLine();
    lineCount += 1;

    while (!line.isNull()) {
        parts = line.split(sep);
        if (parts.size() != 13) {
            const QString msg = QString("Error reading boundingbox file %1. Invalid columns.").arg(fileName);
            throw std::runtime_error(qPrintable(msg));
        }

        NeuronBox box;
        box.id = parts[0].toInt();
        box.dendBox.setMin(Vec3f(parts[ 1].toFloat(), parts[ 2].toFloat(), parts[ 3].toFloat()));
        box.dendBox.setMax(Vec3f(parts[ 4].toFloat(), parts[ 5].toFloat(), parts[ 6].toFloat()));
        box.axonBox.setMin(Vec3f(parts[ 7].toFloat(), parts[ 8].toFloat(), parts[ 9].toFloat()));
        box.axonBox.setMax(Vec3f(parts[10].toFloat(), parts[11].toFloat(), parts[12].toFloat()));

        addNeuronBox(box);

        line = in.readLine();
        lineCount += 1;
    }
    QTextStream(stdout) << "[*] Completed reading " <<  mBoxMap.size() << " bounding boxes.\n";
}


/**
    Constructor.
    Lower and upper corner initialized with zero.
*/
BoundingBox::BoundingBox() :
    mMin( 1.0f),
    mMax(-1.0f)
{
}

/**
    Constructor.
    @param minCorner Lower corner.
    @param maxCorner Upper corner.
*/
BoundingBox::BoundingBox(const Vec3f &minCorner, const Vec3f &maxCorner) :
    mMin(minCorner),
    mMax(maxCorner)
{
}

/**
    Retrieve lower corner.
    @return The lower corner.
*/
Vec3f BoundingBox::getMin() const {
    return mMin;
}

/**
    Retrieve upper corner.
    @return The upper corner.
*/
Vec3f BoundingBox::getMax() const {
    return mMax;
}

/**
    Sets the lower corner.
    @param The lower corner.
*/
void BoundingBox::setMin(const Vec3f &minCorner) {
    mMin = minCorner;
}

/**
    Sets the upper corner.
    @param The upper corner.
*/
void BoundingBox::setMax(const Vec3f &maxCorner) {
    mMax = maxCorner;
}

/**
    Checks whether the bounding box covers a positive range in at least
    one dimension. That is, if the upper corner is larger than the lower
    corner in at least one dimension.
    @return true if bounding box covers a positive range.
*/
bool BoundingBox::isEmpty() const {
    return (mMax.getX()<mMin.getX()) || (mMax.getY()<mMin.getY()) || (mMax.getZ()<mMin.getZ());
}

/**
    Checks whether the specified bounding box intersects with this
    bounding box.
    @param other The other bounding box.
    @return true if bounding box intersects.
*/
bool BoundingBox::intersects(const BoundingBox &other) const {

    if (isEmpty() || other.isEmpty()) {
        return false;
    }
    return ((mMin.getX() < other.mMax.getX())    // xmin1 < xmax2
         && (mMax.getX() > other.mMin.getX())    // xmax1 > xmin2
         && (mMin.getY() < other.mMax.getY())    // ymin1 < ymax2
         && (mMax.getY() > other.mMin.getY())    // ymax1 > ymin2
         && (mMin.getZ() < other.mMax.getZ())    // zmin1 < zmax2
         && (mMax.getZ() > other.mMin.getZ()));  // zmax1 > zmin2
}

/**
    Prints the parameters of the bounding box to stdout.
*/
void BoundingBox::print() const {
    if (isEmpty()) {
        QTextStream(stdout) << "Box is empty.\n";
    }
    else {
        QTextStream(stdout) << "X: " << mMin.getX() << " - " << mMax.getX() << "\n";
        QTextStream(stdout) << "Y: " << mMin.getY() << " - " << mMax.getY() << "\n";
        QTextStream(stdout) << "Z: " << mMin.getZ() << " - " << mMax.getZ() << "\n";
    }
}
