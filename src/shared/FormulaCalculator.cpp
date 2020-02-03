#include "FormulaCalculator.h"
#include "RandomGenerator.h"
#include <QDebug>
#include <iostream>

FormulaCalculator::FormulaCalculator()
{
}

FormulaCalculator::FormulaCalculator(QJsonObject formulas)
{
    mUseCustomSynapseDistributionFormula = formulas["synapseDistributionMode"].toString() == "custom";
    mDefaultSynapseDistributionFormula = "(DSO^k)/fact(k)*exp(-DSO)";
    mSynapseDistributionFormula = formulas["synapseDistributionFormula"].toString().toStdString();
    mUseCustomConnectionProbabilityFormula = formulas["connectionProbabilityMode"].toString() == "formula";
    mConnectionProbabilityFormula = formulas["connectionProbabilityFormula"].toString().toStdString();
    mValid = false;
}

template <typename T>
inline T
factorial(T arg)
{
    float f = 1;
    for (int i = 1; i <= arg; i++)
    {
        f *= (float)i;
    }
    return f;
}

template <typename T>
inline T
nCk(T n, T k)
{
    return factorial(n) / (factorial(k) * factorial(n-k));
}

template <typename T>
inline T
getRandom(float a, float b)
{    
    return (float)SingletonRandom::getInstance()->getNumberUniformDistribution(a, b);
}

template <typename T>
inline T
getRandomGauss(float mu, float sigma)
{    
    return (float)SingletonRandom::getInstance()->getNumberNormalDistribution(mu, sigma);
}

template <typename T>
inline T
getRandomBernoulli(float p)
{    
    return (float)SingletonRandom::getInstance()->getNumberBernoulliDistribution(p);
}

bool
FormulaCalculator::init()
{
    bool valid = true;
    mSymbolTable.add_variable("dso", mCurrentValue_i);
    mSymbolTable.add_variable("DSO", mCurrentValue_i);
    mSymbolTable.add_variable("DSC", mCurrentValue_i);
    mSymbolTable.add_variable("dsc", mCurrentValue_i);
    mSymbolTable.add_variable("k", mCurrentValue_k);
    mSymbolTable.add_function("fact", factorial);
    mSymbolTable.add_function("nCk", nCk);
    mSymbolTable.add_function("rand", getRandom);
    mSymbolTable.add_function("gauss", getRandomGauss);
    mSymbolTable.add_function("bernoulli", getRandomBernoulli);
    mSymbolTable.add_constants();
    mSynapseExpression.register_symbol_table(mSymbolTable);
    mConnectionProbabilityExpression.register_symbol_table(mSymbolTable);
    parser_t parser;
    if (mUseCustomSynapseDistributionFormula)
    {
        valid &= parser.compile(mSynapseDistributionFormula, mSynapseExpression);
    }
    else 
    {
        valid &= parser.compile(mDefaultSynapseDistributionFormula, mSynapseExpression);
    }
    if (mUseCustomConnectionProbabilityFormula)
    {
        valid &= parser.compile(mConnectionProbabilityFormula, mConnectionProbabilityExpression);
    }
    mValid = valid;
    std::cout << "[!] Init formula" << mSynapseDistributionFormula << mConnectionProbabilityFormula << valid;
    return mValid;
}

float
FormulaCalculator::calculateSynapseProbability(float innervation, int k)
{
    mCurrentValue_i = innervation;
    mCurrentValue_k = (float)k;
    if (mValid)
    {
        float synapseProb = mSynapseExpression.value();
        return clampProbability(synapseProb);
    }
    else
    {
        return 0;
    }
}

float
FormulaCalculator::calculateConnectionProbability(float innervation)
{
    if (mValid)
    {
        float prob;
        if (mUseCustomConnectionProbabilityFormula)
        {
            mCurrentValue_i = innervation;
            prob = mConnectionProbabilityExpression.value();
        }
        else
        {
            mCurrentValue_i = innervation;
            mCurrentValue_k = 0;
            prob = 1 - mSynapseExpression.value();
        }
        return clampProbability(prob);
    }
    else
    {
        return 0;
    }
}

    float FormulaCalculator::clampProbability(float probability){
        return std::min((float)1, std::max(float(0), probability));
    }
