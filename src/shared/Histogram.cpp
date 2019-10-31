#include "Histogram.h"
#include <QJsonArray>
#include <math.h>
#include "Util.h"

double Histogram::getBinSize(double expectedMaxValue)
{
  return expectedMaxValue / static_cast<double>(REF_BINCOUNT);
}

/**
    Constructor.
    Default bin size set to 1.0.
*/
Histogram::Histogram()
    : mBinSize(1.0), mNumZeros(0) {}

/** Constructor.
    @param binSize Width of bins.
*/
Histogram::Histogram(const double binSize)
    : mBinSize(binSize), mNumZeros(0) {}

/**
    Adds a sampe value to the histogram.
    @param v The sample value.
*/
void Histogram::addValue(const double v)
{
  if (v < 0)
  {
    throw std::runtime_error("Histogram: No negative values allowed");
  }

  if (Util::isZero(v))
  {
    if (mBins.find(0) == mBins.end())
    {
      mBins[0] = 1;
    }
    else
    {
      mBins[0] += 1;
    }
  }
  else
  {
    const long long binNum = static_cast<long long>(ceil(v / mBinSize));
    if (mBins.find(binNum) == mBins.end())
    {
      mBins[binNum] = 1;
    }
    else
    {
      mBins[binNum] += 1;
    }

    mBins[binNum] += 1;
  }
}

/**
    Adds the same sample value to the histogram k times.
    @param v The sample value.
    @param k The multiplicity.
*/
void Histogram::addValues(const double v, const int k)
{
  if (v < 0.0)
  {
    throw std::runtime_error("Histogram: No negative values allowed");
  }

  for (int i = 0; i < k; i++)
  {
    addValue(v);
  }
}

long long Histogram::getNumberOfValues() const
{
  long long number = 0;
  for (auto it = mBins.begin(); it != mBins.end(); it++)
  {
    number += it->second;
  }
  return number;
}

long long Histogram::getNumberOfZeros() const
{
  auto it = mBins.find(0);
  if (it == mBins.end())
  {
    return 0;
  }
  else
  {
    return it->second;
  }
}

/**
    Creates a json representation of the histogram.
    @returns Histogram as json object.
*/
QJsonObject Histogram::createJson() const
{
  QJsonObject histObj;
  QJsonArray binCenters;
  QJsonArray binCounts;

  for (auto it = mBins.begin(); it != mBins.end(); it++)
  {
    double binCenter = mBinSize * it->first - 0.5 * mBinSize;
    double binCount = static_cast<double>(it->second);
    binCenters.push_back(QJsonValue(binCenter));
    binCounts.push_back(QJsonValue(binCount));
  }

  histObj.insert("numberOfValues", QJsonValue(getNumberOfValues()));
  histObj.insert("numberOfZeros", QJsonValue(getNumberOfZeros()));
  histObj.insert("binSize", QJsonValue(mBinSize));
  histObj.insert("binCenters", binCenters);
  histObj.insert("binCounts", binCounts);

  return histObj;
};

void Histogram::writeFile(FileHelper &fileHelper, QString filename) const
{

  fileHelper.openFile(filename);
  fileHelper.write("bin_index,bin_min_exclusive,bin_max_inclusive,bin_value\n");
  for (auto it = mBins.begin(); it != mBins.end(); it++)
  {
    if (it->first > 0)
    {
      double max = it->first * mBinSize;
      double min = max - mBinSize;
      long long value = it->second;
      fileHelper.write(QString::number(it->first) + "," + QString::number(min) + "," + QString::number(max) + "," + QString::number(value) + "\n");
    }
    else
    {
      fileHelper.write("0,-inf,0," + QString::number(mNumZeros) + "\n");
    }
  }
  fileHelper.closeFile();
}
