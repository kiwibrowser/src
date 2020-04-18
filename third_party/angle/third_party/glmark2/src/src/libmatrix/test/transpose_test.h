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
#ifndef TRANSPOSE_TEST_H_
#define TRANSPOSE_TEST_H_

class MatrixTest;
class Options;

class MatrixTest2x2Transpose : public MatrixTest
{
public:
    MatrixTest2x2Transpose() : MatrixTest("mat2::transpose") {}
    virtual void run(const Options& options);
};

class MatrixTest3x3Transpose : public MatrixTest
{
public:
    MatrixTest3x3Transpose() : MatrixTest("mat3::transpose") {}
    virtual void run(const Options& options);
};

class MatrixTest4x4Transpose : public MatrixTest
{
public:
    MatrixTest4x4Transpose() : MatrixTest("mat4::transpose") {}
    virtual void run(const Options& options);
};
#endif // TRANSPOSE_TEST_H_
