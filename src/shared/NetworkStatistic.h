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
#include "FormulaCalculator.h"
#include "QueryHandler.h"
#include "FileHelper.h"

/**
    Serves as base class for any summary statistic about the neural network.
    Provides interfaces to integrate statistic into the webframework.
*/
class NetworkStatistic {
    
public:
    /**
        Constructor.
        @param networkProps The model data of the network.
        @param parent The Qt parent object. Empty by default.
    */
    NetworkStatistic(const NetworkProps& networkProps, FormulaCalculator& calculator, QueryHandler* handler);

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

    float calculateProbability(float innervation);

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
    void createCSVFile(FileHelper& fileHelper) const;

    virtual void writeSubquery(FileHelper& fileHelper);

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

    void update(NetworkStatistic* stat);

    void complete(NetworkStatistic* stat);

    virtual bool hasSubquery(QString& subquery, QString& subqueryResultKey);

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
    virtual void doCreateCSV(FileHelper& fileHelper) const;

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

    FormulaCalculator& mCalculator;

    /*
        The innervation matrix.
    */
    InnervationMatrix* mInnervationMatrix;

    bool mAborted;

    QueryHandler* mQueryHandler;
    
};

#endif // NETWORKSTATISTIC_H
