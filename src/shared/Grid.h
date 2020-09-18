#ifndef GRID_H
#define GRID_H

#include <QString>
#include <map>
#include <set>
#include <vector>

struct GridProps {
    int id; 
    std::vector<float> center;
    int laminarLocation;
    float corticalDepth;
    int region;
};

struct GridFilter {
  float min_x;
  float max_x;
  float min_y;
  float max_y;
  float min_z;
  float max_z;
  float min_depth;
  float max_depth;
  float min_zAxis;
  float max_zAxis;
  std::set<int> whitelist_region;
};

class Grid
{
  public:
    void load(QString filename);
    std::vector<int> filter(GridFilter filter);
    void save(QString filename, std::vector<int> whitelist);
    void save(QString filename, std::set<int> whitelist = std::set<int>());
  private:
    std::map<int, GridProps> mGrid;

};

#endif