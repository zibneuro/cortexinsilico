#include "FormulaCalculator.h"

FormulaCalculator::FormulaCalculator(QJsonObject formulas)
{
    mSynapseDistributionFormula = formulas["synapseDistributionFormula"].toString().toStdString();
    mUseCustomConnectionProbabilityFormula = formulas["connectionProbabilityMode"].toString() == "formula";
    mConnectionProbabilityFormula = formulas["connectionProbabilityFormula"].toString().toStdString();
    mValid = false;
}

bool FormulaCalculator::init(){
    bool valid = true;        
    mSymbolTable.add_variable("i", mCurrentValue_i);
    mSymbolTable.add_variable("k", mCurrentValue_k);
    mSymbolTable.add_constants();
    mSynapseExpression.register_symbol_table(mSymbolTable);
    mConnectionProbabilityExpression.register_symbol_table(mSymbolTable);
    parser_t parser;
    valid &= parser.compile(mSynapseDistributionFormula, mSynapseExpression);
    if(mUseCustomConnectionProbabilityFormula){
        valid &= parser.compile(mConnectionProbabilityFormula, mConnectionProbabilityExpression);
    }
    return valid;
}


