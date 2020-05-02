#ifndef PYNNEXPORT_H
#define PYNNEXPORT_H

#include "FileHelper.h"
#include "FormulaCalculator.h"
#include "CIS3DNetworkProps.h"
#include <set>
#include <map>
#include <vector>

struct ExportData {
        std::vector<int> preIds;
        std::vector<int> postIds;
        std::vector<float> innervations;
        std::vector<float> probabilities;
    };

class PyNNExport {

public:    

    PyNNExport(const NetworkProps& networkProps, FormulaCalculator& calculator);

    void execute(FileHelper& fileHelper, std::map<int, ExportData>& dataPerUniquePre);

private:

    struct ExportDataExpanded {
        std::vector<int> preIds;
        std::vector<int> postIds;
        std::vector<float> innervations;
        std::vector<float> probabilities;
    };

    struct Connection {
        int preIndex;
        int postIndex;
        float weight;

        bool operator<(const Connection& c){
            if(preIndex != c.preIndex){
                return preIndex < c.preIndex;
            } 
            return postIndex < c.postIndex;
        }
    };  

    struct Meta {
        int numberPre; 
        int numberPost;
        int numberCombined;
    };

    ExportDataExpanded expandData(std::map<int, ExportData>& dataPerUniquePre);
    std::map<int, int> renumberIds(ExportDataExpanded& data);
    std::map<int, int> getReverseMapping(std::map<int, int>& mapping);    
    std::vector<Connection> getConnections(std::map<int, int>& mapping, ExportDataExpanded& data);
    std::vector<Connection> getConnectionsInnervation(std::map<int, int>& mapping, ExportDataExpanded& data);
    std::vector<Connection> filterWeight(std::vector<Connection>& connections, float minWeight = 0.001);
    std::vector<Connection> filterBinary(std::vector<Connection>& connections);
    void writeReverseMapping(FileHelper& fileHelper, std::map<int, int>& index_neuronId);
    void writeConnections(FileHelper& fileHelper, std::vector<Connection>& connections);
    void writeConnectionsBinary(FileHelper& fileHelper, std::vector<Connection>& connections);
    void writeMeta(FileHelper& fileHelper, Meta& metaData);

    const NetworkProps& mNetworkProps;
    FormulaCalculator& mFormulaCalculator;
};

#endif