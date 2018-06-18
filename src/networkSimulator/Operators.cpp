#include "Operators.h"

/**
    Constructor.
    @param theta The connectivity rule parameter.
*/
LogThetaOperator::LogThetaOperator(const float theta) : mTheta(theta) {}

/**
    Takes the logarithm of the field value and multilpies the result
    by theta. If the existing field value is smaller than mEPS, mNEGMAX is
    returned.
    @param The existing field value.
    @return The new field value.
*/
float LogThetaOperator::doCalculate(const float value) const {
    if (value >= mEps) {
        return mTheta * log(value);
    } else {
        return mNEGMAX;
    }
}

/**
    Constructor.
    @param theta The connectivity rule parameter.
*/
AddThetaOperator::AddThetaOperator(const float theta) : mTheta(theta) {}

/**
    Adds theta, if the existing field value is not smaller than mEPS.
    @param The existing field value.
    @return The new field value.
*/
float AddThetaOperator::doCalculate(const float value) const {
    if (value >= mEps) {
        return value + mTheta;
    } else {
        return value;
    }
}

/**
    Constructor.
*/
ExponentiationOperator::ExponentiationOperator() {}

/**
    Exponentiates the field value. If the existing field value is smaller
    than mMinArgument, 0 is returned.
    @param The existing field value.
    @return The new field value.
*/
float ExponentiationOperator::doCalculate(const float value) const {
    if (value >= mMinArgument) {
        return exp(value);
    } else {
        return 0;
    }
}
