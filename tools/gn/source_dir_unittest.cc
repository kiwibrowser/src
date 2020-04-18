// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/err.h"
#include "tools/gn/source_dir.h"
#include "tools/gn/source_file.h"
#include "tools/gn/value.h"

TEST(SourceDir, ResolveRelativeFile) {
  Err err;
  SourceDir base("//base/");
#if defined(OS_WIN)
  base::StringPiece source_root("C:/source/root");
#else
  base::StringPiece source_root("/source/root");
#endif

  // Empty input is an error.
  EXPECT_TRUE(base.ResolveRelativeFile(
      Value(nullptr, std::string()), &err, source_root) == SourceFile());
  EXPECT_TRUE(err.has_error());

  // These things are directories, so should be an error.
  err = Err();
  EXPECT_TRUE(base.ResolveRelativeFile(
      Value(nullptr, "//foo/bar/"), &err, source_root) == SourceFile());
  EXPECT_TRUE(err.has_error());

  err = Err();
  EXPECT_TRUE(base.ResolveRelativeFile(
      Value(nullptr, "bar/"), &err, source_root) == SourceFile());
  EXPECT_TRUE(err.has_error());

  // Absolute paths should be passed unchanged.
  err = Err();
  EXPECT_TRUE(base.ResolveRelativeFile(
      Value(nullptr, "//foo"), &err, source_root) == SourceFile("//foo"));
  EXPECT_FALSE(err.has_error());

  EXPECT_TRUE(base.ResolveRelativeFile(
      Value(nullptr, "/foo"), &err, source_root) == SourceFile("/foo"));
  EXPECT_FALSE(err.has_error());

  // Basic relative stuff.
  EXPECT_TRUE(base.ResolveRelativeFile(
      Value(nullptr, "foo"), &err, source_root) == SourceFile("//base/foo"));
  EXPECT_FALSE(err.has_error());
  EXPECT_TRUE(base.ResolveRelativeFile(
      Value(nullptr, "./foo"), &err, source_root) == SourceFile("//base/foo"));
  EXPECT_FALSE(err.has_error());
  EXPECT_TRUE(base.ResolveRelativeFile(
      Value(nullptr, "../foo"), &err, source_root) == SourceFile("//foo"));
  EXPECT_FALSE(err.has_error());

  // If the given relative path points outside the source root, we
  // expect an absolute path.
#if defined(OS_WIN)
  EXPECT_TRUE(base.ResolveRelativeFile(
          Value(nullptr, "../../foo"), &err, source_root) ==
      SourceFile("/C:/source/foo"));
  EXPECT_FALSE(err.has_error());

  EXPECT_TRUE(base.ResolveRelativeFile(
          Value(nullptr, "//../foo"), &err, source_root) ==
      SourceFile("/C:/source/foo"));
  EXPECT_FALSE(err.has_error());

  EXPECT_TRUE(base.ResolveRelativeFile(
          Value(nullptr, "//../root/foo"), &err, source_root) ==
      SourceFile("/C:/source/root/foo"));
  EXPECT_FALSE(err.has_error());

  EXPECT_TRUE(base.ResolveRelativeFile(
          Value(nullptr, "//../../../foo/bar"), &err, source_root) ==
      SourceFile("/foo/bar"));
  EXPECT_FALSE(err.has_error());
#else
  EXPECT_TRUE(base.ResolveRelativeFile(
          Value(nullptr, "../../foo"), &err, source_root) ==
      SourceFile("/source/foo"));
  EXPECT_FALSE(err.has_error());

  EXPECT_TRUE(base.ResolveRelativeFile(
          Value(nullptr, "//../foo"), &err, source_root) ==
      SourceFile("/source/foo"));
  EXPECT_FALSE(err.has_error());

  EXPECT_TRUE(base.ResolveRelativeFile(
          Value(nullptr, "//../root/foo"), &err, source_root) ==
      SourceFile("/source/root/foo"));
  EXPECT_FALSE(err.has_error());

  EXPECT_TRUE(base.ResolveRelativeFile(
          Value(nullptr, "//../../../foo/bar"), &err, source_root) ==
      SourceFile("/foo/bar"));
  EXPECT_FALSE(err.has_error());
#endif

#if defined(OS_WIN)
  // Note that we don't canonicalize the backslashes to forward slashes.
  // This could potentially be changed in the future which would mean we should
  // just change the expected result.
  EXPECT_TRUE(base.ResolveRelativeFile(
          Value(nullptr, "C:\\foo\\bar.txt"), &err, source_root) ==
      SourceFile("/C:/foo/bar.txt"));
  EXPECT_FALSE(err.has_error());
#endif
}

TEST(SourceDir, ResolveRelativeDir) {
  Err err;
  SourceDir base("//base/");
#if defined(OS_WIN)
  base::StringPiece source_root("C:/source/root");
#else
  base::StringPiece source_root("/source/root");
#endif

  // Empty input is an error.
  EXPECT_TRUE(base.ResolveRelativeDir(
      Value(nullptr, std::string()), &err, source_root) == SourceDir());
  EXPECT_TRUE(err.has_error());

  // Absolute paths should be passed unchanged.
  err = Err();
  EXPECT_TRUE(base.ResolveRelativeDir(
      Value(nullptr, "//foo"), &err, source_root) == SourceDir("//foo/"));
  EXPECT_FALSE(err.has_error());
  EXPECT_TRUE(base.ResolveRelativeDir(
      Value(nullptr, "/foo"), &err, source_root) == SourceDir("/foo/"));
  EXPECT_FALSE(err.has_error());

  // Basic relative stuff.
  EXPECT_TRUE(base.ResolveRelativeDir(
      Value(nullptr, "foo"), &err, source_root) == SourceDir("//base/foo/"));
  EXPECT_FALSE(err.has_error());
  EXPECT_TRUE(base.ResolveRelativeDir(
      Value(nullptr, "./foo"), &err, source_root) == SourceDir("//base/foo/"));
  EXPECT_FALSE(err.has_error());
  EXPECT_TRUE(base.ResolveRelativeDir(
      Value(nullptr, "../foo"), &err, source_root) == SourceDir("//foo/"));
  EXPECT_FALSE(err.has_error());

  // If the given relative path points outside the source root, we
  // expect an absolute path.
#if defined(OS_WIN)
  EXPECT_TRUE(base.ResolveRelativeDir(
          Value(nullptr, "../../foo"), &err, source_root) ==
      SourceDir("/C:/source/foo/"));
  EXPECT_FALSE(err.has_error());
  EXPECT_TRUE(base.ResolveRelativeDir(
          Value(nullptr, "//../foo"), &err, source_root) ==
      SourceDir("/C:/source/foo/"));
  EXPECT_FALSE(err.has_error());
  EXPECT_TRUE(base.ResolveRelativeDir(
          Value(nullptr, "//.."), &err, source_root) ==
      SourceDir("/C:/source/"));
  EXPECT_FALSE(err.has_error());
#else
  EXPECT_TRUE(base.ResolveRelativeDir(
          Value(nullptr, "../../foo"), &err, source_root) ==
      SourceDir("/source/foo/"));
  EXPECT_FALSE(err.has_error());
  EXPECT_TRUE(base.ResolveRelativeDir(
          Value(nullptr, "//../foo"), &err, source_root) ==
      SourceDir("/source/foo/"));
  EXPECT_FALSE(err.has_error());
  EXPECT_TRUE(base.ResolveRelativeDir(
          Value(nullptr, "//.."), &err, source_root) ==
      SourceDir("/source/"));
  EXPECT_FALSE(err.has_error());
#endif

#if defined(OS_WIN)
  // Canonicalize the existing backslashes to forward slashes and add a
  // leading slash if necessary.
  EXPECT_TRUE(base.ResolveRelativeDir(
      Value(nullptr, "\\C:\\foo"), &err) == SourceDir("/C:/foo/"));
  EXPECT_FALSE(err.has_error());
  EXPECT_TRUE(base.ResolveRelativeDir(
      Value(nullptr, "C:\\foo"), &err) == SourceDir("/C:/foo/"));
  EXPECT_FALSE(err.has_error());
#endif
}
