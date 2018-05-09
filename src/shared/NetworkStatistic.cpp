#include "NetworkStatistic.h"


NetworkStatistic::NetworkStatistic(const NetworkProps& networkProps, QObject* parent)
    : QObject(parent), mNetwork(networkProps)
    {
    };


NetworkStatistic::~NetworkStatistic() {
}


void NetworkStatistic::calculate(const IdList& preNeurons, const IdList& postNeurons){
    mConnectionsDone = 0;
    this->doCalculate(preNeurons, postNeurons);
}


QJsonObject NetworkStatistic::createJson(const QString& key, const qint64 fileSizeBytes){
    QJsonObject obj;
    doCreateJson(obj);
    obj.insert("downloadS3key", key);
    obj.insert("fileSize", fileSizeBytes);
    return obj;
}


QJsonObject NetworkStatistic::createJson(){
    QJsonObject obj;
    doCreateJson(obj);
    return obj;
}


QString NetworkStatistic::createCSVFile(const QString& key,
                      const QString& presynSelectionText,
                      const QString& postsynSelectionText,
                      const QString& tmpDir) const{

    QString filename = QString("%1/%2.csv").arg(tmpDir).arg(key);
    QFile csv(filename);
    if (!csv.open(QIODevice::WriteOnly)) {
        QString msg = QString("Cannot open file for saving csv: %1").arg(filename);
        throw std::runtime_error(qPrintable(msg));
    }
    const QChar sep(',');

    QTextStream out(&csv);
    out << "Presynaptic selection:" << sep << "\"" << presynSelectionText << "\"" << "\n";
    out << "Postsynaptic selection:" << sep << "\"" << postsynSelectionText << "\"" << "\n";
    out << "\n";

    doCreateCSV(out,sep);

    return filename;
}


void NetworkStatistic::doCreateJson(QJsonObject& /*obj*/) const{
    // do nothing by default
}


void NetworkStatistic::doCreateCSV(QTextStream& /*out*/, const QChar /*sep*/) const{
    // do nothing by default
}


void NetworkStatistic::reportUpdate() {
    emit update(this);
}


void NetworkStatistic::reportComplete(){
    emit complete(this);
}


long long NetworkStatistic::getNumConnections() const{
    return mNumConnections;
}


long long NetworkStatistic::getConnectionsDone() const{
    return mConnectionsDone;
}
