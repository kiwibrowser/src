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
//     Alexandros Frantzis - Util::split tests
//
#include <iostream>
#include <string>
#include <vector>
#include "libmatrix_test.h"
#include "inverse_test.h"
#include "transpose_test.h"
#include "const_vec_test.h"
#include "shader_source_test.h"
#include "util_split_test.h"

using std::cerr;
using std::cout;
using std::endl;

int
main(int argc, char** argv)
{
    Options testOptions("matrix_test");
    testOptions.parseArgs(argc, argv);
    if (testOptions.showHelp())
    {
        testOptions.printUsage();
        return 0;
    }

    using std::vector;
    vector<MatrixTest*> testVec;
    testVec.push_back(new MatrixTest2x2Inverse());
    testVec.push_back(new MatrixTest3x3Inverse());
    testVec.push_back(new MatrixTest4x4Inverse());
    testVec.push_back(new MatrixTest2x2Transpose());
    testVec.push_back(new MatrixTest3x3Transpose());
    testVec.push_back(new MatrixTest4x4Transpose());
    testVec.push_back(new ShaderSourceBasic());
    testVec.push_back(new UtilSplitTestNormal());
    testVec.push_back(new UtilSplitTestQuoted());

    for (vector<MatrixTest*>::iterator testIt = testVec.begin();
         testIt != testVec.end();
         testIt++)
    {
        MatrixTest* curTest = *testIt;
        if (testOptions.beVerbose())
        {
            cout << "Running test " << curTest->name() << endl;
        }
        curTest->run(testOptions);
        if (!curTest->passed())
        {
            cerr << curTest->name() << " does not work!" << endl;
            return 1;
        }
        if (testOptions.beVerbose())
        {
            cout << curTest->name() << " is okay!" << endl;
        }
    }

    return 0;
}
