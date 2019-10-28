#include "FileHelper.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStringList>
#include <stdexcept>
#include "Util.h"
#include "UtilIO.h"

FileHelper::FileHelper(QJsonObject config) : mConfig(config){};

void FileHelper::initFolder(QString queryId)
{
    mQueryId = queryId;
    mTempFolder = QDir::cleanPath(mConfig["WORKER_TMP_DIR"].toString() +
                                  QDir::separator() + queryId);
    UtilIO::makeDir(mTempFolder);
}

void FileHelper::openFile(QString filename)
{
    QString filepath =
        QDir(mTempFolder).filePath(filename);
    mCurrentFile = new QFile(filepath);
    if (!mCurrentFile->open(QIODevice::WriteOnly))
    {
        const QString msg =
            QString("Cannot open file %1 for writing.").arg(filepath);
        throw std::runtime_error(qPrintable(msg));
    }
    mFiles.push_back(filename);
}

void FileHelper::write(QString line)
{
    QTextStream stream(mCurrentFile);
    stream << line;
}

void FileHelper::closeFile()
{
    mCurrentFile->close();
}

void FileHelper::uploadFolder(QJsonObject &result)
{
    QString key = QString("in-silico-experiment_%1.zip").arg(mQueryId);
    QString zipFilepath = QDir(mTempFolder).filePath(key);

    QProcess zip;
    zip.setWorkingDirectory(mTempFolder);

    QStringList arguments;
    arguments.append("-j");
    arguments.append(zipFilepath);
    for (auto it = mFiles.begin(); it != mFiles.end(); it++)
    {
        arguments.append(*it);
    }

    zip.start("zip", arguments);
    if (!zip.waitForStarted())
    {
        throw std::runtime_error("Error starting zip process");
    }
    if (!zip.waitForFinished())
    {
        throw std::runtime_error("Error completing zip process");
    }

    QString program = mConfig["WORKER_PYTHON_BIN"].toString();
    QStringList arguments2;
    arguments2.append(mConfig["WORKER_S3UPLOAD_SCRIPT"].toString());
    arguments2.append("UPLOAD");
    arguments2.append(key);
    arguments2.append(zipFilepath);
    arguments2.append(mConfig["AWS_ACCESS_KEY_CIS3D"].toString());
    arguments2.append(mConfig["AWS_SECRET_KEY_CIS3D"].toString());
    arguments2.append(mConfig["AWS_S3_REGION_CIS3D"].toString());
    arguments2.append(mConfig["AWS_S3_BUCKET_CIS3D"].toString());
    QProcess::execute(program, arguments2);

    qint64 fileSizeBytes = QFileInfo(zipFilepath).size();    
    result.insert("downloadS3key", key);
    result.insert("fileSize", fileSizeBytes);
}