// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/escape.h"

TEST(Escape, Ninja) {
  EscapeOptions opts;
  opts.mode = ESCAPE_NINJA;
  std::string result = EscapeString("asdf: \"$\\bar", opts, nullptr);
  EXPECT_EQ("asdf$:$ \"$$\\bar", result);
}

TEST(Escape, WindowsCommand) {
  EscapeOptions opts;
  opts.mode = ESCAPE_NINJA_COMMAND;
  opts.platform = ESCAPE_PLATFORM_WIN;

  // Regular string is passed, even if it has backslashes.
  EXPECT_EQ("foo\\bar", EscapeString("foo\\bar", opts, nullptr));

  // Spaces means the string is quoted, normal backslahes untouched.
  bool needs_quoting = false;
  EXPECT_EQ("\"foo\\$ bar\"", EscapeString("foo\\ bar", opts, &needs_quoting));
  EXPECT_TRUE(needs_quoting);

  // Inhibit quoting.
  needs_quoting = false;
  opts.inhibit_quoting = true;
  EXPECT_EQ("foo\\$ bar", EscapeString("foo\\ bar", opts, &needs_quoting));
  EXPECT_TRUE(needs_quoting);
  opts.inhibit_quoting = false;

  // Backslashes at the end of the string get escaped.
  EXPECT_EQ("\"foo$ bar\\\\\\\\\"", EscapeString("foo bar\\\\", opts, nullptr));

  // Backslashes preceding quotes are escaped, and the quote is escaped.
  EXPECT_EQ("\"foo\\\\\\\"$ bar\"", EscapeString("foo\\\" bar", opts, nullptr));
}

TEST(Escape, PosixCommand) {
  EscapeOptions opts;
  opts.mode = ESCAPE_NINJA_COMMAND;
  opts.platform = ESCAPE_PLATFORM_POSIX;

  // : and $ ninja escaped with $. Then Shell-escape backslashes and quotes.
  EXPECT_EQ("a$:\\$ \\\"\\$$\\\\b", EscapeString("a: \"$\\b", opts, nullptr));

  // Some more generic shell chars.
  EXPECT_EQ("a_\\;\\<\\*b", EscapeString("a_;<*b", opts, nullptr));

  // Curly braces must be escaped to avoid brace expansion on systems using
  // bash as default shell..
  EXPECT_EQ("\\{a,b\\}\\{c,d\\}", EscapeString("{a,b}{c,d}", opts, nullptr));
}

TEST(Escape, NinjaPreformatted) {
  EscapeOptions opts;
  opts.mode = ESCAPE_NINJA_PREFORMATTED_COMMAND;

  // Only $ is escaped.
  EXPECT_EQ("a: \"$$\\b<;", EscapeString("a: \"$\\b<;", opts, nullptr));
}
