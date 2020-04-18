//
// Copyright (c) 2012 Linaro Limited
//
// All rights reserved. This program and the accompanying materials
// are made available under the terms of the MIT License which accompanies
// this distribution, and is available at
// http://www.opensource.org/licenses/mit-license.php
//
// Contributors:
//     Alexandros Frantzis - original implementation.
//
#ifndef UTIL_SPLIT_TEST_H_
#define UTIL_SPLIT_TEST_H_

class MatrixTest;
class Options;

class UtilSplitTestNormal : public MatrixTest
{
public:
    UtilSplitTestNormal() : MatrixTest("Util::split::normal") {}
    virtual void run(const Options& options);
};

class UtilSplitTestQuoted : public MatrixTest
{
public:
    UtilSplitTestQuoted() : MatrixTest("Util::split::quoted") {}
    virtual void run(const Options& options);
};
#endif // UTIL_SPLIT_TEST_H_
