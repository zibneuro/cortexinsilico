#pragma once

#include "NeuronSelection.h"
#include "CIS3DVec3.h"
#include <set>
#include <map>

/**
    This class provides memory efficient access to neuron features.
    Pre-filters the features according to the current pre- and
    postsynaptic selection.
*/
class FeatureProvider
{
public:
    FeatureProvider();

    ~FeatureProvider();

    void preprocess(NetworkProps& networkProps, NeuronSelection& selection, bool duplicity = true);

    void preprocessFeatures(NetworkProps& networkProps, NeuronSelection& selection, double eps, bool applyLog, bool normalized = false);

    void init();

    int getNumPre();

    int getNumPost();

    SparseField* getPre(int index);

    SparseField* getPostExc(int index);

    SparseField* getPostAllExc();

    int getPreMultiplicity(int index);

    void load(std::map<int, std::map<int, float> >& neuron_pre,
              float b1,
              std::map<int, std::map<int, float> >& neuron_postExc,
              float b2,
              std::map<int, std::map<int, float> >& neuron_postInh,
              std::map<int, float>& voxel_postAllExc,
              float b3,
              std::map<int, float>& voxel_postAllInh,
              std::map<int, int>& neuron_funct,
              std::map<int, std::set<int> >& voxel_neuronsPre,
              std::map<int, std::set<int> >& voxel_neuronsPostExc,
              std::map<int, std::set<int> >& voxel_neuronsPostInh);

private:
    void saveInitFile(QString fileName);
    void loadInitFile(QString fileName);
    void assertGrid(SparseField* field);
    void writeMapFloat(std::map<int, float>& mapping, std::set<int>& voxelIds, QString folder, QString file);
    void writeMapInt(std::map<int, int>& mapping, QString folder, QString file);
    void writeIndex(std::map<int, std::set<int> >& index, QString folder, QString file);
    void loadIndex(std::map<int, std::set<int> >& index, QString folder, QString fileName);
    void readMapFloat(std::map<int, float>& mapping, QString folder, QString fileName, float coefficient = 1);
    void readMapInt(std::map<int, int>& mapping, QString folder, QString fileName);
    void readIndex(std::map<int, std::set<int> >& index, QString folder, QString file);
    void registerVoxelIds(std::set<int>& voxelIds, std::map<int, float>& field);
    void buildIndex(std::map<int, std::set<int> >& index, std::set<int>& voxelIds, std::map<int, std::map<int, float> >& fields);
    void writeVoxels(SparseField* postAllExc, std::set<int>& voxelIds, QString folder, QString fileName);
    bool inRange(SparseField* postAllExc, QVector<float>& bbMin, QVector<float>& bbMax, int voxelId);
    void intersectSets(std::set<int>& a, std::set<int>& b, bool recurse = true);
    std::set<int> createUnion(std::map<int, std::map<int, float> > fields);
    std::set<int> createUnion(std::set<int>& a, std::set<int>& b);

    Vec3f mGridOrigin;
    Vec3i mGridDimensions;
    SparseField* mPostAllExc;
    QList<SparseField*> mPre;
    QList<int> mPreMultiplicity;
    QList<SparseField*> mPostExc;

    const QString mInitFileName = "init.csv";
};
