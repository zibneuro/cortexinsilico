#include "Histogram.h"
#include <QJsonArray>

Histogram::Histogram()
    : mBinSize(1.0)
    , mNumValues(0)
    , mNumZeros(0)
    , mTotalValue(0.0)
    , mTotalSquaredValue(0.0)
    , mMinValue( std::numeric_limits<float>::infinity())
    , mMaxValue(-std::numeric_limits<float>::infinity())
{
}

Histogram::Histogram(const double binSize)
    : mBinSize(binSize)
    , mNumValues(0)
    , mNumZeros(0)
    , mTotalValue(0.0)
    , mTotalSquaredValue(0.0)
    , mMinValue( std::numeric_limits<float>::infinity())
    , mMaxValue(-std::numeric_limits<float>::infinity())
{
}


void Histogram::addValue(const double v)
{
    if (v < 0.0) {
        throw std::runtime_error("Histogram: No negative values allowed");
    }

    if (v < 0.00001) {
        ++mNumZeros;
    }
    else {
        const int binNum = int(floor(v / mBinSize));

        while (binNum >= mBins.size()) {
            mBins.append(0);
        }

        mBins[binNum] += 1;
    }

    mTotalValue += v;
    mTotalSquaredValue += (v*v);
    mNumValues += 1;

    if (v < mMinValue) {
        mMinValue = v;
    }
    if (v > mMaxValue) {
        mMaxValue = v;
    }
}


int Histogram::getNumberOfBins() const {
    return mBins.size();
}


long long int Histogram::getBinValue(const int binNum) const {
    return mBins.at(binNum);
}


double Histogram::getBinStart(const int binNum) const {
    return binNum * mBinSize;
}


double Histogram::getBinEnd(const int binNum) const {
    return (binNum+1) * mBinSize;
}


long long int Histogram::getNumberOfValues() const {
    return mNumValues;
}


long long Histogram::getNumberOfZeros() const {
    return mNumZeros;
}


double Histogram::getAverage() const {
    if (mNumValues > 0) {
        return mTotalValue/mNumValues;
    }
    else {
        return 0.0;
    }
}


double Histogram::getVariance() const {
    if (mNumValues > 0) {
        const double v = mTotalValue/double(mNumValues);
        const double variance = (mTotalSquaredValue/double(mNumValues)) - (v*v);
        return variance;
    }
    else {
        return 0.0;
    }
}


double Histogram::getStandardDeviation() const {
    return sqrt(getVariance());
}


double Histogram::getMinValue() const {
    return mMinValue;
}


double Histogram::getMaxValue() const {
    return mMaxValue;
}


QJsonObject Histogram::createJson() const {
    QJsonArray histArr = QJsonArray();
    for (int b=0; b<getNumberOfBins(); ++b) {
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
    }
    else {
        histObj.insert("numberOfValues", getNumberOfValues());
        histObj.insert("numberOfZeros", getNumberOfZeros());
        histObj.insert("minValue", getMinValue());
        histObj.insert("maxValue", getMaxValue());
        histObj.insert("histogram", histArr);
    }

    return histObj;
};
