//
// Copyright (c) 2011 Linaro Limited
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the MIT License which accompanies
// this distribution, and is available at
// http://www.opensource.org/licenses/mit-license.php
//
// Contributors:
//     Jesse Barker - original implementation.
//
#include <iostream>
#include "libmatrix_test.h"
#include "const_vec_test.h"
#include "../vec.h"

using LibMatrix::vec2;
using LibMatrix::vec3;
using LibMatrix::vec4;
using std::cout;
using std::endl;

void
Vec2TestConstOperator::run(const Options& options)
{
    const vec2 a(1.0, 1.0);
    const vec2 b(2.0, 2.0);
    vec2 aplusb(a + b);
    vec2 aminusb(a - b);
    vec2 atimesb(a * b);
    vec2 adivb(a / b);
    const float s(2.5);
    vec2 stimesb(s * b);
}

void
Vec3TestConstOperator::run(const Options& options)
{
    const vec3 a(1.0, 1.0, 1.0);
    const vec3 b(2.0, 2.0, 2.0);
    vec3 aplusb(a + b);
    vec3 aminusb(a - b);
    vec3 atimesb(a * b);
    vec3 adivb(a / b);
    const float s(2.5);
    vec3 stimesb(s * b);
}

void
Vec4TestConstOperator::run(const Options& options)
{
    const vec4 a(1.0, 1.0, 1.0, 1.0);
    const vec4 b(2.0, 2.0, 2.0, 2.0);
    vec4 aplusb(a + b);
    vec4 aminusb(a - b);
    vec4 atimesb(a * b);
    vec4 adivb(a / b);
    const float s(2.5);
    vec4 stimesb(s * b);
}
