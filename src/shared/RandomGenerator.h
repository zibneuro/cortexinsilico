#pragma once

#include <random>
#include <QList>

/*
    Utility class for random values and functions 
    based on the mt19937 pseudo random generator.
*/
class RandomGenerator
{
public:
    /*
        Constructor.

        If the specified seed is smaller than 1, a seed from
        a random source is used.

        @param userSeed. The random seed.
    */
    RandomGenerator(int userSeed);

    /*
        Constructor.

        A seed from a random source is used.
    */
    RandomGenerator();

    ~RandomGenerator();

    unsigned int getSeed();

    /*
        Shuffles the entries of the specified list.
        @param list. A list of integers. 
    */
    void shuffleList(QList<int>& list);

    QList<int> getSample(const QList<int> list, int nSamples);

    int getRandomEntry(QList<int> list);

    /*
        Draws a count from a Poisson distribution.
        @param mu. The mean value of the Poisson distribution.
        @return The count.
    */
    int drawPoisson(float mu);

    long drawPoissonDouble(double lamda);

    /*
        Returns whether a fixed random seed was specified by the user.
    */
    bool hasUserSeed();

    /*
        Prints random seed, mt19937 max number, and
        sequence of first 5 random numbers. Checks, if
        list [1,20] is permuted in expected way.
    */
    void testMersenne();

    /*
     * Returns a random number between 0 and the specified value.
     */
    unsigned int drawNumber(unsigned int max);

private:

    long drawPoissonKnuth(double lambda);

    long drawPoissonHoermann(double lambda);

    unsigned int createRandomSeed();

    unsigned int drawNumberRange(unsigned int max);

    double drawNumberZeroOne();

    void permute(QList<int>& list);

    std::mt19937 mRandomGenerator;
    bool mUserSeed;
    unsigned int mSeed;
};
