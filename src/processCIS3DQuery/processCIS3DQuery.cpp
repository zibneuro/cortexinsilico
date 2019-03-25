#include <QDebug>
#include <QtCore>
#include "DataUploadHandler.h"
#include "QueryHandler.h"
#include "SelectionQueryHandler.h"
#include "EvaluationQueryHandler.h"
#include "InnervationQueryHandler.h"
#include "MotifQueryHandler.h"
#include "InDegreeQueryHandler.h"
#include "SpatialInnervationQueryHandler.h"
#include "VoxelQueryHandler.h"
#include "UtilIO.h"


void
myMessageOutput(QtMsgType, const QMessageLogContext&, const QString& msg)
{
    QTextStream cout(stdout, QIODevice::WriteOnly);
    cout << msg << endl;
}

int
main(int argc, char* argv[])
{
    qInstallMessageHandler(myMessageOutput);
    QCoreApplication app(argc, argv);

    const QString specFile = app.arguments().at(1);
    const QJsonObject config = UtilIO::parseSpecFile(specFile);
    const QString operation = app.arguments().at(2);
    const QString queryId = app.arguments().at(3);

    if (operation == "uploadNetworkData")
    {
        DataUploadHandler* handler = new DataUploadHandler();
        QObject::connect(handler, SIGNAL(completedProcessing()), &app, SLOT(quit()), Qt::QueuedConnection);
        handler->uploadNetworkData(config);
    }
    else if (operation == "uploadFile")
    {
        const QString filePath = app.arguments().at(4);
        DataUploadHandler* handler = new DataUploadHandler();
        QObject::connect(handler, SIGNAL(completedProcessing()), &app, SLOT(quit()), Qt::QueuedConnection);
        handler->uploadFile(config, queryId, filePath);
    }
    else if (operation == "processQuery")
    {
        QueryHandler* handler;
        const QString queryFile = config["QUERY_DIRECTORY"].toString() + "/" + queryId + "/" + queryId + ".json"; 
        const QJsonObject query = UtilIO::parseSpecFile(queryFile);
        const QString queryType = query["queryType"].toString();
        if(queryType == "innervation"){
            handler = new InnervationQueryHandler();
            handler->processQuery(config, queryId, query);
        } else if (queryType == "selection") {
            handler = new SelectionQueryHandler();
            handler->processQuery(config, queryId, query);            
        } else {
            //throw std::runtime_error("Invalid query type.");
        }
    } else {
        throw std::runtime_error("Invalid mode.");
    }
    return app.exec();
}
