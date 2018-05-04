#include "CIS3DVec3.h"
#include <math.h>
#include <QString>
#include <QDebug>


Vec3i::Vec3i() {
    mValue[0] = 0;
    mValue[1] = 0;
    mValue[2] = 0;
}


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

bool Vec3i::operator==(const Vec3i& other) const {
    return mValue[0] == other.mValue[0] &&
           mValue[1] == other.mValue[1] &&
           mValue[2] == other.mValue[2];
}


bool Vec3i::operator!=(const Vec3i& other) const {
    return !(*this == other);
}


int& Vec3i::operator[](const int index) {
    return mValue[index];
}


int Vec3i::operator[](const int index) const {
    return mValue[index];
}


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


void Vec3i::print(const QString& msg) const
{
    qDebug() << msg << getX() << getY() << getZ();
}


Vec3i operator+(const Vec3i &va, const Vec3i &vb) {
    return Vec3i(va.getX()+vb.getX(), va.getY()+vb.getY(), va.getZ()+vb.getZ());
}

Vec3i operator-(const Vec3i &va, const Vec3i &vb) {
    return Vec3i(va.getX()-vb.getX(), va.getY()-vb.getY(), va.getZ()-vb.getZ());
}


//---------------------------------------------------------------

Vec3f::Vec3f() {
    mValue[0] = 0.0f;
    mValue[1] = 0.0f;
    mValue[2] = 0.0f;
}


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


float &Vec3f::operator[](const int index) {
    return mValue[index];
}


float Vec3f::operator[](const int index) const {
    return mValue[index];
}


Vec3f operator-(const Vec3f &va, const Vec3f &vb) {
    return Vec3f(va.getX()-vb.getX(), va.getY()-vb.getY(), va.getZ()-vb.getZ());
}


Vec3f operator+(const Vec3f &va, const Vec3f &vb) {
    return Vec3f(va.getX()+vb.getX(), va.getY()+vb.getY(), va.getZ()+vb.getZ());
}


Vec3f operator*(const Vec3f &va, const float &s) {
    return s*va;
}


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


float Vec3f::length() const {
    return sqrt(mValue[0]*mValue[0] + mValue[1]*mValue[1] + mValue[2]*mValue[2]);
}


bool Vec3f::equals(const Vec3f &other, const float eps) const {
    return (*this-other).length() < eps;
}
