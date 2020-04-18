// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/command_format.h"

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/commands.h"
#include "tools/gn/setup.h"
#include "tools/gn/test_with_scheduler.h"

using FormatTest = TestWithScheduler;

#define FORMAT_TEST(n)                                                      \
  TEST_F(FormatTest, n) {                                                   \
    ::Setup setup;                                                          \
    std::string out;                                                        \
    std::string expected;                                                   \
    base::FilePath src_dir;                                                 \
    base::PathService::Get(base::DIR_SOURCE_ROOT, &src_dir);                \
    base::SetCurrentDirectory(src_dir);                                     \
    EXPECT_TRUE(commands::FormatFileToString(                               \
        &setup, SourceFile("//tools/gn/format_test_data/" #n ".gn"), false, \
        &out));                                                             \
    ASSERT_TRUE(base::ReadFileToString(                                     \
        base::FilePath(FILE_PATH_LITERAL("tools/gn/format_test_data/")      \
                           FILE_PATH_LITERAL(#n)                            \
                               FILE_PATH_LITERAL(".golden")),               \
        &expected));                                                        \
    EXPECT_EQ(expected, out);                                               \
  }

// These are expanded out this way rather than a runtime loop so that
// --gtest_filter works as expected for individual test running.
FORMAT_TEST(001)
FORMAT_TEST(002)
FORMAT_TEST(003)
FORMAT_TEST(004)
FORMAT_TEST(005)
FORMAT_TEST(006)
FORMAT_TEST(007)
FORMAT_TEST(008)
FORMAT_TEST(009)
FORMAT_TEST(010)
FORMAT_TEST(011)
FORMAT_TEST(012)
FORMAT_TEST(013)
FORMAT_TEST(014)
FORMAT_TEST(015)
FORMAT_TEST(016)
FORMAT_TEST(017)
FORMAT_TEST(018)
FORMAT_TEST(019)
FORMAT_TEST(020)
FORMAT_TEST(021)
FORMAT_TEST(022)
FORMAT_TEST(023)
FORMAT_TEST(024)
FORMAT_TEST(025)
FORMAT_TEST(026)
FORMAT_TEST(027)
FORMAT_TEST(028)
FORMAT_TEST(029)
FORMAT_TEST(030)
FORMAT_TEST(031)
FORMAT_TEST(032)
FORMAT_TEST(033)
// TODO(scottmg): args+rebase_path unnecessarily split: FORMAT_TEST(034)
FORMAT_TEST(035)
FORMAT_TEST(036)
FORMAT_TEST(037)
FORMAT_TEST(038)
FORMAT_TEST(039)
// TODO(scottmg): Bad break, exceeding 80 col: FORMAT_TEST(040)
FORMAT_TEST(041)
FORMAT_TEST(042)
FORMAT_TEST(043)
// TODO(scottmg): Dewrapped caused exceeding 80 col: FORMAT_TEST(044)
FORMAT_TEST(045)
FORMAT_TEST(046)
FORMAT_TEST(047)
FORMAT_TEST(048)
// TODO(scottmg): Eval is broken (!) and comment output might have extra ,
//                FORMAT_TEST(049)
FORMAT_TEST(050)
FORMAT_TEST(051)
FORMAT_TEST(052)
FORMAT_TEST(053)
FORMAT_TEST(054)
FORMAT_TEST(055)
FORMAT_TEST(056)
FORMAT_TEST(057)
FORMAT_TEST(058)
FORMAT_TEST(059)
FORMAT_TEST(060)
FORMAT_TEST(061)
FORMAT_TEST(062)
FORMAT_TEST(063)
FORMAT_TEST(064)
FORMAT_TEST(065)
FORMAT_TEST(066)
FORMAT_TEST(067)
FORMAT_TEST(068)
FORMAT_TEST(069)
FORMAT_TEST(070)
