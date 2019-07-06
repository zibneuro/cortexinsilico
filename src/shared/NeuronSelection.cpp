#include "NeuronSelection.h"
#include "Columns.h"
#include "Util.h"
#include "UtilIO.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <iostream>

/**
  Empty constructor.
*/
NeuronSelection::NeuronSelection() {
  mPostTarget.push_back(CIS3D::DEND);
  mPostTarget.push_back(CIS3D::DEND);
  mPostTarget.push_back(CIS3D::DEND);
};

NeuronSelection::NeuronSelection(const IdList &selectionA,
                                 const IdList &selectionB,
                                 const IdList &selectionC)
    : mSelectionA(selectionA), mSelectionB(selectionB),
      mSelectionC(selectionC) {
  mPostTarget.push_back(CIS3D::DEND);
  mPostTarget.push_back(CIS3D::DEND);
  mPostTarget.push_back(CIS3D::DEND);

  mBBoxMin.append(-10000);
  mBBoxMin.append(-10000);
  mBBoxMin.append(-10000);
  mBBoxMax.append(10000);
  mBBoxMax.append(10000);
  mBBoxMax.append(10000);
}

IdList NeuronSelection::filterTissueDepth(const NetworkProps &networkProps,
                                          IdList &neuronIds, double sliceRef,
                                          double low, double high,
                                          CIS3D::SliceBand band) {
  if (sliceRef == -9999) {
    return neuronIds;
  }

  IdList pruned;

  for (auto it = neuronIds.begin(); it != neuronIds.end(); ++it) {
    Vec3f soma = networkProps.neurons.getSomaPosition(*it);
    double somaX = (double)soma.getX();
    bool first;
    bool second;
    inSliceRange(somaX, sliceRef, low, high, first, second);
    if (band == CIS3D::SliceBand::FIRST && first) {
      pruned.push_back(*it);
    } else if (band == CIS3D::SliceBand::SECOND && second) {
      pruned.push_back(*it);
    } else if (band == CIS3D::SliceBand::BOTH && (first || second)) {
      pruned.push_back(*it);
    }
  }
  return pruned;
}

/**
    Determines neuron IDs based on a selection string;
    @param selectionString The selection string.
    @param networkProps The model data of the network.
    @return A list of neuron IDs.
*/
IdList NeuronSelection::getSelectedNeurons(const QString selectionString,
                                           const NetworkProps &networkProps,
                                           CIS3D::SynapticSide synapticSide) {
  QJsonDocument doc = QJsonDocument::fromJson(selectionString.toLocal8Bit());
  QJsonArray arr = doc.array();
  SelectionFilter filter =
      Util::getSelectionFilterFromJson(arr, networkProps, synapticSide);
  Util::correctVPMSelectionFilter(filter, networkProps);
  Util::correctInterneuronSelectionFilter(filter, networkProps);
  return networkProps.neurons.getFilteredNeuronIds(filter);
}

/**
  Returns the first neuron subselection for motif statistics.
*/
IdList NeuronSelection::SelectionA() const { return mSelectionA; }

/**
  Returns the second neuron subselection for motif statistics.
*/
IdList NeuronSelection::SelectionB() const { return mSelectionB; }

/**
  Returns the third neuron subselection for motif statistics.
*/
IdList NeuronSelection::SelectionC() const { return mSelectionC; }

CIS3D::SliceBand NeuronSelection::getBandA(int id) const {
  auto it = mBandA.find(id);
  if (it != mBandA.end()) {
    return it->second;
  } else {
    return CIS3D::SliceBand::FIRST;
  }
}

CIS3D::SliceBand NeuronSelection::getBandB(int id) const {
  auto it = mBandB.find(id);
  if (it != mBandB.end()) {
    return it->second;
  } else {
    return CIS3D::SliceBand::FIRST;
  }
}

CIS3D::SliceBand NeuronSelection::getBandC(int id) const {
  auto it = mBandC.find(id);
  if (it != mBandC.end()) {
    return it->second;
  } else {
    return CIS3D::SliceBand::FIRST;
  }
}

/*
    Prints the number of selected neurons for motif statistics.
*/
void NeuronSelection::printSelectionStats() {
  qDebug() << "[*] Number of selected neurons (A,B,C):" << mSelectionA.size()
           << mSelectionB.size() << mSelectionC.size();
}

void NeuronSelection::setBBox(QVector<float> min, QVector<float> max) {
  mBBoxMin = min;
  mBBoxMax = max;
}

QVector<float> NeuronSelection::getBBoxMin() { return mBBoxMin; }

QVector<float> NeuronSelection::getBBoxMax() { return mBBoxMax; }

void NeuronSelection::setPiaSomaDistance(QVector<float> rangePre,
                                         QVector<float> rangePost,
                                         const NetworkProps &networkProps) {
  // qDebug() << rangePre << rangePost;
  mPiaSomaDistancePre = rangePre;
  mPiaSomaDistancePost = rangePost;
  filterPiaSoma(mSelectionA, rangePre, networkProps);
  filterPiaSoma(mSelectionB, rangePost, networkProps);
}

void NeuronSelection::setFullModel(const NetworkProps &networkProps,
                                   bool uniquePre) {
  mSelectionA.clear();
  mSelectionB.clear();

  SelectionFilter preFilter;
  preFilter.synapticSide = CIS3D::PRESYNAPTIC;
  IdList preNeurons = networkProps.neurons.getFilteredNeuronIds(preFilter);
  for (int i = 0; i < preNeurons.size(); i++) {
    int cellTypeId = networkProps.neurons.getCellTypeId(preNeurons[i]);
    if (networkProps.cellTypes.isExcitatory(cellTypeId)) {
      mSelectionA.append(preNeurons[i]);
    }
  }
  if (uniquePre) {
    filterUniquePre(networkProps);
  }

  SelectionFilter postFilter;
  postFilter.synapticSide = CIS3D::POSTSYNAPTIC;
  IdList postNeurons = networkProps.neurons.getFilteredNeuronIds(postFilter);
  for (int i = 0; i < postNeurons.size(); i++) {
    int cellTypeId = networkProps.neurons.getCellTypeId(postNeurons[i]);
    if (networkProps.cellTypes.isExcitatory(cellTypeId)) {
      mSelectionB.append(postNeurons[i]);
    }
  }
}

void NeuronSelection::filterPiaSoma(IdList &neuronIds, QVector<float> range,
                                    const NetworkProps &networkProps) {
  Columns columns;
  if (range.size() == 2) {
    qDebug() << "[*] Filter pia soma distance" << range;
    IdList pruned;
    for (auto it = neuronIds.begin(); it != neuronIds.end(); ++it) {
      int regionId = networkProps.neurons.getRegionId(*it);
      QString regionName = networkProps.regions.getName(regionId);
      Vec3f soma = networkProps.neurons.getSomaPosition(*it);
      if (columns.inRange(regionName, soma, range)) {
        pruned.push_back(*it);
      }
    }
    neuronIds = pruned;
  }
}

void applyTissueDepthFilter(IdList &selectionIds,
                            std::map<int, CIS3D::SliceBand> &selectionBand,
                            const NetworkProps &networkProps, double sliceRef,
                            double low, double high, QString mode) {
  if (mode == "twoSided") {
    selectionIds =
        NeuronSelection::filterTissueDepth(networkProps, selectionIds, sliceRef,
                                           low, high, CIS3D::SliceBand::BOTH);
    IdList first =
        NeuronSelection::filterTissueDepth(networkProps, selectionIds, sliceRef,
                                           low, high, CIS3D::SliceBand::FIRST);
    for (auto it = first.begin(); it != first.end(); it++) {
      selectionBand[*it] = CIS3D::SliceBand::FIRST;
    }

    IdList second =
        NeuronSelection::filterTissueDepth(networkProps, selectionIds, sliceRef,
                                           low, high, CIS3D::SliceBand::SECOND);

    for (auto it = second.begin(); it != second.end(); it++) {
      selectionBand[*it] = CIS3D::SliceBand::SECOND;
    }
  } else {
    selectionIds =
        NeuronSelection::filterTissueDepth(networkProps, selectionIds, sliceRef,
                                           low, high, CIS3D::SliceBand::FIRST);
    for (auto it = selectionIds.begin(); it != selectionIds.end(); it++) {
      selectionBand[*it] = CIS3D::SliceBand::FIRST;
    }
  }
}

void NeuronSelection::filterSlice(const NetworkProps &networkProps,
                                  double sliceRef, QJsonObject &tissueA,
                                  QJsonObject &tissueB, QJsonObject &tissueC) {
  double lowA, lowB, lowC, highA, highB, highC;
  QString modeA, modeB, modeC;
  getTissueDepthParameters(tissueA, lowA, highA, modeA);
  getTissueDepthParameters(tissueB, lowB, highB, modeB);
  getTissueDepthParameters(tissueC, lowC, highC, modeC);

  applyTissueDepthFilter(mSelectionA, mBandA, networkProps, sliceRef, lowA,
                         highA, modeA);
  applyTissueDepthFilter(mSelectionB, mBandB, networkProps, sliceRef, lowB,
                         highB, modeB);
  applyTissueDepthFilter(mSelectionC, mBandC, networkProps, sliceRef, lowC,
                         highC, modeC);
}

void NeuronSelection::setSelectionFromQuery(const QJsonObject &query,
                                            NetworkProps &networkProps,
                                            const QJsonObject &config) {
  // qDebug() << "set selection from query" << query;
  QJsonObject networkSelection = query["networkSelection"].toObject();
  int number = query["networkNumber"].toInt();
  QString networkName = Util::getShortName(networkSelection, number);
  QJsonObject cellSelection = query["cellSelection"].toObject();
  QJsonObject selectionA = cellSelection["selectionA"].toObject();
  QJsonObject selectionB = cellSelection["selectionB"].toObject();
  QJsonObject selectionC = cellSelection["selectionC"].toObject();
  mQueryType = query["queryType"].toString();    
  mSliceUniquePre = Util::matchCells(networkSelection, Util::getOppositeNetworkNumber(number));

  QString dataRoot = Util::getDatasetPath(networkName, config);
  networkProps.setDataRoot(dataRoot);
  networkProps.loadFilesForQuery();

  processSelection(networkSelection, number, networkProps, selectionA,
                   selectionB, selectionC);

  if (Util::matchCells(networkSelection, number)) {
    mSliceUniquePre = true;
    int oppositeNumber = Util::getOppositeNetworkNumber(number);
    QString networkName2 = Util::getShortName(networkSelection, oppositeNumber);
    QString dataRoot2 = Util::getDatasetPath(networkName2, config);
    mMappingDir = config["NEURON_MAPPING_DIRECTORY"].toString();
    NetworkProps networkProps2;
    networkProps2.setDataRoot(dataRoot2);
    networkProps2.loadFilesForQuery();

    processSelection(networkSelection, oppositeNumber, networkProps2,
                     selectionA, selectionB, selectionC, true);

    QDir mappingDir = QDir(mMappingDir);
    QString remappedAxonFile = CIS3D::getRemappedAxonFilePath(mappingDir, networkName, networkName2);
    //networkProps.axonRedundancyMap.loadFlatFile(remappedAxonFile);    
  }
}

IdList getExplicitIds(const QJsonObject &spec, QString key) {
  IdList ids;
  if (spec[key] != QJsonValue::Undefined) {
    QJsonArray jsonIds = spec[key].toArray();
    for (int i = 0; i < jsonIds.size(); i++) {
      ids.append(jsonIds[i].toInt());
    }
  }
  return ids;
}

void NeuronSelection::setInnervationSelection(const QJsonObject &spec,
                                              const NetworkProps &networkProps,
                                              int samplingFactor, int seed) {
  mVoxelWhitelist = UtilIO::getVoxelWhitelist(spec);

  qDebug() << "whitelist size" << mVoxelWhitelist.size();

  IdList explicitA = getExplicitIds(spec, "PRE_NEURON_IDS");
  IdList explicitB = getExplicitIds(spec, "POST_NEURON_IDS");
  if (explicitA.size() > 0 && explicitB.size() > 0) {
    mSelectionA.append(explicitA);
    mSelectionB.append(explicitB);
    return;
  }

  IdList pre = UtilIO::getPreSynapticNeurons(spec, networkProps);
  IdList post = UtilIO::getPostSynapticNeuronIds(spec, networkProps);
  if (samplingFactor == 1) {
    mSelectionA.append(pre);
    mSelectionB.append(post);
  } else {
    RandomGenerator randomGenerator(seed);
    randomGenerator.shuffleList(pre);
    randomGenerator.shuffleList(post);
    for (int i = 0; i < pre.size(); i += samplingFactor) {
      mSelectionA.append(pre[i]);
    }
    for (int i = 0; i < post.size(); i += samplingFactor) {
      mSelectionB.append(post[i]);
    }
  }
}

void NeuronSelection::processSelection(QJsonObject &networkSelection,
                                       int number,
                                       const NetworkProps &networkProps,
                                       QJsonObject &selectionA,
                                       QJsonObject &selectionB,
                                       QJsonObject &selectionC, bool prune) {
  
  QJsonArray conditionsA = selectionA["conditions"].toArray();
  QJsonArray conditionsB = selectionB["conditions"].toArray();
  QJsonArray conditionsC = selectionC["conditions"].toArray();

  bool enabledA = selectionA["enabled"].toBool();
  bool enabledB = selectionB["enabled"].toBool();
  bool enabledC = selectionC["enabled"].toBool();

  CIS3D::SynapticSide sideA = Util::getSynapticSide(selectionA);
  CIS3D::SynapticSide sideB = Util::getSynapticSide(selectionB);
  CIS3D::SynapticSide sideC = Util::getSynapticSide(selectionC);

  QString networkName = Util::getShortName(networkSelection, number);
  if (Util::isSlice(networkName)) {
    correctSynapticSide(sideA, sideB, sideC, enabledA, enabledB, enabledC);
  }

  IdList neuronsA, neuronsB, neuronsC;

  if (enabledA) {
    SelectionFilter filterA =
        Util::getSelectionFilterFromJson(conditionsA, networkProps, sideA);
    Util::correctVPMSelectionFilter(filterA, networkProps);
    Util::correctInterneuronSelectionFilter(filterA, networkProps);
    neuronsA = networkProps.neurons.getFilteredNeuronIds(filterA);
  }

  if (enabledB) {
    SelectionFilter filterB =
        Util::getSelectionFilterFromJson(conditionsB, networkProps, sideB);
    Util::correctVPMSelectionFilter(filterB, networkProps);
    Util::correctInterneuronSelectionFilter(filterB, networkProps);
    neuronsB = networkProps.neurons.getFilteredNeuronIds(filterB);
  }

  if (enabledC) {
    SelectionFilter filterC =
        Util::getSelectionFilterFromJson(conditionsC, networkProps, sideC);
    Util::correctVPMSelectionFilter(filterC, networkProps);
    Util::correctInterneuronSelectionFilter(filterC, networkProps);
    neuronsC = networkProps.neurons.getFilteredNeuronIds(filterC);
  }

  double sliceRef;

  NeuronSelection tmpSelection(neuronsA, neuronsB, neuronsC);

  if (Util::isSlice(networkSelection, number, sliceRef)) {
    QJsonObject tissueA = selectionA["tissueDepth"].toObject();
    QJsonObject tissueB = selectionB["tissueDepth"].toObject();
    QJsonObject tissueC = selectionC["tissueDepth"].toObject();

    tmpSelection.filterSlice(networkProps, sliceRef, tissueA, tissueB, tissueC);
  }

  int samplingFactor, randomSeed;
  if (Util::isSampled(networkSelection, number, samplingFactor, randomSeed)) {
    tmpSelection.sampleDownFactor(samplingFactor, randomSeed);
  }

  if (prune) {
    tmpSelection.setNetworkName(Util::getShortName(networkSelection, number));
    tmpSelection.setDataRoot(networkProps.dataRoot);
    pruneSelection(tmpSelection);
  } else {
    mDataRoot = networkProps.dataRoot;
    mNetworkName = Util::getShortName(networkSelection, number);
    copySelection(tmpSelection);
    CIS3D::Structure postTargetA =
        Util::getPostsynapticTarget(conditionsA);
    CIS3D::Structure postTargetB =
        Util::getPostsynapticTarget(conditionsB);
    CIS3D::Structure postTargetC =
        Util::getPostsynapticTarget(conditionsC);
    setPostTarget(postTargetA, postTargetB, postTargetC);
  }
}

void NeuronSelection::getTissueDepthParameters(QJsonObject &tissueDepth,
                                               double &low, double &high,
                                               QString &mode) {
  low = tissueDepth["low"].toDouble();
  high = tissueDepth["high"].toDouble();
  mode = tissueDepth["mode"].toString();
}

QVector<float> NeuronSelection::getPiaSomaDistancePre() {
  return mPiaSomaDistancePre;
}

QVector<float> NeuronSelection::getPiaSomaDistancePost() {
  return mPiaSomaDistancePost;
}

void NeuronSelection::sampleDown(int maxSize, int seed) {
  RandomGenerator randomGenerator(seed);
  mSelectionA = getDownsampled(mSelectionA, maxSize, randomGenerator);
  mSelectionB = getDownsampled(mSelectionB, maxSize, randomGenerator);
  mSelectionC = getDownsampled(mSelectionC, maxSize, randomGenerator);
}

void NeuronSelection::sampleDownFactor(int samplingFactor, int seed) {
  RandomGenerator randomGenerator(seed);
  mSelectionA =
      getDownsampledFactor(mSelectionA, samplingFactor, randomGenerator);
  mSelectionB =
      getDownsampledFactor(mSelectionB, samplingFactor, randomGenerator);
  mSelectionC =
      getDownsampledFactor(mSelectionC, samplingFactor, randomGenerator);
}

IdList NeuronSelection::filterPreOrBoth(const NetworkProps &networkProps,
                                        IdList ids) {
  IdList pruned;
  for (auto it = ids.begin(); it != ids.end(); it++) {
    if (networkProps.neurons.getSynapticSide(*it) != CIS3D::POSTSYNAPTIC) {
      pruned.append(*it);
    }
  }
  return pruned;
}

void NeuronSelection::setPostTarget(CIS3D::Structure selectionA,
                                    CIS3D::Structure selectionB,
                                    CIS3D::Structure selectionC) {
  mPostTarget.clear();
  mPostTarget.push_back(selectionA);
  mPostTarget.push_back(selectionB);
  mPostTarget.push_back(selectionC);
}

CIS3D::Structure NeuronSelection::getPostTarget(int selectionIndex) const {
  return mPostTarget[selectionIndex];
}

void NeuronSelection::filterUniquePre(const NetworkProps &networkProps) {
  IdList pruned;
  for (int i = 0; i < mSelectionA.size(); i++) {
    int id = mSelectionA[i];
    int mappedId = networkProps.axonRedundancyMap.getNeuronIdToUse(id);
    if (!pruned.contains(mappedId)) {
      pruned.append(mappedId);
    }
  }
  mSelectionA = pruned;
}

IdList NeuronSelection::getDownsampled(IdList &original, int maxSize,
                                       RandomGenerator &randomGenerator) {
  if (original.size() <= maxSize) {
    return original;
  } else {
    randomGenerator.shuffleList(original);
    IdList pruned;
    for (int i = 0; i < maxSize; i++) {
      pruned.append(original[i]);
    }
    return pruned;
  }
}

IdList NeuronSelection::getDownsampledFactor(IdList &original, int factor,
                                             RandomGenerator &randomGenerator) {
  if (factor == -1) {
    return original;
  }
  std::sort(original.begin(), original.end());
  randomGenerator.shuffleList(original);
  IdList pruned;
  for (int i = 0; i < original.size(); i += factor) {
    pruned.append(original[i]);
  }
  return pruned;
}

QString NeuronSelection::getNetworkName() { return mNetworkName; }

void NeuronSelection::setNetworkName(QString name) { mNetworkName = name; }

QString NeuronSelection::getDataRoot() { return mDataRoot; }

void NeuronSelection::setDataRoot(QString dataRoot) { mDataRoot = dataRoot; }

bool NeuronSelection::isSelectionValid(QJsonObject &selection, QString index,
                                       QString &errorMessage) {
  if (!selection["enabled"].toBool()) {
    return true;
  } else {
    int count = 0;
    if (index == "A") {
      count = mSelectionA.size();
    }
    if (index == "B") {
      count = mSelectionB.size();
    }
    if (index == "C") {
      count = mSelectionC.size();
    }
    if (count == 0) {
      errorMessage += selection["label"].toString() +
                      " is empty (try to modify filter settings).";
      return false;
    } else {
      return true;
    }
  }
}

/**
 * @brief Returns a set of explicitly selected voxels (specification parameter
 * VOXEL_WHITELIST).
 * @return The selected voxels. Empty by default.
 */
std::set<int> NeuronSelection::getVoxelWhitelist() { return mVoxelWhitelist; }

bool NeuronSelection::isValid(QJsonObject &query, QString &errorMessage) {
  QJsonObject cellSelection = query["cellSelection"].toObject();
  QJsonObject selectionA = cellSelection["selectionA"].toObject();
  QJsonObject selectionB = cellSelection["selectionB"].toObject();
  QJsonObject selectionC = cellSelection["selectionC"].toObject();
  errorMessage = "";
  if (isSelectionValid(selectionA, "A", errorMessage) &&
      isSelectionValid(selectionB, "B", errorMessage) &&
      isSelectionValid(selectionC, "C", errorMessage)) {
    return true;
  } else {
    return false;
  };
}

bool NeuronSelection::useSliceUniquePre(){
    return mSliceUniquePre;
}

bool NeuronSelection::inSliceBand(double somaX, double min, double max) {
  return somaX >= min && somaX <= max;
}

void NeuronSelection::inSliceRange(double somaX, double sliceRef, double low,
                                   double high, bool &first, bool &second) {
  double sliceWidth = 300;
  first = inSliceBand(somaX, sliceRef + low, sliceRef + high);
  second = inSliceBand(somaX, sliceRef + sliceWidth - high,
                       sliceRef + sliceWidth - low);
}

void NeuronSelection::clear() {
  mSelectionA.clear();
  mSelectionB.clear();
  mSelectionC.clear();
  mBandA.clear();
  mBandB.clear();
  mBandC.clear();
}

void NeuronSelection::copySelection(NeuronSelection &selection) {
  clear();

  mSelectionA = selection.SelectionA();
  mSelectionB = selection.SelectionB();
  mSelectionC = selection.SelectionC();

  for (int i = 0; i < mSelectionA.size(); i++) {
    int id = mSelectionA[i];
    mBandA[id] = selection.getBandA(id);
  }

  for (int i = 0; i < mSelectionB.size(); i++) {
    int id = mSelectionB[i];
    mBandB[id] = selection.getBandB(id);
  }

  for (int i = 0; i < mSelectionC.size(); i++) {
    int id = mSelectionC[i];
    mBandC[id] = selection.getBandC(id);
  }
}

void NeuronSelection::pruneSelection(NeuronSelection &selection) {
  std::map<int, int> mapping = readMapping(selection);

  std::set<int> allowedA = getAllowedIds(selection.SelectionA(), mapping);
  pruneIds(mSelectionA, allowedA);

  std::set<int> allowedB = getAllowedIds(selection.SelectionB(), mapping);
  pruneIds(mSelectionB, allowedB);

  std::set<int> allowedC = getAllowedIds(selection.SelectionC(), mapping);
  pruneIds(mSelectionC, allowedC);
}

std::set<int> NeuronSelection::getAllowedIds(const IdList &selection,
                                             std::map<int, int> &mapping) {
  std::set<int> allowed;
  for (int i = 0; i < selection.size(); i++) {
    int id = selection[i];
    allowed.insert(mapping[id]);
  }
  return allowed;
}

void NeuronSelection::pruneIds(IdList &selection, std::set<int> &allowed) {
  IdList prunedSelection;
  for (int i = 0; i < selection.size(); i++) {
    int id = selection[i];
    if (allowed.find(id) != allowed.end()) {
      prunedSelection.push_back(id);
    }
  }
  selection = prunedSelection;
}

std::map<int, int> NeuronSelection::readMapping(NeuronSelection &selection) {
  QString networkName = selection.getNetworkName();
  QDir mappingDir = QDir(mMappingDir);
  QString path =
      CIS3D::getMappingFilePath(mappingDir, networkName, mNetworkName);

  std::map<int, int> mapping;

  QFile mappingFile(path);
  if (mappingFile.open(QIODevice::ReadOnly)) {
    QTextStream in(&mappingFile);
    while (!in.atEnd()) {
      QString line = in.readLine();
      line = line.trimmed();
      QStringList parts = line.split(" ");

      int id1 = parts[0].toInt();
      int id2 = parts[1].toInt();
      mapping[id1] = id2;
    }
  } else {
    const QString msg =
        QString("Error reading index file. Could not open file %1").arg(path);
    throw std::runtime_error(qPrintable(msg));
  }

  return mapping;
}

std::map<int, int>
NeuronSelection::doGetMultiplicities(const NetworkProps &network,
                                     IdList &selection) {
  std::map<int, int> multiplicities;
  for (int i = 0; i < selection.size(); i++) {
    int preId = selection[i];
    int mappedId = network.axonRedundancyMap.getNeuronIdToUse(preId);
    if (multiplicities.find(mappedId) == multiplicities.end()) {
      multiplicities[mappedId] = 1;
    } else {
      multiplicities[mappedId] += 1;
    }
  }
  return multiplicities;
}

std::map<int, int>
NeuronSelection::getMultiplicities(const NetworkProps &network,
                                   QString selectionIndex) {
  if (selectionIndex == "A") {
    return doGetMultiplicities(network, mSelectionA);
  } else if (selectionIndex == "B") {
    return doGetMultiplicities(network, mSelectionB);
  } else {
    return doGetMultiplicities(network, mSelectionC);
  }
}

void NeuronSelection::correctSynapticSide(CIS3D::SynapticSide &sideA,
                                          CIS3D::SynapticSide &sideB,
                                          CIS3D::SynapticSide &sideC,
                                          bool /*aEnabled*/, bool /*bEnabled*/,
                                          bool cEnabled) {  
  
  QString evaluationQuery = "innervation";
  if (mQueryType == "selection") {    
    if(cEnabled){
      if (sideA == CIS3D::PRESYNAPTIC) {
        evaluationQuery = "inDegree";
      } else {
        evaluationQuery = "triplet";
      }
    }    
  } else {
    evaluationQuery = mQueryType;
  }

  CIS3D::SynapticSide remappedSide = mSliceUniquePre ? CIS3D::POSTSYNAPTIC_MAPPED : CIS3D::POSTSYNAPTIC;


  if(evaluationQuery == "innervation" || evaluationQuery == "spatialInnervation"){
      sideA = remappedSide;
  }

  if(evaluationQuery == "inDegree"){    
      sideA = remappedSide;
      sideB = remappedSide;    
  }

  if(evaluationQuery == "triplet"){
    sideA = remappedSide;
    sideB = remappedSide;
    sideC = remappedSide;
  }

}
