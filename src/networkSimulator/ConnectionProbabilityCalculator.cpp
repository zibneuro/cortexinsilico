#include "ConnectionProbabilityCalculator.h"
#include "CIS3DConstantsHelpers.h"
#include "CIS3DSparseField.h"
#include "CIS3DSparseVectorSet.h"
#include "SparseFieldCalculator.h"
#include "Typedefs.h"
#include <mutex>
#include <omp.h>
#include <iomanip> 
#include <QTextStream>
#include <QIODevice>
#include <QFile>
#include "Distribution.h"

/*
    Constructor.
    @param featureProvider The features of the neuron selections.
*/
ConnectionProbabilityCalculator::ConnectionProbabilityCalculator(
    FeatureProvider &featureProvider)
    : mFeatureProvider(featureProvider) {
  mNumPre = mFeatureProvider.getNumPre();
  mNumPost = mFeatureProvider.getNumPost();
}

/*
    Calculates the connection probability for the specified
    rule parameters.
    @param The connectivity rule parameters.
    @return The connection probability.
*/
double ConnectionProbabilityCalculator::calculate(QVector<float> parameters) {

  // qDebug() << "[*] Start simulation.";

  SparseField *postAll = mFeatureProvider.getPostAllExc();
  Statistics innervationHistogram;
  SparseFieldCalculator fieldCalculator;
  std::mutex mutex;
  std::mutex mutex2;
  for (int i = 0; i < mNumPre; i++) {
//#pragma omp parallel for schedule(dynamic)
    for (int j = 0; j < mNumPost; j++) {
      mutex.lock();
      SparseField *pre = mFeatureProvider.getPre(i);
      SparseField *post = mFeatureProvider.getPostExc(j);
      int multiplicity = mFeatureProvider.getPreMultiplicity(i);
      mutex.unlock();
      float innervation = fieldCalculator.calculatePetersRule(
          *pre, *post, *postAll, parameters[0], parameters[1], parameters[2],
          parameters[3]);
      mutex2.lock();
      for (int k = 0; k < multiplicity; k++) {
        innervationHistogram.addSample(innervation);
      }
      mutex2.unlock();
    }
  }

  // qDebug() << "[*] Finish simulation.";

  return calculateProbability(innervationHistogram.getMean());
}

void ConnectionProbabilityCalculator::calculateSynapse(QVector<float> parameters){
  
  float eps = 0.000001;
  float b0 = parameters[0];
  float b1 = parameters[1];
  float b2 = parameters[2];
  float b3 = parameters[3];

  std::map<int, float> postAllField = mFeatureProvider.getPostAllExc()->getModifiedCopy(b3,eps);

  std::vector<std::map<int,float> > preFields;
  for(int i=0; i<mNumPre; i++){
    preFields.push_back(mFeatureProvider.getPre(i)->getModifiedCopy(b1,eps));
  }

  std::vector<std::map<int,float> > postFields;
  for(int i=0; i<mNumPost; i++){
    postFields.push_back(mFeatureProvider.getPostExc(i)->getModifiedCopy(b2,eps));
  }

  std::vector<int> empty(mNumPost,0);
  std::vector<std::vector<int> > synapses(mNumPre,empty);

  for(int i=0; i<mNumPre; i++){    
    #pragma omp parallel for schedule(dynamic)
    for(int j=0; j<mNumPost; j++){
      Distribution dist;
      for (std::map<int,float>::iterator itPre=preFields[i].begin(); itPre!=preFields[i].end(); ++itPre)    {  
        std::map<int,float>::iterator itPost = postFields[j].find(itPre->first);
        std::map<int,float>::iterator itPostAll = postAllField.find(itPre->first);
        if(itPost != postFields[j].end() && itPostAll != postAllField.end()){
          float mu = exp(b0 + itPre->second + itPost->second + itPostAll->second);
          int synapsesInVoxel = dist.drawSynapseCount(mu);
          synapses[i][j] = synapses[i][j] + synapsesInVoxel;
        }
    }
  }
  }
 
 
  QString filename;
  filename.sprintf("%+06.3f_%+06.3f_%+06.3f_%+06.3f", parameters[0],parameters[1],parameters[2],parameters[3]);
  QFile file(filename);
  if (!file.open(QIODevice::WriteOnly)) {
      const QString msg = QString("Cannot open file %1 for writing.").arg(filename);
      throw std::runtime_error(qPrintable(msg));
  }
   QTextStream out(&file);
  for(int i=0; i<mNumPre; i++){
    for(int j=0; j<mNumPost; j++){
    out << synapses[i][j];
    if(j+1<mNumPost){
      out << " ";
    }
      }
      if(i+1 <mNumPre){ 
      out << "\n"; 
      }
  }
}

double
ConnectionProbabilityCalculator::calculateProbability(double innervationMean) {
  return 1 - exp(-1 * innervationMean);
}
