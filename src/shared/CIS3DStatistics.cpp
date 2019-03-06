#include "CIS3DStatistics.h"
#include <QDebug>
#include <cmath>
#include <cstdio>
#include <limits>

/**
    Constructor.
*/
Statistics::Statistics()
    : mNumberOfSamples(0)
    , mMinimum(std::numeric_limits<double>::max())
    , mMaximum(std::numeric_limits<double>::min())
    , mSum(0.0)
    , mSumSquared(0.0)
    , mWindowSize(100)
{
}

/**
    Adds a sample to the set.
    @param value The sample to add.
*/
void
Statistics::addSample(const double value)
{
    if (value < mMinimum)
    {
        mMinimum = value;
    }
    if (value > mMaximum)
    {
        mMaximum = value;
    }
    ++mNumberOfSamples;
    mSum += value;
    mSumSquared += (value * value);

    if (mLastMeans.size() >= mWindowSize)
    {
        mLastMeans.pop_front();
    }
    mLastMeans.push_back(getMean());
}

/**
    Calculates sum of all samples.
    @return The sum.
*/
double
Statistics::getSum() const
{
    return mSum;
}

/**
    Calculates mean value of all samples.
    @return The mean.
*/
double
Statistics::getMean() const
{
    if (mNumberOfSamples > 0)
    {
        return mSum / double(mNumberOfSamples);
    }
    else
    {
        return 0.0;
    }
}

/**
    Determines minimum value of all samples.
    @return The minimum value.
*/
double
Statistics::getMinimum() const
{
    return mMinimum;
}

/**
    Determines maximum value of all samples.
    @return The maximum value.
*/
double
Statistics::getMaximum() const
{
    return mMaximum;
}

/**
    Calculates standard deviation of the samples.
    @return The standard deviation.
*/
double
Statistics::getStandardDeviation() const
{
    if (mMinimum == mMaximum)
    {
        return 0;
    }
    else
    {
        return sqrt(getVariance());
    }
}

/**
    Calculates variance of the samples.
    @return The variance.
*/
double
Statistics::getVariance() const
{
    if (mNumberOfSamples > 0)
    {
        const double v = mSum / double(mNumberOfSamples);
        const double variance = (mSumSquared / double(mNumberOfSamples)) - (v * v);
        if (variance != variance)
        {
            return 0;
        }
        return variance;
    }
    else
    {
        return 0.0;
    }
}

/**
    Returns the number of samples.
    @return Number of samples.
*/
unsigned int
Statistics::getNumberOfSamples() const
{
    return mNumberOfSamples;
}

/**
    Determines whether the statistic has converged based
    on the maximum deviation in the last 50 mean values.
    @param maxVariance The maximum deviation in the last 50 mean values.
    @return True, if the statistic has converged.
*/
bool
Statistics::hasConverged(double maxDeviation)
{
    if (mLastMeans.size() < mWindowSize)
    {
        return false;
    }
    else
    {
        Statistics stat;
        for (unsigned int i = 0; i < mWindowSize; i++)
        {
            stat.addSample(mLastMeans[i]);
        }
        double delta = stat.getMaximum() - stat.getMinimum();
        if (delta <= maxDeviation)
        {
            qDebug() << "Mean" << getNumberOfSamples() << stat.getMean() << stat.getVariance()
                     << delta;
            return true;
        }
        return false;
    }
}

/**
    Writes the statistics to console.
*/
void
Statistics::print() const
{
    printf("----Statistics----\n");
    printf("Num samples:\t%u\n", mNumberOfSamples);
    printf("Sum:\t%.3f\n", mSum);
    printf("Mean:\t%f +/- %.3f\n", getMean(), getStandardDeviation());
    printf("Range:\t%.3f - %.3f\n", getMinimum(), getMaximum());
    printf("------------------\n");
}

void
Statistics::write(QTextStream& out, QString label)
{
    const QChar sep(',');
    out << label << sep
        << "Average" << sep << getMean() << sep
        << "StDev" << sep << getStandardDeviation() << sep
        << "Min" << sep << getMinimum() << sep
        << "Max" << sep << getMaximum() << "\n";
}