#ifndef DISTRIBUTION_H
#define DISTRIBUTION_H

#include <vector>
#include <QJsonArray>
#include "FileHelper.h"

class Distribution
{
  public:
    Distribution();
    void addSample(int subvolume, double value);
    QJsonArray getJson();
    void writeFile(FileHelper& fileHelper, QString filename, QString unit = "");

  private:
    std::vector<int> mSubvolume;
    std::vector<double> mData;
};

#endif