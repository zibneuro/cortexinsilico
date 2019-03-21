#include "FormulaCalculator.h"
#include <QDebug>
#include <iostream>

FormulaCalculator::FormulaCalculator()
{
}

FormulaCalculator::FormulaCalculator(QJsonObject formulas)
{
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

bool
FormulaCalculator::init()
{
    bool valid = true;
    mSymbolTable.add_variable("i", mCurrentValue_i);
    mSymbolTable.add_variable("k", mCurrentValue_k);
    mSymbolTable.add_function("fact", factorial);
    mSymbolTable.add_constants();
    mSynapseExpression.register_symbol_table(mSymbolTable);
    mConnectionProbabilityExpression.register_symbol_table(mSymbolTable);
    parser_t parser;
    valid &= parser.compile(mSynapseDistributionFormula, mSynapseExpression);
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
        return synapseProb;
    }
    else
    {
        int nfak = 1;
        float innervationPow = 1;
        for (int i = 1; i <= k; i++)
        {
            innervationPow *= innervation;
            nfak *= i;
        }
        float synapseProb = innervationPow * exp(-innervation) / nfak;
        return synapseProb;
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

        if (prob < 0)
        {
            return 0;
        }
        else if (prob > 1)
        {
            return 1;
        }
        else
        {
            return prob;
        }
    }
    else
    {
        return 1 - exp(-innervation);
    }
}
