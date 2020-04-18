//
// Copyright (c) 2010-2011 Linaro Limited
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the MIT License which accompanies
// this distribution, and is available at
// http://www.opensource.org/licenses/mit-license.php
//
// Contributors:
//     Jesse Barker - original implementation.
//
#ifndef VEC_H_
#define VEC_H_

#include <iostream> // only needed for print() functions...
#include <math.h>

namespace LibMatrix
{
// A template class for creating, managing and operating on a 2-element vector
// of any type you like (intended for built-in types, but as long as it 
// supports the basic arithmetic and assignment operators, any type should
// work).
template<typename T>
class tvec2
{
public:
    tvec2() :
        x_(0),
        y_(0) {}
    tvec2(const T t) :
        x_(t),
        y_(t) {}
    tvec2(const T x, const T y) :
        x_(x),
        y_(y) {}
    tvec2(const tvec2& v) :
        x_(v.x_),
        y_(v.y_) {}
    ~tvec2() {}

    // Print the elements of the vector to standard out.
    // Really only useful for debug and test.
    void print() const
    {
        std::cout << "| " << x_ << " " << y_ << " |" << std::endl;
    }

    // Allow raw data access for API calls and the like.
    // For example, it is valid to pass a tvec2<float> into a call to
    // the OpenGL command "glUniform2fv()".
    operator const T*() const { return &x_;}

    // Get and set access members for the individual elements.
    const T x() const { return x_; }
    const T y() const { return y_; }

    void x(const T& val) { x_ = val; }
    void y(const T& val) { y_ = val; }

    // A direct assignment of 'rhs' to this.  Return a reference to this.
    tvec2& operator=(const tvec2& rhs)
    {
        if (this != &rhs)
        {
            x_ = rhs.x_;
            y_ = rhs.y_;
        }
        return *this;
    }

    // Divide this by a scalar.  Return a reference to this.
    tvec2& operator/=(const T& rhs)
    {
        x_ /= rhs;
        y_ /= rhs;
        return *this;
    }

    // Divide a copy of this by a scalar.  Return the copy.
    const tvec2 operator/(const T& rhs) const
    {
        return tvec2(*this) /= rhs;
    }

    // Component-wise divide of this by another vector.
    // Return a reference to this.
    tvec2& operator/=(const tvec2& rhs)
    {
        x_ /= rhs.x_;
        y_ /= rhs.y_;
        return *this;
    }

    // Component-wise divide of a copy of this by another vector.
    // Return the copy.
    const tvec2 operator/(const tvec2& rhs) const
    {
        return tvec2(*this) /= rhs;
    }

    // Multiply this by a scalar.  Return a reference to this.
    tvec2& operator*=(const T& rhs)
    {
        x_ *= rhs;
        y_ *= rhs;
        return *this;
    }

    // Multiply a copy of this by a scalar.  Return the copy.
    const tvec2 operator*(const T& rhs) const
    {
        return tvec2(*this) *= rhs;
    }

    // Component-wise multiply of this by another vector.
    // Return a reference to this.
    tvec2& operator*=(const tvec2& rhs)
    {
        x_ *= rhs.x_;
        y_ *= rhs.y_;
        return *this;
    }

    // Component-wise multiply of a copy of this by another vector.
    // Return the copy.
    const tvec2 operator*(const tvec2& rhs) const
    {
        return tvec2(*this) *= rhs;
    }

    // Add a scalar to this.  Return a reference to this.
    tvec2& operator+=(const T& rhs)
    {
        x_ += rhs;
        y_ += rhs;
        return *this;
    }
    
    // Add a scalar to a copy of this.  Return the copy.
    const tvec2 operator+(const T& rhs) const
    {
        return tvec2(*this) += rhs;
    }

    // Component-wise addition of another vector to this.
    // Return a reference to this.
    tvec2& operator+=(const tvec2& rhs)
    {
        x_ += rhs.x_;
        y_ += rhs.y_;
        return *this;
    }

    // Component-wise addition of another vector to a copy of this.
    // Return the copy.
    const tvec2 operator+(const tvec2& rhs) const
    {
        return tvec2(*this) += rhs;
    }

    // Subtract a scalar from this.  Return a reference to this.
    tvec2& operator-=(const T& rhs)
    {
        x_ -= rhs;
        y_ -= rhs;
        return *this;
    }
    
    // Subtract a scalar from a copy of this.  Return the copy.
    const tvec2 operator-(const T& rhs) const
    {
        return tvec2(*this) -= rhs;
    }

    // Component-wise subtraction of another vector from this.
    // Return a reference to this.
    tvec2& operator-=(const tvec2& rhs)
    {
        x_ -= rhs.x_;
        y_ -= rhs.y_;
        return *this;
    }

    // Component-wise subtraction of another vector from a copy of this.
    // Return the copy.
    const tvec2 operator-(const tvec2& rhs) const
    {
        return tvec2(*this) -= rhs;
    }

    // Compute the length of this and return it.
    float length() const
    {
        return sqrt(dot(*this, *this));
    }

    // Make this a unit vector.
    void normalize()
    {
        float l = length();
        x_ /= l;
        y_ /= l;
    }

    // Compute the dot product of two vectors.
    static T dot(const tvec2& v1, const tvec2& v2)
    {
        return (v1.x_ * v2.x_) + (v1.y_ * v2.y_); 
    }

private:
    T x_;
    T y_;
};

// A template class for creating, managing and operating on a 3-element vector
// of any type you like (intended for built-in types, but as long as it 
// supports the basic arithmetic and assignment operators, any type should
// work).
template<typename T>
class tvec3
{
public:
    tvec3() :
        x_(0),
        y_(0),
        z_(0) {}
    tvec3(const T t) :
        x_(t),
        y_(t),
        z_(t) {}
    tvec3(const T x, const T y, const T z) :
        x_(x),
        y_(y),
        z_(z) {}
    tvec3(const tvec3& v) :
        x_(v.x_),
        y_(v.y_),
        z_(v.z_) {}
    ~tvec3() {}

    // Print the elements of the vector to standard out.
    // Really only useful for debug and test.
    void print() const
    {
        std::cout << "| " << x_ << " " << y_ << " " << z_ << " |" << std::endl;
    }

    // Allow raw data access for API calls and the like.
    // For example, it is valid to pass a tvec3<float> into a call to
    // the OpenGL command "glUniform3fv()".
    operator const T*() const { return &x_;}

    // Get and set access members for the individual elements.
    const T x() const { return x_; }
    const T y() const { return y_; }
    const T z() const { return z_; }

    void x(const T& val) { x_ = val; }
    void y(const T& val) { y_ = val; }
    void z(const T& val) { z_ = val; }

    // A direct assignment of 'rhs' to this.  Return a reference to this.
    tvec3& operator=(const tvec3& rhs)
    {
        if (this != &rhs)
        {
            x_ = rhs.x_;
            y_ = rhs.y_;
            z_ = rhs.z_;
        }
        return *this;
    }

    // Divide this by a scalar.  Return a reference to this.
    tvec3& operator/=(const T& rhs)
    {
        x_ /= rhs;
        y_ /= rhs;
        z_ /= rhs;
        return *this;
    }

    // Divide a copy of this by a scalar.  Return the copy.
    const tvec3 operator/(const T& rhs) const
    {
        return tvec3(*this) /= rhs;
    }

    // Component-wise divide of this by another vector.
    // Return a reference to this.
    tvec3& operator/=(const tvec3& rhs)
    {
        x_ /= rhs.x_;
        y_ /= rhs.y_;
        z_ /= rhs.z_;
        return *this;
    }

    // Component-wise divide of a copy of this by another vector.
    // Return the copy.
    const tvec3 operator/(const tvec3& rhs) const
    {
        return tvec3(*this) /= rhs;
    }

    // Multiply this by a scalar.  Return a reference to this.
    tvec3& operator*=(const T& rhs)
    {
        x_ *= rhs;
        y_ *= rhs;
        z_ *= rhs;
        return *this;
    }

    // Multiply a copy of this by a scalar.  Return the copy.
    const tvec3 operator*(const T& rhs) const
    {
        return tvec3(*this) *= rhs;
    }

    // Component-wise multiply of this by another vector.
    // Return a reference to this.
    tvec3& operator*=(const tvec3& rhs)
    {
        x_ *= rhs.x_;
        y_ *= rhs.y_;
        z_ *= rhs.z_;
        return *this;
    }

    // Component-wise multiply of a copy of this by another vector.
    // Return the copy.
    const tvec3 operator*(const tvec3& rhs) const
    {
        return tvec3(*this) *= rhs;
    }

    // Add a scalar to this.  Return a reference to this.
    tvec3& operator+=(const T& rhs)
    {
        x_ += rhs;
        y_ += rhs;
        z_ += rhs;
        return *this;
    }

    // Add a scalar to a copy of this.  Return the copy.
    const tvec3 operator+(const T& rhs) const
    {
        return tvec3(*this) += rhs;
    }

    // Component-wise addition of another vector to this.
    // Return a reference to this.
    tvec3& operator+=(const tvec3& rhs)
    {
        x_ += rhs.x_;
        y_ += rhs.y_;
        z_ += rhs.z_;
        return *this;
    }

    // Component-wise addition of another vector to a copy of this.
    // Return the copy.
    const tvec3 operator+(const tvec3& rhs) const
    {
        return tvec3(*this) += rhs;
    }

    // Subtract a scalar from this.  Return a reference to this.
    tvec3& operator-=(const T& rhs)
    {
        x_ -= rhs;
        y_ -= rhs;
        z_ -= rhs;
        return *this;
    }

    // Subtract a scalar from a copy of this.  Return the copy.
    const tvec3 operator-(const T& rhs) const
    {
        return tvec3(*this) -= rhs;
    }

    // Component-wise subtraction of another vector from this.
    // Return a reference to this.
    tvec3& operator-=(const tvec3& rhs)
    {
        x_ -= rhs.x_;
        y_ -= rhs.y_;
        z_ -= rhs.z_;
        return *this;
    }

    // Component-wise subtraction of another vector from a copy of this.
    // Return the copy.
    const tvec3 operator-(const tvec3& rhs) const
    {
        return tvec3(*this) -= rhs;
    }

    // Compute the length of this and return it.
    float length() const
    {
        return sqrt(dot(*this, *this));
    }

    // Make this a unit vector.
    void normalize()
    {
        float l = length();
        x_ /= l;
        y_ /= l;
        z_ /= l;
    }

    // Compute the dot product of two vectors.
    static T dot(const tvec3& v1, const tvec3& v2)
    {
        return (v1.x_ * v2.x_) + (v1.y_ * v2.y_) + (v1.z_ * v2.z_); 
    }

    // Compute the cross product of two vectors.
    static tvec3 cross(const tvec3& u, const tvec3& v)
    {
        return tvec3((u.y_ * v.z_) - (u.z_ * v.y_),
                    (u.z_ * v.x_) - (u.x_ * v.z_),
                    (u.x_ * v.y_) - (u.y_ * v.x_));
    }

private:
    T x_;
    T y_;
    T z_;
};

// A template class for creating, managing and operating on a 4-element vector
// of any type you like (intended for built-in types, but as long as it 
// supports the basic arithmetic and assignment operators, any type should
// work).
template<typename T>
class tvec4
{
public:
    tvec4() :
        x_(0),
        y_(0),
        z_(0),
        w_(0) {}
    tvec4(const T t) :
        x_(t),
        y_(t),
        z_(t),
        w_(t) {}
    tvec4(const T x, const T y, const T z, const T w) :
        x_(x),
        y_(y),
        z_(z),
        w_(w) {}
    tvec4(const tvec4& v) :
        x_(v.x_),
        y_(v.y_),
        z_(v.z_),
        w_(v.w_) {}
    ~tvec4() {}

    // Print the elements of the vector to standard out.
    // Really only useful for debug and test.
    void print() const
    {
        std::cout << "| " << x_ << " " << y_ << " " << z_ << " " << w_ << " |" << std::endl;
    }

    // Allow raw data access for API calls and the like.
    // For example, it is valid to pass a tvec4<float> into a call to
    // the OpenGL command "glUniform4fv()".
    operator const T*() const { return &x_;}

    // Get and set access members for the individual elements.
    const T x() const { return x_; }
    const T y() const { return y_; }
    const T z() const { return z_; }
    const T w() const { return w_; }

    void x(const T& val) { x_ = val; }
    void y(const T& val) { y_ = val; }
    void z(const T& val) { z_ = val; }
    void w(const T& val) { w_ = val; }

    // A direct assignment of 'rhs' to this.  Return a reference to this.
    tvec4& operator=(const tvec4& rhs)
    {
        if (this != &rhs)
        {
            x_ = rhs.x_;
            y_ = rhs.y_;
            z_ = rhs.z_;
            w_ = rhs.w_;
        }
        return *this;
    }

    // Divide this by a scalar.  Return a reference to this.
    tvec4& operator/=(const T& rhs)
    {
        x_ /= rhs;
        y_ /= rhs;
        z_ /= rhs;
        w_ /= rhs;
        return *this;
    }

    // Divide a copy of this by a scalar.  Return the copy.
    const tvec4 operator/(const T& rhs) const
    {
        return tvec4(*this) /= rhs;
    }

    // Component-wise divide of this by another vector.
    // Return a reference to this.
    tvec4& operator/=(const tvec4& rhs)
    {
        x_ /= rhs.x_;
        y_ /= rhs.y_;
        z_ /= rhs.z_;
        w_ /= rhs.w_;
        return *this;
    }

    // Component-wise divide of a copy of this by another vector.
    // Return the copy.
    const tvec4 operator/(const tvec4& rhs) const
    {
        return tvec4(*this) /= rhs;
    }

    // Multiply this by a scalar.  Return a reference to this.
    tvec4& operator*=(const T& rhs)
    {
        x_ *= rhs;
        y_ *= rhs;
        z_ *= rhs;
        w_ *= rhs;
        return *this;
    }

    // Multiply a copy of this by a scalar.  Return the copy.
    const tvec4 operator*(const T& rhs) const
    {
        return tvec4(*this) *= rhs;
    }

    // Component-wise multiply of this by another vector.
    // Return a reference to this.
    tvec4& operator*=(const tvec4& rhs)
    {
        x_ *= rhs.x_;
        y_ *= rhs.y_;
        z_ *= rhs.z_;
        w_ *= rhs.w_;
        return *this;
    }

    // Component-wise multiply of a copy of this by another vector.
    // Return the copy.
    const tvec4 operator*(const tvec4& rhs) const
    {
        return tvec4(*this) *= rhs;
    }

    // Add a scalar to this.  Return a reference to this.
    tvec4& operator+=(const T& rhs)
    {
        x_ += rhs;
        y_ += rhs;
        z_ += rhs;
        w_ += rhs;
        return *this;
    }

    // Add a scalar to a copy of this.  Return the copy.
    const tvec4 operator+(const T& rhs) const
    {
        return tvec4(*this) += rhs;
    }

    // Component-wise addition of another vector to this.
    // Return a reference to this.
    tvec4& operator+=(const tvec4& rhs)
    {
        x_ += rhs.x_;
        y_ += rhs.y_;
        z_ += rhs.z_;
        w_ += rhs.w_;
        return *this;
    }

    // Component-wise addition of another vector to a copy of this.
    // Return the copy.
    const tvec4 operator+(const tvec4& rhs) const
    {
        return tvec4(*this) += rhs;
    }

    // Subtract a scalar from this.  Return a reference to this.
    tvec4& operator-=(const T& rhs)
    {
        x_ -= rhs;
        y_ -= rhs;
        z_ -= rhs;
        w_ -= rhs;
        return *this;
    }

    // Subtract a scalar from a copy of this.  Return the copy.
    const tvec4 operator-(const T& rhs) const
    {
        return tvec4(*this) -= rhs;
    }

    // Component-wise subtraction of another vector from this.
    // Return a reference to this.
    tvec4& operator-=(const tvec4& rhs)
    {
        x_ -= rhs.x_;
        y_ -= rhs.y_;
        z_ -= rhs.z_;
        w_ -= rhs.w_;
        return *this;
    }

    // Component-wise subtraction of another vector from a copy of this.
    // Return the copy.
    const tvec4 operator-(const tvec4& rhs) const
    {
        return tvec4(*this) -= rhs;
    }

    // Compute the length of this and return it.
    float length() const
    {
        return sqrt(dot(*this, *this));
    }

    // Make this a unit vector.
    void normalize()
    {
        float l = length();
        x_ /= l;
        y_ /= l;
        z_ /= l;
        w_ /= l;
    }

    // Compute the dot product of two vectors.
    static T dot(const tvec4& v1, const tvec4& v2)
    {
        return (v1.x_ * v2.x_) + (v1.y_ * v2.y_) + (v1.z_ * v2.z_) + (v1.w_ * v2.w_); 
    }

private:
    T x_;
    T y_;
    T z_;
    T w_;
};

//
// Convenience typedefs.  These are here to present a homogeneous view of these
// objects with respect to shader source.
//
typedef tvec2<float> vec2;
typedef tvec3<float> vec3;
typedef tvec4<float> vec4;

typedef tvec2<double> dvec2;
typedef tvec3<double> dvec3;
typedef tvec4<double> dvec4;

typedef tvec2<int> ivec2;
typedef tvec3<int> ivec3;
typedef tvec4<int> ivec4;

typedef tvec2<unsigned int> uvec2;
typedef tvec3<unsigned int> uvec3;
typedef tvec4<unsigned int> uvec4;

typedef tvec2<bool> bvec2;
typedef tvec3<bool> bvec3;
typedef tvec4<bool> bvec4;

} // namespace LibMatrix

// Global operators to allow for things like defining a new vector in terms of
// a product of a scalar and a vector
template<typename T>
const LibMatrix::tvec2<T> operator*(const T t, const LibMatrix::tvec2<T>& v)
{
    return v * t;
}

template<typename T>
const LibMatrix::tvec3<T> operator*(const T t, const LibMatrix::tvec3<T>& v)
{
    return v * t;
}

template<typename T>
const LibMatrix::tvec4<T> operator*(const T t, const LibMatrix::tvec4<T>& v)
{
    return v * t;
}

#endif // VEC_H_
