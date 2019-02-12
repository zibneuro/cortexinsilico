#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <QList>
#include <QJsonObject>

/**
    Collects sample values and represents them as a histogram.
*/
class Histogram
{
public:
    /**
        Constructor.
        Default bin size set to 1.0.
    */
    Histogram();

    /** Constructor.
        @param binSize Width of bins.
    */
    Histogram(const double binSize);

    /**
        Adds a sample value to the histogram.
        @param v The sample value.
    */
    void addValue(const double v);

    /**
        Adds the same sample value to the histogram k times.
        @param v The sample value.
        @param k The multiplicity.
    */
    void addValues(const double v, const int k);

    /**
        Retrieves the number of bins covering all current samples.
        @returns Number of bins.
    */
    int getNumberOfBins() const;

    /**
        Retrieves number of samples in specified bin.
        @param binNum Id of the bin.
        @returns Number of samples in the bin.
    */
    long long int getBinValue(const int binNum) const;

    /**
        Retrieves lower end of value range for the specified bin.
        @param binNum Id of the bin.
        @returns Lower end of value range.
    */
    double getBinStart(const int binNum) const;

    /**
        Retrieves upper end of value range for the specified bin.
        @param binNum Id of the bin.
        @returns Upper end of value range.
    */
    double getBinEnd(const int binNum) const;

    /**
        Retrieves total number of samples.
        @returns Number of samples.
    */
    long long getNumberOfValues() const;

    /**
        Retrieves number of samples with value zero.
        @returns Number of samples.
    */
    long long getNumberOfZeros() const;

    /**
        Determines average value of samples.
        @returns Average value.
    */
    double getAverage() const;

    /**
        Determines variance of samples.
        @returns Variance.
    */
    double getVariance() const;

    /**
        Determines standard deviation of samples.
        @returns Standard deviation.
    */
    double getStandardDeviation() const;

    /**
        Retrieves minimum sample value.
        @returns Minimum value.
    */
    double getMinValue() const;

    /**
        Retrieves highest sample value.
        @returns Maximum value.
    */
    double getMaxValue() const;

    /**
        Creates a json representation of the histogram.
        @returns Histogram as json object.
    */
    QJsonObject createJson() const;

private:
    float      mBinSize;
    QList<long long int> mBins;

    long long  mNumValues;
    long long  mNumZeros;
    double     mTotalValue;
    double     mTotalSquaredValue;
    double     mMean;
    double     mMinValue;
    double     mMaxValue;
};

#endif // HISTOGRAM_H
