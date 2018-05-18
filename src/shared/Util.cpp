#include "Typedefs.h"
#include "Util.h"
#include "CIS3DAxonRedundancyMap.h"
#include <QJsonObject>
#include <QJsonArray>

/**
    Checks whether two neurons overlap based on their bounding box.
    @param n1 Properties of first neurons.
    @param n2 Properties of second neurons.
    @returns True if the neurons overlap.
*/
bool Util::overlap(const NeuronProps& n1, const NeuronProps& n2) {
    const bool intersect = n1.boundingBox.intersects(n2.boundingBox);
    return intersect;
}

/**
    Determines the unique neurons accounting for axon redundancy.
    @param preNeuronList The IDs of presynaptic neurons.
    @param networkProps The model data of the network.
    @returns The IDs of unique presynaptic neurons.
*/
QList<int> Util::getUniquePreNeurons(const QList<int>& preNeuronsList,
                               const NetworkProps& networkProps)
{
    const AxonRedundancyMap& axonMap = networkProps.axonRedundancyMap;
    QList<int> unique;

    for (int i=0; i<preNeuronsList.size(); ++i) {
        const int preId = preNeuronsList[i];
        const int mappedId = axonMap.getNeuronIdToUse(preId);
        if (preId == mappedId) {
            unique.append(preId);
        }
    }

    return unique;
}

/**
    Creates a mapping (cellType,region) -> (neuron ids).
    @param propsMap The neuron properties.
    @returns A mapping (hash) with (cellType, region) as hash-key.
*/
IdsPerCellTypeRegion Util::sortByCellTypeRegion(const PropsMap& propsMap) {

    IdsPerCellTypeRegion  sorted;

    for (PropsMap::ConstIterator it=propsMap.constBegin(); it!=propsMap.constEnd(); ++it) {
        const int neuronId = it.key();
        const int cellTypeId = it.value().cellTypeId;
        const int regionId = it.value().regionId;

        CellTypeRegion ctr(cellTypeId, regionId);
        IdsPerCellTypeRegion::iterator i = sorted.find(ctr);
        if (i == sorted.end()) {
            QList<int> list;
            list.append(neuronId);
            sorted.insert(ctr, list);
        }
        else {
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
IdsPerCellTypeRegion Util::sortByCellTypeRegionIDs(const IdList& neuronIds, const NetworkProps& networkProps){

    IdsPerCellTypeRegion  sorted;

    for (IdList::ConstIterator it=neuronIds.constBegin(); it!=neuronIds.constEnd(); ++it) {
        const int neuronId = *it;
        const int cellTypeId = networkProps.neurons.getCellTypeId(neuronId);
        const int regionId = networkProps.neurons.getRegionId(neuronId);

        CellTypeRegion ctr(cellTypeId, regionId);
        IdsPerCellTypeRegion::iterator i = sorted.find(ctr);
        if (i == sorted.end()) {
            IdList list;
            list.append(neuronId);
            sorted.insert(ctr, list);
        }
        else {
            i.value().append(neuronId);
        }
    }

    return sorted;
}


QJsonObject getFilterItemWithName(const QString& name, const QJsonArray& filters) {
    for (int i=0; i<filters.size(); ++i) {
        QJsonObject obj = filters.at(i).toObject();
        const QString nameId = obj["ID"].toString();
        if (nameId == name) {
            return obj;
        }
    }
    return QJsonObject();
}


QList<int> filterListKeeping(const QList<int>& elementsToKeep, const QList<int>& list) {
    QList<int> result;
    for (int i=0; i<list.size(); ++i) {
        const int item = list[i];
        if (elementsToKeep.contains(item)) {
            result.append(item);
        }
    }
    return result;
}


QList<int> filterListRemoving(const QList<int>& elementsToRemove, const QList<int>& list) {
    QList<int> result;
    for (int i=0; i<list.size(); ++i) {
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
SelectionFilter Util::getSelectionFilterFromJson(const QJsonArray& jsonArray, const NetworkProps& network) {
    SelectionFilter filter;

    const QJsonObject ctObj = getFilterItemWithName(QString("cellType"), jsonArray);
    const QJsonObject excObj = getFilterItemWithName(QString("isExcitatory"), jsonArray);

    QList<int> cellTypeIds;
    CIS3D::NeuronType neuronType = CIS3D::EXC_OR_INH;

    if (!ctObj.isEmpty()) {
        const QJsonArray value = ctObj["value"].toArray();
        for (int v=0; v<value.size(); ++v) {
            const QString ctName = value.at(v).toString();
            const int ctId = network.cellTypes.getId(ctName);
            cellTypeIds.append(ctId);
        }
    }
    if (!excObj.isEmpty()) {
        const QString value = excObj["value"].toString();
        if (value == "Yes") {
            neuronType = CIS3D::EXCITATORY;
        }
        else if (value == "No") {
            neuronType = CIS3D::INHIBITORY;
        }
        else {
            const QString msg = QString("[-] Invalid selection filter value (IsExcitatory): %1").arg(value);
            std::runtime_error(qPrintable(msg));
        }
    }

    if (cellTypeIds.isEmpty() && neuronType == CIS3D::EXC_OR_INH) {
        // Do nothing.
    }
    else if (cellTypeIds.isEmpty() && neuronType == CIS3D::EXCITATORY) {
        // Add all excitatory types
        const QList<int> allCellTypeIds = network.cellTypes.getAllCellTypeIds();
        for (int c=0; c<allCellTypeIds.size(); ++c) {
            const int ctid = allCellTypeIds.at(c);
            if (network.cellTypes.isExcitatory(ctid)) {
                filter.cellTypeIds.append(ctid);
            }
        }
    }
    else if (cellTypeIds.isEmpty() && neuronType == CIS3D::INHIBITORY) {
        // Add all inhibitory types
        const QList<int> allCellTypeIds = network.cellTypes.getAllCellTypeIds();
        for (int c=0; c<allCellTypeIds.size(); ++c) {
            const int ctid = allCellTypeIds.at(c);
            if (!network.cellTypes.isExcitatory(ctid)) {
                filter.cellTypeIds.append(ctid);
            }
        }
    }
    else if (!cellTypeIds.isEmpty() && neuronType == CIS3D::EXC_OR_INH) {
        // Add all selected types
        filter.cellTypeIds = cellTypeIds;
    }
    else if (!cellTypeIds.isEmpty() && neuronType == CIS3D::EXCITATORY) {
        // Select only excitatory types
        for (int c=0; c<cellTypeIds.size(); ++c) {
            const int ctid = cellTypeIds.at(c);
            if (network.cellTypes.isExcitatory(ctid)) {
                filter.cellTypeIds.append(ctid);
            }
        }
    }
    else if (cellTypeIds.isEmpty() && neuronType == CIS3D::INHIBITORY) {
        // Select only inhibitory types
        for (int c=0; c<cellTypeIds.size(); ++c) {
            const int ctid = cellTypeIds.at(c);
            if (!network.cellTypes.isExcitatory(ctid)) {
                filter.cellTypeIds.append(ctid);
            }
        }
    }

    const QJsonObject llObj = getFilterItemWithName("laminarLocation", jsonArray);
    if (!llObj.isEmpty()) {
        const QJsonArray value = llObj["value"].toArray();
        for (int i=0; i<value.size(); ++i) {
            const QString v = value.at(i).toString();
            if (v == "Infragranular") {
                filter.laminarLocations.append(CIS3D::INFRAGRANULAR);
            }
            else if (v == "Granular") {
                filter.laminarLocations.append(CIS3D::GRANULAR);
            }
            else if (v == "Supragranular") {
                filter.laminarLocations.append(CIS3D::SUPRAGRANULAR);
            }
            else {
                const QString msg = QString("[-] Invalid selection filter value (Laminar Location): %1").arg(v);
                std::runtime_error(qPrintable(msg));
            }
        }
    }

    const QJsonObject ncObj = getFilterItemWithName("nearestColumn", jsonArray);
    if (!ncObj.isEmpty()) {
        const QJsonArray value = ncObj["value"].toArray();
        for (int v=0; v<value.size(); ++v) {
            const QString colName = value.at(v).toString();
            const int colId = network.regions.getId(colName);
            filter.nearestColumnIds.append(colId);
        }
    }

    const QJsonObject regObj = getFilterItemWithName("regions", jsonArray);

    if (regObj.isEmpty()) {
        filter.regionIds = network.regions.getAllRegionIds();
    }
    else {
        const QJsonArray value = ctObj["value"].toArray();
        for (int v=0; v<value.size(); ++v) {
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
        }
        else if (value == "No") {
            filter.regionIds = filterListRemoving(columnRegionIds, filter.regionIds);
        }
    }

    const QJsonObject insideS1Obj = getFilterItemWithName("insideS1", jsonArray);
    if (!insideS1Obj.isEmpty()) {
        const QString value = insideS1Obj["value"].toString();
        QList<int> filteredRegionIds;
        const int S1id = network.regions.getId("S1");
        if (value == "Yes") {
            for (int r=0; r<filter.regionIds.size(); ++r) {
                const int regionId = filter.regionIds.at(r);
                if (network.regions.isInSubtree(regionId, S1id)) {
                    filteredRegionIds.append(regionId);
                }
            }
        }
        else if (value == "No") {
            for (int r=0; r<filter.regionIds.size(); ++r) {
                const int regionId = filter.regionIds.at(r);
                if (!network.regions.isInSubtree(regionId, S1id)) {
                    filteredRegionIds.append(regionId);
                }
            }
        }
        else {
            const QString msg = QString("[-] Invalid selection filter value (Inside S1): %1").arg(value);
            std::runtime_error(qPrintable(msg));
        }
        filter.regionIds = filteredRegionIds;
    }

    return filter;
}
