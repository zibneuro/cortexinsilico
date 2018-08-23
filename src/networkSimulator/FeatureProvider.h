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

    void preprocessFeatures(NetworkProps& networkProps, NeuronSelection& selection, double eps);

    void init();

    int getNumPre();

    int getNumPost();

    SparseField* getPre(int index);

    SparseField* getPostExc(int index);

    SparseField* getPostAllExc();

    int getPreMultiplicity(int index);

private:
    void saveInitFile(QString fileName);
    void loadInitFile(QString fileName);
    void assertGrid(SparseField* field);
    void writeMapFloat(std::map<int, float>& mapping, QString folder, QString file);
    void writeMapInt(std::map<int, int>& mapping, QString folder, QString file);
    void registerVoxelIds(std::set<int>& voxelIds, std::map<int, float>& field);
    void writeVoxels(SparseField* postAllExc, std::set<int>& voxelIds, QString folder, QString fileName);

    Vec3f mGridOrigin;
    Vec3i mGridDimensions;
    SparseField* mPostAllExc;
    QList<SparseField*> mPre;
    QList<int> mPreMultiplicity;
    QList<SparseField*> mPostExc;

    const QString mInitFileName = "init.csv";
};