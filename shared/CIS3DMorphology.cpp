#include "CIS3DMorphology.h"
#include "CIS3DBoundingBoxes.h"
#include <QFile>
#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <stdexcept>


CIS3D::Structure getStructureFromName(const QString& name) {
    if (name.toLower().contains("soma")) {
        return CIS3D::SOMA;
    }
    else if (name.toLower().contains("axon")) {
        return CIS3D::AXON;
    }
    else if (name.toLower().contains("apical")) {
        return CIS3D::APICAL;
    }
    else if (name.toLower().contains("dend")) {
        return CIS3D::DEND;
    }
    else {
        return CIS3D::UNKNOWN;
    }
}


void Section::print() const
{
    qDebug() << "Section:" << name << sectionID << CIS3D::getStructureName(structure);
    for (int p=0; p<points.size(); ++p) {
        QString pointStr = QString("%1\t%2\t%3\tDiam: %4\tSectionX: %5\tSomaDist: %6")
                .arg(points[p].coords.getX(), 6, 'f')
                .arg(points[p].coords.getY(), 6, 'f')
                .arg(points[p].coords.getZ(), 6, 'f')
                .arg(points[p].diameter, 6, 'f')
                .arg(points[p].sectionX, 6, 'f')
                .arg(points[p].distanceToSoma, 6, 'f');
        qDebug().noquote() << pointStr;
    }
    qDebug() << "Parent:" << parentName << parentX << "\n";
}


Section::Section() :
    parentX(0.0f),
    sectionID(-1)
{
}


Section::Section(const QString& n) :
    name(n),
    parentX(0.0f),
    sectionID(-1)
{
    structure = getStructureFromName(n);
}



Morphology::Morphology()
{
}


Morphology::Morphology(const QString &hocFile)
{
    parseHoc(hocFile);
}

const QString identifier("\\w+");
const QString number("[-+]?[0-9]*" + QRegularExpression::escape(".") + "?[0-9]+(?:[eE][-+]?[0-9]+)?");

bool isComment(const QString& line) {
    const QString commentPattern = QRegularExpression::escape("/*") + ".*" + QRegularExpression::escape("*/");
    QRegularExpression commentRE(commentPattern);
    QRegularExpressionMatch match = commentRE.match(line);
    return match.hasMatch();
}


bool isConnect(const QString& line,
               QString& currentName,
               float& currentX,
               QString& parentName,
               float& parentX)
{
    const QString pattern = QRegularExpression::escape("{") +
                            "connect " +
                            "(" + identifier + ")" + QRegularExpression::escape("(") + "(" + number + ")" + QRegularExpression::escape(")") +
                            ", " +
                            "(" + identifier + ")" + QRegularExpression::escape("(") + "(" + number + ")" + QRegularExpression::escape(")") +
                            QRegularExpression::escape("}");

    QRegularExpression re(pattern);
    QRegularExpressionMatch match = re.match(line);
    if (match.hasMatch()) {
        currentName = match.captured(1);
        currentX = match.captured(2).toFloat();
        parentName = match.captured(3);
        parentX = match.captured(4).toFloat();
        return true;
    }
    return false;
}


bool isPt3dadd(const QString& line,
               float& x,
               float& y,
               float& z,
               float& r)
{
    const QString pattern = QRegularExpression::escape("{") +
                            "pt3dadd" +
                            QRegularExpression::escape("(") +
                            "(" + number + ")" + "," + " *" +
                            "(" + number + ")" + "," + " *" +
                            "(" + number + ")" + "," + " *" +
                            "(" + number + ")" +
                            QRegularExpression::escape(")") +
                            QRegularExpression::escape("}");

    QRegularExpression re(pattern);
    QRegularExpressionMatch match = re.match(line);
    if (match.hasMatch()) {
        x = match.captured(1).toFloat();
        y = match.captured(2).toFloat();
        z = match.captured(3).toFloat();
        r = match.captured(4).toFloat();
        return true;
    }
    return false;
}


bool isPt3dclear(const QString& line)
{
    const QString pattern = QRegularExpression::escape("{pt3dclear()}");
    QRegularExpression re(pattern);
    QRegularExpressionMatch match = re.match(line);
    return match.hasMatch();
}


bool isPiaDistance(const QString& line)
{
    const QString pattern = QRegularExpression::escape("{PiaDistance = ") + number + QRegularExpression::escape("}");
    QRegularExpression re(pattern);
    QRegularExpressionMatch match = re.match(line);
    return match.hasMatch();
}


bool isNSeg(const QString& line)
{
    const QString pattern = QRegularExpression::escape("{nseg = ") + "\\d+" + QRegularExpression::escape("}");
    QRegularExpression re(pattern);
    QRegularExpressionMatch match = re.match(line);
    return match.hasMatch();
}


bool isStrdef(const QString& line)
{
    const QString pattern = QRegularExpression::escape("{strdef color color = ") + ".+" + QRegularExpression::escape("}");
    QRegularExpression re(pattern);
    QRegularExpressionMatch match = re.match(line);
    return match.hasMatch();
}


bool isCreate(const QString& line,
              QString& currentName)
{
    const QString pattern = QRegularExpression::escape("{") +
                            "create " +
                            "(" + identifier + ")" +
                            QRegularExpression::escape("}");

    QRegularExpression re(pattern);
    QRegularExpressionMatch match = re.match(line);
    if (match.hasMatch()) {
        currentName = match.captured(1);
        return true;
    }
    return false;
}


bool isAccess(const QString& line,
              QString& currentName)
{
    const QString pattern = QRegularExpression::escape("{") +
                            "access " +
                            "(" + identifier + ")" +
                            QRegularExpression::escape("}");

    QRegularExpression re(pattern);
    QRegularExpressionMatch match = re.match(line);
    if (match.hasMatch()) {
        currentName = match.captured(1);
        return true;
    }
    return false;
}


MorphologyPoint getIntersectionPoint(const QList<MorphologyPoint>& points,
                                     const float fraction)
{
    if (fraction >= 1) {
        return points.last();
    }

    if (fraction <= 0) {
        return points.first();
    }

    float totalLength = 0.0;
    QList<float> cumLength;
    cumLength.append(0.0f);

    for (int i=1; i<points.size(); ++i) {
        const MorphologyPoint& p = points[i-1];
        const MorphologyPoint& q = points[i];
        const Vec3f diff = q.coords - p.coords;
        const float length = diff.length();
        totalLength += length;
        cumLength.append(totalLength);
    }

    QList<float> normCumLength;
    for (int i=0; i<cumLength.size(); ++i) {
        normCumLength.append(cumLength[i]/totalLength);
    }

    int index = 0;
    while (normCumLength[index] < fraction) {
        ++index;
    }

    if (index == 0) {
        return points.first();
    }

    const float denom = normCumLength[index]-normCumLength[index-1];
    if (fabs(denom) < 1.e-5) {
        return points[index];
    }

    const float alpha = (fraction - normCumLength[index-1])/denom;
    const MorphologyPoint p = points[index-1];
    const MorphologyPoint q = points[index];
    const Vec3f coordDiff = q.coords - p.coords;
    const float diamDiff  = q.diameter - p.diameter;
    const float distDiff  = q.distanceToSoma - p.distanceToSoma;
    const float sectionXDiff = q.sectionX - p.sectionX;

    MorphologyPoint iPoint;
    iPoint.coords = p.coords + alpha * coordDiff;
    iPoint.diameter = p.diameter + alpha * diamDiff;
    iPoint.distanceToSoma = p.distanceToSoma + alpha * distDiff;
    iPoint.sectionX = p.sectionX + alpha * sectionXDiff;

    return iPoint;
}


void assignDistanceToSectionsConnectedTo(const QString& parentName, SectionMap& sections) {
    for (SectionMap::Iterator it=sections.begin(); it!=sections.end(); ++it) {
        Section& sec = it.value();
        if (sec.parentName == parentName) {
            const MorphologyPoint iPt = getIntersectionPoint(sections.value(sec.parentName).points, sec.parentX);
            const float initialSegmentLength = (sec.points[0].coords - iPt.coords).length();
            sec.points[0].distanceToSoma = iPt.distanceToSoma + initialSegmentLength;
            for (int p=1; p<sec.points.size(); ++p) {
                const float segmentLength = (sec.points[p-1].coords - sec.points[p].coords).length();
                sec.points[p].distanceToSoma = sec.points[p-1].distanceToSoma + segmentLength;
            }
            assignDistanceToSectionsConnectedTo(sec.name, sections);
        }
    }
}


void Morphology::parseHoc(const QString &hocFile)
{
    QFile hoc(hocFile);
    if (!hoc.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString msg = QString("Error: cannot open file %1").arg(hocFile);
        throw std::runtime_error(qPrintable(msg));
    }

    mSections.clear();
    int lineCount = 1;

    QString currentName, parentName;
    float currentX, parentX;
    float x, y, z, r;

    QString activeSection;
    int nextSectionId = 0;

    while (!hoc.atEnd()) {
        const QString line = QString(hoc.readLine()).trimmed();

        if (isCreate(line, currentName)) {
            mSections.insert(currentName, Section(currentName));
            mSections[currentName].sectionID = nextSectionId;
            ++nextSectionId;
        }
        else if (isAccess(line, currentName)) {
            activeSection = currentName;
        }
        else if (isPt3dadd(line, x, y, z, r)) {
            MorphologyPoint p;
            p.coords = Vec3f(x, y, z);
            p.diameter = r;
            p.distanceToSoma = -1.0f;
            mSections[activeSection].points.append(p);
        }
        else if (isConnect(line, currentName, currentX, parentName, parentX)) {
            if (fabs(currentX) > 1.e-4) {
                throw std::runtime_error("Error: Only connections at section start supported");
            }
            mSections[currentName].parentName = parentName;
            mSections[currentName].parentX = parentX;
        }
        else if (line.isEmpty()) {
        }
        else if (isComment(line)) {
        }
        else if (isPt3dclear(line)) {
        }
        else if (isStrdef(line)) {
        }
        else if (isNSeg(line)) {
        }
        else if (isPiaDistance(line)) {
        }
        else {
            qDebug() << "UNKNOWN STATEMENT" << line;
        }

        ++lineCount;
    }

    // Add the fractional position (sectionX) of each point along the section
    for (SectionMap::Iterator it=mSections.begin(); it!=mSections.end(); ++it) {
        Section& section = it.value();
        QList<float> cumulativeLength;
        if (section.parentName.isEmpty()) {
            cumulativeLength.append(0.0f);
        }
        else {
            const MorphologyPoint iPt = getIntersectionPoint(mSections.value(section.parentName).points, section.parentX);
            const float initialSegmentLength = (section.points[0].coords - iPt.coords).length();
            cumulativeLength.append(initialSegmentLength);
        }

        for (int p=1; p<section.points.size(); ++p) {
            const float segmentLength = (section.points[p].coords - section.points[p-1].coords).length();
            cumulativeLength.append(cumulativeLength[p-1] + segmentLength);
        }

        const float totalLength = cumulativeLength.last();
        for (int p=0; p<section.points.size(); ++p) {
            section.points[p].sectionX = cumulativeLength[p]/totalLength;
        }
    }

    // Add the distance to the soma to each point
    for (SectionMap::Iterator it=mSections.begin(); it!=mSections.end(); ++it) {
        Section& sec = it.value();
        if (sec.structure == CIS3D::SOMA) {
            for (int p=0; p<sec.points.size(); ++p) {
                sec.points[p].distanceToSoma = 0.0f;
            }
        }
        assignDistanceToSectionsConnectedTo(sec.name, mSections);
    }

    /*
    for (SectionMap::ConstIterator it=mSections.constBegin(); it!=mSections.constEnd(); ++it) {
        it.value().print();
    }
    */

    hoc.close();
}


bool LiangBarskyClipTest(const float p, const float q, float& t0, float& t1)
{
    if (p < 0) {
        float t(q / p);
        if (t > t1) {
            return false;
        }
        if (t > t0) {
            t0 = t;
        }
    }
    else if (p > 0) {
        float t(q / p);
        if (t < t0) {
            return false;
        }
        if (t < t1) {
            t1 = t;
        }
    }
    else if (q < 0) {
        return false;
    }
    return true;
}


bool clip(const BoundingBox& box,
          const MorphologyPoint& p0,
          const MorphologyPoint& p1,
          MorphologyPoint& q0,
          MorphologyPoint& q1)
{
    float t0(0.0);
    float t1(1.0);

    q0 = p0;
    q1 = p1;

    const float Dx(q1.coords.getX() - q0.coords.getX());
    if (LiangBarskyClipTest(-Dx, q0.coords.getX() - box.getMin().getX(), t0, t1))
    {
        if (LiangBarskyClipTest(Dx, box.getMax().getX() - q0.coords.getX(), t0, t1))
        {
            const float Dy(q1.coords.getY() - q0.coords.getY());
            if (LiangBarskyClipTest(-Dy, q0.coords.getY() - box.getMin().getY(), t0, t1))
            {
                if (LiangBarskyClipTest(Dy, box.getMax().getY() - q0.coords.getY(), t0, t1))
                {
                    const float Dz(q1.coords.getZ() - q0.coords.getZ());
                    if (LiangBarskyClipTest(-Dz, q0.coords.getZ() - box.getMin().getZ(), t0, t1))
                    {
                        if (LiangBarskyClipTest(Dz, box.getMax().getZ() - q0.coords.getZ(), t0, t1))
                        {
                            const float DDiam = q1.diameter - q0.diameter;
                            const float DDist = q1.distanceToSoma - q0.distanceToSoma;
                            const float DSectionX = q1.sectionX - q0.sectionX;
                            if (t0 > 0)
                            {
                                q0.coords.setX(p0.coords.getX() + t0 * Dx);
                                q0.coords.setY(p0.coords.getY() + t0 * Dy);
                                q0.coords.setZ(p0.coords.getZ() + t0 * Dz);
                                q0.diameter = p0.diameter + t0 * DDiam;
                                q0.distanceToSoma = p0.distanceToSoma + t0 * DDist;
                                q0.sectionX = p0.sectionX + t0 * DSectionX;
                            }
                            if (t1 < 1)
                            {
                                q1.coords.setX(p0.coords.getX() + t1 * Dx);
                                q1.coords.setY(p0.coords.getY() + t1 * Dy);
                                q1.coords.setZ(p0.coords.getZ() + t1 * Dz);
                                q1.diameter = p0.diameter + t1 * DDiam;
                                q1.distanceToSoma = p0.distanceToSoma + t1 * DDist;
                                q1.sectionX = p0.sectionX + t1 * DSectionX;
                            }
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;

}


double getTruncatedConeArea(const double height,
                            const double radius1,
                            const double radius2)
{
    const double radiusDiff = radius2 - radius1;
    const double slantedHeight = sqrt(height*height + radiusDiff*radiusDiff);
    const double area = M_PI * (radius1 + radius2) * slantedHeight;
    return area;
}


SegmentList splitSegmentByGrid(const MorphologyPoint& p0,
                               const MorphologyPoint& p1,
                               const CIS3D::Structure& structure,
                               const int sectionID,
                               const SparseField& grid)
{
    SegmentList result;
    const Vec3i c0 = grid.getVoxelContainingPoint(p0.coords);
    const Vec3i c1 = grid.getVoxelContainingPoint(p1.coords);

    if (c0 == c1) {
        Segment s;
        s.p0 = p0;
        s.p1 = p1;
        s.length = (p0.coords-p1.coords).length();
        //s.area = getTruncatedConeArea(s.length, p0.diameter*0.5, p1.diameter*0.5); // Correct
        // Amira NeuroNet uses diameter instead of radius to compute surface.
        // This is a bug: it systematically increases inhibitory PST densities.
        // However, when it is required to compute results that are
        // consistent with this behaviour, uncomment the following line,
        // and the similar line in the next block:
        s.area = getTruncatedConeArea(s.length, p0.diameter, p1.diameter);         // Bug
        s.structure = structure;
        s.voxel = c0;
        s.sectionID = sectionID;
        result.append(s);
    }
    else {
        for (int z=qMin(c0.getZ(), c1.getZ()); z<=qMax(c0.getZ(), c1.getZ()); ++z) {
            for (int y=qMin(c0.getY(), c1.getY()); y<=qMax(c0.getY(), c1.getY()); ++y) {
                for (int x=qMin(c0.getX(), c1.getX()); x<=qMax(c0.getX(), c1.getX()); ++x) {
                    const Vec3i cell(x, y, z);
                    const BoundingBox box = grid.getVoxelBox(cell);
                    MorphologyPoint q0, q1;
                    if (clip(box, p0, p1, q0, q1)) {
                        Segment s;
                        s.p0 = q0;
                        s.p1 = q1;
                        s.length = (q0.coords-q1.coords).length();
                        //s.area = getTruncatedConeArea(s.length, q0.diameter * 0.5, q1.diameter * 0.5); // Correct
                        s.area = getTruncatedConeArea(s.length, q0.diameter, q1.diameter);             // Bug
                        s.structure = structure;
                        s.voxel = cell;
                        s.sectionID = sectionID;
                        result.append(s);
                    }
                }
            }
        }
    }
    return result;
}


SegmentList Morphology::getGridSegments(const SparseField &grid) const
{
    SegmentList segments;
    for (SectionMap::ConstIterator it=mSections.constBegin(); it!=mSections.constEnd(); ++it) {
        const Section& sec = it.value();
        if (!sec.parentName.isEmpty()) {
            MorphologyPoint p = getIntersectionPoint(mSections.value(sec.parentName).points, sec.parentX);
            SegmentList splitSegments = splitSegmentByGrid(p, sec.points[0], sec.structure, sec.sectionID, grid);
            segments.append(splitSegments);
        }

        for (int p=1; p<sec.points.size(); ++p) {
            SegmentList splitSegments = splitSegmentByGrid(sec.points[p-1], sec.points[p], sec.structure, sec.sectionID, grid);
            segments.append(splitSegments);
        }
    }

    return segments;
}


BoundingBox Morphology::getBoundingBox() const
{
    Vec3f minPt, maxPt;
    for (SectionMap::ConstIterator it=mSections.constBegin(); it!=mSections.constEnd(); ++it) {
        const Section& sec = it.value();
        if (it == mSections.constBegin()) {
            minPt = sec.points[0].coords;
            maxPt = sec.points[0].coords;
        }
        for (int p=0; p<sec.points.size(); ++p) {
            const Vec3f& pt = sec.points[p].coords;
            if (pt.getX() < minPt.getX()) minPt.setX(pt.getX());
            if (pt.getY() < minPt.getY()) minPt.setY(pt.getY());
            if (pt.getZ() < minPt.getZ()) minPt.setZ(pt.getZ());
            if (pt.getX() > maxPt.getX()) maxPt.setX(pt.getX());
            if (pt.getY() > maxPt.getY()) maxPt.setY(pt.getY());
            if (pt.getZ() > maxPt.getZ()) maxPt.setZ(pt.getZ());
        }
    }
    return BoundingBox(minPt, maxPt);
}


int Morphology::getNumberOfSections() const {
    return mSections.size();
}

QString Morphology::getSectionName(const int sectionID) const
{
    for (SectionMap::ConstIterator it=mSections.constBegin(); it!=mSections.constEnd(); ++it) {
        const Section& section = it.value();
        if (section.sectionID == sectionID) {
            return section.name;
        }
    }
    const QString msg = QString("Error: cannot get section name for section %1").arg(sectionID);
    throw std::runtime_error(qPrintable(msg));
}


CIS3D::Structure Morphology::getSectionStructure(const int sectionID) const
{
    for (SectionMap::ConstIterator it=mSections.constBegin(); it!=mSections.constEnd(); ++it) {
        const Section& section = it.value();
        if (section.sectionID == sectionID) {
            return section.structure;
        }
    }
    const QString msg = QString("Error: cannot get section structure for section %1").arg(sectionID);
    throw std::runtime_error(qPrintable(msg));

}
