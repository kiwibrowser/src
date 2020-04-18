// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/c_include_iterator.h"
#include "tools/gn/input_file.h"
#include "tools/gn/location.h"

namespace {

bool RangeIs(const LocationRange& range,
             int line, int begin_char, int end_char) {
  return range.begin().line_number() == line &&
         range.end().line_number() == line &&
         range.begin().column_number() == begin_char &&
         range.end().column_number() == end_char;
}

}  // namespace

TEST(CIncludeIterator, Basic) {
  std::string buffer;
  buffer.append("// Some comment\n");
  buffer.append("\n");
  buffer.append("#include \"foo/bar.h\"\n");
  buffer.append("\n");
  buffer.append("#include <stdio.h>\n");
  buffer.append("\n");
  buffer.append(" #include \"foo/baz.h\"\n");  // Leading whitespace
  buffer.append("#include \"la/deda.h\"\n");
  // Line annotated with "// nogncheck"
  buffer.append("#include \"should_be_skipped.h\"  // nogncheck\n");
  buffer.append("#import \"weird_mac_import.h\"\n");
  buffer.append("\n");
  buffer.append("void SomeCode() {\n");

  InputFile file(SourceFile("//foo.cc"));
  file.SetContents(buffer);

  CIncludeIterator iter(&file);

  base::StringPiece contents;
  LocationRange range;
  EXPECT_TRUE(iter.GetNextIncludeString(&contents, &range));
  EXPECT_EQ("foo/bar.h", contents);
  EXPECT_TRUE(RangeIs(range, 3, 11, 20)) << range.begin().Describe(true);

  EXPECT_TRUE(iter.GetNextIncludeString(&contents, &range));
  EXPECT_EQ("foo/baz.h", contents);
  EXPECT_TRUE(RangeIs(range, 7, 12, 21)) << range.begin().Describe(true);

  EXPECT_TRUE(iter.GetNextIncludeString(&contents, &range));
  EXPECT_EQ("la/deda.h", contents);
  EXPECT_TRUE(RangeIs(range, 8, 11, 20)) << range.begin().Describe(true);

  // The line annotated with "nogncheck" should be skipped.

  EXPECT_TRUE(iter.GetNextIncludeString(&contents, &range));
  EXPECT_EQ("weird_mac_import.h", contents);
  EXPECT_TRUE(RangeIs(range, 10, 10, 28)) << range.begin().Describe(true);

  EXPECT_FALSE(iter.GetNextIncludeString(&contents, &range));
}

// Tests that we don't search for includes indefinitely.
TEST(CIncludeIterator, GiveUp) {
  std::string buffer;
  for (size_t i = 0; i < 1000; i++)
    buffer.append("x\n");
  buffer.append("#include \"foo/bar.h\"\n");

  InputFile file(SourceFile("//foo.cc"));
  file.SetContents(buffer);

  base::StringPiece contents;
  LocationRange range;

  CIncludeIterator iter(&file);
  EXPECT_FALSE(iter.GetNextIncludeString(&contents, &range));
  EXPECT_TRUE(contents.empty());
}

// Don't count blank lines, comments, and preprocessor when giving up.
TEST(CIncludeIterator, DontGiveUp) {
  std::string buffer;
  for (size_t i = 0; i < 1000; i++)
    buffer.push_back('\n');
  for (size_t i = 0; i < 1000; i++)
    buffer.append("// comment\n");
  for (size_t i = 0; i < 1000; i++)
    buffer.append("#preproc\n");
  buffer.append("#include \"foo/bar.h\"\n");

  InputFile file(SourceFile("//foo.cc"));
  file.SetContents(buffer);

  base::StringPiece contents;
  LocationRange range;

  CIncludeIterator iter(&file);
  EXPECT_TRUE(iter.GetNextIncludeString(&contents, &range));
  EXPECT_EQ("foo/bar.h", contents);
}

// Tests that we'll tolerate some small numbers of non-includes interspersed
// with real includes.
TEST(CIncludeIterator, TolerateNonIncludes) {
  const size_t kSkip = CIncludeIterator::kMaxNonIncludeLines - 2;
  const size_t kGroupCount = 100;

  std::string include("foo/bar.h");

  // Allow a series of includes with blanks in between.
  std::string buffer;
  for (size_t group = 0; group < kGroupCount; group++) {
    for (size_t i = 0; i < kSkip; i++)
      buffer.append("foo\n");
    buffer.append("#include \"" + include + "\"\n");
  }

  InputFile file(SourceFile("//foo.cc"));
  file.SetContents(buffer);

  base::StringPiece contents;
  LocationRange range;

  CIncludeIterator iter(&file);
  for (size_t group = 0; group < kGroupCount; group++) {
    EXPECT_TRUE(iter.GetNextIncludeString(&contents, &range));
    EXPECT_EQ(include, contents.as_string());
  }
  EXPECT_FALSE(iter.GetNextIncludeString(&contents, &range));
}

// Tests that comments of the form
//    /*
//     *
//     */
// are not counted toward the non-include line count.
TEST(CIncludeIterator, CStyleComments) {
  std::string buffer("/*");
  for (size_t i = 0; i < 1000; i++)
    buffer.append(" *\n");
  buffer.append(" */\n\n");
  buffer.append("#include \"foo/bar.h\"\n");

  InputFile file(SourceFile("//foo.cc"));
  file.SetContents(buffer);

  base::StringPiece contents;
  LocationRange range;

  CIncludeIterator iter(&file);
  EXPECT_TRUE(iter.GetNextIncludeString(&contents, &range));
  EXPECT_EQ("foo/bar.h", contents);
}
