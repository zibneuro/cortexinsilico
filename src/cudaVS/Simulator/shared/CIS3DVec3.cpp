#include "CIS3DVec3.h"
#include <math.h>
#include <QString>
#include <QDebug>

/**
    Constructor.
    Initializes vector to (0,0,0).
*/
Vec3i::Vec3i() {
    mValue[0] = 0;
    mValue[1] = 0;
    mValue[2] = 0;
}

/**
    Constructor.
    @param x The x component.
    @param y The y component.
    @param z The z component.
*/
Vec3i::Vec3i(int x, int y, int z) {
    mValue[0] = x;
    mValue[1] = y;
    mValue[2] = z;
}

int Vec3i::getX() const {
    return mValue[0];
}


int Vec3i::getY() const {
    return mValue[1];
}


int Vec3i::getZ() const {
    return mValue[2];
}


void Vec3i::setX(int v) {
    mValue[0] = v;
}


void Vec3i::setY(int v) {
    mValue[1] = v;
}


void Vec3i::setZ(int v) {
    mValue[2] = v;
}

/**
    Checks equality.
    @param other The other vector.
    @return True if both vectors are indetical in all components.
*/
bool Vec3i::operator==(const Vec3i& other) const {
    return mValue[0] == other.mValue[0] &&
           mValue[1] == other.mValue[1] &&
           mValue[2] == other.mValue[2];
}

/**
    Checks equality.
    @param other The other vector.
    @return True if both vectors differ in at least one component.
*/
bool Vec3i::operator!=(const Vec3i& other) const {
    return !(*this == other);
}

/**
    Retrieves value in the specified component.
    @param index The component [0-2]
    @return The value.
*/
int& Vec3i::operator[](const int index) {
    return mValue[index];
}

/**
    Retrieves value in the specified component.
    @param index The component [0-2]
    @return The value.
*/
int Vec3i::operator[](const int index) const {
    return mValue[index];
}

/**
    Determines whether this vector is smaller than another vector.
    The comparison is done component-wise, with prevalence x->y->z.
    @return True if this vector is smaller.
*/
bool Vec3i::operator<(const Vec3i &other) const
{
    if (getX() == other.getX()) {
        if (getY() == other.getY()) {
            return getZ() < other.getZ();
        }
        else {
            return getY() < other.getY();
        }
    }
    else {
        return getX() < other.getX();
    }
}

/**
    Prints the specified message followed by the values of x,y,z to qDebug().
    @param msg The message.
*/
void Vec3i::print(const QString& msg) const
{
    qDebug() << msg << getX() << getY() << getZ();
}

/**
    Adds the specified vectors creating a new vector.
    @param va The first addend.
    @param vb The second addend.
    @return va + vb
*/
Vec3i operator+(const Vec3i &va, const Vec3i &vb) {
    return Vec3i(va.getX()+vb.getX(), va.getY()+vb.getY(), va.getZ()+vb.getZ());
}

/**
    Subtracts the specified vectors creating a new vector.
    @param va The minuend.
    @param vb The subtrahend.
    @return va - vb
*/
Vec3i operator-(const Vec3i &va, const Vec3i &vb) {
    return Vec3i(va.getX()-vb.getX(), va.getY()-vb.getY(), va.getZ()-vb.getZ());
}


//---------------------------------------------------------------

/**
    Constructor.
    Initializes vector to (0.0,0.0,0.0).
*/
Vec3f::Vec3f() {
    mValue[0] = 0.0f;
    mValue[1] = 0.0f;
    mValue[2] = 0.0f;
}

/**
    Constructor.
    @param x The x component.
    @param y The y component.
    @param z The z component.
*/
Vec3f::Vec3f(float val) {
    mValue[0] = val;
    mValue[1] = val;
    mValue[2] = val;
}


Vec3f::Vec3f(float x, float y, float z) {
    mValue[0] = x;
    mValue[1] = y;
    mValue[2] = z;
}


float Vec3f::getX() const {
    return mValue[0];
}


float Vec3f::getY() const {
    return mValue[1];
}


float Vec3f::getZ() const {
    return mValue[2];
}

/**
    Retrieves value in the specified component.
    @param index The component [0-2]
    @return The value.
*/
float &Vec3f::operator[](const int index) {
    return mValue[index];
}

/**
    Retrieves value in the specified component.
    @param index The component [0-2]
    @return The value.
*/
float Vec3f::operator[](const int index) const {
    return mValue[index];
}

/**
    Subtracts the specified vectors creating a new vector.
    @param va The minuend.
    @param vb The subtrahend.
    @return va - vb
*/
Vec3f operator-(const Vec3f &va, const Vec3f &vb) {
    return Vec3f(va.getX()-vb.getX(), va.getY()-vb.getY(), va.getZ()-vb.getZ());
}

/**
    Adds the specified vectors creating a new vector.
    @param va The first addend.
    @param vb The second addend.
    @return va + vb
*/
Vec3f operator+(const Vec3f &va, const Vec3f &vb) {
    return Vec3f(va.getX()+vb.getX(), va.getY()+vb.getY(), va.getZ()+vb.getZ());
}

/**
    Scales the specified vector component-wise creating a new vector.
    @param va The vector to scale.
    @param s The scaling factor.
    @return s * va
*/
Vec3f operator*(const Vec3f &va, const float &s) {
    return s*va;
}

/**
    Scales the specified vector component-wise creating a new vector.
    @param s The scaling factor.
    @param va The vector to scale.    
    @return s * va
*/
Vec3f operator*(const float& s, const Vec3f &va) {
    return Vec3f(va.getX()*s, va.getY()*s, va.getZ()*s);
}


void Vec3f::setX(float v) {
    mValue[0] = v;
}


void Vec3f::setY(float v) {
    mValue[1] = v;
}


void Vec3f::setZ(float v) {
    mValue[2] = v;
}

/**
    Determines the length of the vector.
    @return The Euclidean norm.
*/
float Vec3f::length() const {
    return sqrt(mValue[0]*mValue[0] + mValue[1]*mValue[1] + mValue[2]*mValue[2]);
}

/**
    Determines whether this vector equals another vector. Calculates the
    delta vector and checks whetehr its length is below the numerical tolerance.
    @param other The other vector.
    @param eps The numerical tolerance.
    @return True if both are equal.
*/
bool Vec3f::equals(const Vec3f &other, const float eps) const {
    return (*this-other).length() < eps;
}
