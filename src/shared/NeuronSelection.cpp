#include "NeuronSelection.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include "Util.h"
#include "UtilIO.h"

/**
  Empty constructor.
*/
NeuronSelection::NeuronSelection(){};

/**
    Constructor for a second order network statistic, containing pre-
    and postsynaptic neuron selections.
    @param presynaptic The presynaptic neuron IDs.
    @param postsynaptic The postsynaptic neuron IDs.
*/
NeuronSelection::NeuronSelection(const IdList& presynaptic, const IdList& postsynaptic)
    : mPresynaptic(presynaptic), mPostsynaptic(postsynaptic){};

/**
      Determines a innervation statistic selection from a specification file.
      @param spec The spec file with the filter definition.
      @param networkProps the model data of the network.
*/
void NeuronSelection::setInnervationSelection(const QJsonObject& spec,
                                              const NetworkProps& networkProps) {
    mPresynaptic.clear();
    mPresynaptic.append(UtilIO::getPreSynapticNeurons(spec, networkProps));
    mPostsynaptic.clear();
    mPostsynaptic.append(UtilIO::getPostSynapticNeuronIds(spec, networkProps));
}

/**
    Determines a triplet motif statistic selection from a specification file.
    @param spec The spec file with the filter definition.
    @param networkProps the model data of the network.
*/
void NeuronSelection::setTripletSelection(const QJsonObject& spec,
                                          const NetworkProps& networkProps) {
    QJsonObject tmpSpec;

    tmpSpec["NEURON_REGIONS"] = spec["MOTIF_A_REGIONS"];
    tmpSpec["NEURON_CELLTYPES"] = spec["MOTIF_A_CELLTYPES"];
    tmpSpec["NEURON_IDS"] = spec["MOTIF_A_IDS"];

    mMotifA.clear();
    mMotifA.append(UtilIO::getNeuronIds(tmpSpec, networkProps));

    tmpSpec["NEURON_REGIONS"] = spec["MOTIF_B_REGIONS"];
    tmpSpec["NEURON_CELLTYPES"] = spec["MOTIF_B_CELLTYPES"];
    tmpSpec["NEURON_IDS"] = spec["MOTIF_B_IDS"];
    mMotifB.clear();
    mMotifB.append(UtilIO::getNeuronIds(tmpSpec, networkProps));

    tmpSpec["NEURON_REGIONS"] = spec["MOTIF_C_REGIONS"];
    tmpSpec["NEURON_CELLTYPES"] = spec["MOTIF_C_CELLTYPES"];
    tmpSpec["NEURON_IDS"] = spec["MOTIF_C_IDS"];
    mMotifC.clear();
    mMotifC.append(UtilIO::getNeuronIds(tmpSpec, networkProps));
}

/**
    Determines a triplet motif statistic selection from selection strings.
    @param motifASelString The first selection string.
    @param motifBSelString The second selection string.
    @param motifASelString The third selection string.
    @param networkProps the model data of the network.
*/
void NeuronSelection::setTripletSelection(const QString motifASelString,
                                          const QString motifBSelString,
                                          const QString motifCSelString,
                                          const NetworkProps& networkProps) {
    mMotifA.clear();
    mMotifA.append(getSelectedNeurons(motifASelString, networkProps));
    mMotifB.clear();
    mMotifB.append(getSelectedNeurons(motifBSelString, networkProps));
    mMotifC.clear();
    mMotifC.append(getSelectedNeurons(motifCSelString, networkProps));
}

/**
    Determines neuron IDs based on a selection string;
    @param selectionString The selection string.
    @param networkProps The model data of the network.
    @return A list of neuron IDs.
*/
IdList NeuronSelection::getSelectedNeurons(const QString selectionString,
                                           const NetworkProps& networkProps) {
    QJsonDocument doc = QJsonDocument::fromJson(selectionString.toLocal8Bit());
    QJsonArray arr = doc.array();
    SelectionFilter filter = Util::getSelectionFilterFromJson(arr, networkProps, CIS3D::BOTH_SIDES);
    return networkProps.neurons.getFilteredNeuronIds(filter);
}

/**
  Returns the presynaptic subselection.
*/
IdList NeuronSelection::Presynaptic() const { return mPresynaptic; }

/**
  Returns the postynaptic subselection.
*/
IdList NeuronSelection::Postsynaptic() const { return mPostsynaptic; }

/**
  Returns the first neuron subselection for motif statistics.
*/
IdList NeuronSelection::MotifA() const { return mMotifA; }

/**
  Returns the second neuron subselection for motif statistics.
*/
IdList NeuronSelection::MotifB() const { return mMotifB; }

/**
  Returns the third neuron subselection for motif statistics.
*/
IdList NeuronSelection::MotifC() const { return mMotifC; }

/*
    Prints the number of selected neurons for motif statistics.
*/
void NeuronSelection::printMotifStats() {
    qDebug() << "[*] Number of selected neurons (motif A,B,C):" << mMotifA.size() << mMotifB.size()
             << mMotifC.size();
}