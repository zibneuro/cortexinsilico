#ifndef DISTRIBUTION_H
#define DISTRIBUTION_H

#include <vector>
#include <QJsonArray>

class Distribution
{
  public:
    Distribution();
    void addSample(double value);
    QJsonArray getJson();

  private:
    std::vector<double> mData;
};

#endif