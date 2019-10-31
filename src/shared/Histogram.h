#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <QList>
#include <QJsonObject>
#include <QTextStream>
#include <QString>
#include "FileHelper.h"
#include <map>

/**
    Collects sample values and represents them as a histogram.
*/
class Histogram
{
  public:

    static const int REF_BINCOUNT = 10000;
    static double getBinSize(double expectedMaxValue);

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
        Creates a json representation of the histogram.
        @returns Histogram as json object.
    */
    QJsonObject createJson() const;

    void writeFile(FileHelper &fileHelper, QString filename) const;

  private:
    double mBinSize;
    std::map<long long, long long> mBins;    
    long long mNumZeros;
    
};

#endif // HISTOGRAM_H
