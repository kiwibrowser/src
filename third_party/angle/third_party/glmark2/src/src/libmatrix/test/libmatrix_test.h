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
#ifndef LIBMATRIX_TEST_H_
#define LIBMATRIX_TEST_H_

class Options
{
    Options();
    static const std::string verbose_name_;
    static const std::string help_name_;
    std::string app_name_;
    bool show_help_;
    bool verbose_;
public:
    Options(const std::string& app_name) :
        app_name_(app_name),
        show_help_(false),
        verbose_(false) {}
    ~Options() {}
    bool beVerbose() const { return verbose_; }
    bool showHelp() const { return show_help_; }
    void parseArgs(int argc, char** argv);
    void printUsage();
};

class MatrixTest
{
    std::string name_;
protected:
    bool pass_;
    MatrixTest();
public:
    MatrixTest(const std::string& name) : 
        name_(name),
        pass_(false) {}
    ~MatrixTest();
    const std::string& name() const { return name_; }
    virtual void run(const Options& options) = 0;
    const bool passed() const { return pass_; }
};

#endif // LIBMATRIX_TEST_H_
