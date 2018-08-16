#pragma once

#include "NeuronSelection.h"

/**
    This class provides memory efficient access to neuron features.
    Pre-filters the features according to the current pre- and 
    postsynaptic selection.
*/
class FeatureProvider {

public:

    FeatureProvider();

    ~FeatureProvider();

    /*
        Initializes the features according to the specified collection.
        @selection The neuron selection.
    */
    void preprocess(NetworkProps& networkProps, NeuronSelection& selection);
    
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
    
    SparseField* mPostAllExc;    
    QList<SparseField*> mPre;
    QList<int> mPreMultiplicity;
    QList<SparseField*> mPostExc; 

    const QString mInitFileName = "init.csv";
};