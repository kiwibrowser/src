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
#include <iostream>
#include "libmatrix_test.h"
#include "transpose_test.h"
#include "../mat.h"

using LibMatrix::mat2;
using LibMatrix::mat3;
using LibMatrix::mat4;
using std::cout;
using std::endl;

void
MatrixTest2x2Transpose::run(const Options& options)
{
    // First, a simple test to ensure that the transpose of the identity is
    // the identity.
    if (options.beVerbose())
    {
        cout << endl << "Assertion 1: Transpose of the identity is the identity." << endl << endl;
    }

    mat2 m;

    if (options.beVerbose())
    {
        cout << "Starting with mat2 (should be identity): " << endl << endl;
        m.print();
    }

    m.transpose();

    if (options.beVerbose())
    {
        cout << endl << "Transpose of identity (should be identity): " << endl << endl;
        m.print();
    }

    mat2 mi;
    if (m != mi)
    {
        // FAIL! Transpose of the identity is the identity.
        return;
    }

    // At this point, we have 2 identity matrices.
    // Next, set an element in the matrix and transpose twice.  We should see
    // the original matrix (with i,j set).
    if (options.beVerbose())
    {
        cout << endl << "Assertion 2: Transposing a matrix twice yields the original matrix." << endl << endl;
    }

    m[0][1] = 6.3;

    if (options.beVerbose())
    {
        cout << "Matrix should now have (0, 1) == 6.300000" << endl << endl;
        m.print();
    }
    
    mi = m;

    m.transpose().transpose();

    if (options.beVerbose())
    {
        cout << endl << "Matrix should now have (0, 1) == 6.300000" << endl << endl;
        m.print();
    }
    
    if (m != mi)
    {
        // FAIL! Transposing the same matrix twice should yield the original.
        return;
    }    

    // Next, reset mi back to the identity.  Set element element j,i in this
    // matrix and transpose m.  They should now be equal.
    if (options.beVerbose())
    {
        cout << endl << "Assertion 3: Transpose of matrix (i,j) == x is equal to matrix (j,i) == x." << endl << endl;
    }

    mi.setIdentity();
    mi[1][0] = 6.3;

    m.transpose();

    if (options.beVerbose())
    {
        cout << "Matrix should now have (1, 0) == 6.300000" << endl << endl;
        m.print();
        cout << endl;
    }
    
    if (m == mi)
    {
        pass_ = true;
    }

    // FAIL! Transposing the same matrix twice should yield the original.
}

void
MatrixTest3x3Transpose::run(const Options& options)
{
    // First, a simple test to ensure that the transpose of the identity is
    // the identity.
    if (options.beVerbose())
    {
        cout << endl << "Assertion 1: Transpose of the identity is the identity." << endl << endl;
    }

    mat3 m;

    if (options.beVerbose())
    {
        cout << "Starting with mat2 (should be identity): " << endl << endl;
        m.print();
    }

    m.transpose();

    if (options.beVerbose())
    {
        cout << endl << "Transpose of identity (should be identity): " << endl << endl;
        m.print();
    }

    mat3 mi;
    if (m != mi)
    {
        // FAIL! Transpose of the identity is the identity.
        return;
    }

    // At this point, we have 2 identity matrices.
    // Next, set an element in the matrix and transpose twice.  We should see
    // the original matrix (with i,j set).
    if (options.beVerbose())
    {
        cout << endl << "Assertion 2: Transposing a matrix twice yields the original matrix." << endl << endl;
    }

    m[0][1] = 6.3;

    if (options.beVerbose())
    {
        cout << "Matrix should now have (0, 1) == 6.300000" << endl << endl;
        m.print();
    }
    
    mi = m;

    m.transpose().transpose();

    if (options.beVerbose())
    {
        cout << endl << "Matrix should now have (0, 1) == 6.300000" << endl << endl;
        m.print();
    }
    
    if (m != mi)
    {
        // FAIL! Transposing the same matrix twice should yield the original.
        return;
    }    

    // Next, reset mi back to the identity.  Set element element j,i in this
    // matrix and transpose m.  They should now be equal.
    if (options.beVerbose())
    {
        cout << endl << "Assertion 3: Transpose of matrix (i,j) == x is equal to matrix (j,i) == x." << endl << endl;
    }

    mi.setIdentity();
    mi[1][0] = 6.3;

    m.transpose();

    if (options.beVerbose())
    {
        cout << "Matrix should now have (1, 0) == 6.300000" << endl << endl;
        m.print();
        cout << endl;
    }
    
    if (m == mi)
    {
        pass_ = true;
    }

    // FAIL! Transposing the same matrix twice should yield the original.
}

void
MatrixTest4x4Transpose::run(const Options& options)
{
    // First, a simple test to ensure that the transpose of the identity is
    // the identity.
    if (options.beVerbose())
    {
        cout << endl << "Assertion 1: Transpose of the identity is the identity." << endl << endl;
    }

    mat4 m;

    if (options.beVerbose())
    {
        cout << "Starting with mat2 (should be identity): " << endl << endl;
        m.print();
    }

    m.transpose();

    if (options.beVerbose())
    {
        cout << endl << "Transpose of identity (should be identity): " << endl << endl;
        m.print();
    }

    mat4 mi;
    if (m != mi)
    {
        // FAIL! Transpose of the identity is the identity.
        return;
    }

    // At this point, we have 2 identity matrices.
    // Next, set an element in the matrix and transpose twice.  We should see
    // the original matrix (with i,j set).
    if (options.beVerbose())
    {
        cout << endl << "Assertion 2: Transposing a matrix twice yields the original matrix." << endl << endl;
    }

    m[0][1] = 6.3;

    if (options.beVerbose())
    {
        cout << "Matrix should now have (0, 1) == 6.300000" << endl << endl;
        m.print();
    }
    
    mi = m;

    m.transpose().transpose();

    if (options.beVerbose())
    {
        cout << endl << "Matrix should now have (0, 1) == 6.300000" << endl << endl;
        m.print();
    }
    
    if (m != mi)
    {
        // FAIL! Transposing the same matrix twice should yield the original.
        return;
    }    

    // Next, reset mi back to the identity.  Set element element j,i in this
    // matrix and transpose m.  They should now be equal.
    if (options.beVerbose())
    {
        cout << endl << "Assertion 3: Transpose of matrix (i,j) == x is equal to matrix (j,i) == x." << endl << endl;
    }

    mi.setIdentity();
    mi[1][0] = 6.3;

    m.transpose();

    if (options.beVerbose())
    {
        cout << "Matrix should now have (1, 0) == 6.300000" << endl << endl;
        m.print();
        cout << endl;
    }
    
    if (m == mi)
    {
        pass_ = true;
    }

    // FAIL! Transposing the same matrix twice should yield the original.
}
