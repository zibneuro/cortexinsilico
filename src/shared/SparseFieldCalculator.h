#ifndef SPARSEFIELDCALCULATOR_H
#define SPARSEFIELDCALCULATOR_H

class SparseField;

class SparseFieldCalculator{
public:
    /**
        Computes the innervation field according to the generalized dense
        Peters' rule.
        @param bouton The bouton density of one neuron.
        @param pst The post synaptic target density of one neuron.
        @param pstAll The post synaptic target density of all neurons.
        @param theta1 First rule parameter.
        @param theta2 Second rule parameter.
        @param theta3 Third rule parameter.
        @param theta4 Fourth rule parameter.
        @return The innervation density between both neurons.
        @throws runtime_error if the spacing differs or if the shift between
            the fields is not an integer multiple of the voxel spacing.
    */
    SparseField calculatePetersRule(const SparseField& bouton,
                                const SparseField& pst,
                                const SparseField& pstAll,
                                const float theta1,
                                const float theta2,
                                const float theta3,
                                const float theta4);
};

#endif //SPARSEFIELDCALCULATOR_H
