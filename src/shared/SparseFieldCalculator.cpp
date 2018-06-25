#include "SparseFieldCalculator.h"
#include "CIS3DSparseField.h"
#include "CIS3DVec3.h"
#include "math.h"

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
SparseField SparseFieldCalculator::calculatePetersRule(const SparseField& bouton,
                                                       const SparseField& pst,
                                                       const SparseField& pstAll,
                                                       const float theta1, const float theta2,
                                                       const float theta3, const float theta4) {
    const Vec3i offset1 = bouton.getCoordinates().getOffsetTo(pst.getCoordinates());
    const Vec3i offset2 = bouton.getCoordinates().getOffsetTo(pstAll.getCoordinates());

    SparseField result = SparseField(bouton.getCoordinates());

    int newValueIdx = 0;
    for (SparseField::LocationIndexToValueIndexMap::ConstIterator it1 = bouton.mIndexMap.begin();
         it1 != bouton.mIndexMap.end(); ++it1) {
        const SparseField::LocationIndexT index1 = it1.key();

        const Vec3i loc1 = bouton.getXYZFromIndex(index1);
        const Vec3i loc2 = loc1 + offset1;
        const Vec3i loc3 = loc1 + offset2;

        const SparseField::LocationIndexT index2 = pst.getIndexFromXYZ(loc2);
        const SparseField::LocationIndexT index3 = pst.getIndexFromXYZ(loc3);

        SparseField::LocationIndexToValueIndexMap::ConstIterator it2 = pst.mIndexMap.find(index2);
        SparseField::LocationIndexToValueIndexMap::ConstIterator it3 =
            pstAll.mIndexMap.find(index3);

        if ((it2 != pst.mIndexMap.end()) && (it3 != pstAll.mIndexMap.end())) {
            const float& v1 = bouton.mField[it1.value()];
            const float& v2 = pst.mField[it2.value()];
            const float& v3 = pstAll.mField[it3.value()];

            const float eps = 0.000001;
            float y = 0;

            if ((v1 > eps) && (v2 > eps) && (v3 > eps)) {
                y = exp(theta1 + theta2 * log(v1) + theta3 * log(v2) + theta4 * log(v3));
            }

            result.mIndexMap.insert(index1, newValueIdx);
            result.mField.push_back(y);
            ++newValueIdx;
        }
    }

    return result;
}

/*
    Creates a field in which non zero entries from the specified
    field are represented as ones.

    @param field The non binary field.
    @return The binary field. 
*/
SparseField SparseFieldCalculator::binarize(const SparseField& field) {
    SparseField result = SparseField(field.getCoordinates());
    int newValueIdx = 0;
    for (SparseField::LocationIndexToValueIndexMap::ConstIterator it = field.mIndexMap.begin();
         it != field.mIndexMap.end(); ++it) {
        const SparseField::LocationIndexT index = it.key();
        const float value = field.mField[it.value()]; 
        if(value > 0){
            result.mIndexMap.insert(index, newValueIdx);
            result.mField.push_back(1);
            ++newValueIdx;
        }        
    }
    return result;
}