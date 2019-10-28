#ifndef FILEHELPER_H
#define FILEHELPER_H

#include <QJsonObject>
#include <QFile>
#include <QTextStream>

class FileHelper
{
  public:
    FileHelper();

    void initFolder(QJsonObject config, QString queryId);
    void openFile(QString filename);
    void write(QString line);
    void closeFile();
    void uploadFolder(QJsonObject& result);

  private:
    QJsonObject mConfig;
    QString mQueryId;
    QString mTempFolder;
    std::vector<QString> mFiles;
    QFile* mCurrentFile;
};

#endif