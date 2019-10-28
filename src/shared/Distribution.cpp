#include "Distribution.h"

Distribution::Distribution(){};

void Distribution::addSample(int subvolume, double value)
{
    mSubvolume.push_back(subvolume);
    mData.push_back(value);
};

QJsonArray Distribution::getJson()
{
    QJsonArray array;
    for (auto it = mData.begin(); it != mData.end(); it++)
    {
        array.push_back(*it);
    }
    return array;
}

void Distribution::writeFile(FileHelper& fileHelper, QString filename, QString unit){
    fileHelper.openFile(filename);
    fileHelper.write("subvolume_id,value " + unit + "\n");
    for(unsigned int i =0 ;i < mSubvolume.size(); i++){
        fileHelper.write(QString::number(mSubvolume[i])+","+QString::number(mData[i])+"\n");
    }
    fileHelper.closeFile();
}