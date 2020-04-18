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
#ifndef CONST_VEC_TEST_H_
#define CONST_VEC_TEST_H_

class MatrixTest;
class Options;

class Vec2TestConstOperator : public MatrixTest
{
public:
    Vec2TestConstOperator() : MatrixTest("vec2::const") {}
    virtual void run(const Options& options);
};

class Vec3TestConstOperator : public MatrixTest
{
public:
    Vec3TestConstOperator() : MatrixTest("vec3::const") {}
    virtual void run(const Options& options);
};

class Vec4TestConstOperator : public MatrixTest
{
public:
    Vec4TestConstOperator() : MatrixTest("vec4::const") {}
    virtual void run(const Options& options);
};

#endif // CONST_VEC_TEST_H_
