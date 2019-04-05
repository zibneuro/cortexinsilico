#include "RandomGenerator.h"
#include <ctime>
#include <QDebug>

RandomGenerator::RandomGenerator(int userSeed)
{
    unsigned int seed;
    if (userSeed < 1)
    {
        seed = getRandomSeed();
        mUserSeed = false;
    }
    else
    {
        seed = (unsigned int)userSeed;
        //qDebug() << "Seed" << seed;
        mUserSeed = true;
    }
    mRandomGenerator = std::mt19937(seed);
}

RandomGenerator::RandomGenerator(){
    mUserSeed = false;
}

RandomGenerator::~RandomGenerator()
{
    //qDebug() << "destroy random generator";
}

void
RandomGenerator::shuffleList(QList<int>& list)
{
    std::shuffle(list.begin(), list.end(), mRandomGenerator);
}

int RandomGenerator::getRandomEntry(QList<int> list){
    if(list.size() == 0){
        return -1;
    } else {
        shuffleList(list);
        return list[0];
    }
}

int
RandomGenerator::drawPoisson(float mu)
{
    if (mu > 0)
    {
        std::poisson_distribution<> distribution(mu);
        return distribution(mRandomGenerator);
    }
    else
    {
        return 0;
    }
}

bool
RandomGenerator::hasUserSeed()
{
    return mUserSeed;
}

unsigned int
RandomGenerator::getRandomSeed()
{
    //std::time(nullptr)
    std::random_device rd;
    return rd();
}

void RandomGenerator::testMersenne(int seed){
    std::mt19937 generator(seed);
    qDebug() << generator();
}

unsigned int RandomGenerator::drawNumber(unsigned int max){
    std::uniform_int_distribution<unsigned int> uniformDistribution(0,max);
    unsigned int randomNumber = uniformDistribution(mRandomGenerator);
    return randomNumber;
}
