// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/functions.h"
#include "tools/gn/parse_tree.h"
#include "tools/gn/test_with_scope.h"

namespace {

std::string RebaseOne(Scope* scope,
                      const char* input,
                      const char* to_dir,
                      const char* from_dir) {
  std::vector<Value> args;
  args.push_back(Value(nullptr, input));
  args.push_back(Value(nullptr, to_dir));
  args.push_back(Value(nullptr, from_dir));

  Err err;
  FunctionCallNode function;
  Value result = functions::RunRebasePath(scope, &function, args, &err);
  bool is_string = result.type() == Value::STRING;
  EXPECT_TRUE(is_string);

  return result.string_value();
}

}  // namespace

TEST(RebasePath, Strings) {
  TestWithScope setup;
  Scope* scope = setup.scope();
  scope->set_source_dir(SourceDir("//tools/gn/"));

  // Build-file relative paths.
  EXPECT_EQ("../../tools/gn", RebaseOne(scope, ".", "//out/Debug", "."));
  EXPECT_EQ("../../tools/gn/", RebaseOne(scope, "./", "//out/Debug", "."));
  EXPECT_EQ("../../tools/gn/foo", RebaseOne(scope, "foo", "//out/Debug", "."));
  EXPECT_EQ("../..", RebaseOne(scope, "../..", "//out/Debug", "."));
  EXPECT_EQ("../../", RebaseOne(scope, "../../", "//out/Debug", "."));

  // Without a source root defined, we cannot move out of the source tree.
  EXPECT_EQ("../..", RebaseOne(scope, "../../..", "//out/Debug", "."));

  // Source-absolute input paths.
  EXPECT_EQ("./", RebaseOne(scope, "//", "//", "//"));
  EXPECT_EQ("foo", RebaseOne(scope, "//foo", "//", "//"));
  EXPECT_EQ("foo/", RebaseOne(scope, "//foo/", "//", "//"));
  EXPECT_EQ("../../foo/bar", RebaseOne(scope, "//foo/bar", "//out/Debug", "."));
  EXPECT_EQ("./", RebaseOne(scope, "//foo/", "//foo/", "//"));
  EXPECT_EQ(".", RebaseOne(scope, "//foo", "//foo", "//"));

  // Test slash conversion.
  EXPECT_EQ("foo/bar", RebaseOne(scope, "foo/bar", ".", "."));
  EXPECT_EQ("foo/bar", RebaseOne(scope, "foo\\bar", ".", "."));

  // Test system path output.
#if defined(OS_WIN)
  setup.build_settings()->SetRootPath(base::FilePath(L"C:/path/to/src"));
  EXPECT_EQ("C:/path/to/src", RebaseOne(scope, ".", "", "//"));
  EXPECT_EQ("C:/path/to/src/", RebaseOne(scope, "//", "", "//"));
  EXPECT_EQ("C:/path/to/src/foo", RebaseOne(scope, "foo", "", "//"));
  EXPECT_EQ("C:/path/to/src/foo/", RebaseOne(scope, "foo/", "", "//"));
  EXPECT_EQ("C:/path/to/src/tools/gn/foo", RebaseOne(scope, "foo", "", "."));
  EXPECT_EQ("C:/path/to/other/tools",
            RebaseOne(scope, "//../other/tools", "", "//"));
  EXPECT_EQ("C:/path/to/src/foo/bar",
            RebaseOne(scope, "//../src/foo/bar", "", "//"));
  EXPECT_EQ("C:/path/to", RebaseOne(scope, "//..", "", "//"));
  EXPECT_EQ("C:/path", RebaseOne(scope, "../../../..", "", "."));
  EXPECT_EQ("C:/path/to/external/dir/",
            RebaseOne(scope, "//../external/dir/", "", "//"));

#else
  setup.build_settings()->SetRootPath(base::FilePath("/path/to/src"));
  EXPECT_EQ("/path/to/src", RebaseOne(scope, ".", "", "//"));
  EXPECT_EQ("/path/to/src/", RebaseOne(scope, "//", "", "//"));
  EXPECT_EQ("/path/to/src/foo", RebaseOne(scope, "foo", "", "//"));
  EXPECT_EQ("/path/to/src/foo/", RebaseOne(scope, "foo/", "", "//"));
  EXPECT_EQ("/path/to/src/tools/gn/foo", RebaseOne(scope, "foo", "", "."));
  EXPECT_EQ("/path/to/other/tools",
            RebaseOne(scope, "//../other/tools", "", "//"));
  EXPECT_EQ("/path/to/src/foo/bar",
            RebaseOne(scope, "//../src/foo/bar", "", "//"));
  EXPECT_EQ("/path/to", RebaseOne(scope, "//..", "", "//"));
  EXPECT_EQ("/path", RebaseOne(scope, "../../../..", "", "."));
  EXPECT_EQ("/path/to/external/dir/",
            RebaseOne(scope, "//../external/dir/", "", "//"));
#endif
}

TEST(RebasePath, StringsSystemPaths) {
  TestWithScope setup;
  Scope* scope = setup.scope();

#if defined(OS_WIN)
  setup.build_settings()->SetBuildDir(SourceDir("C:/ssd/out/Debug"));
  setup.build_settings()->SetRootPath(base::FilePath(L"C:/hdd/src"));

  // Test system absolute to-dir.
  EXPECT_EQ("../../ssd/out/Debug",
            RebaseOne(scope, ".", "//", "C:/ssd/out/Debug"));
  EXPECT_EQ("../../ssd/out/Debug/",
            RebaseOne(scope, "./", "//", "C:/ssd/out/Debug"));
  EXPECT_EQ("../../ssd/out/Debug/foo",
            RebaseOne(scope, "foo", "//", "C:/ssd/out/Debug"));
  EXPECT_EQ("../../ssd/out/Debug/foo/",
            RebaseOne(scope, "foo/", "//", "C:/ssd/out/Debug"));

  // Test system absolute from-dir.
  EXPECT_EQ("../../../hdd/src",
            RebaseOne(scope, ".", "C:/ssd/out/Debug", "//"));
  EXPECT_EQ("../../../hdd/src/",
            RebaseOne(scope, "./", "C:/ssd/out/Debug", "//"));
  EXPECT_EQ("../../../hdd/src/foo",
            RebaseOne(scope, "foo", "C:/ssd/out/Debug", "//"));
  EXPECT_EQ("../../../hdd/src/foo/",
            RebaseOne(scope, "foo/", "C:/ssd/out/Debug", "//"));
#else
  setup.build_settings()->SetBuildDir(SourceDir("/ssd/out/Debug"));
  setup.build_settings()->SetRootPath(base::FilePath("/hdd/src"));

  // Test system absolute to-dir.
  EXPECT_EQ("../../ssd/out/Debug",
            RebaseOne(scope, ".", "//", "/ssd/out/Debug"));
  EXPECT_EQ("../../ssd/out/Debug/",
            RebaseOne(scope, "./", "//", "/ssd/out/Debug"));
  EXPECT_EQ("../../ssd/out/Debug/foo",
            RebaseOne(scope, "foo", "//", "/ssd/out/Debug"));
  EXPECT_EQ("../../ssd/out/Debug/foo/",
            RebaseOne(scope, "foo/", "//", "/ssd/out/Debug"));

  // Test system absolute from-dir.
  EXPECT_EQ("../../../hdd/src",
            RebaseOne(scope, ".", "/ssd/out/Debug", "//"));
  EXPECT_EQ("../../../hdd/src/",
            RebaseOne(scope, "./", "/ssd/out/Debug", "//"));
  EXPECT_EQ("../../../hdd/src/foo",
            RebaseOne(scope, "foo", "/ssd/out/Debug", "//"));
  EXPECT_EQ("../../../hdd/src/foo/",
            RebaseOne(scope, "foo/", "/ssd/out/Debug", "//"));
#endif
}

// Test list input.
TEST(RebasePath, List) {
  TestWithScope setup;
  setup.scope()->set_source_dir(SourceDir("//tools/gn/"));

  std::vector<Value> args;
  args.push_back(Value(nullptr, Value::LIST));
  args[0].list_value().push_back(Value(nullptr, "foo.txt"));
  args[0].list_value().push_back(Value(nullptr, "bar.txt"));
  args.push_back(Value(nullptr, "//out/Debug/"));
  args.push_back(Value(nullptr, "."));

  Err err;
  FunctionCallNode function;
  Value ret = functions::RunRebasePath(setup.scope(), &function, args, &err);
  EXPECT_FALSE(err.has_error());

  ASSERT_EQ(Value::LIST, ret.type());
  ASSERT_EQ(2u, ret.list_value().size());

  EXPECT_EQ("../../tools/gn/foo.txt", ret.list_value()[0].string_value());
  EXPECT_EQ("../../tools/gn/bar.txt", ret.list_value()[1].string_value());
}

TEST(RebasePath, Errors) {
  TestWithScope setup;

  // No arg input should issue an error.
  Err err;
  std::vector<Value> args;
  FunctionCallNode function;
  Value ret = functions::RunRebasePath(setup.scope(), &function, args, &err);
  EXPECT_TRUE(err.has_error());
}
