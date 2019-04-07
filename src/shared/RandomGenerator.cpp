#include "RandomGenerator.h"
#include <ctime>
#include <QDebug>
#include <math.h>
#include <stdexcept>

RandomGenerator::RandomGenerator(int userSeed)
{
    if (userSeed < 1)
    {
        mUserSeed = false;
        mSeed = createRandomSeed();
    }
    else
    {
        mUserSeed = true;
        mSeed = static_cast<unsigned int>(userSeed);
    }
    mRandomGenerator = std::mt19937(mSeed);
}

RandomGenerator::RandomGenerator(){
    mUserSeed = false;
}

RandomGenerator::~RandomGenerator()
{
    //qDebug() << "destroy random generator";
}

unsigned int RandomGenerator::getSeed(){
    return mSeed;
}

void
RandomGenerator::shuffleList(QList<int>& list)
{
    std::shuffle(list.begin(), list.end(), mRandomGenerator);
}

int
RandomGenerator::getRandomEntry(QList<int> list){
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
        std::poisson_distribution<> distribution(static_cast<double>(mu));
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

/*
 * Adapted from numpy implementation:
 * https://github.com/numpy/numpy/tree/master/numpy/random/mtrand
 */
long RandomGenerator::drawPoissonDouble(double lambda){
    if (lambda >= 10)
    {
        return drawPoissonHoermann(lambda);
    }
    else if (lambda == 0)
    {
        return 0;
    }
    else
    {
        return drawPoissonKnuth(lambda);
    }
}

/*
 * Adapted from numpy implementation:
 * https://github.com/numpy/numpy/tree/master/numpy/random/mtrand
 */
long RandomGenerator::drawPoissonKnuth(double lambda){
    long X;
    double prod, U, enlam;

    enlam = exp(-lambda);
    X = 0;
    prod = 1.0;
    while (1)
    {
        U = drawNumberZeroOne();
        prod *= U;
        if (prod > enlam)
        {
            X += 1;
        }
        else
        {
            return X;
        }
    }
}

/*
 * Adapted from numpy implementation:
 * https://github.com/numpy/numpy/tree/master/numpy/random/mtrand
 *
 * log-gamma function to support some of these distributions. The
 * algorithm comes from SPECFUN by Shanjie Zhang and Jianming Jin and their
 * book "Computation of Special Functions", 1996, John Wiley & Sons, Inc.
 */
#define M_PI 3.14159265358979323846264338328
static double loggam(double x)
{
    double x0, x2, xp, gl, gl0;
    long k, n;

    static double a[10] = {8.333333333333333e-02,-2.777777777777778e-03,
                           7.936507936507937e-04,-5.952380952380952e-04,
                           8.417508417508418e-04,-1.917526917526918e-03,
                           6.410256410256410e-03,-2.955065359477124e-02,
                           1.796443723688307e-01,-1.39243221690590e+00};
    x0 = x;
    n = 0;
    if ((x == 1.0) || (x == 2.0))
    {
        return 0.0;
    }
    else if (x <= 7.0)
    {
        n = (long)(7 - x);
        x0 = x + n;
    }
    x2 = 1.0/(x0*x0);
    xp = 2*M_PI;
    gl0 = a[9];
    for (k=8; k>=0; k--)
    {
        gl0 *= x2;
        gl0 += a[k];
    }
    gl = gl0/x0 + 0.5*log(xp) + (x0-0.5)*log(x0) - x0;
    if (x <= 7.0)
    {
        for (k=1; k<=n; k++)
        {
            gl -= log(x0-1.0);
            x0 -= 1.0;
        }
    }
    return gl;
}

/*
 * Adapted from numpy implementation:
 * https://github.com/numpy/numpy/tree/master/numpy/random/mtrand
 *
 * The transformed rejection method for generating Poisson random variables
 * W. Hoermann
 * Insurance: Mathematics and Economics 12, 39-45 (1993)
 */
#define LS2PI 0.91893853320467267
#define TWELFTH 0.083333333333333333333333
long RandomGenerator::drawPoissonHoermann(double lambda){
    long k;
    double U, V, slam, loglam, a, b, invalpha, vr, us;

    slam = sqrt(lambda);
    loglam = log(lambda);
    b = 0.931 + 2.53*slam;
    a = -0.059 + 0.02483*b;
    invalpha = 1.1239 + 1.1328/(b-3.4);
    vr = 0.9277 - 3.6224/(b-2);

    while (1)
    {
        U = drawNumberZeroOne() - 0.5;
        V = drawNumberZeroOne();
        us = 0.5 - fabs(U);
        k = (long)floor((2*a/us + b)*U + lambda + 0.43);
        if ((us >= 0.07) && (V <= vr))
        {
            return k;
        }
        if ((k < 0) ||
                ((us < 0.013) && (V > us)))
        {
            continue;
        }
        if ((log(V) + log(invalpha) - log(a/(us*us)+b)) <=
                (-lambda + k*loglam - loggam(k+1)))
        {
            return k;
        }


    }
}

unsigned int
RandomGenerator::createRandomSeed()
{
    //std::time(nullptr)
    std::random_device rd;
    return rd();
}

void RandomGenerator::testMersenne(){
    qDebug() << "seed:" << mSeed;
    qDebug() << "mt19937 min value:" << mRandomGenerator.min();
    qDebug() << "mt19937 max value:" << mRandomGenerator.max();

    QList<int> l1;
    for(unsigned i = 0; i<10; i++){
        l1.append(drawNumberRange(100));
    }
    qDebug() << "random numbers [0,100]:" << l1;


    QList<int> l2;
    for(int i=1; i<=10; i++){
        l2.append(i);
    }
    permute(l2);
    qDebug() << "Permutation [1,10]:" << l2;

    QList<long> l3;
    for(int i=1; i<=10; i++){
        l3.append(drawPoissonDouble(7));
    }
    qDebug() << "Poisson draws (lambda=7):" << l3;


    QList<long> l4;
    for(int i=1; i<=10; i++){
        l4.append(drawPoissonDouble(77));
    }
    qDebug() << "Poisson draws (lambda=77):" << l4;

}

unsigned int RandomGenerator::drawNumber(unsigned int max){
    std::uniform_int_distribution<unsigned int> uniformDistribution(0,max);
    unsigned int randomNumber = uniformDistribution(mRandomGenerator);
    return randomNumber;
}

unsigned int RandomGenerator::drawNumberRange(unsigned int max){
    if(max == 0){
        return 0;
    }
    double maxDouble = static_cast<double>(max + 1);
    unsigned int nBits = static_cast<unsigned int>(std::ceil(std::log2(maxDouble)));
    if(nBits > 32){
        throw std::overflow_error("random number range exceeds 32 bits");
    }
    unsigned int nShiftBits = 32 - nBits;
    unsigned int randomNumber = mRandomGenerator.max();
    while(randomNumber > max){
        randomNumber = mRandomGenerator() >> nShiftBits;
    }
    return randomNumber;
}

double RandomGenerator::drawNumberZeroOne(){
    double random = static_cast<double>(mRandomGenerator());
    double max = static_cast<double>(mRandomGenerator.max());
    return random / max;
}

void RandomGenerator::permute(QList<int>& list){
    // Fisher-Yates shuffle
    for(int i = list.size() - 1; i > 0; i--){
        int j = static_cast<int>(drawNumberRange(static_cast<unsigned int>(i)));
        list.swap(i, j);
    }
}
