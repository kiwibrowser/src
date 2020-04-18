// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/exec_process.h"

#include "base/command_line.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_WIN)
#include "base/strings/utf_string_conversions.h"
#endif

namespace internal {

// TODO(cjhopman): Enable these tests when windows ExecProcess handles stderr.
// 'python' is not runnable on Windows. Adding ["cmd", "/c"] fails because
// CommandLine does unusual reordering of args.
#if !defined(OS_WIN)
namespace {
bool ExecPython(const std::string& command,
                std::string* std_out,
                std::string* std_err,
                int* exit_code) {
  base::ScopedTempDir temp_dir;
  CHECK(temp_dir.CreateUniqueTempDir());
  base::CommandLine::StringVector args;
#if defined(OS_WIN)
  args.push_back(L"python");
  args.push_back(L"-c");
  args.push_back(base::UTF8ToUTF16(command));
#else
  args.push_back("python");
  args.push_back("-c");
  args.push_back(command);
#endif
  return ExecProcess(base::CommandLine(args), temp_dir.GetPath(), std_out,
                     std_err, exit_code);
}
}  // namespace

TEST(ExecProcessTest, TestExitCode) {
  std::string std_out, std_err;
  int exit_code;

  ASSERT_TRUE(
      ExecPython("import sys; sys.exit(0)", &std_out, &std_err, &exit_code));
  EXPECT_EQ(0, exit_code);

  ASSERT_TRUE(
      ExecPython("import sys; sys.exit(1)", &std_out, &std_err, &exit_code));
  EXPECT_EQ(1, exit_code);

  ASSERT_TRUE(
      ExecPython("import sys; sys.exit(253)", &std_out, &std_err, &exit_code));
  EXPECT_EQ(253, exit_code);

  ASSERT_TRUE(
      ExecPython("throw Exception()", &std_out, &std_err, &exit_code));
  EXPECT_EQ(1, exit_code);
}

// Test that large output is handled correctly. There are various ways that this
// could potentially fail. For example, non-blocking Linux pipes have a 65536
// byte buffer and, if stdout is non-blocking, python will throw an IOError when
// a write exceeds the buffer size.
TEST(ExecProcessTest, TestLargeOutput) {
  base::ScopedTempDir temp_dir;
  std::string std_out, std_err;
  int exit_code;

  ASSERT_TRUE(ExecPython(
      "import sys; print 'o' * 1000000", &std_out, &std_err, &exit_code));
  EXPECT_EQ(0, exit_code);
  EXPECT_EQ(1000001u, std_out.size());
}

TEST(ExecProcessTest, TestStdoutAndStderrOutput) {
  base::ScopedTempDir temp_dir;
  std::string std_out, std_err;
  int exit_code;

  ASSERT_TRUE(ExecPython(
      "import sys; print 'o' * 10000; print >>sys.stderr, 'e' * 10000",
      &std_out,
      &std_err,
      &exit_code));
  EXPECT_EQ(0, exit_code);
  EXPECT_EQ(10001u, std_out.size());
  EXPECT_EQ(10001u, std_err.size());

  std_out.clear();
  std_err.clear();
  ASSERT_TRUE(ExecPython(
      "import sys; print >>sys.stderr, 'e' * 10000; print 'o' * 10000",
      &std_out,
      &std_err,
      &exit_code));
  EXPECT_EQ(0, exit_code);
  EXPECT_EQ(10001u, std_out.size());
  EXPECT_EQ(10001u, std_err.size());
}

TEST(ExecProcessTest, TestOneOutputClosed) {
  std::string std_out, std_err;
  int exit_code;

  ASSERT_TRUE(ExecPython("import sys; sys.stderr.close(); print 'o' * 10000",
                         &std_out,
                         &std_err,
                         &exit_code));
  EXPECT_EQ(0, exit_code);
  EXPECT_EQ(10001u, std_out.size());
  EXPECT_EQ(std_err.size(), 0u);

  std_out.clear();
  std_err.clear();
  ASSERT_TRUE(ExecPython(
      "import sys; sys.stdout.close(); print >>sys.stderr, 'e' * 10000",
      &std_out,
      &std_err,
      &exit_code));
  EXPECT_EQ(0, exit_code);
  EXPECT_EQ(0u, std_out.size());
  EXPECT_EQ(10001u, std_err.size());
}
#endif
}  // namespace internal
