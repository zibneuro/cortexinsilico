#pragma once

#include "NeuronSelection.h"

/**
    This class provides memory efficient access to neuron features.
    Pre-filters the features according to the current pre- and
    postsynaptic selection.
*/
class FeatureProvider {

public:
  /*
      Constructor.
  */
  FeatureProvider();

  /*
      Destructor.
  */
  ~FeatureProvider();

  /*
      Preprocesses the specified neurons selection creating
      a file init.csv.
      @param networkProps The model data.
      @param selection The neuron selection.
  */
  void preprocess(NetworkProps &networkProps, NeuronSelection &selection);

  /*
    Loads the features for simulation based on an init.csv file.
  */
  void init();

  /*
    Returns the number of unique presynaptic neurons.
  */
  int getNumPre();

  /*
    Returns the number of postsynaptic neurons.
  */
  int getNumPost();

  /*
    Returns the presynaptic target density for
    the specified index [0..numPreUnique)
  */
  SparseField *getPre(int index);

  /*
    Returns the postsynaptic target density for
    the specified index [0..numPost) for excitatory
    presynaptic neurons.
  */
  SparseField *getPostExc(int index);

  /*
    Returns the postsynaptic target density for
    for excitatory presynaptic neurons.
  */
  SparseField *getPostAllExc();

  /*
    Returns the multiplicity of the
    specified presynaptic neuron [0..numPreUnique)
  */
  int getPreMultiplicity(int index);

private:
  void saveInitFile(QString fileName);

  void loadInitFile(QString fileName);

  SparseField *mPostAllExc;
  QList<SparseField *> mPre;
  QList<int> mPreMultiplicity;
  QList<SparseField *> mPostExc;

  const QString mInitFileName = "init.csv";
};