#ifndef STATISTICS_H
#define STATISTICS_H

#include <QFlags>
#include <deque>
#include <QTextStream>
#include "FileHelper.h"

/**
    Collects a set of samples and computes basic statistics.
*/
class Statistics {
   public:
    /**
        Constructor.
    */
    Statistics();

    /**
        Adds a sample to the set.
        @param value The sample to add.
    */
    void addSample(const double value);

    /**
        Calculates sum of all samples.
        @return The sum.
    */
    double getSum() const;

    /**
        Calculates mean value of all samples.
        @return The mean.
    */
    double getMean() const;

    /**
        Determines minimum value of all samples.
        @return The minimum value.
    */
    double getMinimum() const;

    /**
        Determines maximum value of all samples.
        @return The maximum value.
    */
    double getMaximum() const;

    /**
        Calculates standard deviation of the samples.
        @return The standard deviation.
    */
    double getStandardDeviation() const;

    /**
        Calculates variance of the samples.
        @return The variance.
    */
    double getVariance() const;

    /**
        Returns the number of samples.
        @return Number of samples.
    */
    unsigned int getNumberOfSamples() const;

    /**
        Determines whether the statistic has converged based
        on the maximum deviation in the last 50 mean values.
        @param maxVariance The maximum deviation in the last 50 mean values.
        @return True, if the statistic has converged.
    */
    bool hasConverged(double maxDeviation);

    /**
        Writes the statistics to console.
    */
    void print() const;

    QString getLineCsv(QString description);

    static QString getHeaderCsv();

    static QString getLineSingleValue(QString description, double value);

    static QString getLineSingleValue(QString description, QString value);

    enum Field {
        Sum = 1 << 0,
        Mean = 1 << 1,
        Minimum = 1 << 2,
        Maximum = 1 << 3,
        StandardDeviation = 1 << 4,
        Variance = 1 << 5
    };



    void write(QTextStream& out, QString label);

    Q_DECLARE_FLAGS(Fields, Field)

   private:
    unsigned int mNumberOfSamples;
    double mMinimum;
    double mMaximum;
    double mSum;
    double mSumSquared;
    std::deque<double> mLastMeans;
    unsigned int mWindowSize;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Statistics::Fields)

#endif  // STATISTICS_H
