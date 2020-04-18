// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The original source code is from:
// http://src.chromium.org/viewvc/chrome/trunk/src/base/strings/string_split_unittest.cc?revision=216633

#include "util/string_split.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace {

using i18n::addressinput::SplitString;

TEST(StringSplitTest, SplitString) {
  std::vector<std::string> r;

  SplitString(std::string(), ',', &r);
  EXPECT_EQ(0U, r.size());

  SplitString("a,b,c", ',', &r);
  ASSERT_EQ(3U, r.size());
  EXPECT_EQ(r[0], "a");
  EXPECT_EQ(r[1], "b");
  EXPECT_EQ(r[2], "c");

  SplitString("a, b, c", ',', &r);
  ASSERT_EQ(3U, r.size());
  EXPECT_EQ(r[0], "a");
  EXPECT_EQ(r[1], " b");
  EXPECT_EQ(r[2], " c");

  SplitString("a,,c", ',', &r);
  ASSERT_EQ(3U, r.size());
  EXPECT_EQ(r[0], "a");
  EXPECT_EQ(r[1], "");
  EXPECT_EQ(r[2], "c");

  SplitString("   ", '*', &r);
  EXPECT_EQ(1U, r.size());

  SplitString("foo", '*', &r);
  ASSERT_EQ(1U, r.size());
  EXPECT_EQ(r[0], "foo");

  SplitString("foo ,", ',', &r);
  ASSERT_EQ(2U, r.size());
  EXPECT_EQ(r[0], "foo ");
  EXPECT_EQ(r[1], "");

  SplitString(",", ',', &r);
  ASSERT_EQ(2U, r.size());
  EXPECT_EQ(r[0], "");
  EXPECT_EQ(r[1], "");

  SplitString("\t\ta\t", '\t', &r);
  ASSERT_EQ(4U, r.size());
  EXPECT_EQ(r[0], "");
  EXPECT_EQ(r[1], "");
  EXPECT_EQ(r[2], "a");
  EXPECT_EQ(r[3], "");

  SplitString("\ta\t\nb\tcc", '\n', &r);
  ASSERT_EQ(2U, r.size());
  EXPECT_EQ(r[0], "\ta\t");
  EXPECT_EQ(r[1], "b\tcc");

  SplitString("   ", '*', &r);
  ASSERT_EQ(1U, r.size());
  EXPECT_EQ(r[0], "   ");

  SplitString("\t  \ta\t ", '\t', &r);
  ASSERT_EQ(4U, r.size());
  EXPECT_EQ(r[0], "");
  EXPECT_EQ(r[1], "  ");
  EXPECT_EQ(r[2], "a");
  EXPECT_EQ(r[3], " ");

  SplitString("\ta\t\nb\tcc", '\n', &r);
  ASSERT_EQ(2U, r.size());
  EXPECT_EQ(r[0], "\ta\t");
  EXPECT_EQ(r[1], "b\tcc");
}

}  // namespace
