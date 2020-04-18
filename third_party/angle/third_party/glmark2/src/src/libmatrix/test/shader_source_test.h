//
// Copyright (c) 2012 Linaro Limited
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the MIT License which accompanies
// this distribution, and is available at
// http://www.opensource.org/licenses/mit-license.php
//
// Contributors:
//     Jesse Barker - original implementation.
//
#ifndef SHADER_SOURCE_TEST_H_
#define SHADER_SOURCE_TEST_H_

class MatrixTest;
class Options;

class ShaderSourceBasic : public MatrixTest
{
public:
    ShaderSourceBasic() : MatrixTest("ShaderSource::basic") {}
    virtual void run(const Options& options);
};

class ShaderSourceAddConstGlobal : public MatrixTest
{
public:
    ShaderSourceAddConstGlobal() : MatrixTest("ShaderSource::AddConstGlobal") {}
    virtual void run(const Options& options);
};

#endif // SHADER_SOURCE_TEST_H
