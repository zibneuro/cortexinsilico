#ifndef VOXELQUERYHANDLER_H
#define VOXELQUERYHANDLER_H

#include "CIS3DNetworkProps.h"
#include "CIS3DStatistics.h"
#include "Histogram.h"
#include "QueryHelpers.h"
#include "FeatureProvider.h"
#include "RandomGenerator.h"
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QtNetwork/QNetworkAccessManager>

class VoxelQueryHandler : public QObject
{
    Q_OBJECT

public:
    VoxelQueryHandler(QObject* parent = 0);

    void process(const QString& voxelQueryId, const QJsonObject& config);

signals:
    void completedProcessing();
    void exitWithError(int);

private slots:
    void replyGetQueryFinished(QNetworkReply* reply);
    void replyPutResultFinished(QNetworkReply* reply);

private:
    QJsonObject
    createJsonResult();
    float calculateSynapseProbability(float innervation, int k);
    void readIndex(QString indexFile, std::vector<double> origin, std::vector<int> dimensions, std::set<int>& voxelIds, std::set<int>& neuronIds);
    template <typename T>
    void createStatistics(std::map<int, T>& values, Statistics& stat, Histogram& histogram);


    QString mQueryId;
    QJsonObject mConfig;
    QString mDataRoot;
    QNetworkAccessManager mNetworkManager;
    NetworkProps mNetwork;
    QString mLoginUrl;
    QString mLogoutUrl;
    QString mQueryUrl;
    AuthInfo mAuthInfo;
    QJsonObject mCurrentJsonData;
    Statistics mStatistics;
    QString mTempFolder;

    int mNumberInnervatedVoxels;
    int mNumberTotalVoxels;
    Statistics mPresynapticCellsPerVoxel;
    Histogram mPresynapticCellsPerVoxelH;
    Statistics mPostsynapticCellsPerVoxel;
    Histogram mPostsynapticCellsPerVoxelH;
    Statistics mBoutonsPerVoxel;
    Histogram mBoutonsPerVoxelH;
    Statistics mPostsynapticSitesPerVoxel;
    Histogram mPostsynapticSitesPerVoxelH;

    std::map<int, float> mMapBoutonsPerVoxel;
    std::map<int, int> mMapPreCellsPerVoxel;
    std::map<int, float> mMapPostsynapticSitesPerVoxel;
    std::map<int, int> mMapPostCellsPerVoxel;
    std::set<int> mSelectedVoxels;
    std::set<int> mPreInnervatedVoxels;
    std::set<int> mPostInnervatedVoxels;
    std::map<int, float> mSynapsesPerVoxel;    
    Statistics mSynapsesPerConnection;
    std::map<int, std::vector<float> > mSynapsesPerConnectionOccurrences;
    
    bool mAborted;

    void logoutAndExit(const int exitCode);
};

#endif // VOXELQUERYHANDLER_H