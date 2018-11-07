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

    ~RandomGenerator();

    /*
        Shuffles the entries of the specified list.
        @param list. A list of integers. 
    */
    void shuffleList(QList<int>& list);

    /*
        Draws a count from a Poisson distribution.
        @param mu. The mean value of the Poisson distribution.
        @return The count.
    */
    int drawPoisson(float mu);

    /*
        Returns whether a fixed random seed was specified by the user.
    */
    bool hasUserSeed();

    /*
        Prints first random value for specified seed.
    */
    static void testMersenne(int seed);

private:

    unsigned int getRandomSeed();

    std::mt19937 mRandomGenerator;
    bool mUserSeed;
};