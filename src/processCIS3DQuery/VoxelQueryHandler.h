#ifndef VOXELQUERYHANDLER_H
#define VOXELQUERYHANDLER_H

#include "QueryHandler.h"
#include "CIS3DNetworkProps.h"
#include "CIS3DStatistics.h"
#include "FeatureProvider.h"
#include "Histogram.h"
#include "QueryHelpers.h"
#include "RandomGenerator.h"
#include "Distribution.h"
#include <QJsonObject>
#include <QString>
#include <vector>


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

  QString getMode(QJsonObject& mQuery);
  void determineCellCounts(int voxelId);
  void determineBranchLengths(int voxelId);
  void determineSnippets(int voxelId);

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
  Distribution mSynapsesCubicMicron;
  Distribution mAxonDendriteRatio;

  std::map<int, QString> mVoxelIdVoxelStr;
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
  std::map<int, int> mPreCellbodiesPerVoxel;
  std::map<int, int> mPostCellbodiesPerVoxel;
  std::map<int, float> mAxonLengthPerVoxel;
  std::map<int, float> mDendriteLengthPerVoxel;
  std::vector<QString> mSubvolumes;
  std::set<int> mPreIds;
  std::set<int> mPostIds;
  std::map<int, float> mVariabilityCellbodies;
  std::map<int, float> mVariabilityAxon;
  std::map<int, float> mVariabilityDendrite;
  std::map<int, std::vector<float> > mTestOutput; 
};

#endif // VOXELQUERYHANDLER_H
