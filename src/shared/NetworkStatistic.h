#ifndef NETWORKSTATISTIC_H
#define NETWORKSTATISTIC_H

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <QTextStream>
#include "CIS3DNetworkProps.h"
#include "CIS3DSparseVectorSet.h"
#include "CIS3DStatistics.h"
#include "Histogram.h"
#include "InnervationMatrix.h"
#include "NeuronSelection.h"
#include "SparseVectorCache.h"
#include "Typedefs.h"

/**
    Serves as base class for any summary statistic about the neural network.
    Provides interfaces to integrate statistic into the webframework.
*/
class NetworkStatistic : public QObject {
    Q_OBJECT

   public:
    /**
        Constructor.
        @param networkProps The model data of the network.
        @param parent The Qt parent object. Empty by default.
    */
    NetworkStatistic(const NetworkProps& networkProps, QObject* parent = 0);

    /**
        Constructor.
        @param networkProps The model data of the network.
        @param cache Cache of preloaded innervation values.
        @param parent The Qt parent object. Empty by default.
    */
    NetworkStatistic(const NetworkProps& networkProps, const SparseVectorCache& cache,
                     QObject* parent = 0);

    /**
        Destructor.
    */
    virtual ~NetworkStatistic();

    /**
        Starts calculation with the specified neurons. To be called from the
        webframework.
        @param selection The selected neurons.
    */
    void calculate(const NeuronSelection& selection);

    /**
        Creates a JSON object representing the statistic. To be called from
        the webframework.
        @param key The S3 key under which JSON object is strored.
        @param fileSizeBytes The file size (applies, when JSON object
            references csv file).
        @return The JSON object.
    */
    QJsonObject createJson(const QString& key, const qint64 fileSizeBytes);

    /**
        Creates a JSON object representing the statistic. To be called from
        the webframework.
        @return The JSON object.
    */
    QJsonObject createJson();

    /**
        Creates a csv-file of the statistic.
        @param key The S3 key under which the csv-file is stored.
        @param presynSelectionText Textual description of presynaptic filter.
        @param postsynSelectionText Textual description of postsynaptic filter.
        @param tmpDir Folder where the file is initially created.
        @returns The file name.
    */
    virtual QString createCSVFile(const QString& key, const QString& presynSelectionText,
                                  const QString& postsynSelectionText, const QString& tmpDir) const;

    /**
        Creates a csv-file of the statistic.
        @param key The S3 key under which the csv-file is stored.
        @param motifASelectionText Textual description of the first filter.
        @param motifBSelectionText Textual description of the second filter.
        @param motifCSelectionText Textual description of the third filter.
        @param tmpDir Folder where the file is initially created.
        @returns The file name.
    */
    virtual QString createCSVFile(const QString& key, const QString& motifASelectionText,
                                  const QString& motifBSelectionText,
                                  const QString& motifCSelectionText, const QString& tmpDir) const;

    /**
        Retrieves the total number of connections to be analysed.
        @return The number of connections.
    */
    long long getNumConnections() const;

    /**
        Retrieves the number of connections analysed to this point.
        @return The number of connections.
    */
    long long getConnectionsDone() const;

    void abort();

   signals:
    // Signals intermediate results are available
    //
    // param stat: the statistic being computed
    void update(NetworkStatistic* stat);

    // Signals that the computation has been completed
    //
    // param stat: the statistic being computed
    void complete(NetworkStatistic* stat);

   protected:
    /**
        Performs the actual computation based on the specified neurons.
        @param selection The selected neurons.
    */
    virtual void doCalculate(const NeuronSelection& selection) = 0;

    /**
        Adds the result values to a JSON object
        @param obj: JSON object to which the values are appended
    */
    virtual void doCreateJson(QJsonObject& obj) const;

    /**
        Writes the result values to file stream (CSV).
        @param out The file stream to which the values are written.
        @param sep The separator between parameter name and value.
    */
    virtual void doCreateCSV(QTextStream& out, const QChar sep) const;

    /**
     Signals availability of intermediate results, to be called from derived classes
    */
    void reportUpdate();

    /**
        Signals completion of calculation, to be called from base classes
    */
    void reportComplete();

    /**
        The model data of the network.
    */
    const NetworkProps& mNetwork;

    /**
        The total number of connections (presyn. neuron x postsyn. neuron)
        to be analysed.
    */
    long long mNumConnections;

    /**
        The number of connections (presyn. neuron x postsyn. neuron)
        analysed to this point.
    */
    long long mConnectionsDone;

    /**
        A filename-based cache for SparseVectorSets (containing innervation
        matrix entries) that have already been loaded.
    */
    SparseVectorCache mCache;

    /*
        The innervation matrix.
    */
    InnervationMatrix* mConnectome;

    bool mAborted;
};

#endif  // NETWORKSTATISTIC_H
