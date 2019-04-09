#include "NetworkStatistic.h"

/**
    Constructor.
    @param networkProps The model data of the network.
    @param parent The Qt parent object. Empty by default.
*/
NetworkStatistic::NetworkStatistic(const NetworkProps& networkProps, FormulaCalculator& calculator, QueryHandler* handler)
    : mNetwork(networkProps)
    , mCalculator(calculator)
    , mQueryHandler(handler)
{
    mCache = SparseVectorCache();
    mInnervationMatrix = new InnervationMatrix(networkProps);
    mAborted = false;
};

/**
    Destructor.
*/
NetworkStatistic::~NetworkStatistic()
{
    delete mInnervationMatrix;
}

/**
    Starts calculation with the specified neurons. To be called from the
    webframework.
    @param selection The selected neurons.
*/
void
NetworkStatistic::calculate(const NeuronSelection& selection)
{
    mConnectionsDone = 0;
    this->doCalculate(selection);
}

float
NetworkStatistic::calculateProbability(float innervation)
{
    return mCalculator.calculateConnectionProbability(innervation);
}

/**
    Creates a JSON object representing the statistic. To be called from
    the webframework.
    @param key The S3 key under which JSON object is strored.
    @param fileSizeBytes The file size (applies, when JSON object
        references csv file).
    @return The JSON object.
*/
QJsonObject
NetworkStatistic::createJson(const QString& key, const qint64 fileSizeBytes)
{
    QJsonObject obj;
    doCreateJson(obj);
    obj.insert("downloadS3key", key);
    obj.insert("fileSize", fileSizeBytes);
    return obj;
}

/**
    Creates a JSON object representing the statistic. To be called from
    the webframework.
    @return The JSON object.
*/
QJsonObject
NetworkStatistic::createJson()
{
    QJsonObject obj;
    doCreateJson(obj);
    return obj;
}

/**
    Creates a csv-file of the statistic.
    @param key The S3 key under which the csv-file is stored.
    @param presynSelectionText Textual description of presynaptic filter.
    @param postsynSelectionText Textual description of postsynaptic filter.
`   @param tmpDir Folder where the file is initially created.
    @returns The file name.
*/
QString
NetworkStatistic::createCSVFile(const QString& key,
                                const QString& fileHeader,
                                const QString& tmpDir) const
{
    QString filename = QString("%1/%2.csv").arg(tmpDir).arg(key);
    QFile csv(filename);
    if (!csv.open(QIODevice::WriteOnly))
    {
        QString msg = QString("Cannot open file for saving csv: %1").arg(filename);
        throw std::runtime_error(qPrintable(msg));
    }
    const QChar sep(',');

    QTextStream out(&csv);
    out << fileHeader;

    doCreateCSV(out, sep);

    return filename;
}

void
NetworkStatistic::abort()
{
    mAborted = true;
}

void
NetworkStatistic::setOriginalPreIds(QList<int> preIdsA, QList<int> preIdsB, QList<int> preIdsC)
{
    mInnervationMatrix->setOriginalPreIds(preIdsA, preIdsB, preIdsC);
}

/**
    Adds the result values to a JSON object
    @param obj: JSON object to which the values are appended
*/
void
NetworkStatistic::doCreateJson(QJsonObject& /*obj*/) const
{
    // do nothing by default
}

/**
    Writes the result values to file stream (CSV).
    @param out The file stream to which the values are written.
    @param sep The separator between parameter name and value.
*/
void
NetworkStatistic::doCreateCSV(QTextStream& /*out*/, const QChar /*sep*/) const
{
    // do nothing by default
}

// Signals intermediate results are available
//
// param stat: the statistic being computed
void
NetworkStatistic::reportUpdate()
{
    mQueryHandler->reportUpdate(this);
}

// Signals that the computation has been completed
//
// param stat: the statistic being computed
void
NetworkStatistic::reportComplete()
{
    mQueryHandler->reportComplete(this);
}

/**
    Retrieves the total number of connections to be analysed.
    @return The number of connections.
*/
long long
NetworkStatistic::getNumConnections() const
{
    return mNumConnections;
}

/**
    Retrieves the number of connections analysed to this point.
    @return The number of connections.
*/
long long
NetworkStatistic::getConnectionsDone() const
{
    return mConnectionsDone;
}
