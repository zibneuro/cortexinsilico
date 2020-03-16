#ifndef SUBVOLUME_H
#define SUBVOLUME_H

#include <QString>
#include <set>
#include <map>
#include <vector>

struct PropsPresynaptic{
    float length;
    float boutons;
    int branchlets;
};

struct PropsPostsynaptic{
    float length;
    float pstExc;
    float pstInh;
    int branchlets;
};

class Subvolume {

public:
    int SID;
    std::map<int, PropsPresynaptic> presynaptic;
    std::map<int, PropsPostsynaptic> postsynaptic;
    std::set<int> cellbodies;

    void load(QString dataDir, int sid, std::set<int>& whitelistPre, std::set<int>& whitelistPreMapped, std::set<int>& whitelistPost);

private:
    void loadPresynaptic(QString filename, std::set<int>& whitelistPre);
    void loadPostsynaptic(QString filename, std::set<int>& whitelistPost);
    void loadCellbodies(QString filename, std::set<int>& whitelistPre, std::set<int>& whitelistPost);
};

#endif