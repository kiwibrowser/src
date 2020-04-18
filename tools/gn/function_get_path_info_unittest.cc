// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/functions.h"
#include "tools/gn/test_with_scope.h"

namespace {

class GetPathInfoTest : public testing::Test {
 public:
  GetPathInfoTest() : testing::Test() {
    setup_.scope()->set_source_dir(SourceDir("//src/foo/"));
  }

  // Convenience wrapper to call GetLabelInfo.
  std::string Call(const std::string& input, const std::string& what) {
    FunctionCallNode function;

    std::vector<Value> args;
    args.push_back(Value(nullptr, input));
    args.push_back(Value(nullptr, what));

    Err err;
    Value result = functions::RunGetPathInfo(setup_.scope(), &function,
                                             args, &err);
    if (err.has_error()) {
      EXPECT_TRUE(result.type() == Value::NONE);
      return std::string();
    }
    return result.string_value();
  }

 protected:
  TestWithScope setup_;
};

}  // namespace

TEST_F(GetPathInfoTest, File) {
  EXPECT_EQ("bar.txt", Call("foo/bar.txt", "file"));
  EXPECT_EQ("bar.txt", Call("bar.txt", "file"));
  EXPECT_EQ("bar.txt", Call("/bar.txt", "file"));
  EXPECT_EQ("", Call("foo/", "file"));
  EXPECT_EQ("", Call("//", "file"));
  EXPECT_EQ("", Call("/", "file"));
}

TEST_F(GetPathInfoTest, Name) {
  EXPECT_EQ("bar", Call("foo/bar.txt", "name"));
  EXPECT_EQ("bar", Call("bar.", "name"));
  EXPECT_EQ("", Call("/.txt", "name"));
  EXPECT_EQ("", Call("foo/", "name"));
  EXPECT_EQ("", Call("//", "name"));
  EXPECT_EQ("", Call("/", "name"));
}

TEST_F(GetPathInfoTest, Extension) {
  EXPECT_EQ("txt", Call("foo/bar.txt", "extension"));
  EXPECT_EQ("", Call("bar.", "extension"));
  EXPECT_EQ("txt", Call("/.txt", "extension"));
  EXPECT_EQ("", Call("f.oo/", "extension"));
  EXPECT_EQ("", Call("//", "extension"));
  EXPECT_EQ("", Call("/", "extension"));
}

TEST_F(GetPathInfoTest, Dir) {
  EXPECT_EQ("foo", Call("foo/bar.txt", "dir"));
  EXPECT_EQ(".", Call("bar.txt", "dir"));
  EXPECT_EQ("foo/bar", Call("foo/bar/baz", "dir"));
  EXPECT_EQ("//foo", Call("//foo/", "dir"));
  EXPECT_EQ("//.", Call("//", "dir"));
  EXPECT_EQ("/foo", Call("/foo/", "dir"));
  EXPECT_EQ("/.", Call("/", "dir"));
}

// Note "current dir" is "//src/foo"
TEST_F(GetPathInfoTest, AbsPath) {
  EXPECT_EQ("//src/foo/foo/bar.txt", Call("foo/bar.txt", "abspath"));
  EXPECT_EQ("//src/foo/bar.txt", Call("bar.txt", "abspath"));
  EXPECT_EQ("//src/foo/bar/", Call("bar/", "abspath"));
  EXPECT_EQ("//foo", Call("//foo", "abspath"));
  EXPECT_EQ("//foo/", Call("//foo/", "abspath"));
  EXPECT_EQ("//", Call("//", "abspath"));
  EXPECT_EQ("/foo", Call("/foo", "abspath"));
  EXPECT_EQ("/foo/", Call("/foo/", "abspath"));
  EXPECT_EQ("/", Call("/", "abspath"));
}

// Note build dir is "//out/Debug/".
TEST_F(GetPathInfoTest, OutDir) {
  EXPECT_EQ("//out/Debug/obj/src/foo/foo", Call("foo/bar.txt", "out_dir"));
  EXPECT_EQ("//out/Debug/obj/src/foo/bar", Call("bar/", "out_dir"));
  EXPECT_EQ("//out/Debug/obj/src/foo", Call(".", "out_dir"));
  EXPECT_EQ("//out/Debug/obj/src/foo", Call("bar", "out_dir"));
  EXPECT_EQ("//out/Debug/obj/foo", Call("//foo/bar.txt", "out_dir"));
  // System paths go into the ABS_PATH obj directory.
  EXPECT_EQ("//out/Debug/obj/ABS_PATH/foo", Call("/foo/bar.txt", "out_dir"));
#if defined(OS_WIN)
  EXPECT_EQ("//out/Debug/obj/ABS_PATH/C/foo",
            Call("/C:/foo/bar.txt", "out_dir"));
#endif
}

// Note build dir is "//out/Debug/".
TEST_F(GetPathInfoTest, GenDir) {
  EXPECT_EQ("//out/Debug/gen/src/foo/foo", Call("foo/bar.txt", "gen_dir"));
  EXPECT_EQ("//out/Debug/gen/src/foo/bar", Call("bar/", "gen_dir"));
  EXPECT_EQ("//out/Debug/gen/src/foo", Call(".", "gen_dir"));
  EXPECT_EQ("//out/Debug/gen/src/foo", Call("bar", "gen_dir"));
  EXPECT_EQ("//out/Debug/gen/foo", Call("//foo/bar.txt", "gen_dir"));
  // System paths go into the ABS_PATH gen directory
  EXPECT_EQ("//out/Debug/gen/ABS_PATH/foo", Call("/foo/bar.txt", "gen_dir"));
#if defined(OS_WIN)
  EXPECT_EQ("//out/Debug/gen/ABS_PATH/C/foo",
            Call("/C:/foo/bar.txt", "gen_dir"));
#endif
}
