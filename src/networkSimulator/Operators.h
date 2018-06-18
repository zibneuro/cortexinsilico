#include "CIS3DSparseField.h"

#ifndef OPERATORS_H
#define OPERATORS_H

/**
    Takes the logarithm of a field value and multiplies the result
    with theta.
*/
class LogThetaOperator : public SparseFieldOperator {
   public:
    /**
        Constructor.
        @param theta The connectivity rule parameter.
    */
    LogThetaOperator(float theta);

   protected:
    /**
        Takes the logarithm of the field value and multilpies the result
        by theta. If the existing field value is smaller than mEPS, mNEGMAX is
        returned.
        @param The existing field value.
        @return The new field value.
    */
    float doCalculate(const float value) const override;

   private:
    float mTheta;
    const float mEps = 0.000001;
    const float mNEGMAX = -10;
};

/**
    Adds theta to the field value, if the field value is not 0.
*/
class AddThetaOperator : public SparseFieldOperator {
   public:
    /**
        Constructor.
        @param theta The connectivity rule parameter.
    */
    AddThetaOperator(float theta);

   protected:
    /**
        Adds theta, if the existing field value is not smaller than mEPS.
        @param The existing field value.
        @return The new field value.
    */
    float doCalculate(const float value) const override;

   private:
    float mTheta;
    const float mEps = 0.000001;
};

/**
    Computes the exponent of the field value.
*/
class ExponentiationOperator : public SparseFieldOperator {
   public:
    /**
        Constructor.
    */
    ExponentiationOperator();

   protected:
    /**
        Exponentiates the field value. If the existing field value is smaller
        than mMinArgument, 0 is returned.
        @param The existing field value.
        @return The new field value.
    */
    float doCalculate(const float value) const override;

   private:
    float mTheta;
    const float mMinArgument = -10;
};

#endif  // OPERATORS_H
