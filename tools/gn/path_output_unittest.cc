// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sstream>

#include "base/files/file_path.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/output_file.h"
#include "tools/gn/path_output.h"
#include "tools/gn/source_dir.h"
#include "tools/gn/source_file.h"

TEST(PathOutput, Basic) {
  SourceDir build_dir("//out/Debug/");
  base::StringPiece source_root("/source/root");
  PathOutput writer(build_dir, source_root, ESCAPE_NONE);
  {
    // Normal source-root path.
    std::ostringstream out;
    writer.WriteFile(out, SourceFile("//foo/bar.cc"));
    EXPECT_EQ("../../foo/bar.cc", out.str());
  }
  {
    // File in the root dir.
    std::ostringstream out;
    writer.WriteFile(out, SourceFile("//foo.cc"));
    EXPECT_EQ("../../foo.cc", out.str());
  }
  {
    // Files in the output dir.
    std::ostringstream out;
    writer.WriteFile(out, SourceFile("//out/Debug/foo.cc"));
    out << " ";
    writer.WriteFile(out, SourceFile("//out/Debug/bar/baz.cc"));
    EXPECT_EQ("foo.cc bar/baz.cc", out.str());
  }
#if defined(OS_WIN)
  {
    // System-absolute path.
    std::ostringstream out;
    writer.WriteFile(out, SourceFile("/C:/foo/bar.cc"));
    EXPECT_EQ("C:/foo/bar.cc", out.str());
  }
#else
  {
    // System-absolute path.
    std::ostringstream out;
    writer.WriteFile(out, SourceFile("/foo/bar.cc"));
    EXPECT_EQ("/foo/bar.cc", out.str());
  }
#endif
}

// Same as basic but the output dir is the root.
TEST(PathOutput, BasicInRoot) {
  SourceDir build_dir("//");
  base::StringPiece source_root("/source/root");
  PathOutput writer(build_dir, source_root, ESCAPE_NONE);
  {
    // Normal source-root path.
    std::ostringstream out;
    writer.WriteFile(out, SourceFile("//foo/bar.cc"));
    EXPECT_EQ("foo/bar.cc", out.str());
  }
  {
    // File in the root dir.
    std::ostringstream out;
    writer.WriteFile(out, SourceFile("//foo.cc"));
    EXPECT_EQ("foo.cc", out.str());
  }
}

TEST(PathOutput, NinjaEscaping) {
  SourceDir build_dir("//out/Debug/");
  base::StringPiece source_root("/source/root");
  PathOutput writer(build_dir, source_root, ESCAPE_NINJA);
  {
    // Spaces and $ in filenames.
    std::ostringstream out;
    writer.WriteFile(out, SourceFile("//foo/foo bar$.cc"));
    EXPECT_EQ("../../foo/foo$ bar$$.cc", out.str());
  }
  {
    // Not other weird stuff
    std::ostringstream out;
    writer.WriteFile(out, SourceFile("//foo/\"foo\".cc"));
    EXPECT_EQ("../../foo/\"foo\".cc", out.str());
  }
}

TEST(PathOutput, NinjaForkEscaping) {
  SourceDir build_dir("//out/Debug/");
  base::StringPiece source_root("/source/root");
  PathOutput writer(build_dir, source_root, ESCAPE_NINJA_COMMAND);

  // Spaces in filenames should get quoted on Windows.
  writer.set_escape_platform(ESCAPE_PLATFORM_WIN);
  {
    std::ostringstream out;
    writer.WriteFile(out, SourceFile("//foo/foo bar.cc"));
    EXPECT_EQ("\"../../foo/foo$ bar.cc\"", out.str());
  }

  // Spaces in filenames should get escaped on Posix.
  writer.set_escape_platform(ESCAPE_PLATFORM_POSIX);
  {
    std::ostringstream out;
    writer.WriteFile(out, SourceFile("//foo/foo bar.cc"));
    EXPECT_EQ("../../foo/foo\\$ bar.cc", out.str());
  }

  // Quotes should get blackslash-escaped on Windows and Posix.
  writer.set_escape_platform(ESCAPE_PLATFORM_WIN);
  {
    std::ostringstream out;
    writer.WriteFile(out, SourceFile("//foo/\"foobar\".cc"));
    // Our Windows code currently quotes the whole thing in this case for
    // code simplicity, even though it's strictly unnecessary. This might
    // change in the future.
    EXPECT_EQ("\"../../foo/\\\"foobar\\\".cc\"", out.str());
  }
  writer.set_escape_platform(ESCAPE_PLATFORM_POSIX);
  {
    std::ostringstream out;
    writer.WriteFile(out, SourceFile("//foo/\"foobar\".cc"));
    EXPECT_EQ("../../foo/\\\"foobar\\\".cc", out.str());
  }

  // Backslashes should get escaped on non-Windows and preserved on Windows.
  writer.set_escape_platform(ESCAPE_PLATFORM_WIN);
  {
    std::ostringstream out;
    writer.WriteFile(out, OutputFile("foo\\bar.cc"));
    EXPECT_EQ("foo\\bar.cc", out.str());
  }
  writer.set_escape_platform(ESCAPE_PLATFORM_POSIX);
  {
    std::ostringstream out;
    writer.WriteFile(out, OutputFile("foo\\bar.cc"));
    EXPECT_EQ("foo\\\\bar.cc", out.str());
  }
}

TEST(PathOutput, InhibitQuoting) {
  SourceDir build_dir("//out/Debug/");
  base::StringPiece source_root("/source/root");
  PathOutput writer(build_dir, source_root, ESCAPE_NINJA_COMMAND);
  writer.set_inhibit_quoting(true);

  writer.set_escape_platform(ESCAPE_PLATFORM_WIN);
  {
    // We should get unescaped spaces in the output with no quotes.
    std::ostringstream out;
    writer.WriteFile(out, SourceFile("//foo/foo bar.cc"));
    EXPECT_EQ("../../foo/foo$ bar.cc", out.str());
  }

  writer.set_escape_platform(ESCAPE_PLATFORM_POSIX);
  {
    // Escapes the space.
    std::ostringstream out;
    writer.WriteFile(out, SourceFile("//foo/foo bar.cc"));
    EXPECT_EQ("../../foo/foo\\$ bar.cc", out.str());
  }
}

TEST(PathOutput, WriteDir) {
  {
    SourceDir build_dir("//out/Debug/");
    base::StringPiece source_root("/source/root");
    PathOutput writer(build_dir, source_root, ESCAPE_NINJA);
    {
      std::ostringstream out;
      writer.WriteDir(out, SourceDir("//foo/bar/"),
                      PathOutput::DIR_INCLUDE_LAST_SLASH);
      EXPECT_EQ("../../foo/bar/", out.str());
    }
    {
      std::ostringstream out;
      writer.WriteDir(out, SourceDir("//foo/bar/"),
                      PathOutput::DIR_NO_LAST_SLASH);
      EXPECT_EQ("../../foo/bar", out.str());
    }

    // Output source root dir.
    {
      std::ostringstream out;
      writer.WriteDir(out, SourceDir("//"),
                      PathOutput::DIR_INCLUDE_LAST_SLASH);
      EXPECT_EQ("../../", out.str());
    }
    {
      std::ostringstream out;
      writer.WriteDir(out, SourceDir("//"),
                      PathOutput::DIR_NO_LAST_SLASH);
      EXPECT_EQ("../..", out.str());
    }

    // Output system root dir.
    {
      std::ostringstream out;
      writer.WriteDir(out, SourceDir("/"),
                      PathOutput::DIR_INCLUDE_LAST_SLASH);
      EXPECT_EQ("/", out.str());
    }
    {
      std::ostringstream out;
      writer.WriteDir(out, SourceDir("/"),
                      PathOutput::DIR_INCLUDE_LAST_SLASH);
      EXPECT_EQ("/", out.str());
    }
    {
      std::ostringstream out;
      writer.WriteDir(out, SourceDir("/"),
                      PathOutput::DIR_NO_LAST_SLASH);
      EXPECT_EQ("/.", out.str());
    }

    // Output inside current dir.
    {
      std::ostringstream out;
      writer.WriteDir(out, SourceDir("//out/Debug/"),
                      PathOutput::DIR_INCLUDE_LAST_SLASH);
      EXPECT_EQ("./", out.str());
    }
    {
      std::ostringstream out;
      writer.WriteDir(out, SourceDir("//out/Debug/"),
                      PathOutput::DIR_NO_LAST_SLASH);
      EXPECT_EQ(".", out.str());
    }
    {
      std::ostringstream out;
      writer.WriteDir(out, SourceDir("//out/Debug/foo/"),
                      PathOutput::DIR_INCLUDE_LAST_SLASH);
      EXPECT_EQ("foo/", out.str());
    }
    {
      std::ostringstream out;
      writer.WriteDir(out, SourceDir("//out/Debug/foo/"),
                      PathOutput::DIR_NO_LAST_SLASH);
      EXPECT_EQ("foo", out.str());
    }

    // WriteDir using an OutputFile.
    {
      std::ostringstream out;
      writer.WriteDir(out, OutputFile("foo/"),
                      PathOutput::DIR_INCLUDE_LAST_SLASH);
      EXPECT_EQ("foo/", out.str());
    }
    {
      std::ostringstream out;
      writer.WriteDir(out, OutputFile("foo/"),
                      PathOutput::DIR_NO_LAST_SLASH);
      EXPECT_EQ("foo", out.str());
    }
    {
      std::ostringstream out;
      writer.WriteDir(out, OutputFile(),
                      PathOutput::DIR_INCLUDE_LAST_SLASH);
      EXPECT_EQ("", out.str());
    }
  }
  {
    // Empty build dir writer.
    base::StringPiece source_root("/source/root");
    PathOutput root_writer(SourceDir("//"), source_root, ESCAPE_NINJA);
    {
      std::ostringstream out;
      root_writer.WriteDir(out, SourceDir("//"),
                           PathOutput::DIR_INCLUDE_LAST_SLASH);
      EXPECT_EQ("./", out.str());
    }
    {
      std::ostringstream out;
      root_writer.WriteDir(out, SourceDir("//"),
                           PathOutput::DIR_NO_LAST_SLASH);
      EXPECT_EQ(".", out.str());
    }
  }
}
