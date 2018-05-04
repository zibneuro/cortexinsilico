#include "CIS3DStatistics.h"
#include <limits>
#include <cmath>
#include <cstdio>

Statistics::Statistics() :
    mNumberOfSamples(0),
    mMinimum(std::numeric_limits<double>::max()),
    mMaximum(std::numeric_limits<double>::min()),
    mSum(0.0),
    mSumSquared(0.0)
{
}


void Statistics::addSample(const double value) {
    if (value < mMinimum) {
        mMinimum = value;
    }
    if (value > mMaximum) {
        mMaximum = value;
    }
    ++mNumberOfSamples;
    mSum += value;
    mSumSquared += (value*value);
}


double Statistics::getSum() const {
    return mSum;
}


double Statistics::getMean() const {
    if (mNumberOfSamples > 0) {
        return mSum/double(mNumberOfSamples);
    }
    else {
        return 0.0;
    }
}


double Statistics::getMinimum() const {
    return mMinimum;
}


double Statistics::getMaximum() const {
    return mMaximum;
}


double Statistics::getStandardDeviation() const {
    return sqrt(getVariance());
}


double Statistics::getVariance() const {
    if (mNumberOfSamples > 0) {
        const double v = mSum/double(mNumberOfSamples);
        const double variance = (mSumSquared/double(mNumberOfSamples)) - (v*v);
        return variance;
    }
    else {
        return 0.0;
    }
}


unsigned int Statistics::getNumberOfSamples() const {
    return mNumberOfSamples;
}


void Statistics::print() const {
    printf("----Statistics----\n");
    printf("Num samples:\t%u\n", mNumberOfSamples);
    printf("Sum:\t%.3f\n", mSum);
    printf("Mean:\t%f +/- %.3f\n", getMean(), getStandardDeviation());
    printf("Range:\t%.3f - %.3f\n", getMinimum(), getMaximum());
    printf("------------------\n");
}


QJsonObject Statistics::createJson() const {
    QJsonObject obj;
    obj.insert("average", getMean());
    obj.insert("stdev", getStandardDeviation());
    obj.insert("min", getMinimum());
    obj.insert("max", getMaximum());
    return obj;
}
