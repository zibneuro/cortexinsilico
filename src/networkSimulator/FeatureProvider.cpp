#include "FeatureProvider.h"
#include <QList>
#include "Typedefs.h"
#include "FeatureReader.h"

FeatureProvider::FeatureProvider() {}

void FeatureProvider::init(NeuronSelection selection) { mSelection = selection; 
    FeatureReader reader;
    QList<Feature> features = reader.load("features.csv");
        
}

int FeatureProvider::getNumPre() { return mSelection.Presynaptic().size(); }

IdList FeatureProvider::getPre() { return mSelection.Presynaptic(); }

int FeatureProvider::getNumPost() { return mSelection.Postsynaptic().size(); }

QList<FeatureSet> FeatureProvider::getVoxelFeatures() { return mVoxelFeatures; }