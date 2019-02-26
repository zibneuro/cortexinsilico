#ifndef FORMULACALCULATOR_H
#define FORMULACALCULATOR_H

#include <QJsonObject>
#include "FormulaParser.h"

typedef exprtk::symbol_table<float> symbol_table_t;
typedef exprtk::expression<float> expression_t;
typedef exprtk::parser<float> parser_t;

class FormulaCalculator
{
public:    
    FormulaCalculator(QJsonObject formulas);
    bool init();
    float calculateSynapseProbability(float innervation, int k);
    float calculateConnectionProbability(float innervation);

private:
    std::string mSynapseDistributionFormula;
    bool mUseCustomConnectionProbabilityFormula;
    std::string mConnectionProbabilityFormula;

    symbol_table_t mSymbolTable;
    expression_t mSynapseExpression;
    expression_t mConnectionProbabilityExpression;
    float mCurrentValue_i;
    float mCurrentValue_k;
    bool mValid;
};

#endif // FORMULACALCULATOR_H