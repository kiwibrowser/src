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
#include "inverse_test.h"
#include "../mat.h"

using LibMatrix::mat2;
using LibMatrix::mat3;
using LibMatrix::mat4;
using std::cout;
using std::endl;

void
MatrixTest2x2Inverse::run(const Options& options)
{
    mat2 m;

    if (options.beVerbose())
    {
        cout << "Starting with mat2 (should be identity): " << endl << endl;
        m.print();
    }

    m[0][1] = -2.5;
    
    if (options.beVerbose())
    {
        cout << endl << "Matrix should now have (0, 1) == -2.500000" << endl << endl;
        m.print();
    }
    
    mat2 mi(m);

    if (options.beVerbose())
    {
        cout << endl << "Copy of previous matrix (should have (0, 1) == -2.500000)" << endl << endl;
        mi.print();
    }

    mi.inverse();

    if (options.beVerbose())
    {
        cout << endl << "Inverse of copy: " << endl << endl;
        mi.print();
    }

    mat2 i = m * mi;

    if (options.beVerbose())
    {
        cout << endl << "Product of original and inverse (should be identity): " << endl << endl;
        i.print();
    }

    mat2 ident;
    if (i == ident)
    {
        pass_ = true;
    }
}

void
MatrixTest3x3Inverse::run(const Options& options)
{
    mat3 m;

    if (options.beVerbose())
    {
        cout << "Starting with mat3 (should be identity): " << endl << endl;
        m.print();
    }

    m[1][2] = -2.5;
    
    if (options.beVerbose())
    {
        cout << endl << "Matrix should now have (1, 2) == -2.500000" << endl << endl;
        m.print();
    }

    mat3 mi(m);

    if (options.beVerbose())
    {
        cout << endl << "Copy of previous matrix (should have (1, 2) == -2.500000)" << endl << endl;
        mi.print();
    }

    mi.inverse();

    if (options.beVerbose())
    {
        cout << endl << "Inverse of copy: " << endl << endl;
        mi.print();
    }

    mat3 i = m * mi;

    if (options.beVerbose())
    {
        cout << endl << "Product of original and inverse (should be identity): " << endl << endl;
        i.print();
    }

    mat3 ident;
    if (i == ident)
    {
        pass_ = true;
    }
}

void
MatrixTest4x4Inverse::run(const Options& options)
{
    mat4 m;

    if (options.beVerbose())
    {
        cout << "Starting with mat4 (should be identity): " << endl << endl;
        m.print();
    }

    m[2][3] = -2.5;
    
    if (options.beVerbose())
    {
        cout << endl << "Matrix should now have (2, 3) == -2.500000" << endl << endl;
        m.print();
    }

    mat4 mi(m);

    if (options.beVerbose())
    {
        cout << endl << "Copy of previous matrix (should have (2, 3) == -2.500000)" << endl << endl;
        mi.print();
    }

    mi.inverse();

    if (options.beVerbose())
    {
        cout << endl << "Inverse of copy: " << endl << endl;
        mi.print();
    }

    mat4 i = m * mi;

    if (options.beVerbose())
    {
        cout << endl <<  "Product of original and inverse (should be identity): " << endl << endl;
        i.print();
    }

    mat4 ident;
    if (i == ident)
    {
        pass_ = true;
    }
}

