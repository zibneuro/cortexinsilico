#ifndef VOXELQUERYHANDLER_H
#define VOXELQUERYHANDLER_H

#include "QueryHandler.h"
#include "CIS3DNetworkProps.h"
#include "CIS3DStatistics.h"
#include "FeatureProvider.h"
#include "Histogram.h"
#include "QueryHelpers.h"
#include "RandomGenerator.h"
#include <QJsonObject>
#include <QString>

class VoxelQueryHandler : public QueryHandler {

public:
  VoxelQueryHandler();

private:
  void doProcessQuery() override;
  QString getResultKey() override;
  bool initSelection() override;

  void setFilterString(QString mode, QString preFilter, QString postFilter,
                       QString advancedSettings, std::vector<double> origin,
                       std::vector<int> dimensions);

  QJsonObject createJsonResult(bool createFile);
  float calculateSynapseProbability(float innervation, int k);
  void readIndex(QString indexFile, std::vector<double> origin,
                 std::vector<int> dimensions, std::set<int> &voxelIds,
                 std::set<int> &neuronIds);
  template <typename T>
  void createStatistics(std::map<int, T> &values, Statistics &stat,
                        Histogram &histogram);

  Statistics mStatistics;
  QString mTempFolder;

  int mNumberInnervatedVoxels;
  int mNumberTotalVoxels;
  Statistics mPresynapticCellsPerVoxel;
  Histogram mPresynapticCellsPerVoxelH;
  Statistics mPostsynapticCellsPerVoxel;
  Histogram mPostsynapticCellsPerVoxelH;  
  Statistics mBoutonsPerVoxel;
  Histogram mBoutonsPerVoxelH;
  Statistics mPostsynapticSitesPerVoxel;
  Histogram mPostsynapticSitesPerVoxelH;

  std::map<int, float> mMapBoutonsPerVoxel;
  std::map<int, int> mMapPreCellsPerVoxel;
  std::map<int, int> mMapPreBranchesPerVoxel;
  std::map<int, float> mMapPostsynapticSitesPerVoxel;
  std::map<int, int> mMapPostCellsPerVoxel;
  std::map<int, int> mMapPostBranchesPerVoxel;
  std::set<int> mSelectedVoxels;
  std::set<int> mPreInnervatedVoxels;
  std::set<int> mPostInnervatedVoxels;
  std::map<int, float> mSynapsesPerVoxel;
  Statistics mSynapsesPerConnection;
  std::map<int, std::vector<float>> mSynapsesPerConnectionOccurrences;
  QString mFilterString;


};

#endif // VOXELQUERYHANDLER_H
