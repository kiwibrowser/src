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
#include <iostream>
#include <string>
#include <vector>
#include "libmatrix_test.h"
#include "util_split_test.h"
#include "../util.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;

template <typename T> static bool
areVectorsEqual(vector<T>& vec1, vector<T>& vec2)
{
    if (vec1.size() != vec2.size())
        return false;

    for (unsigned int i = 0; i < vec1.size(); i++)
    {
        if (vec1[i] != vec2[i])
            return false;
    }

    return true;
}

template <typename T> static void
printVector(vector<T>& vec)
{
    cout << "[";
    for (unsigned int i = 0; i < vec.size(); i++)
    {
        cout << '"' << vec[i] << '"';
        if (i < vec.size() - 1)
            cout << ", ";
    }
    cout << "]";
}

void
UtilSplitTestNormal::run(const Options& options)
{
    const string test1("abc def ghi");
    const string test2(" abc: def :ghi ");
    vector<string> expected1;
    vector<string> expected2;
    vector<string> results;

    expected1.push_back("abc");
    expected1.push_back("def");
    expected1.push_back("ghi");

    expected2.push_back(" abc");
    expected2.push_back(" def ");
    expected2.push_back("ghi ");

    if (options.beVerbose())
    {
        cout << "Testing string \"" << test1 << "\"" << endl;
    }

    Util::split(test1, ' ', results, Util::SplitModeNormal);

    if (options.beVerbose())
    {
        cout << "Split result: ";
        printVector(results);
        cout << endl << "Expected: ";
        printVector(expected1);
        cout << endl;
    }

    if (!areVectorsEqual(results, expected1))
    {
        return;
    }

    results.clear();

    if (options.beVerbose())
    {
        cout << "Testing string \"" << test2 << "\"" << endl;
    }

    Util::split(test2, ':', results, Util::SplitModeNormal);

    if (options.beVerbose())
    {
        cout << "Split result: ";
        printVector(results);
        cout << endl << "Expected: ";
        printVector(expected2);
        cout << endl;
    }

    if (!areVectorsEqual(results, expected2))
    {
        return;
    }

    pass_ = true;
}

void
UtilSplitTestQuoted::run(const Options& options)
{
    const string test1("abc \"def' ghi\" klm\\ nop -b qr:title='123 \"456'");
    const string test2("abc: def='1:2:3:'ghi : \":jk\"");
    vector<string> expected1;
    vector<string> expected2;
    vector<string> results;

    expected1.push_back("abc");
    expected1.push_back("def' ghi");
    expected1.push_back("klm nop");
    expected1.push_back("-b");
    expected1.push_back("qr:title=123 \"456");

    expected2.push_back("abc");
    expected2.push_back(" def=1:2:3:ghi ");
    expected2.push_back(" :jk");

    if (options.beVerbose())
    {
        cout << "Testing string \"" << test1 << "\"" << endl;
    }

    Util::split(test1, ' ', results, Util::SplitModeQuoted);

    if (options.beVerbose())
    {
        cout << "Split result: ";
        printVector(results);
        cout << endl << "Expected: ";
        printVector(expected1);
        cout << endl;
    }

    if (!areVectorsEqual(results, expected1))
    {
        return;
    }

    results.clear();

    if (options.beVerbose())
    {
        cout << "Testing string \"" << test2 << "\"" << endl;
    }

    Util::split(test2, ':', results, Util::SplitModeQuoted);

    if (options.beVerbose())
    {
        cout << "Split result: ";
        printVector(results);
        cout << endl << "Expected: ";
        printVector(expected2);
        cout << endl;
    }

    if (!areVectorsEqual(results, expected2))
    {
        return;
    }

    pass_ = true;
}
