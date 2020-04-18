//
// Copyright (c) 2010 Linaro Limited
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the MIT License which accompanies
// this distribution, and is available at
// http://www.opensource.org/licenses/mit-license.php
//
// Contributors:
//     Jesse Barker - original implementation.
//
#ifndef MAT_H_
#define MAT_H_
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include "vec.h"
#ifndef USE_EXCEPTIONS
// If we're not throwing exceptions, we'll need the logger to make sure the
// caller is informed of errors.
#include "log.h"
#endif // USE_EXCEPTIONS

namespace LibMatrix
{
// Proxy class for providing the functionality of a doubly-dimensioned array
// representation of matrices.  Each matrix class defines its operator[]
// to return an ArrayProxy.  The ArrayProxy then returns the appropriate item
// from its operator[].
template<typename T, unsigned int dimension>
class ArrayProxy
{
public:
    ArrayProxy(T* data) { data_ = data; }
    ~ArrayProxy() { data_ = 0; }
    T& operator[](int index)
    {
        return data_[index * dimension];
    }
    const T& operator[](int index) const
    {
        return data_[index * dimension];
    }
private:
    T* data_;
};


// Programming interfaces to all matrix objects are represented row-centric 
// (i.e. C/C++ style references to the data appear as matrix[row][column]).  
// However, the internal data representation is column-major, so when using 
// the raw data access member to treat the data as a singly-dimensioned array,
// it does not have to be transposed.
//
// A template class for creating, managing and operating on a 2x2 matrix
// of any type you like (intended for built-in types, but as long as it 
// supports the basic arithmetic and assignment operators, any type should
// work).
template<typename T>
class tmat2
{
public:
    tmat2()
    {
        setIdentity();
    }
    tmat2(const tmat2& m)
    {
        m_[0] = m.m_[0];
        m_[1] = m.m_[1];
        m_[2] = m.m_[2];
        m_[3] = m.m_[3];
    }
    tmat2(const T& c0r0, const T& c0r1, const T& c1r0, const T& c1r1)
    {
        m_[0] = c0r0;
        m_[1] = c0r1;
        m_[2] = c1r0;
        m_[3] = c1r1;
    }
    ~tmat2() {}

    // Reset this to the identity matrix.
    void setIdentity()
    {
        m_[0] = 1;
        m_[1] = 0;
        m_[2] = 0;
        m_[3] = 1;
    }

    // Transpose this.  Return a reference to this.
    tmat2& transpose()
    {
        T tmp_val = m_[1];
        m_[1] = m_[2];
        m_[2] = tmp_val;
        return *this;
    }

    // Compute the determinant of this and return it.
    T determinant()
    {
        return (m_[0] * m_[3]) - (m_[2] * m_[1]);
    }

    // Invert this.  Return a reference to this.
    //
    // NOTE: If this is non-invertible, we will
    //       throw to avoid undefined behavior.
    tmat2& inverse()
    {
        T d(determinant());
        if (d == static_cast<T>(0))
        {
#ifdef USE_EXCEPTIONS
            throw std::runtime_error("Matrix is noninvertible!!!!");
#else // !USE_EXCEPTIONS
            Log::error("Matrix is noninvertible!!!!\n");
            return *this;
#endif // USE_EXCEPTIONS
        }
        T c0r0(m_[3] / d);
        T c0r1(-m_[1] / d);
        T c1r0(-m_[2] / d);
        T c1r1(m_[0] / d);
        m_[0] = c0r0;
        m_[1] = c0r1;
        m_[2] = c1r0;
        m_[3] = c1r1;        
        return *this;
    }

    // Print the elements of the matrix to standard out.
    // Really only useful for debug and test.
    void print() const
    {
        static const int precision(6);
        // row 0
        std::cout << "| ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[0];
        std::cout << " ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[2];
        std::cout << " |" << std::endl;
        // row 1
        std::cout << "| ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[1];
        std::cout << " ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[3];
        std::cout << " |" << std::endl;
    }

    // Allow raw data access for API calls and the like.
    // For example, it is valid to pass a tmat2<float> into a call to
    // the OpenGL command "glUniformMatrix2fv()".
    operator const T*() const { return &m_[0];}

    // Test if 'rhs' is equal to this.
    bool operator==(const tmat2& rhs) const
    {
        return m_[0] == rhs.m_[0] &&
               m_[1] == rhs.m_[1] &&
               m_[2] == rhs.m_[2] &&
               m_[3] == rhs.m_[3];
    }

    // Test if 'rhs' is not equal to this.
    bool operator!=(const tmat2& rhs) const
    {
        return !(*this == rhs);
    }

    // A direct assignment of 'rhs' to this.  Return a reference to this.
    tmat2& operator=(const tmat2& rhs)
    {
        if (this != &rhs)
        {
            m_[0] = rhs.m_[0];
            m_[1] = rhs.m_[1];
            m_[2] = rhs.m_[2];
            m_[3] = rhs.m_[3];
        }
        return *this;
    }

    // Add another matrix to this.  Return a reference to this.
    tmat2& operator+=(const tmat2& rhs)
    {
        m_[0] += rhs.m_[0];
        m_[1] += rhs.m_[1];
        m_[2] += rhs.m_[2];
        m_[3] += rhs.m_[3];
        return *this;
    }

    // Add another matrix to a copy of this.  Return the copy.
    const tmat2 operator+(const tmat2& rhs)
    {
        return tmat2(*this) += rhs;
    }

    // Subtract another matrix from this.  Return a reference to this.
    tmat2& operator-=(const tmat2& rhs)
    {
        m_[0] -= rhs.m_[0];
        m_[1] -= rhs.m_[1];
        m_[2] -= rhs.m_[2];
        m_[3] -= rhs.m_[3];
        return *this;
    }

    // Subtract another matrix from a copy of this.  Return the copy.
    const tmat2 operator-(const tmat2& rhs)
    {
        return tmat2(*this) += rhs;
    }

    // Multiply this by another matrix.  Return a reference to this.
    tmat2& operator*=(const tmat2& rhs)
    {
        T c0r0((m_[0] * rhs.m_[0]) + (m_[2] * rhs.m_[1]));
        T c0r1((m_[1] * rhs.m_[0]) + (m_[3] * rhs.m_[1]));
        T c1r0((m_[0] * rhs.m_[2]) + (m_[2] * rhs.m_[3]));
        T c1r1((m_[1] * rhs.m_[2]) + (m_[3] * rhs.m_[3]));
        m_[0] = c0r0;
        m_[1] = c0r1;
        m_[2] = c1r0;
        m_[3] = c1r1;
        return *this;
    }

    // Multiply a copy of this by another matrix.  Return the copy.
    const tmat2 operator*(const tmat2& rhs)
    {
        return tmat2(*this) *= rhs;
    }

    // Multiply this by a scalar.  Return a reference to this.
    tmat2& operator*=(const T& rhs)
    {
        m_[0] *= rhs;
        m_[1] *= rhs;
        m_[2] *= rhs;
        m_[3] *= rhs;
        return *this;
    }

    // Multiply a copy of this by a scalar.  Return the copy.
    const tmat2 operator*(const T& rhs)
    {
        return tmat2(*this) *= rhs;
    }

    // Divide this by a scalar.  Return a reference to this.
    tmat2& operator/=(const T& rhs)
    {
        m_[0] /= rhs;
        m_[1] /= rhs;
        m_[2] /= rhs;
        m_[3] /= rhs;
        return *this;
    }

    // Divide a copy of this by a scalar.  Return the copy.
    const tmat2 operator/(const T& rhs)
    {
        return tmat2(*this) /= rhs;
    }

    // Use an instance of the ArrayProxy class to support double-indexed
    // references to a matrix (i.e., m[1][1]).  See comments above the
    // ArrayProxy definition for more details.
    ArrayProxy<T, 2> operator[](int index)
    {
        return ArrayProxy<T, 2>(&m_[index]);
    }
    const ArrayProxy<T, 2> operator[](int index) const
    {
        return ArrayProxy<T, 2>(const_cast<T*>(&m_[index]));
    }

private:
    T m_[4];
};

// Multiply a scalar and a matrix just like the member operator, but allow
// the scalar to be the left-hand operand.
template<typename T>
const tmat2<T> operator*(const T& lhs, const tmat2<T>& rhs)
{
    return tmat2<T>(rhs) * lhs;
}

// Multiply a copy of a vector and a matrix (matrix is right-hand operand).
// Return the copy.
template<typename T>
const tvec2<T> operator*(const tvec2<T>& lhs, const tmat2<T>& rhs)
{
    T x((lhs.x() * rhs[0][0]) + (lhs.y() * rhs[1][0]));
    T y((lhs.x() * rhs[0][1]) + (lhs.y() * rhs[1][1]));
    return tvec2<T>(x,y);
}

// Multiply a copy of a vector and a matrix (matrix is left-hand operand).
// Return the copy.
template<typename T>
const tvec2<T> operator*(const tmat2<T>& lhs, const tvec2<T>& rhs)
{
    T x((lhs[0][0] * rhs.x()) + (lhs[0][1] * rhs.y()));
    T y((lhs[1][0] * rhs.x()) + (lhs[1][1] * rhs.y()));
    return tvec2<T>(x, y);
}

// Compute the outer product of two vectors.  Return the resultant matrix.
template<typename T>
const tmat2<T> outer(const tvec2<T>& a, const tvec2<T>& b)
{
    tmat2<T> product;
    product[0][0] = a.x() * b.x();
    product[0][1] = a.x() * b.y();
    product[1][0] = a.y() * b.x();
    product[1][1] = a.y() * b.y();
    return product;
}

// A template class for creating, managing and operating on a 3x3 matrix
// of any type you like (intended for built-in types, but as long as it 
// supports the basic arithmetic and assignment operators, any type should
// work).
template<typename T>
class tmat3
{
public:
    tmat3()
    {
        setIdentity();
    }
    tmat3(const tmat3& m)
    {
        m_[0] = m.m_[0];
        m_[1] = m.m_[1];
        m_[2] = m.m_[2];
        m_[3] = m.m_[3];
        m_[4] = m.m_[4];
        m_[5] = m.m_[5];
        m_[6] = m.m_[6];
        m_[7] = m.m_[7];
        m_[8] = m.m_[8];
    }
    tmat3(const T& c0r0, const T& c0r1, const T& c0r2,
          const T& c1r0, const T& c1r1, const T& c1r2,
          const T& c2r0, const T& c2r1, const T& c2r2)
    {
        m_[0] = c0r0;
        m_[1] = c0r1;
        m_[2] = c0r2;
        m_[3] = c1r0;
        m_[4] = c1r1;
        m_[5] = c1r2;
        m_[6] = c2r0;
        m_[7] = c2r1;
        m_[8] = c2r2;
    }
    ~tmat3() {}

    // Reset this to the identity matrix.
    void setIdentity()
    {
        m_[0] = 1;
        m_[1] = 0;
        m_[2] = 0;
        m_[3] = 0;
        m_[4] = 1;
        m_[5] = 0;
        m_[6] = 0;
        m_[7] = 0;
        m_[8] = 1;
    }

    // Transpose this.  Return a reference to this.
    tmat3& transpose()
    {
        T tmp_val = m_[1];
        m_[1] = m_[3];
        m_[3] = tmp_val;
        tmp_val = m_[2];
        m_[2] = m_[6];
        m_[6] = tmp_val;
        tmp_val = m_[5];
        m_[5] = m_[7];
        m_[7] = tmp_val;
        return *this;
    }

    // Compute the determinant of this and return it.
    T determinant()
    {
        tmat2<T> minor0(m_[4], m_[5], m_[7], m_[8]);
        tmat2<T> minor3(m_[1], m_[2], m_[7], m_[8]);
        tmat2<T> minor6(m_[1], m_[2], m_[4], m_[5]);
        return (m_[0] * minor0.determinant()) - 
               (m_[3] * minor3.determinant()) +
               (m_[6] * minor6.determinant());
    }

    // Invert this.  Return a reference to this.
    //
    // NOTE: If this is non-invertible, we will
    //       throw to avoid undefined behavior.
    tmat3& inverse()
    {
        T d(determinant());
        if (d == static_cast<T>(0))
        {
#ifdef USE_EXCEPTIONS
            throw std::runtime_error("Matrix is noninvertible!!!!");
#else // !USE_EXCEPTIONS
            Log::error("Matrix is noninvertible!!!!\n");
            return *this;
#endif // USE_EXCEPTIONS
        }
        tmat2<T> minor0(m_[4], m_[5], m_[7], m_[8]);
        tmat2<T> minor1(m_[7], m_[8], m_[1], m_[2]);
        tmat2<T> minor2(m_[1], m_[2], m_[4], m_[5]);
        tmat2<T> minor3(m_[6], m_[8], m_[3], m_[5]);
        tmat2<T> minor4(m_[0], m_[2], m_[6], m_[8]);
        tmat2<T> minor5(m_[3], m_[5], m_[0], m_[2]);
        tmat2<T> minor6(m_[3], m_[4], m_[6], m_[7]);
        tmat2<T> minor7(m_[6], m_[7], m_[0], m_[1]);
        tmat2<T> minor8(m_[0], m_[1], m_[3], m_[4]);
        m_[0] = minor0.determinant() / d;
        m_[1] = minor1.determinant() / d;
        m_[2] = minor2.determinant() / d;
        m_[3] = minor3.determinant() / d;
        m_[4] = minor4.determinant() / d;
        m_[5] = minor5.determinant() / d;
        m_[6] = minor6.determinant() / d;
        m_[7] = minor7.determinant() / d;
        m_[8] = minor8.determinant() / d;
        return *this;       
    }

    // Print the elements of the matrix to standard out.
    // Really only useful for debug and test.
    void print() const
    {
        static const int precision(6);
        // row 0
        std::cout << "| ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[0];
        std::cout << " ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[3];
        std::cout << " ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[6];
        std::cout << " |" << std::endl;
        // row 1
        std::cout << "| ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[1];
        std::cout << " ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[4];
        std::cout << " ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[7];
        std::cout << " |" << std::endl;
        // row 2
        std::cout << "| ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[2];
        std::cout << " ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[5];
        std::cout << " ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[8];
        std::cout << " |" << std::endl;
    }

    // Allow raw data access for API calls and the like.
    // For example, it is valid to pass a tmat3<float> into a call to
    // the OpenGL command "glUniformMatrix3fv()".
    operator const T*() const { return &m_[0];}

    // Test if 'rhs' is equal to this.
    bool operator==(const tmat3& rhs) const
    {
        return m_[0] == rhs.m_[0] &&
               m_[1] == rhs.m_[1] &&
               m_[2] == rhs.m_[2] &&
               m_[3] == rhs.m_[3] &&
               m_[4] == rhs.m_[4] &&
               m_[5] == rhs.m_[5] &&
               m_[6] == rhs.m_[6] &&
               m_[7] == rhs.m_[7] &&
               m_[8] == rhs.m_[8];
    }

    // Test if 'rhs' is not equal to this.
    bool operator!=(const tmat3& rhs) const
    {
        return !(*this == rhs);
    }

    // A direct assignment of 'rhs' to this.  Return a reference to this.
    tmat3& operator=(const tmat3& rhs)
    {
        if (this != &rhs)
        {
            m_[0] = rhs.m_[0];
            m_[1] = rhs.m_[1];
            m_[2] = rhs.m_[2];
            m_[3] = rhs.m_[3];
            m_[4] = rhs.m_[4];
            m_[5] = rhs.m_[5];
            m_[6] = rhs.m_[6];
            m_[7] = rhs.m_[7];
            m_[8] = rhs.m_[8];
        }
        return *this;
    }

    // Add another matrix to this.  Return a reference to this.
    tmat3& operator+=(const tmat3& rhs)
    {
        m_[0] += rhs.m_[0];
        m_[1] += rhs.m_[1];
        m_[2] += rhs.m_[2];
        m_[3] += rhs.m_[3];
        m_[4] += rhs.m_[4];
        m_[5] += rhs.m_[5];
        m_[6] += rhs.m_[6];
        m_[7] += rhs.m_[7];
        m_[8] += rhs.m_[8];
        return *this;
    }

    // Add another matrix to a copy of this.  Return the copy.
    const tmat3 operator+(const tmat3& rhs)
    {
        return tmat3(*this) += rhs;
    }

    // Subtract another matrix from this.  Return a reference to this.
    tmat3& operator-=(const tmat3& rhs)
    {
        m_[0] -= rhs.m_[0];
        m_[1] -= rhs.m_[1];
        m_[2] -= rhs.m_[2];
        m_[3] -= rhs.m_[3];
        m_[4] -= rhs.m_[4];
        m_[5] -= rhs.m_[5];
        m_[6] -= rhs.m_[6];
        m_[7] -= rhs.m_[7];
        m_[8] -= rhs.m_[8];
        return *this;
    }

    // Subtract another matrix from a copy of this.  Return the copy.
    const tmat3 operator-(const tmat3& rhs)
    {
        return tmat3(*this) -= rhs;
    }

    // Multiply this by another matrix.  Return a reference to this.
    tmat3& operator*=(const tmat3& rhs)
    {
        T c0r0((m_[0] * rhs.m_[0]) + (m_[3] * rhs.m_[1]) + (m_[6] * rhs.m_[2]));
        T c0r1((m_[1] * rhs.m_[0]) + (m_[4] * rhs.m_[1]) + (m_[7] * rhs.m_[2]));
        T c0r2((m_[2] * rhs.m_[0]) + (m_[5] * rhs.m_[1]) + (m_[8] * rhs.m_[2]));
        T c1r0((m_[0] * rhs.m_[3]) + (m_[3] * rhs.m_[4]) + (m_[6] * rhs.m_[5]));
        T c1r1((m_[1] * rhs.m_[3]) + (m_[4] * rhs.m_[4]) + (m_[7] * rhs.m_[5]));
        T c1r2((m_[2] * rhs.m_[3]) + (m_[5] * rhs.m_[4]) + (m_[8] * rhs.m_[5]));
        T c2r0((m_[0] * rhs.m_[6]) + (m_[3] * rhs.m_[7]) + (m_[6] * rhs.m_[8]));
        T c2r1((m_[1] * rhs.m_[6]) + (m_[4] * rhs.m_[7]) + (m_[7] * rhs.m_[8]));
        T c2r2((m_[2] * rhs.m_[6]) + (m_[5] * rhs.m_[7]) + (m_[8] * rhs.m_[8]));
        m_[0] = c0r0;
        m_[1] = c0r1;
        m_[2] = c0r2;
        m_[3] = c1r0;
        m_[4] = c1r1;
        m_[5] = c1r2;
        m_[6] = c2r0;
        m_[7] = c2r1;
        m_[8] = c2r2;
        return *this;
    }

    // Multiply a copy of this by another matrix.  Return the copy.
    const tmat3 operator*(const tmat3& rhs)
    {
        return tmat3(*this) *= rhs;
    }

    // Multiply this by a scalar.  Return a reference to this.
    tmat3& operator*=(const T& rhs)
    {
        m_[0] *= rhs;
        m_[1] *= rhs;
        m_[2] *= rhs;
        m_[3] *= rhs;
        m_[4] *= rhs;
        m_[5] *= rhs;
        m_[6] *= rhs;
        m_[7] *= rhs;
        m_[8] *= rhs;
        return *this;
    }

    // Multiply a copy of this by a scalar.  Return the copy.
    const tmat3 operator*(const T& rhs)
    {
        return tmat3(*this) *= rhs;
    }

    // Divide this by a scalar.  Return a reference to this.
    tmat3& operator/=(const T& rhs)
    {
        m_[0] /= rhs;
        m_[1] /= rhs;
        m_[2] /= rhs;
        m_[3] /= rhs;
        m_[4] /= rhs;
        m_[5] /= rhs;
        m_[6] /= rhs;
        m_[7] /= rhs;
        m_[8] /= rhs;
        return *this;
    }

    // Divide a copy of this by a scalar.  Return the copy.
    const tmat3 operator/(const T& rhs)
    {
        return tmat3(*this) /= rhs;
    }

    // Use an instance of the ArrayProxy class to support double-indexed
    // references to a matrix (i.e., m[1][1]).  See comments above the
    // ArrayProxy definition for more details.
    ArrayProxy<T, 3> operator[](int index)
    {
        return ArrayProxy<T, 3>(&m_[index]);
    }
    const ArrayProxy<T, 3> operator[](int index) const
    {
        return ArrayProxy<T, 3>(const_cast<T*>(&m_[index]));
    }

private:
    T m_[9];
};

// Multiply a scalar and a matrix just like the member operator, but allow
// the scalar to be the left-hand operand.
template<typename T>
const tmat3<T> operator*(const T& lhs, const tmat3<T>& rhs)
{
    return tmat3<T>(rhs) * lhs;
}

// Multiply a copy of a vector and a matrix (matrix is right-hand operand).
// Return the copy.
template<typename T>
const tvec3<T> operator*(const tvec3<T>& lhs, const tmat3<T>& rhs)
{
    T x((lhs.x() * rhs[0][0]) + (lhs.y() * rhs[1][0]) + (lhs.z() * rhs[2][0]));
    T y((lhs.x() * rhs[0][1]) + (lhs.y() * rhs[1][1]) + (lhs.z() * rhs[2][1]));
    T z((lhs.x() * rhs[0][2]) + (lhs.y() * rhs[1][2]) + (lhs.z() * rhs[2][2]));
    return tvec3<T>(x, y, z);
}

// Multiply a copy of a vector and a matrix (matrix is left-hand operand).
// Return the copy.
template<typename T>
const tvec3<T> operator*(const tmat3<T>& lhs, const tvec3<T>& rhs)
{
    T x((lhs[0][0] * rhs.x()) + (lhs[0][1] * rhs.y()) + (lhs[0][2] * rhs.z()));
    T y((lhs[1][0] * rhs.x()) + (lhs[1][1] * rhs.y()) + (lhs[1][2] * rhs.z()));
    T z((lhs[2][0] * rhs.x()) + (lhs[2][1] * rhs.y()) + (lhs[2][2] * rhs.z()));
    return tvec3<T>(x, y, z);
}

// Compute the outer product of two vectors.  Return the resultant matrix.
template<typename T>
const tmat3<T> outer(const tvec3<T>& a, const tvec3<T>& b)
{
    tmat3<T> product;
    product[0][0] = a.x() * b.x();
    product[0][1] = a.x() * b.y();
    product[0][2] = a.x() * b.z();
    product[1][0] = a.y() * b.x();
    product[1][1] = a.y() * b.y();
    product[1][2] = a.y() * b.z();
    product[2][0] = a.z() * b.x();
    product[2][1] = a.z() * b.y();
    product[2][2] = a.z() * b.z();
    return product;
}

// A template class for creating, managing and operating on a 4x4 matrix
// of any type you like (intended for built-in types, but as long as it 
// supports the basic arithmetic and assignment operators, any type should
// work).
template<typename T>
class tmat4
{
public:
    tmat4()
    {
        setIdentity();
    }
    tmat4(const tmat4& m)
    {
        m_[0] = m.m_[0];
        m_[1] = m.m_[1];
        m_[2] = m.m_[2];
        m_[3] = m.m_[3];
        m_[4] = m.m_[4];
        m_[5] = m.m_[5];
        m_[6] = m.m_[6];
        m_[7] = m.m_[7];
        m_[8] = m.m_[8];
        m_[9] = m.m_[9];
        m_[10] = m.m_[10];
        m_[11] = m.m_[11];
        m_[12] = m.m_[12];
        m_[13] = m.m_[13];
        m_[14] = m.m_[14];
        m_[15] = m.m_[15];
    }
    ~tmat4() {}

    // Reset this to the identity matrix.
    void setIdentity()
    {
        m_[0] = 1;
        m_[1] = 0;
        m_[2] = 0;
        m_[3] = 0;
        m_[4] = 0;
        m_[5] = 1;
        m_[6] = 0;
        m_[7] = 0;
        m_[8] = 0;
        m_[9] = 0;
        m_[10] = 1;
        m_[11] = 0;
        m_[12] = 0;
        m_[13] = 0;
        m_[14] = 0;
        m_[15] = 1;
    }

    // Transpose this.  Return a reference to this.
    tmat4& transpose()
    {
        T tmp_val = m_[1];
        m_[1] = m_[4];
        m_[4] = tmp_val;
        tmp_val = m_[2];
        m_[2] = m_[8];
        m_[8] = tmp_val;
        tmp_val = m_[3];
        m_[3] = m_[12];
        m_[12] = tmp_val;
        tmp_val = m_[6];
        m_[6] = m_[9];
        m_[9] = tmp_val;
        tmp_val = m_[7];
        m_[7] = m_[13];
        m_[13] = tmp_val;
        tmp_val = m_[11];
        m_[11] = m_[14];
        m_[14] = tmp_val;
        return *this;
    }

    // Compute the determinant of this and return it.
    T determinant()
    {
        tmat3<T> minor0(m_[5], m_[6], m_[7], m_[9], m_[10], m_[11], m_[13], m_[14], m_[15]);
        tmat3<T> minor4(m_[1], m_[2], m_[3], m_[9], m_[10], m_[11], m_[13], m_[14], m_[15]);
        tmat3<T> minor8(m_[1], m_[2], m_[3], m_[5], m_[6], m_[7], m_[13], m_[14], m_[15]);
        tmat3<T> minor12(m_[1], m_[2], m_[3], m_[5], m_[6], m_[7], m_[9], m_[10], m_[11]);
        return (m_[0] * minor0.determinant()) -
               (m_[4] * minor4.determinant()) +
               (m_[8] * minor8.determinant()) -
               (m_[12] * minor12.determinant());
    }

    // Invert this.  Return a reference to this.
    //
    // NOTE: If this is non-invertible, we will
    //       throw to avoid undefined behavior.
    tmat4& inverse()
    {
        T d(determinant());
        if (d == static_cast<T>(0))
        {
#ifdef USE_EXCEPTIONS
            throw std::runtime_error("Matrix is noninvertible!!!!");
#else // !USE_EXCEPTIONS
            Log::error("Matrix is noninvertible!!!!\n");
            return *this;
#endif // USE_EXCEPTIONS
        }
        tmat3<T> minor0(m_[5], m_[6], m_[7], m_[9], m_[10], m_[11], m_[13], m_[14], m_[15]);
        tmat3<T> minor1(m_[1], m_[2], m_[3], m_[13], m_[14], m_[15], m_[9], m_[10], m_[11]);
        tmat3<T> minor2(m_[1], m_[2], m_[3], m_[5], m_[6], m_[7], m_[13], m_[14], m_[15]);
        tmat3<T> minor3(m_[1], m_[2], m_[3], m_[9], m_[10], m_[11], m_[5], m_[6], m_[7]);

        tmat3<T> minor4(m_[4], m_[6], m_[7], m_[12], m_[14], m_[15], m_[8], m_[10], m_[11]);
        tmat3<T> minor5(m_[0], m_[2], m_[3], m_[8], m_[10], m_[11], m_[12], m_[14], m_[15]);
        tmat3<T> minor6(m_[0], m_[2], m_[3], m_[12], m_[14], m_[15], m_[4], m_[6], m_[7]);
        tmat3<T> minor7(m_[0], m_[2], m_[3], m_[4], m_[6], m_[7], m_[8], m_[10], m_[11]);

        tmat3<T> minor8(m_[4], m_[5], m_[7], m_[8], m_[9], m_[11], m_[12], m_[13], m_[15]);
        tmat3<T> minor9(m_[0], m_[1], m_[3], m_[12], m_[13], m_[15], m_[8], m_[9], m_[11]);
        tmat3<T> minor10(m_[0], m_[1], m_[3], m_[4], m_[5], m_[7], m_[12], m_[13], m_[15]);
        tmat3<T> minor11(m_[0], m_[1], m_[3], m_[8], m_[9], m_[11], m_[4], m_[5], m_[7]);

        tmat3<T> minor12(m_[4], m_[5], m_[6], m_[12], m_[13], m_[14], m_[8], m_[9], m_[10]);
        tmat3<T> minor13(m_[0], m_[1], m_[2], m_[8], m_[9], m_[10], m_[12], m_[13], m_[14]);
        tmat3<T> minor14(m_[0], m_[1], m_[2], m_[12], m_[13], m_[14], m_[4], m_[5], m_[6]);
        tmat3<T> minor15(m_[0], m_[1], m_[2], m_[4], m_[5], m_[6], m_[8], m_[9], m_[10]);
        m_[0] = minor0.determinant() / d;
        m_[1] = minor1.determinant() / d;
        m_[2] = minor2.determinant() / d;
        m_[3] = minor3.determinant() / d;
        m_[4] = minor4.determinant() / d;
        m_[5] = minor5.determinant() / d;
        m_[6] = minor6.determinant() / d;
        m_[7] = minor7.determinant() / d;
        m_[8] = minor8.determinant() / d;
        m_[9] = minor9.determinant() / d;
        m_[10] = minor10.determinant() / d;
        m_[11] = minor11.determinant() / d;
        m_[12] = minor12.determinant() / d;
        m_[13] = minor13.determinant() / d;
        m_[14] = minor14.determinant() / d;
        m_[15] = minor15.determinant() / d;
        return *this;
    }

    // Print the elements of the matrix to standard out.
    // Really only useful for debug and test.
    void print() const
    {
        static const int precision(6);
        // row 0
        std::cout << "| ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[0];
        std::cout << " ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[4];
        std::cout << " ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[8];
        std::cout << " ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[12];
        std::cout << " |" << std::endl;
        // row 1
        std::cout << "| ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[1];
        std::cout << " ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[5];
        std::cout << " ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[9];
        std::cout << " ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[13];
        std::cout << " |" << std::endl;
        // row 2
        std::cout << "| ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[2];
        std::cout << " ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[6];
        std::cout << " ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[10];
        std::cout << " ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[14];
        std::cout << " |" << std::endl;
        // row 3
        std::cout << "| ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[3];
        std::cout << " ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[7];
        std::cout << " ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[11];
        std::cout << " ";
        std::cout << std::fixed << std::showpoint << std::setprecision(precision) << m_[15];
        std::cout << " |" << std::endl;
    }

    // Allow raw data access for API calls and the like.
    // For example, it is valid to pass a tmat4<float> into a call to
    // the OpenGL command "glUniformMatrix4fv()".
    operator const T*() const { return &m_[0];}

    // Test if 'rhs' is equal to this.
    bool operator==(const tmat4& rhs) const
    {
        return m_[0] == rhs.m_[0] &&
               m_[1] == rhs.m_[1] &&
               m_[2] == rhs.m_[2] &&
               m_[3] == rhs.m_[3] &&
               m_[4] == rhs.m_[4] &&
               m_[5] == rhs.m_[5] &&
               m_[6] == rhs.m_[6] &&
               m_[7] == rhs.m_[7] &&
               m_[8] == rhs.m_[8] &&
               m_[9] == rhs.m_[9] &&
               m_[10] == rhs.m_[10] &&
               m_[11] == rhs.m_[11] &&
               m_[12] == rhs.m_[12] &&
               m_[13] == rhs.m_[13] &&
               m_[14] == rhs.m_[14] &&
               m_[15] == rhs.m_[15];
    }

    // Test if 'rhs' is not equal to this.
    bool operator!=(const tmat4& rhs) const
    {
        return !(*this == rhs);
    }

    // A direct assignment of 'rhs' to this.  Return a reference to this.
    tmat4& operator=(const tmat4& rhs)
    {
        if (this != &rhs)
        {
            m_[0] = rhs.m_[0];
            m_[1] = rhs.m_[1];
            m_[2] = rhs.m_[2];
            m_[3] = rhs.m_[3];
            m_[4] = rhs.m_[4];
            m_[5] = rhs.m_[5];
            m_[6] = rhs.m_[6];
            m_[7] = rhs.m_[7];
            m_[8] = rhs.m_[8];
            m_[9] = rhs.m_[9];
            m_[10] = rhs.m_[10];
            m_[11] = rhs.m_[11];
            m_[12] = rhs.m_[12];
            m_[13] = rhs.m_[13];
            m_[14] = rhs.m_[14];
            m_[15] = rhs.m_[15];
        }
        return *this;
    }

    // Add another matrix to this.  Return a reference to this.
    tmat4& operator+=(const tmat4& rhs)
    {
        m_[0] += rhs.m_[0];
        m_[1] += rhs.m_[1];
        m_[2] += rhs.m_[2];
        m_[3] += rhs.m_[3];
        m_[4] += rhs.m_[4];
        m_[5] += rhs.m_[5];
        m_[6] += rhs.m_[6];
        m_[7] += rhs.m_[7];
        m_[8] += rhs.m_[8];
        m_[9] += rhs.m_[9];
        m_[10] += rhs.m_[10];
        m_[11] += rhs.m_[11];
        m_[12] += rhs.m_[12];
        m_[13] += rhs.m_[13];
        m_[14] += rhs.m_[14];
        m_[15] += rhs.m_[15];
        return *this;
    }

    // Add another matrix to a copy of this.  Return the copy.
    const tmat4 operator+(const tmat4& rhs)
    {
        return tmat4(*this) += rhs;
    }

    // Subtract another matrix from this.  Return a reference to this.
    tmat4& operator-=(const tmat4& rhs)
    {
        m_[0] -= rhs.m_[0];
        m_[1] -= rhs.m_[1];
        m_[2] -= rhs.m_[2];
        m_[3] -= rhs.m_[3];
        m_[4] -= rhs.m_[4];
        m_[5] -= rhs.m_[5];
        m_[6] -= rhs.m_[6];
        m_[7] -= rhs.m_[7];
        m_[8] -= rhs.m_[8];
        m_[9] -= rhs.m_[9];
        m_[10] -= rhs.m_[10];
        m_[11] -= rhs.m_[11];
        m_[12] -= rhs.m_[12];
        m_[13] -= rhs.m_[13];
        m_[14] -= rhs.m_[14];
        m_[15] -= rhs.m_[15];
        return *this;
    }

    // Subtract another matrix from a copy of this.  Return the copy.
    const tmat4 operator-(const tmat4& rhs)
    {
        return tmat4(*this) -= rhs;
    }

    // Multiply this by another matrix.  Return a reference to this.
    tmat4& operator*=(const tmat4& rhs)
    {
        T c0r0((m_[0] * rhs.m_[0]) + (m_[4] * rhs.m_[1]) + (m_[8] * rhs.m_[2]) + (m_[12] * rhs.m_[3]));
        T c0r1((m_[1] * rhs.m_[0]) + (m_[5] * rhs.m_[1]) + (m_[9] * rhs.m_[2]) + (m_[13] * rhs.m_[3]));
        T c0r2((m_[2] * rhs.m_[0]) + (m_[6] * rhs.m_[1]) + (m_[10] * rhs.m_[2]) + (m_[14] * rhs.m_[3]));
        T c0r3((m_[3] * rhs.m_[0]) + (m_[7] * rhs.m_[1]) + (m_[11] * rhs.m_[2]) + (m_[15] * rhs.m_[3]));
        T c1r0((m_[0] * rhs.m_[4]) + (m_[4] * rhs.m_[5]) + (m_[8] * rhs.m_[6]) + (m_[12] * rhs.m_[7]));
        T c1r1((m_[1] * rhs.m_[4]) + (m_[5] * rhs.m_[5]) + (m_[9] * rhs.m_[6]) + (m_[13] * rhs.m_[7]));
        T c1r2((m_[2] * rhs.m_[4]) + (m_[6] * rhs.m_[5]) + (m_[10] * rhs.m_[6]) + (m_[14] * rhs.m_[7]));
        T c1r3((m_[3] * rhs.m_[4]) + (m_[7] * rhs.m_[5]) + (m_[11] * rhs.m_[6]) + (m_[15] * rhs.m_[7]));
        T c2r0((m_[0] * rhs.m_[8]) + (m_[4] * rhs.m_[9]) + (m_[8] * rhs.m_[10]) + (m_[12] * rhs.m_[11]));
        T c2r1((m_[1] * rhs.m_[8]) + (m_[5] * rhs.m_[9]) + (m_[9] * rhs.m_[10]) + (m_[13] * rhs.m_[11]));
        T c2r2((m_[2] * rhs.m_[8]) + (m_[6] * rhs.m_[9]) + (m_[10] * rhs.m_[10]) + (m_[14] * rhs.m_[11]));
        T c2r3((m_[3] * rhs.m_[8]) + (m_[7] * rhs.m_[9]) + (m_[11] * rhs.m_[10]) + (m_[15] * rhs.m_[11]));
        T c3r0((m_[0] * rhs.m_[12]) + (m_[4] * rhs.m_[13]) + (m_[8] * rhs.m_[14]) + (m_[12] * rhs.m_[15]));
        T c3r1((m_[1] * rhs.m_[12]) + (m_[5] * rhs.m_[13]) + (m_[9] * rhs.m_[14]) + (m_[13] * rhs.m_[15]));
        T c3r2((m_[2] * rhs.m_[12]) + (m_[6] * rhs.m_[13]) + (m_[10] * rhs.m_[14]) + (m_[14] * rhs.m_[15]));
        T c3r3((m_[3] * rhs.m_[12]) + (m_[7] * rhs.m_[13]) + (m_[11] * rhs.m_[14]) + (m_[15] * rhs.m_[15]));
        m_[0] = c0r0;
        m_[1] = c0r1;
        m_[2] = c0r2;
        m_[3] = c0r3;
        m_[4] = c1r0;
        m_[5] = c1r1;
        m_[6] = c1r2;
        m_[7] = c1r3;
        m_[8] = c2r0;
        m_[9] = c2r1;
        m_[10] = c2r2;
        m_[11] = c2r3;
        m_[12] = c3r0;
        m_[13] = c3r1;
        m_[14] = c3r2;
        m_[15] = c3r3;
        return *this;
    }

    // Multiply a copy of this by another matrix.  Return the copy.
    const tmat4 operator*(const tmat4& rhs)
    {
        return tmat4(*this) *= rhs;
    }

    // Multiply this by a scalar.  Return a reference to this.
    tmat4& operator*=(const T& rhs)
    {
        m_[0] *= rhs;
        m_[1] *= rhs;
        m_[2] *= rhs;
        m_[3] *= rhs;
        m_[4] *= rhs;
        m_[5] *= rhs;
        m_[6] *= rhs;
        m_[7] *= rhs;
        m_[8] *= rhs;
        m_[9] *= rhs;
        m_[10] *= rhs;
        m_[11] *= rhs;
        m_[12] *= rhs;
        m_[13] *= rhs;
        m_[14] *= rhs;
        m_[15] *= rhs;
        return *this;
    }

    // Multiply a copy of this by a scalar.  Return the copy.
    const tmat4 operator*(const T& rhs)
    {
        return tmat4(*this) *= rhs;
    }

    // Divide this by a scalar.  Return a reference to this.
    tmat4& operator/=(const T& rhs)
    {
        m_[0] /= rhs;
        m_[1] /= rhs;
        m_[2] /= rhs;
        m_[3] /= rhs;
        m_[4] /= rhs;
        m_[5] /= rhs;
        m_[6] /= rhs;
        m_[7] /= rhs;
        m_[8] /= rhs;
        m_[9] /= rhs;
        m_[10] /= rhs;
        m_[11] /= rhs;
        m_[12] /= rhs;
        m_[13] /= rhs;
        m_[14] /= rhs;
        m_[15] /= rhs;
        return *this;
    }

    // Divide a copy of this by a scalar.  Return the copy.
    const tmat4 operator/(const T& rhs)
    {
        return tmat4(*this) /= rhs;
    }

    // Use an instance of the ArrayProxy class to support double-indexed
    // references to a matrix (i.e., m[1][1]).  See comments above the
    // ArrayProxy definition for more details.
    ArrayProxy<T, 4> operator[](int index)
    {
        return ArrayProxy<T, 4>(&m_[index]);
    }
    const ArrayProxy<T, 4> operator[](int index) const
    {
        return ArrayProxy<T, 4>(const_cast<T*>(&m_[index]));
    }

private:
    T m_[16];
};

// Multiply a scalar and a matrix just like the member operator, but allow
// the scalar to be the left-hand operand.
template<typename T>
const tmat4<T> operator*(const T& lhs, const tmat4<T>& rhs)
{
    return tmat4<T>(rhs) * lhs;
}

// Multiply a copy of a vector and a matrix (matrix is right-hand operand).
// Return the copy.
template<typename T>
const tvec4<T> operator*(const tvec4<T>& lhs, const tmat4<T>& rhs)
{
    T x((lhs.x() * rhs[0][0]) + (lhs.y() * rhs[1][0]) + (lhs.z() * rhs[2][0]) + (lhs.w() * rhs[3][0]));
    T y((lhs.x() * rhs[0][1]) + (lhs.y() * rhs[1][1]) + (lhs.z() * rhs[2][1]) + (lhs.w() * rhs[3][1]));
    T z((lhs.x() * rhs[0][2]) + (lhs.y() * rhs[1][2]) + (lhs.z() * rhs[2][2]) + (lhs.w() * rhs[3][2]));
    T w((lhs.x() * rhs[0][3]) + (lhs.y() * rhs[1][3]) + (lhs.z() * rhs[2][3]) + (lhs.w() * rhs[3][3]));
    return tvec4<T>(x, y, z, w);
}

// Multiply a copy of a vector and a matrix (matrix is left-hand operand).
// Return the copy.
template<typename T>
const tvec4<T> operator*(const tmat4<T>& lhs, const tvec4<T>& rhs)
{
    T x((lhs[0][0] * rhs.x()) + (lhs[0][1] * rhs.y()) + (lhs[0][2] * rhs.z()) + (lhs[0][3] * rhs.w()));
    T y((lhs[1][0] * rhs.x()) + (lhs[1][1] * rhs.y()) + (lhs[1][2] * rhs.z()) + (lhs[1][3] * rhs.w()));
    T z((lhs[2][0] * rhs.x()) + (lhs[2][1] * rhs.y()) + (lhs[2][2] * rhs.z()) + (lhs[2][3] * rhs.w()));
    T w((lhs[3][0] * rhs.x()) + (lhs[3][1] * rhs.y()) + (lhs[3][2] * rhs.z()) + (lhs[3][3] * rhs.w()));
    return tvec4<T>(x, y, z, w);
}

// Compute the outer product of two vectors.  Return the resultant matrix.
template<typename T>
const tmat4<T> outer(const tvec4<T>& a, const tvec4<T>& b)
{
    tmat4<T> product;
    product[0][0] = a.x() * b.x();
    product[0][1] = a.x() * b.y();
    product[0][2] = a.x() * b.z();
    product[0][3] = a.x() * b.w();
    product[1][0] = a.y() * b.x();
    product[1][1] = a.y() * b.y();
    product[1][2] = a.y() * b.z();
    product[1][3] = a.y() * b.w();
    product[2][0] = a.z() * b.x();
    product[2][1] = a.z() * b.y();
    product[2][2] = a.z() * b.z();
    product[2][3] = a.z() * b.w();
    product[3][0] = a.w() * b.x();
    product[3][1] = a.w() * b.y();
    product[3][2] = a.w() * b.z();
    product[3][3] = a.w() * b.w();
    return product;
}

//
// Convenience typedefs.  These are here to present a homogeneous view of these
// objects with respect to shader source.
//
typedef tmat2<float> mat2;
typedef tmat3<float> mat3;
typedef tmat4<float> mat4;

typedef tmat2<double> dmat2;
typedef tmat3<double> dmat3;
typedef tmat4<double> dmat4;

typedef tmat2<int> imat2;
typedef tmat3<int> imat3;
typedef tmat4<int> imat4;

typedef tmat2<unsigned int> umat2;
typedef tmat3<unsigned int> umat3;
typedef tmat4<unsigned int> umat4;

typedef tmat2<bool> bmat2;
typedef tmat3<bool> bmat3;
typedef tmat4<bool> bmat4;

namespace Mat4
{

//
// Some functions to generate transformation matrices that used to be provided
// by OpenGL.
//
mat4 translate(float x, float y, float z);
mat4 scale(float x, float y, float z);
mat4 rotate(float angle, float x, float y, float z);
mat4 frustum(float left, float right, float bottom, float top, float near, float far);
mat4 ortho(float left, float right, float bottom, float top, float near, float far);
mat4 perspective(float fovy, float aspect, float zNear, float zFar);
mat4 lookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ);

} // namespace Mat4
} // namespace LibMatrix
#endif // MAT_H_
