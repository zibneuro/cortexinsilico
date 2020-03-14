#include "Util.h"
#include "CIS3DAxonRedundancyMap.h"
#include "CIS3DStatistics.h"
#include "Histogram.h"
#include "Typedefs.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QTextStream>
#include <cmath>
#include <iostream>
#include <vector>

/**
    Checks whether two neurons overlap based on their bounding box.
    @param n1 Properties of first neurons.
    @param n2 Properties of second neurons.
    @returns True if the neurons overlap.
*/
bool Util::overlap(const NeuronProps &n1, const NeuronProps &n2) {
  const bool intersect = n1.boundingBox.intersects(n2.boundingBox);
  return intersect;
}

/**
    Determines the unique neurons accounting for axon redundancy.
    @param preNeuronList The IDs of presynaptic neurons.
    @param networkProps The model data of the network.
    @returns The IDs of unique presynaptic neurons.
*/
QList<int> Util::getUniquePreNeurons(const QList<int> &preNeuronsList,
                                     const NetworkProps &networkProps) {
  QSet<int> unique;
  const AxonRedundancyMap &axonMap = networkProps.axonRedundancyMap;
  for (int i = 0; i < preNeuronsList.size(); ++i) {
    const int preId = preNeuronsList[i];
    const int mappedId = axonMap.getNeuronIdToUse(preId);
    unique.insert(mappedId);
  }
  return unique.toList();
}

/**
    Creates a mapping (cellType,region) -> (neuron ids).
    @param propsMap The neuron properties.
    @returns A mapping (hash) with (cellType, region) as hash-key.
*/
IdsPerCellTypeRegion Util::sortByCellTypeRegion(const PropsMap &propsMap) {
  IdsPerCellTypeRegion sorted;

  for (PropsMap::ConstIterator it = propsMap.constBegin();
       it != propsMap.constEnd(); ++it) {
    const int neuronId = it.key();
    const int cellTypeId = it.value().cellTypeId;
    const int regionId = it.value().regionId;

    CellTypeRegion ctr(cellTypeId, regionId);
    IdsPerCellTypeRegion::iterator i = sorted.find(ctr);
    if (i == sorted.end()) {
      QList<int> list;
      list.append(neuronId);
      sorted.insert(ctr, list);
    } else {
      i.value().append(neuronId);
    }
  }

  return sorted;
}

/**
    Creates a mapping (cellType,region) -> (neuron ids) for the
    specified subset of neurons.
    @param neuronIds A subset of neuron IDs.
    @param networkProps The model data of the network.
    @returns A mapping (hash) with (cellType, region) as hash-key.
*/
IdsPerCellTypeRegion
Util::sortByCellTypeRegionIDs(const IdList &neuronIds,
                              const NetworkProps &networkProps) {
  IdsPerCellTypeRegion sorted;

  for (IdList::ConstIterator it = neuronIds.constBegin();
       it != neuronIds.constEnd(); ++it) {
    const int neuronId = *it;
    const int cellTypeId = networkProps.neurons.getCellTypeId(neuronId);
    const int regionId = networkProps.neurons.getRegionId(neuronId);

    CellTypeRegion ctr(cellTypeId, regionId);
    IdsPerCellTypeRegion::iterator i = sorted.find(ctr);
    if (i == sorted.end()) {
      IdList list;
      list.append(neuronId);
      sorted.insert(ctr, list);
    } else {
      i.value().append(neuronId);
    }
  }

  return sorted;
}

QJsonObject getFilterItemWithName(const QString &name,
                                  const QJsonArray &filters) {
  for (int i = 0; i < filters.size(); ++i) {
    QJsonObject obj = filters.at(i).toObject();
    const QString nameId = obj["ID"].toString();
    if (nameId == name) {
      return obj;
    }
  }
  return QJsonObject();
}

QList<int> filterListKeeping(const QList<int> &elementsToKeep,
                             const QList<int> &list) {
  QList<int> result;
  for (int i = 0; i < list.size(); ++i) {
    const int item = list[i];
    if (elementsToKeep.contains(item)) {
      result.append(item);
    }
  }
  return result;
}

QList<int> filterListRemoving(const QList<int> &elementsToRemove,
                              const QList<int> &list) {
  QList<int> result;
  for (int i = 0; i < list.size(); ++i) {
    const int item = list[i];
    if (!elementsToRemove.contains(item)) {
      result.append(item);
    }
  }
  return result;
}

/**
    Creates a selection filter for neurons.
    @param jsonArray The filter query as received from  the webframework.
    @param network The model data of the network.
    @returns A selection filter that can be applied to the (CIS3D)Neurons class.
    @throws runtime_error if the selection filter is not valid.
*/
SelectionFilter
Util::getSelectionFilterFromJson(const QJsonArray &jsonArray,
                                 const NetworkProps &network,
                                 const CIS3D::SynapticSide synapticSide) {
  SelectionFilter filter;

  qDebug() << "selection filter";
  qDebug() << jsonArray;

  const QJsonObject ctObj =
      getFilterItemWithName(QString("cellType"), jsonArray);
  const QJsonObject excObj =
      getFilterItemWithName(QString("isExcitatory"), jsonArray);

  double corticalDepthMin;
  double corticalDepthMax;
  getRange(jsonArray, "corticalDepth", -99999, -99999, corticalDepthMin,
           corticalDepthMax);
  if (corticalDepthMin != -99999 && corticalDepthMax != -99999) {
    filter.corticalDepth.push_back(static_cast<float>(corticalDepthMin));
    filter.corticalDepth.push_back(static_cast<float>(corticalDepthMax));
  }

  QList<int> cellTypeIds;
  CIS3D::NeuronType neuronType = CIS3D::EXC_OR_INH;

  filter.synapticSide = synapticSide;

  if (!ctObj.isEmpty()) {
    const QJsonArray value = ctObj["value"].toArray();
    for (int v = 0; v < value.size(); ++v) {
      const QString ctName = value.at(v).toString();
      const int ctId = network.cellTypes.getId(ctName);
      cellTypeIds.append(ctId);
    }
  }
  if (!excObj.isEmpty()) {
    const QString value = excObj["value"].toString();
    if (value == "Yes") {
      neuronType = CIS3D::EXCITATORY;
    } else if (value == "No") {
      neuronType = CIS3D::INHIBITORY;
    } else {
      const QString msg =
          QString("[-] Invalid selection filter value (IsExcitatory): %1")
              .arg(value);
      std::runtime_error(qPrintable(msg));
    }
  }

  if (cellTypeIds.isEmpty() && neuronType == CIS3D::EXC_OR_INH) {
    // Do nothing.
  } else if (cellTypeIds.isEmpty() && neuronType == CIS3D::EXCITATORY) {
    // Add all excitatory types
    const QList<int> allCellTypeIds = network.cellTypes.getAllCellTypeIds();
    for (int c = 0; c < allCellTypeIds.size(); ++c) {
      const int ctid = allCellTypeIds.at(c);
      if (network.cellTypes.isExcitatory(ctid)) {
        filter.cellTypeIds.append(ctid);
      }
    }
  } else if (cellTypeIds.isEmpty() && neuronType == CIS3D::INHIBITORY) {
    // Add all inhibitory types
    const QList<int> allCellTypeIds = network.cellTypes.getAllCellTypeIds();
    for (int c = 0; c < allCellTypeIds.size(); ++c) {
      const int ctid = allCellTypeIds.at(c);
      if (!network.cellTypes.isExcitatory(ctid)) {
        filter.cellTypeIds.append(ctid);
      }
    }
  } else if (!cellTypeIds.isEmpty() && neuronType == CIS3D::EXC_OR_INH) {
    // Add all selected types
    filter.cellTypeIds = cellTypeIds;
  } else if (!cellTypeIds.isEmpty() && neuronType == CIS3D::EXCITATORY) {
    // Select only excitatory types
    for (int c = 0; c < cellTypeIds.size(); ++c) {
      const int ctid = cellTypeIds.at(c);
      if (network.cellTypes.isExcitatory(ctid)) {
        filter.cellTypeIds.append(ctid);
      }
    }
  } else if (cellTypeIds.isEmpty() && neuronType == CIS3D::INHIBITORY) {
    // Select only inhibitory types
    for (int c = 0; c < cellTypeIds.size(); ++c) {
      const int ctid = cellTypeIds.at(c);
      if (!network.cellTypes.isExcitatory(ctid)) {
        filter.cellTypeIds.append(ctid);
      }
    }
  }

  const QJsonObject llObj = getFilterItemWithName("laminarLocation", jsonArray);
  if (!llObj.isEmpty()) {
    const QJsonArray value = llObj["value"].toArray();
    for (int i = 0; i < value.size(); ++i) {
      const QString v = value.at(i).toString();
      if (v == "Infragranular") {
        filter.laminarLocations.append(CIS3D::INFRAGRANULAR);
      } else if (v == "Granular") {
        filter.laminarLocations.append(CIS3D::GRANULAR);
      } else if (v == "Supragranular") {
        filter.laminarLocations.append(CIS3D::SUPRAGRANULAR);
      } else if (v == "I") {
        filter.laminarLocations.append(CIS3D::LAYER1);
      } else if (v == "II") {
        filter.laminarLocations.append(CIS3D::LAYER2);
      } else if (v == "III") {
        filter.laminarLocations.append(CIS3D::LAYER3);
      } else if (v == "IV") {
        filter.laminarLocations.append(CIS3D::LAYER4);
      } else if (v == "V") {
        filter.laminarLocations.append(CIS3D::LAYER5);
      } else if (v == "VI") {
        filter.laminarLocations.append(CIS3D::LAYER6);
      } else {
        const QString msg =
            QString("[-] Invalid selection filter value (Laminar Location): %1")
                .arg(v);
        std::runtime_error(qPrintable(msg));
      }
    }
  }

  const QJsonObject ncObj = getFilterItemWithName("nearestColumn", jsonArray);
  if (!ncObj.isEmpty()) {
    const QJsonArray value = ncObj["value"].toArray();
    for (int v = 0; v < value.size(); ++v) {
      const QString colName = value.at(v).toString();
      const int colId = network.regions.getId(colName);
      filter.nearestColumnIds.append(colId);
    }
  }

  const QJsonObject regObj = getFilterItemWithName("regions", jsonArray);

  if (regObj.isEmpty()) {
    filter.regionIds = network.regions.getAllRegionIds();
  } else {
    const QJsonArray value = ctObj["value"].toArray();
    for (int v = 0; v < value.size(); ++v) {
      const QString regName = value.at(v).toString();
      const int regId = network.regions.getId(regName);
      filter.regionIds.append(regId);
    }
  }

  const QJsonObject icObj = getFilterItemWithName("insideColumn", jsonArray);
  if (!icObj.isEmpty()) {
    QList<int> columnRegionIds;
    columnRegionIds.append(network.regions.getId("A1"));
    columnRegionIds.append(network.regions.getId("A2"));
    columnRegionIds.append(network.regions.getId("A3"));
    columnRegionIds.append(network.regions.getId("A4"));
    columnRegionIds.append(network.regions.getId("B1"));
    columnRegionIds.append(network.regions.getId("B2"));
    columnRegionIds.append(network.regions.getId("B3"));
    columnRegionIds.append(network.regions.getId("B4"));
    columnRegionIds.append(network.regions.getId("C1"));
    columnRegionIds.append(network.regions.getId("C2"));
    columnRegionIds.append(network.regions.getId("C3"));
    columnRegionIds.append(network.regions.getId("C4"));
    columnRegionIds.append(network.regions.getId("D1"));
    columnRegionIds.append(network.regions.getId("D2"));
    columnRegionIds.append(network.regions.getId("D3"));
    columnRegionIds.append(network.regions.getId("D4"));
    columnRegionIds.append(network.regions.getId("E1"));
    columnRegionIds.append(network.regions.getId("E2"));
    columnRegionIds.append(network.regions.getId("E3"));
    columnRegionIds.append(network.regions.getId("E4"));
    columnRegionIds.append(network.regions.getId("Alpha"));
    columnRegionIds.append(network.regions.getId("Beta"));
    columnRegionIds.append(network.regions.getId("Gamma"));
    columnRegionIds.append(network.regions.getId("Delta"));

    const QString value = icObj["value"].toString();
    if (value == "Yes") {
      filter.regionIds = filterListKeeping(columnRegionIds, filter.regionIds);
    } else if (value == "No") {
      filter.regionIds = filterListRemoving(columnRegionIds, filter.regionIds);
    }
  }

  const QJsonObject insideS1Obj = getFilterItemWithName("insideS1", jsonArray);
  if (!insideS1Obj.isEmpty()) {
    const QString value = insideS1Obj["value"].toString();
    QList<int> filteredRegionIds;

    const int S1id = network.regions.getId("S1");
    QList<int> allRegions = network.regions.getAllRegionIds();
    QList<int> insideRegions;
    for (int i = 0; i < allRegions.size(); i++) {
      int regionId = allRegions[i];
      if (network.regions.isInSubtree(regionId, S1id)) {
        insideRegions.append(regionId);
      }
    }

    if (value == "Yes") {
      filteredRegionIds = filterListKeeping(insideRegions, filter.regionIds);
    } else if (value == "No") {
      filteredRegionIds = filterListRemoving(insideRegions, filter.regionIds);
    } else {
      const QString msg = "Invalid S1 predicate";
      std::runtime_error(qPrintable(msg));
    }
    if (filteredRegionIds.size() == 0) {
      filteredRegionIds.append(network.regions.getId("Brain"));
    }
    filter.regionIds = filteredRegionIds;
  }

  return filter;
}

QJsonArray mergeJsonArrays(QList<QJsonArray> arrays, bool emptyMeansAll) {
  QSet<QString> filterValues;
  QJsonArray merged;
  bool atLeastOneEmpty = false;

  for (int i = 0; i < arrays.size(); i++) {
    QJsonArray array = arrays[i];
    if (array.size() == 0) {
      atLeastOneEmpty = true;
    } else {
      for (int j = 0; j < array.size(); j++) {
        filterValues.insert(array[j].toString());
      }
    }
  }

  if (!(emptyMeansAll && atLeastOneEmpty)) {
    QSetIterator<QString> i(filterValues);
    while (i.hasNext()) {
      merged.push_back(i.next());
    }
  }
  return merged;
}

QList<QJsonArray> retrieveStatisticFilters(const QJsonObject &spec,
                                           const QString filterField) {
  QList<QJsonArray> list;
  QJsonArray statisticDefinitions = spec["STATISTIC_DEFINTIONS"].toArray();
  for (int i = 0; i < statisticDefinitions.size(); i++) {
    QJsonObject definition = statisticDefinitions[i].toObject();
    list.push_back(definition[filterField].toArray());
  }
  return list;
}

void addGenerationField(QJsonObject &spec, const QString field,
                        const bool emptyMeansAll) {
  QList<QJsonArray> arrays = retrieveStatisticFilters(spec, field);
  QJsonArray mergedArray = mergeJsonArrays(arrays, emptyMeansAll);
  spec[field] = mergedArray;
}

/**
    Determines and sets the neuron filter for the generation of the connectome
    based on the filters defined in the various statistics (the generation
   filter is the union of all statistic filters).
    @param spec The JSON specification object with the statistic definitions.
        The fields of the generation filter are appended.
*/
void Util::addGenerationFilter(QJsonObject &spec) {
  addGenerationField(spec, "PRE_NEURON_REGIONS", true);
  addGenerationField(spec, "PRE_NEURON_CELLTYPES", true);
  addGenerationField(spec, "PRE_NEURON_IDS", false);
  addGenerationField(spec, "POST_NEURON_REGIONS", true);
  addGenerationField(spec, "POST_NEURON_CELLTYPES", true);
  addGenerationField(spec, "POST_NEURON_IDS", false);
}

/**
    Creates a JSON report of a statistic.

    @param statistics The statistics to report.
    @return The JSON object.
*/
QJsonObject Util::createJsonStatistic(const Statistics &statistics) {
  QJsonObject obj;
  obj.insert("average", statistics.getMean());
  obj.insert("stdev", statistics.getStandardDeviation());
  obj.insert("min", statistics.getMinimum());
  obj.insert("max", statistics.getMaximum());
  return obj;
}

QJsonArray Util::createJsonArray(const std::vector<double> &vector) {
  QJsonArray array;
  for (auto it = vector.begin(); it != vector.end(); ++it) {
    array.push_back(*it);
  }
  return array;
}

/**
    Checks whether two values are almost equal.
    @param a First value.
    @param b Second value.
    @param eps Tolerance.
    @return True, if the values are almost equal.
*/
bool Util::almostEqual(double a, double b, double eps) {
  return std::fabs(a - b) <= eps;
}

bool Util::isZero(double a) { return std::fabs(a) < EPSILON; }

/*
    Handles case that only VPM is selected in combination with nearest column.
    In this case, the nearest column name X is replaced by X_Barreloid and set
   as region filter.
    @param selection The selection filter.
    @param networkProps The model data.
*/
void Util::correctVPMSelectionFilter(SelectionFilter &filter,
                                     const NetworkProps &networkProps) {
  if (filter.cellTypeIds.size() == 1) {
    int cellTypeId = filter.cellTypeIds[0];
    if (networkProps.cellTypes.getName(cellTypeId) == "VPM") {
      filter.regionIds.clear();
      for (int i = 0; i < filter.nearestColumnIds.size(); i++) {
        QString column =
            networkProps.regions.getName(filter.nearestColumnIds[i]);
        column.append("_Barreloid");
        if (networkProps.regions.nameExists(column)) {
          filter.regionIds.push_back(networkProps.regions.getId(column));
        }
      }
      filter.nearestColumnIds.clear();
    }
  }
}

void Util::correctInterneuronSelectionFilter(SelectionFilter &filter,
                                             const NetworkProps &networkProps) {
  if (filter.cellTypeIds.size() == 0) {
    QList<int> exc = networkProps.cellTypes.getAllCellTypeIds(true);
    for (int i = 0; i < exc.size(); i++) {
      filter.cellTypeIds.push_back(exc[i]);
    }
  }
}

std::vector<double> Util::getHeatMap(double value, double min, double max) {
  std::vector<double> result;
  double range = max - min;
  if (range <= 0) {
    result.push_back(0);
    result.push_back(0);
    result.push_back(0);
    return result;
  }
  double relValue = value / max;

  double red = 1 * relValue;
  double blue = 1 - relValue * 1;

  result.push_back(red);
  result.push_back(0);
  result.push_back(blue);
  return result;
}

void Util::getMinMedMax(std::vector<float> in, float &min, float &med,
                        float &max) {
  if (in.size() == 0) {
    min = 0;
    med = 0;
    max = 0;
    return;
  } else if (in.size() == 1) {
    min = in[0];
    med = in[0];
    max = in[0];
    return;
  }

  std::sort(in.begin(), in.end());
  unsigned int iLast = in.size() - 1;
  min = in[0];
  max = in[iLast];

  bool even = in.size() % 2 == 0;
  if (even) {
    int half = in.size() / 2;
    float a = in[half - 1];
    float b = in[half];
    med = (a + b) / 2;
  } else {
    int half = (in.size() - 1) / 2;
    med = in[half];
  }
}

QString Util::writeFormulaDescription(QJsonObject &formula) {
  QString s = "";
  if (formula["synapseDistributionFormulaEnabled"].toBool()) {
    s += "Synapse probability distribution formula:,";
    s = s + "\"" + formula["synapseDistributionFormula"].toString() + "\"";
  }
  if (formula["connectionProbabilityFormulaEnabled"].toBool()) {
    s += "Neuron-to-neuron connection probability formula:,";
    if (formula["connectionProbabilityMode"].toString() == "derive") {
      s = s + "\"" + "p(DSO)=1-P_DSO(0)" + "\"";
    } else {
      s = s + "\"" + formula["connectionProbabilityFormula"].toString() + "\"";
    }
  }

  s += "\n";
  return s;
}

QString Util::writeFormulaSelectionDescription(QJsonObject &formulaSelection,
                                               int number) {
  QJsonObject formulas;
  if (number == 1) {
    formulas = formulaSelection["formulasNetwork1"].toObject();
  } else {
    formulas = formulaSelection["formulasNetwork2"].toObject();
  }
  return writeFormulaDescription(formulas);
}

QString Util::getResultFileHeader(const QJsonObject &query) {
  int networkNumber = query["networkNumber"].toInt();
  QString queryType = query["queryType"].toString();
  bool isTriplet = queryType == "triplet";
  bool isInDegree = queryType == "inDegree";

  QJsonObject networkSelection = query["networkSelection"].toObject();

  int oppositeNumber = getOppositeNetworkNumber(networkNumber);

  // ######### WRITE NETWORK SELECTION #########

  QString s = "";
  s += writeNetworkDescription(networkSelection, networkNumber);

  if (matchCells(networkSelection, networkNumber)) {
    s += "   match cells to " +
         writeNetworkDescription(networkSelection, oppositeNumber, false);
  }

  // ######### CELL SELECTION #########

  QJsonObject cellSelection = query["cellSelection"].toObject();
  double sliceRef;
  bool showSlice = isSlice(networkSelection, networkNumber, sliceRef) ||
                   matchCells(networkSelection, networkNumber);
  s += writeCellSelectionDescription(cellSelection, showSlice);

  if (isTriplet || isInDegree) {
    QJsonObject sampleSelection = query["sampleSelection"].toObject();
    s += writeSampleSelectionDescription(sampleSelection, networkNumber);
  }

  // ######### WRITE FORMULA DESCRIPTION #########
  QJsonObject formulaSelection = query["formulaSelection"].toObject();
  s += writeFormulaSelectionDescription(formulaSelection, networkNumber);

  return s;
}

QString Util::writeFilterConditions(QJsonArray conditions) {
  QString s = "";
  for (int i = 0; i < conditions.size(); i++) {
    if (i != 0) {
      s += " AND ";
    }
    s += conditions[i].toObject()["valueAsText"].toString();
  }
  return "\"" + s + "\"";
}

QString Util::writeSelectionDescription(QJsonObject &selection, bool isSlice) {
  QString s = "";
  if (!selection["enabled"].toBool()) {
    return s;
  }
  QString label = selection["label"].toString();
  s += label + ":\n";
  s += "   Filter:," +
       writeFilterConditions(selection["conditions"].toArray()) + "\n";
  if (isSlice) {
    QJsonObject tissueDepth = selection["tissueDepth"].toObject();
    s += writeTissueDepthDescription(tissueDepth);
  }
  return s;
}

QString Util::writeCellSelectionDescription(QJsonObject &cellSelection,
                                            bool isSlice) {
  QString s = "";
  QJsonObject selectionA = cellSelection["selectionA"].toObject();
  s += writeSelectionDescription(selectionA, isSlice);
  QJsonObject selectionB = cellSelection["selectionB"].toObject();
  s += writeSelectionDescription(selectionB, isSlice);
  QJsonObject selectionC = cellSelection["selectionC"].toObject();
  s += writeSelectionDescription(selectionC, isSlice);
  return s;
}

QString Util::writeNetworkDescription(QJsonObject &networkSelection, int number,
                                      bool capital) {
  QString s = "";

  if (capital) {
    s += "Neural network:,";
  } else {
    s += "neural network:,";
  }
  s += "\"" + getLongName(networkSelection, number) + "\"\n";

  int samplingFactor = -1;
  int randomSeed = -1;
  if (isSampled(networkSelection, number, samplingFactor, randomSeed)) {
    s += "Sampling factor:,";
    s += QString::number(samplingFactor) + "\n";
    s += "Random seed:,";
    s += QString::number(randomSeed) + "\n";
  }

  return s;
}

QString Util::writeTissueDepthDescription(QJsonObject &tissueDepth) {
  QString s = "";

  const double tissueLow = tissueDepth["low"].toDouble();
  const double tissueHigh = tissueDepth["high"].toDouble();
  QString tissueMode = tissueDepth["mode"].toString();

  QString mode = tissueMode == "oneSided" ? "one-sided" : "two-sided";
  s += "   Tissue depth mode:," + mode + "\n";
  s += "   Tissue depth low:," + QString::number(tissueLow) + "\n";
  s += "   Tissue depth high:," + QString::number(tissueHigh) + "\n";

  return s;
}

CIS3D::Structure Util::getPostsynapticTarget(QJsonArray &conditions) {
  bool exists;
  QJsonObject condition =
      getCondition(conditions, "postsynapticTarget", exists);
  if (exists) {
    QJsonArray values = condition["value"].toArray();
    if (values.size() != 1) {
      return CIS3D::DEND;
    } else {
      if (values[0].toString() == "Basal") {
        return CIS3D::BASAL;
      } else {
        return CIS3D::APICAL;
      }
    }
  } else {
    return CIS3D::DEND;
  }
}

QString Util::getPostFolderName(CIS3D::Structure target) {
  if (target == CIS3D::APICAL) {
    return "features_postApicalExc";
  } else if (target == CIS3D::BASAL) {
    return "features_postBasalExc";
  } else {
    return "features_postExc";
  }
}

QString Util::getIndexFileName(CIS3D::Structure target) {
  if (target == CIS3D::APICAL) {
    return "voxel_indexApical";
  } else if (target == CIS3D::BASAL) {
    return "voxel_indexBasal";
  } else {
    return "voxel_indexAll";
  }
}

QString Util::getBranchIndexFileName(CIS3D::Structure /*target*/) {
  return "voxel_indexBranchAll";
  /*
  if (target == CIS3D::APICAL)
  {
      return "voxel_indexApical";
  }
  else if (target == CIS3D::BASAL)
  {
      return "voxel_indexBasal";
  }
  else
  {
      return "voxel_indexBranchAll";
  }
  */
}

QString Util::getInnervationFolderName(CIS3D::Structure target) {
  if (target == CIS3D::APICAL) {
    return "innervationApical";
  } else if (target == CIS3D::BASAL) {
    return "innervationBasal";
  } else {
    return "innervation";
  }
}

QString Util::getNetwork(QJsonObject &spec, int &samplingFactor) {
  QString name = spec["network"].toString();
  if (name == "RBCk") {
    name = "RBC";
    samplingFactor = spec["samplingFactor"].toInt();
  } else {
    samplingFactor = -1;
  }
  return name;
}

int Util::getSliceRef(QString network) {
  if (network == "")
    return -9999;
  else if (!network.contains("Truncated")) {
    return -9999;
  } else {
    int refX = network.mid(12, 3).toInt();
    return -refX;
  }
}

bool Util::isSlice(QJsonObject &networkSpec, int number, double &sliceRef) {
  QString network1 = networkSpec["network1"].toString();
  QString network2 = networkSpec["network2"].toString();
  sliceRef = -9999;
  if (number == 1) {
    sliceRef = (double)getSliceRef(network1);
  } else {
    sliceRef = (double)getSliceRef(network2);
  }
  return sliceRef != -9999;
}

bool Util::isFull(QJsonObject &networkSpec, int number) {
  QString network = "";
  if (number == 1) {
    network = networkSpec["network1"].toString();
  } else {
    network = networkSpec["network2"].toString();
  }

  return !network.contains("Truncated") && !network.isEmpty();
}

bool Util::matchCells(QJsonObject &networkSpec, int number) {
  double sliceRef;
  bool full = isFull(networkSpec, number);
  bool oppositeSlice =
      isSlice(networkSpec, getOppositeNetworkNumber(number), sliceRef);
  bool option = networkSpec["matchCells"].toBool();
  return full && oppositeSlice && option;
}

bool Util::matchCellOptionSet(QJsonObject &networkSpec) {
  bool option = networkSpec["matchCells"].toBool();
  return option;
}

int Util::getOppositeNetworkNumber(int number) {
  if (number == 1) {
    return 2;
  } else {
    return 1;
  }
}

bool Util::isSampled(QJsonObject &networkSpec, int number, int &samplingFactor,
                     int &randomSeed) {
  QString network = "";

  if (number == 1) {
    network = networkSpec["network1"].toString();
    samplingFactor = networkSpec["samplingFactor1"].toInt();
    randomSeed = networkSpec["randomSeed1"].toInt();
  } else {
    network = networkSpec["network2"].toString();
    samplingFactor = networkSpec["samplingFactor2"].toInt();
    randomSeed = networkSpec["randomSeed2"].toInt();
  }

  return network == "RBCk";
}

QString Util::getLongName(QJsonObject &networkSpec, int number) {
  if (number == 1) {
    return networkSpec["network1Long"].toString();
  } else {
    return networkSpec["network2Long"].toString();
  }
}

QString Util::getShortName(QJsonObject &networkSpec, int number) {
  QString shortName;
  if (number == 1) {
    shortName = networkSpec["network1"].toString();
  } else {
    shortName = networkSpec["network2"].toString();
  }
  if (shortName == "RBCk") {
    return "RBC";
  } else {
    return shortName;
  }
}

void Util::getSampleSettings(QJsonObject &sampleSettings, int network,
                             int &sampleSize, int &randomSeed, bool &enabled) {
  if (network == 1) {
    sampleSize = sampleSettings["samplesNetwork1"].toInt();
    randomSeed = sampleSettings["randomSeedNetwork1"].toInt();
    enabled = sampleSettings["enabledNetwork1"].toBool();
  } else {
    sampleSize = sampleSettings["samplesNetwork2"].toInt();
    randomSeed = sampleSettings["randomSeedNetwork2"].toInt();
    enabled = sampleSettings["enabledNetwork2"].toBool();
  }
}

CIS3D::SynapticSide Util::getSynapticSide(QJsonObject &selectionFilter) {
  QString side = selectionFilter["synapticSide"].toString();
  if (side == "presynaptic") {
    return CIS3D::PRESYNAPTIC;
  } else if (side == "postsynaptic") {
    return CIS3D::POSTSYNAPTIC;
  } else if (side == "both") {
    return CIS3D::BOTH_SIDES;
  } else {
    throw std::runtime_error("Invalid side");
  }
}

QString Util::getDatasetPath(const QString &datasetShortName,
                             const QJsonObject &config) {
  const QJsonValue datasetsJson = config["WORKER_DATASETS_CIS3D"];
  if (!datasetsJson.isArray()) {
    throw std::runtime_error(
        "QueryHelpers::getDatasetPath: WORKER_DATASETS_CIS3D is not an array");
  }

  const QJsonArray datasetsArray = datasetsJson.toArray();

  for (int i = 0; i < datasetsArray.size(); ++i) {
    const QJsonValue &datasetJson = datasetsArray.at(i);

    if (!datasetJson.isObject()) {
      throw std::runtime_error(
          "QueryHelpers::getDatasetPath: dataset entry is not an object");
    }

    const QJsonObject dataset = datasetJson.toObject();

    if (!dataset.contains(QString("shortName"))) {
      throw std::runtime_error("QueryHelpers::getDatasetPath: dataset entry "
                               "has no field 'shortName'");
    }

    const QJsonValue shortNameJson = dataset.value(QString("shortName"));
    if (!shortNameJson.isString()) {
      throw std::runtime_error("QueryHelpers::getDatasetPath: dataset entry "
                               "'shortName' is not a string");
    }

    const QString shortName = shortNameJson.toString();

    if (shortName == datasetShortName) {
      if (!dataset.contains(QString("path"))) {
        throw std::runtime_error(
            "QueryHelpers::getDatasetPath: dataset entry has no field 'path'");
      }

      const QJsonValue pathJson = dataset.value(QString("path"));
      if (!pathJson.isString()) {
        throw std::runtime_error("QueryHelpers::getDatasetPath: dataset entry "
                                 "'path' is not a string");
      }

      return pathJson.toString();
    }
  }

  throw std::runtime_error(
      "QueryHelpers::getDatasetPath: no path found for dataset shortName");
}

QString Util::writeVoxelSelectionDescription(QJsonObject &voxelSelection) {
  QJsonObject filter = voxelSelection["voxelFilter"].toObject();
  QString mode = filter["mode"].toString();
  QString s = "";

  
  s += "Filter mode:,";
  if (mode == "voxel") {
    s += "All neurons in sub-volume (A)\n";
  } else if (mode == "prePostVoxel") {
    s += "Neurons meeting filter conditions in sub-volume (A & B & C)\n";
  } else {
    s += "Neurons meeting filter conditions in complete volume (B & C)\n";
  }
  
  s += "Sub-volume selection (A):\n";
  s += "  Sub-volume origin (x y z) in \u00B5m:,";
  s += QString("%1 %2 %3\n")
           .arg(filter["originX"].toDouble())
           .arg(filter["originY"].toDouble())
           .arg(filter["originZ"].toDouble());
  s += "  Sub-volume dimensions (nx ny nz) x 50\u00B5m:,";
  s += QString("%1 %2 %3\n")
           .arg(filter["dimX"].toDouble())
           .arg(filter["dimY"].toDouble())
           .arg(filter["dimZ"].toDouble());  
  return s;
}

QString Util::writeSampleSelectionDescription(QJsonObject &sampleSelection,
                                              int network) {
  QString description = sampleSelection["description"].toString();
  QString s = "";

  int nSamples = network == 1 ? sampleSelection["samplesNetwork1"].toInt()
                              : sampleSelection["samplesNetwork2"].toInt();
  int seed = network == 1 ? sampleSelection["randomSeedNetwork1"].toInt()
                          : sampleSelection["randomSeedNetwork2"].toInt();
  s = s + description + ":," + QString::number(nSamples) + "\n";
  s = s + "   Random seed for sample:" + "," + QString::number(seed) + "\n";

  return s;
}

QString Util::formatVolume(int nVoxels) {
  if (std::isnan(nVoxels)) {
    return "---";
  } else {
    double volume = 0.05 * 0.05 * 0.05 * nVoxels;
    return QString::number(volume, 'f', 6);
  }
}

bool Util::isSlice(QString networkName) {
  return networkName.contains("Truncated");
}

QJsonObject Util::getCondition(const QJsonArray &conditions, QString id,
                               bool &exists) {
  exists = false;
  for (int i = 0; i < conditions.size(); i++) {
    QJsonObject condition = conditions[i].toObject();
    if (condition["ID"].toString() == id) {
      exists = true;
      return condition;
    }
  }
  QJsonObject foo;
  return foo;
}

void Util::getRange(const QJsonArray &conditions, QString id, double defaultMin,
                    double defaultMax, double &min, double &max) {
  bool exists;
  min = defaultMin;
  max = defaultMax;
  QJsonObject condition = getCondition(conditions, id, exists);
  if (exists) {
    QJsonArray value = condition["value"].toArray();
    min = value[0].toString().toDouble();
    max = value[1].toString().toDouble();
  }
}

std::set<int> Util::getPermittedSubvolumeRegionIds(QJsonArray &conditions,
                                                   Regions &regions) {
  std::set<int> regionIds;
  bool exists;
  QJsonObject condition = getCondition(conditions, "nearestColumn", exists);
  if (!exists) {
    return regionIds;
  } else {
    QJsonArray value = condition["value"].toArray();
    for (int i = 0; i < value.size(); i++) {
      QString columnName = value[i].toString();
      QString septumName = "S1_Septum_" + columnName;
      regionIds.insert(regions.getId(columnName));
      regionIds.insert(regions.getId(septumName));
    }
  }
  return regionIds;
}

double Util::convertToCubicMicron(double value) { return value * 0.000008; }

std::set<int> Util::listToSet(IdList list){
  std::set<int> ids;
  for(int i=0; i<list.size(); i++){
    ids.insert(list[i]);
  }
  return ids;
}