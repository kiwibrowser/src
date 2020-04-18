// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/pattern.h"

namespace {

struct Case {
  const char* pattern;
  const char* candidate;
  bool expected_match;
};

}  // namespace

TEST(Pattern, Matches) {
  Case pattern_cases[] = {
    // Empty pattern matches only empty string.
    { "", "", true },
    { "", "foo", false },
    // Exact matches.
    { "foo", "foo", true },
    { "foo", "bar", false },
    // Path boundaries.
    { "\\b", "", true },
    { "\\b", "/", true },
    { "\\b\\b", "/", true },
    { "\\b\\b\\b", "", false },
    { "\\b\\b\\b", "/", true },
    { "\\b", "//", false },
    { "\\bfoo\\b", "foo", true },
    { "\\bfoo\\b", "/foo/", true },
    { "\\b\\bfoo", "/foo", true },
    // *
    { "*", "", true },
    { "*", "foo", true },
    { "*foo", "foo", true },
    { "*foo", "gagafoo", true },
    { "*foo", "gagafoob", false },
    { "foo*bar", "foobar", true },
    { "foo*bar", "foo-bar", true },
    { "foo*bar", "foolalalalabar", true },
    { "foo*bar", "foolalalalabaz", false },
    { "*a*b*c*d*", "abcd", true },
    { "*a*b*c*d*", "1a2b3c4d5", true },
    { "*a*b*c*d*", "1a2b3c45", false },
    { "*\\bfoo\\b*", "foo", true },
    { "*\\bfoo\\b*", "/foo/", true },
    { "*\\bfoo\\b*", "foob", false },
    { "*\\bfoo\\b*", "lala/foo/bar/baz", true },
  };
  for (size_t i = 0; i < arraysize(pattern_cases); i++) {
    const Case& c = pattern_cases[i];
    Pattern pattern(c.pattern);
    bool result = pattern.MatchesString(c.candidate);
    EXPECT_EQ(c.expected_match, result) << i << ": \"" << c.pattern
        << "\", \"" << c.candidate << "\"";
  }
}
