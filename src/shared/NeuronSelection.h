#include "CIS3DConstantsHelpers.h"
#include "CIS3DNetworkProps.h"
#include "RandomGenerator.h"
#include "Typedefs.h"
#include <QJsonObject>
#include <QString>
#include <QVector>
#include <map>
#include <set>

#ifndef NEURONSELECTION_H
#define NEURONSELECTION_H

/**
    Encapsulates the selected neuron subppulations that
    are part of an analytic query.
*/
class NeuronSelection {
public:
  /**
    Empty constructor.
  */
  NeuronSelection();

  NeuronSelection(const IdList &selectionA, const IdList &selectionB,
                  const IdList &selectionC);

  /**
    Filters neurons in a slice model based on tissue depth.
    param side: 0 left band, 1 right band, 2 any band
  */
  static IdList
  filterTissueDepth(const NetworkProps &networkProps, IdList &preFiltered,
                    double sliceRef, double low, double high,
                    CIS3D::SliceBand band = CIS3D::SliceBand::BOTH);

  void setPiaSomaDistance(QVector<float> rangePre, QVector<float> rangePost,
                          const NetworkProps &networkProps);

  void setFullModel(const NetworkProps &networkProps, bool uniquePre = true);

  void filterPiaSoma(IdList &neuronIds, QVector<float> range,
                     const NetworkProps &networkProps);

  void filterSlice(const NetworkProps &networkProps, double sliceRef,
                   QJsonObject &tissueA, QJsonObject &tissueB,
                   QJsonObject &tissueC);

  void setInnervationSelection(const QJsonObject &spec,
                               const NetworkProps &networkProps,
                               int samplingFactor = 1, int seed = -1);

  void setSelectionFromQuery(const QJsonObject &query,
                             NetworkProps &networkProps,
                             const QJsonObject &config);

  void setFixedSelection(QString filenameA, QString filenameB, QString filenameC, QString networkName, NetworkProps &networkProps, const QJsonObject &config);

  void loadFixedInto(QString filename, IdList& selection, NetworkProps &networkProps, bool updateAxon = true);

  /**
      Determines neuron IDs based on a selection string;
      @param selectionString The selection string.
      @param networkProps The model data of the network.
      @return A list of neuron IDs.
  */
  IdList
  getSelectedNeurons(const QString selectionString,
                     const NetworkProps &networkProps,
                     CIS3D::SynapticSide synapticSide = CIS3D::BOTH_SIDES);

  /**
    Returns the neuron selection A.
  */
  IdList SelectionA() const;

  /**
    Returns the neuron selection B.
  */
  IdList SelectionB() const;

  /**
    Returns the neuron selection C.
  */
  IdList SelectionC() const;

  CIS3D::SliceBand getBandA(int id) const;

  CIS3D::SliceBand getBandB(int id) const;

  CIS3D::SliceBand getBandC(int id) const;

  void printSelectionStats();

  void setBBox(QVector<float> min, QVector<float> max);

  QVector<float> getBBoxMin();

  QVector<float> getBBoxMax();

  QVector<float> getPiaSomaDistancePre();

  QVector<float> getPiaSomaDistancePost();

  void filterUniquePre(const NetworkProps &networkProps);

  void sampleDown(int maxSize, int seed);

  void sampleDownFactor(int samplingFactor, int seed);

  static IdList filterPreOrBoth(const NetworkProps &networkProps, IdList ids);

  CIS3D::Structure getPostTarget(int selectionIndex) const;

  static IdList getDownsampledFactor(IdList &original, int factor,
                                     RandomGenerator &randomGenerator);

  QString getNetworkName();

  void setNetworkName(QString name);

  QString getDataRoot();

  void setDataRoot(QString dataRoot);

  bool isValid(QJsonObject &query, QString &errorMessage);

  std::map<int, int> doGetMultiplicities(const NetworkProps &network,
                                         IdList &selection);

  std::map<int, int> getMultiplicities(const NetworkProps &network,
                                       QString selectionIndex);

  /**
   * @brief Returns a set of explicitly selected voxels (specification parameter
   * VOXEL_WHITELIST).
   * @return The selected voxels. Empty by default.
   */
  std::set<int> getVoxelWhitelist();

  bool useSliceUniquePre();

  int getRBCId(int neuronId) const;

private:
  static bool inSliceBand(double somaX, double min, double max);

  static void inSliceRange(double somaX, double sliceRef, double low,
                           double high, bool &first, bool &second);

  IdList getDownsampled(IdList &original, int maxSize,
                        RandomGenerator &randomGenerator);

  void processSelection(QJsonObject &networkSelection, int number,
                        const NetworkProps &networkProps,
                        QJsonObject &selectionA, QJsonObject &selectionB,
                        QJsonObject &selectionC, bool prune = false);

  void setPostTarget(CIS3D::Structure selectionA, CIS3D::Structure selectionB,
                     CIS3D::Structure selectionC = CIS3D::DEND);

  void copySelection(NeuronSelection &selection);

  void pruneSelection(NeuronSelection &selection);

  void getTissueDepthParameters(QJsonObject &tissueDepth, double &low,
                                double &high, QString &mode);

  void clear();

  std::set<int> getAllowedIds(const IdList &selection,
                              std::map<int, int> &mapping);

  void pruneIds(IdList &selection, std::set<int> &allowed);

  std::map<int, int> readMapping(NeuronSelection &selection);

  std::map<int, int> readMapping(QString path);

  bool isSelectionValid(QJsonObject &selection, QString index,
                        QString &errorMessage);

  void correctSynapticSide(CIS3D::SynapticSide &sideA,
                           CIS3D::SynapticSide &sideB,
                           CIS3D::SynapticSide &sideC,
                           bool aEnabled,
                           bool bEnabled,
                           bool cEnabled);

  IdList mSelectionA;
  IdList mSelectionB;
  IdList mSelectionC;

  std::vector<CIS3D::Structure> mPostTarget;
  std::map<int, CIS3D::SliceBand> mBandA;
  std::map<int, CIS3D::SliceBand> mBandB;
  std::map<int, CIS3D::SliceBand> mBandC;

  QVector<float> mBBoxMin;
  QVector<float> mBBoxMax;
  QVector<float> mPiaSomaDistancePre;
  QVector<float> mPiaSomaDistancePost;
  QString mNetworkName;
  QString mDataRoot;
  QString mMappingDir;
  std::set<int> mVoxelWhitelist;
  QString mQueryType;
  bool mIsSlice; 
  bool mSliceUniquePre;
  std::map<int, int> mMappingSliceRBC;
};

#endif
