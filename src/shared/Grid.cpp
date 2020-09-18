#include "Grid.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <math.h>

void Grid::load(QString filename) {
  qDebug() << "load" << filename;
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    const QString msg =
        QString("Error reading features file. Could not open file %1")
            .arg(filename);
    throw std::runtime_error(qPrintable(msg));
  }

  QTextStream in(&file);
  in.readLine();
  QString line = in.readLine();
  line = in.readLine();
  while (!line.isNull()) {
    QStringList parts = line.split(',');
    GridProps props;
    props.id = parts[0].toInt();
    QStringList centerStr = parts[1].split("_");
    std::vector<float> center;
    center.push_back(centerStr[0].toFloat());
    center.push_back(centerStr[1].toFloat());
    center.push_back(centerStr[2].toFloat());
    props.center = center;
    props.laminarLocation = parts[2].toInt();
    props.corticalDepth = parts[3].toFloat();
    props.region = parts[4].toInt();
    mGrid[props.id] = props;
    line = in.readLine();
  }
}

std::vector<int> Grid::filter(GridFilter filter) {
  std::vector<int> ids;
  for (auto it = mGrid.begin(); it != mGrid.end(); it++) {
    GridProps p = it->second;
    float x = p.center[0];
    float y = p.center[1];
    float z = p.center[2];
    float d = p.corticalDepth;
    float r = sqrt(x*x + y*y);
    if (x >= filter.min_x && x <= filter.max_x && y >= filter.min_y &&
        y <= filter.max_y && z >= filter.min_z && z <= filter.max_z &&
        d >= filter.min_depth && d <= filter.max_depth && r>= filter.min_zAxis && r <= filter.max_zAxis) {
      if (filter.whitelist_region.empty() ||
          (filter.whitelist_region.find(p.region) !=
           filter.whitelist_region.end())) {
        ids.push_back(it->first);
      }
    }
  }
  return ids;
}

void Grid::save(QString filename, std::vector<int> whitelist) {
  std::set<int> foo;
  for (unsigned int i = 0; i < whitelist.size(); i++) {
    foo.insert(whitelist[i]);
  }
  save(filename, foo);
}

void Grid::save(QString filename, std::set<int> whitelist) {
  QFile voxelFile(filename);
  if (!voxelFile.open(QIODevice::WriteOnly)) {
    const QString msg =
        QString("Cannot open file %1 for writing.").arg(filename);
    throw std::runtime_error(qPrintable(msg));
  }
  QTextStream stream(&voxelFile);
  stream << "id,subvolume_center_x,subvolume_center_y,subvolume_center_z,"
            "laminar_location,cortical_depth,region\n";
  for (auto it = mGrid.begin(); it != mGrid.end(); it++) {
    if (whitelist.empty() || whitelist.find(it->first) != whitelist.end()) {
      GridProps props = it->second;
      stream << it->first << "," << props.center[0] << "," << props.center[1]
             << "," << props.center[2] << "," << props.laminarLocation << ","
             << props.corticalDepth << "," << props.region << "\n";
    }
  }
  voxelFile.close();
}