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
#include <math.h>
#include "mat.h"

namespace LibMatrix
{
namespace Mat4
{

mat4
translate(float x, float y, float z)
{
    mat4 t;
    t[0][3] = x;
    t[1][3] = y;
    t[2][3] = z;
    return t;
}

mat4
scale(float x, float y, float z)
{
    mat4 s;
    s[0][0] = x;
    s[1][1] = y;
    s[2][2] = z;
    return s;
}

//
// As per the OpenGL "red book" definition of rotation, from the appendix
// on Homogeneous Coordinates and Transformation Matrices, the "upper left"
// 3x3 portion of the result matrix is formed by:
//
// M = uuT + (cos a)(I - uuT) + (sin a)S
//
// where u is the normalized input vector, uuT is the outer product of that
// vector and its transpose, I is the identity matrix and S is the matrix:
//
// |  0  -z'  y' |
// |  z'  0  -x' |
// | -y'  x'  0  |
//
// where x', y' and z' are the elements of u
//
mat4
rotate(float angle, float x, float y, float z)
{
    vec3 u(x, y, z);
    u.normalize();
    mat3 uuT = outer(u, u);
    mat3 s;   
    s[0][0] = 0;
    s[0][1] = -u.z();
    s[0][2] = u.y();
    s[1][0] = u.z();
    s[1][1] = 0;
    s[1][2] = -u.x();
    s[2][0] = -u.y();
    s[2][1] = u.x();
    s[2][2] = 0;
    mat3 i;
    i -= uuT;
    // degrees to radians
    float angleRadians(angle * M_PI / 180.0);
    i *= cos(angleRadians);
    s *= sin(angleRadians);
    i += s;
    mat3 m = uuT + i;
    mat4 r;
    r[0][0] = m[0][0];
    r[0][1] = m[0][1];
    r[0][2] = m[0][2];
    r[1][0] = m[1][0];
    r[1][1] = m[1][1];
    r[1][2] = m[1][2];
    r[2][0] = m[2][0];
    r[2][1] = m[2][1];
    r[2][2] = m[2][2];
    return r;
}

mat4
frustum(float left, float right, float bottom, float top, float near, float far)
{
    float twiceNear(2 * near);
    float width(right - left);
    float height(top - bottom);
    float depth(far - near);
    mat4 f;
    f[0][0] = twiceNear / width;
    f[0][2] = (right + left) / width;
    f[1][1] = twiceNear / height;
    f[1][2] = (top + bottom) / height;
    f[2][2] = -(far + near) / depth;
    f[2][3] = -(twiceNear * far) / depth;
    f[3][2] = -1;
    f[3][3] = 0;
    return f;
}

mat4
ortho(float left, float right, float bottom, float top, float near, float far)
{
    float width(right - left);
    float height(top - bottom);
    float depth(far - near);
    mat4 o;
    o[0][0] = 2 / width;
    o[0][3] = (right + left) / width;
    o[1][1] = 2 / height;
    o[1][3] = (top + bottom) / height;
    o[2][2] = -2 / depth;
    o[2][3] = (far + near) / depth;
    return o;
}

mat4
perspective(float fovy, float aspect, float zNear, float zFar)
{
    // degrees to radians
    float fovyRadians(fovy * M_PI / 180.0);
    // cotangent(x) = 1/tan(x)
    float f = 1/tan(fovyRadians / 2);
    float depth(zNear - zFar);
    mat4 p;
    p[0][0] = f / aspect;
    p[1][1] = f;
    p[2][2] = (zFar + zNear) / depth;
    p[2][3] = (2 * zFar * zNear) / depth;
    p[3][2] = -1;
    p[3][3] = 0;
    return p;
}

mat4 lookAt(float eyeX, float eyeY, float eyeZ, 
    float centerX, float centerY, float centerZ, 
    float upX, float upY, float upZ)
{
    vec3 f(centerX - eyeX, centerY - eyeY, centerZ - eyeZ);
    f.normalize();
    vec3 up(upX, upY, upZ);
    vec3 s = vec3::cross(f, up);
    vec3 u = vec3::cross(s, f);
    s.normalize();
    u.normalize();
    mat4 la;
    la[0][0] = s.x();
    la[0][1] = s.y();
    la[0][2] = s.z();
    la[1][0] = u.x();
    la[1][1] = u.y();
    la[1][2] = u.z();
    la[2][0] = -f.x();
    la[2][1] = -f.y();
    la[2][2] = -f.z();
    la *= translate(-eyeX, -eyeY, -eyeZ);
    return la;
}

} // namespace Mat4

} // namespace LibMatrix
