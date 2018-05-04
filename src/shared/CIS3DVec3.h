#ifndef VEC3_H
#define VEC3_H

class QString;

/**
    Implements a three dimensional vector of integers.
*/
class Vec3i {

public:
    /**
        Constructor.
        Initializes vector to (0,0,0).
    */
    Vec3i();

    /**
        Constructor.
        @param x The x component.
        @param y The y component.
        @param z The z component.
    */
    Vec3i(int x, int y, int z);

    /**
        Retrieves x, y, and z.
        @return The value in the respective component.
    */
    int getX() const;
    int getY() const;
    int getZ() const;

    /**
        Sets x, y, and z.
        @param v The value to set in the respective component.
    */
    void setX(int v);
    void setY(int v);
    void setZ(int v);

    /**
        Checks equality.
        @param other The other vector.
        @return True if both vectors are indetical in all components.
    */
    bool operator==(const Vec3i& other) const;

    /**
        Checks equality.
        @param other The other vector.
        @return True if both vectors differ in at least one component.
    */
    bool operator!=(const Vec3i& other) const;

    /**
        Retrieves value in the specified component.
        @param index The component [0-2]
        @return The value.
    */
    int& operator[](const int index);
    int operator[](const int index) const;

    /**
        Determines whether this vector is smaller than another vector.
        The comparison is done component-wise, with prevalence x->y->z.
        @return True if this vector is smaller.
    */
    bool operator<(const Vec3i& other) const;

    /**
        Prints the specified message followed by the values of x,y,z to qDebug().
        @param msg The message.
    */
    void print(const QString &msg) const;

private:
    int mValue[3];
};

/**
    Adds the specified vectors creating a new vector.
    @param va The first addend.
    @param vb The second addend.
    @return va + vb
*/
Vec3i operator+(const Vec3i& va, const Vec3i& vb);

/**
    Subtracts the specified vectors creating a new vector.
    @param va The minuend.
    @param vb The subtrahend.
    @return va - vb
*/
Vec3i operator-(const Vec3i& va, const Vec3i& vb);


/**
    Implements a three dimensional vector of floats.
*/
class Vec3f {

public:

    /**
        Constructor.
        Initializes vector to (0.0,0.0,0.0).
    */
    Vec3f();

    /**
        Constructor.
        @param val The value that is set in all components.
    */
    Vec3f(float val);

    /**
        Constructor.
        @param x The x component.
        @param y The y component.
        @param z The z component.
    */
    Vec3f(float xval, float yval, float zval);

    /**
        Retrieves x, y, and z.
        @return The value in the respective component.
    */
    float getX() const;
    float getY() const;
    float getZ() const;

    /**
        Sets x, y, and z.
        @param v The value to set in the respective component.
    */
    void setX(float v);
    void setY(float v);
    void setZ(float v);

    /**
        Determines the length of the vector.
        @return The Euclidean norm.
    */
    float length() const;

    /**
        Determines whether this vector equals another vector. Calculates the
        delta vector and checks whetehr its length is below the numerical tolerance. 
        @param other The other vector.
        @param eps The numerical tolerance.
        @return True if both are equal.
    */
    bool equals(const Vec3f& other, const float eps=1.e-4) const;

    /**
        Retrieves value in the specified component.
        @param index The component [0-2]
        @return The value.
    */
    float& operator[](const int index);
    float operator[](const int index) const;

private:
    float mValue[3];
};

/**
    Subtracts the specified vectors creating a new vector.
    @param va The minuend.
    @param vb The subtrahend.
    @return va - vb
*/
Vec3f operator-(const Vec3f& va, const Vec3f& vb);

/**
    Adds the specified vectors creating a new vector.
    @param va The first addend.
    @param vb The second addend.
    @return va + vb
*/
Vec3f operator+(const Vec3f& va, const Vec3f& vb);

/**
    Scales the specified vector component-wise creating a new vector.
    @param va The vector to scale.
    @param s The scaling factor.
    @return s * va
*/
Vec3f operator*(const Vec3f& va, const float& s);
Vec3f operator*(const float& s, const Vec3f& va);


#endif // VEC3_H
