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
#ifndef INVERSE_TEST_H_
#define INVERSE_TEST_H_

class MatrixTest;
class Options;

class MatrixTest2x2Inverse : public MatrixTest
{
public:
    MatrixTest2x2Inverse() : MatrixTest("mat2::inverse") {}
    virtual void run(const Options& options);
};

class MatrixTest3x3Inverse : public MatrixTest
{
public:
    MatrixTest3x3Inverse() : MatrixTest("mat3::inverse") {}
    virtual void run(const Options& options);
};

class MatrixTest4x4Inverse : public MatrixTest
{
public:
    MatrixTest4x4Inverse() : MatrixTest("mat4::inverse") {}
    virtual void run(const Options& options);
};

#endif // INVERSE_TEST_H_
