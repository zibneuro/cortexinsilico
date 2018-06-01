#include <QtCore>
#include <QDebug>
#include "EvaluationQueryHandler.h"
#include "SelectionQueryHandler.h"
#include "NetworkDataUploadHandler.h"

QJsonObject parseSpecFile(const QString& fileName) {
    QFile jsonFile(fileName);
    if (!jsonFile.open(QIODevice::ReadOnly)) {
        const QString msg = QString("Cannot open file for reading: %1").arg(fileName);
        throw std::runtime_error(qPrintable(msg));
    }
    QByteArray data = jsonFile.readAll();
    QJsonDocument doc(QJsonDocument::fromJson(data));
    return doc.object();
}


void printUsage() {
    qDebug() << "Usage: ./processCIS3DQuery <config-file> [evaluationQuery|selectionQuery|networkDataUpload] <query-id>";
}


void myMessageOutput(QtMsgType, const QMessageLogContext&, const QString &msg) {
    QTextStream cout(stdout, QIODevice::WriteOnly);
    cout << msg << endl;
}


int main(int argc, char *argv[])
{
    qInstallMessageHandler(myMessageOutput);
    QCoreApplication app(argc, argv);

    if (app.arguments().size() < 3 || app.arguments().size() > 4) {
        qDebug() << "Wrong number of arguments.";
        printUsage();
        return 1;
    }

    const QString specFile = app.arguments().at(1);
    const QJsonObject config = parseSpecFile(specFile);

    const QString queryType = app.arguments().at(2);
    if (queryType != "evaluationQuery" &&
        queryType != "selectionQuery" &&
        queryType != "networkDataUpload")
    {
        qDebug() << "Invalid query type.";
        printUsage();
        return 1;
    }

    if (queryType == "networkDataUpload" && app.arguments().size() == 4) {
        qDebug() << "Wrong number of arguments for networkDataUpload." << app.arguments().at(2);
        printUsage();
        return 1;
    }

    if (queryType != "networkDataUpload" && app.arguments().size() == 3) {
        qDebug() << "Wrong number of arguments for" << queryType;
        printUsage();
        return 1;
    }

    QString queryId;
    if (queryType != "networkDataUpload") {
        queryId = app.arguments().at(3);
    }

    if (queryType == "evaluationQuery") {
        EvaluationQueryHandler *handler = new EvaluationQueryHandler();
        QObject::connect(handler, SIGNAL(completedProcessing()), &app, SLOT(quit()), Qt::QueuedConnection);
        handler->process(queryId, config);
    }
    else if (queryType == "selectionQuery") {
        SelectionQueryHandler *handler = new SelectionQueryHandler();
        QObject::connect(handler, SIGNAL(completedProcessing()), &app, SLOT(quit()), Qt::QueuedConnection);
        handler->process(queryId, config);
    }
    else if (queryType == "networkDataUpload") {
        NetworkDataUploadHandler *handler = new NetworkDataUploadHandler();
        QObject::connect(handler, SIGNAL(completedProcessing()), &app, SLOT(quit()), Qt::QueuedConnection);
        handler->process(config);
    }

    return app.exec();
}
