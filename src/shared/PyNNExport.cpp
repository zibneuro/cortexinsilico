#include "PyNNExport.h"
#include "RandomGenerator.h"
#include <algorithm>
#include <QDebug>


PyNNExport::PyNNExport(const NetworkProps& networkProps, FormulaCalculator& calculator) : 
    mNetworkProps(networkProps), mFormulaCalculator(calculator) {};

void PyNNExport::execute(FileHelper& fileHelper, std::map<int, ExportData>& dataPerUniquePre) {
    ExportDataExpanded data = expandData(dataPerUniquePre);
    std::map<int, int> mapping = renumberIds(data);
    std::map<int, int> reverseMapping = getReverseMapping(mapping);
    //std::vector<Connection> connections = getConnections(mapping, data);
    std::vector<Connection> connections = getConnectionsInnervation(mapping, data);
    std::vector<Connection> connectionsFiltered = filterWeight(connections);
    //std::vector<Connection> connectionsBinary = filterBinary(connections);

    Meta metaData;
    metaData.numberPre = static_cast<int>(data.preIds.size());
    metaData.numberPost = static_cast<int>(data.postIds.size());
    metaData.numberCombined = static_cast<int>(mapping.size());
    
    writeReverseMapping(fileHelper, reverseMapping); 
    writeConnections(fileHelper, connectionsFiltered);
    //writeConnectionsBinary(fileHelper, connectionsBinary);
    writeMeta(fileHelper, metaData);
}

PyNNExport::ExportDataExpanded PyNNExport::expandData(std::map<int, ExportData>& dataPerUniquePre) {
    ExportDataExpanded expanded;
    for(auto it = dataPerUniquePre.begin(); it != dataPerUniquePre.end(); it++){
        std::vector<int> preIdsCurrent = it->second.preIds;
        std::vector<int> postIdsCurrent = it->second.postIds;
        std::vector<float> probabilitiesCurrent = it->second.probabilities;
        std::vector<float> innervationsCurrent = it->second.innervations;

        for(auto itPre = preIdsCurrent.begin(); itPre != preIdsCurrent.end(); itPre++){
            for(unsigned int i = 0; i < postIdsCurrent.size(); i++){
                int postId = postIdsCurrent[i];
                float probability = probabilitiesCurrent[i];
                float innervation = innervationsCurrent[i];
                expanded.preIds.push_back(*itPre);
                expanded.postIds.push_back(postId);
                expanded.probabilities.push_back(probability);
                expanded.innervations.push_back(innervation);
            }
        }
    }    
    return expanded;
}

std::map<int, int> PyNNExport::renumberIds(ExportDataExpanded& data) {
    std::map<int, int> mapping;
    std::set<int> ids;
    for(auto it = data.preIds.begin(); it != data.preIds.end(); it++){
        ids.insert(*it);
    }
    for(auto it = data.postIds.begin(); it != data.postIds.end(); it++){
        ids.insert(*it);
    }
    int k = 0;
    for(auto it = ids.begin(); it != ids.end(); it++){
        mapping[*it] = k;
        k++;   
    }    
    return mapping;
}

std::map<int, int> PyNNExport::getReverseMapping(std::map<int, int>& mapping) {
    std::map<int, int> reverseMapping;
    for(auto it = mapping.begin(); it != mapping.end(); it++){
        reverseMapping[it->second] = it->first;
    }
    return reverseMapping;
}

void PyNNExport::writeReverseMapping(FileHelper& fileHelper, std::map<int, int>& index_neuronId) {
    fileHelper.openFile("neuron_ids.txt");
    fileHelper.write("index neuron_id\n");
    for(auto it = index_neuronId.begin(); it != index_neuronId.end(); it++){
        QString line = QString::number(it->first) + " " + QString::number(it->second) + "\n";
        fileHelper.write(line);
    }
    fileHelper.closeFile();
}

std::vector<PyNNExport::Connection> PyNNExport::getConnections(std::map<int, int>& mapping, ExportDataExpanded& data){
    std::vector<Connection> connections;
    for(unsigned int i=0; i<data.preIds.size(); i++){
        Connection c;
        c.preIndex = mapping[data.preIds[i]];
        c.postIndex = mapping[data.postIds[i]];        
        c.weight = data.probabilities[i];        
        connections.push_back(c);
    }
    std::sort(connections.begin(), connections.end());
    return connections;
}

std::vector<PyNNExport::Connection> PyNNExport::getConnectionsInnervation(std::map<int, int>& mapping, ExportDataExpanded& data){
    std::vector<Connection> connections;
    for(unsigned int i=0; i<data.preIds.size(); i++){
        Connection c;
        c.preIndex = mapping[data.preIds[i]];
        c.postIndex = mapping[data.postIds[i]];        
        c.weight = 0.002 * data.innervations[i];        
        connections.push_back(c);
    }
    std::sort(connections.begin(), connections.end());
    return connections;
}

std::vector<PyNNExport::Connection> PyNNExport::filterWeight(std::vector<Connection>& connections, float minWeight){
    std::vector<Connection> filtered;
    for(auto it = connections.begin(); it!= connections.end(); it++){
        if(it->weight >= minWeight){
            Connection c;
            c.preIndex = it->preIndex;
            c.postIndex = it->postIndex;
            c.weight = it->weight;
            filtered.push_back(c);
        }
    }
    return filtered; 
}

std::vector<PyNNExport::Connection> PyNNExport::filterBinary(std::vector<Connection>& connections){
    std::vector<Connection> filtered;
    for(auto it = connections.begin(); it!= connections.end(); it++){
        float prob = (float)SingletonRandom::getInstance()->getNumberBernoulliDistribution(it->weight);
        if(prob > 0){
            Connection c;
            c.preIndex = it->preIndex;
            c.postIndex = it->postIndex;
            c.weight = 1;
            filtered.push_back(c);
        }
    }
    return filtered; 
}

void PyNNExport::writeConnections(FileHelper& fileHelper, std::vector<Connection>& connections){
    fileHelper.openFile("connections.txt");
    fileHelper.write("# columns = [\"i\", \"j\", \"weight\"]\n");
    for(auto it = connections.begin(); it != connections.end(); it++){        
        QString line = QString::number(it->preIndex) + " " + QString::number(it->postIndex) + " " + QString::number(it->weight,'g',3) + "\n";                
        fileHelper.write(line);
    }
    fileHelper.closeFile();  
}

void PyNNExport::writeConnectionsBinary(FileHelper& fileHelper, std::vector<Connection>& connections){
    fileHelper.openFile("connections_binary_realization.txt");
    fileHelper.write("# columns = [\"i\", \"j\"]\n");
    for(auto it = connections.begin(); it != connections.end(); it++){        
        QString line = QString::number(it->preIndex) + " " + QString::number(it->postIndex) + "\n";                
        fileHelper.write(line);
    }
    fileHelper.closeFile();  
}

void PyNNExport::writeMeta(FileHelper& fileHelper, Meta& metaData){
    fileHelper.openFile("meta.json");
    fileHelper.write("{\n");
    fileHelper.writeJsonKeyValueNumeric("number_pre", metaData.numberPre);
    fileHelper.writeJsonKeyValueNumeric("number_post", metaData.numberPost);
    fileHelper.writeJsonKeyValueNumeric("number_combined", metaData.numberCombined, false);
    fileHelper.write("}\n");
    fileHelper.closeFile();
}