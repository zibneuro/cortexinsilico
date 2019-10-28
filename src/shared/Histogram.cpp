#include "Histogram.h"
#include <QJsonArray>
#include <math.h>

/**
    Constructor.
    Default bin size set to 1.0.
*/
Histogram::Histogram()
    : mBinSize(1.0), mNumValues(0), mNumZeros(0), mTotalValue(0.0),
      mTotalSquaredValue(0.0),
      mMinValue(std::numeric_limits<float>::infinity()),
      mMaxValue(-std::numeric_limits<float>::infinity()) {}

/** Constructor.
    @param binSize Width of bins.
*/
Histogram::Histogram(const double binSize)
    : mBinSize(binSize), mNumValues(0), mNumZeros(0), mTotalValue(0.0),
      mTotalSquaredValue(0.0),
      mMinValue(std::numeric_limits<float>::infinity()),
      mMaxValue(-std::numeric_limits<float>::infinity()) {}

/**
    Adds a sampe value to the histogram.
    @param v The sample value.
*/
void Histogram::addValue(const double v) {
  if (v < 0.0) {
    throw std::runtime_error("Histogram: No negative values allowed");
  }

  if (v < 0.00001) {
    ++mNumZeros;
  } else {
    const int binNum = int(floor(v / mBinSize));

    while (binNum >= mBins.size()) {
      mBins.append(0);
    }

    mBins[binNum] += 1;
  }

  mTotalValue += v;
  mTotalSquaredValue += (v * v);
  mNumValues += 1;

  if (v < mMinValue) {
    mMinValue = v;
  }
  if (v > mMaxValue) {
    mMaxValue = v;
  }
}

/**
    Adds the same sample value to the histogram k times.
    @param v The sample value.
    @param k The multiplicity.
*/
void Histogram::addValues(const double v, const int k) {
  if (v < 0.0) {
    throw std::runtime_error("Histogram: No negative values allowed");
  }

  if (v < 0.00001) {
    mNumZeros += k;
  }
  // else {
  const int binNum = int(floor(v / mBinSize));

  while (binNum >= mBins.size()) {
    mBins.append(0);
  }

  mBins[binNum] += k;
  //}

  mTotalValue += k * v;
  mTotalSquaredValue += k * (v * v);
  mNumValues += k;

  if (v < mMinValue) {
    mMinValue = v;
  }
  if (v > mMaxValue) {
    mMaxValue = v;
  }
}

/**
    Retrieves the number of bins covering all current samples.
    @returns Number of bins.
*/
int Histogram::getNumberOfBins() const { return mBins.size(); }

/**
    Retrieves number of samples in specified bin.
    @param binNum Id of the bin.
    @returns Number of samples in the bin.
*/
long long int Histogram::getBinValue(const int binNum) const {
  return mBins.at(binNum);
}

/**
    Retrieves lower end of value range for the specified bin.
    @param binNum Id of the bin.
    @returns Lower end of value range.
*/
double Histogram::getBinStart(const int binNum) const {
  return binNum * mBinSize;
}

/**
    Retrieves upper end of value range for the specified bin.
    @param binNum Id of the bin.
    @returns Upper end of value range.
*/
double Histogram::getBinEnd(const int binNum) const {
  return (binNum + 1) * mBinSize;
}

/**
    Retrieves total number of samples.
    @returns Number of samples.
*/
long long int Histogram::getNumberOfValues() const { return mNumValues; }

/**
    Retrieves number of samples with value zero.
    @returns Number of samples.
*/
long long Histogram::getNumberOfZeros() const { return mNumZeros; }

/**
    Determines average value of samples.
    @returns Average value.
*/
double Histogram::getAverage() const {
  if (mNumValues > 0) {
    return mTotalValue / mNumValues;
  } else {
    return 0.0;
  }
}

/**
    Determines variance of samples.
    @returns Variance.
*/
double Histogram::getVariance() const {
  if (mNumValues > 0) {
    const double v = mTotalValue / double(mNumValues);
    const double variance = (mTotalSquaredValue / double(mNumValues)) - (v * v);
    return variance;
  } else {
    return 0.0;
  }
}

/**
    Determines standard deviation of samples.
    @returns Standard deviation.
*/
double Histogram::getStandardDeviation() const { return sqrt(getVariance()); }

/**
    Retrieves minimum sample value.
    @returns Minimum value.
*/
double Histogram::getMinValue() const { return mMinValue; }
/**
    Retrieves highest sample value.
    @returns Maximum value.
*/
double Histogram::getMaxValue() const { return mMaxValue; }

/**
    Creates a json representation of the histogram.
    @returns Histogram as json object.
*/
QJsonObject Histogram::createJson() const {
  QJsonArray histArr = QJsonArray();
  for (int b = 0; b < getNumberOfBins(); ++b) {
    QJsonObject binObj;
    binObj["BinNumber"] = b;
    binObj["BinStart"] = getBinStart(b);
    binObj["BinEnd"] = getBinEnd(b);
    binObj["Value"] = getBinValue(b);
    histArr.append(binObj);
  }

  QJsonObject histObj;
  if (getNumberOfValues() == 0) {
    histObj.insert("numberOfValues", 0);
    histObj.insert("numberOfZeros", 0);
    histObj.insert("minValue", 0);
    histObj.insert("maxValue", 0);
    histObj.insert("histogram", histArr);
  } else {
    histObj.insert("numberOfValues", getNumberOfValues());
    histObj.insert("numberOfZeros", getNumberOfZeros());
    histObj.insert("minValue", getMinValue());
    histObj.insert("maxValue", getMaxValue());
    histObj.insert("histogram", histArr);
  }

  return histObj;
};

void Histogram::write(QTextStream &out, QString label) {
  const QString sep(',');
  out << label << " histogram\n";
  out << "Number of non-zero values:" << sep << getNumberOfValues() << "\n";
  out << "Number of zero values:" << sep << getNumberOfZeros() << "\n";
  out << "\n";
  out << "Bin" << sep << "Bin range min" << sep << "Bin range max" << sep
      << "Value"
      << "\n";
  for (int b = 0; b < getNumberOfBins(); ++b) {
    out << b << sep << getBinStart(b) << sep << getBinEnd(b) << sep
        << getBinValue(b) << "\n";
  }
  out << "\n";
}

void Histogram::writeFile(FileHelper &fileHelper, QString filename) {
  fileHelper.openFile(filename);
  fileHelper.write("bin_index,bin_min,bin_max,bin_value\n");
  fileHelper.write("-1,0,0," + QString::number(mNumZeros) + "\n");
  for (int b = 0; b < getNumberOfBins(); ++b) {
    fileHelper.write(QString::number(b) + "," +
                     QString::number(getBinStart(b)) + "," +
                     QString::number(getBinEnd(b)) + "," +
                     QString::number(getBinValue(b)) + "\n");
  }
  fileHelper.closeFile();
}
