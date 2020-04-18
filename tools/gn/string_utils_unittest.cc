// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/string_utils.h"

#include <stdint.h>

#include <memory>
#include <utility>

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/err.h"
#include "tools/gn/scope.h"
#include "tools/gn/settings.h"
#include "tools/gn/token.h"
#include "tools/gn/value.h"

namespace {

bool CheckExpansionCase(const char* input, const char* expected, bool success) {
  Scope scope(static_cast<const Settings*>(nullptr));
  int64_t one = 1;
  scope.SetValue("one", Value(nullptr, one), nullptr);
  scope.SetValue("onestring", Value(nullptr, "one"), nullptr);

  // Nested scope called "onescope" with a value "one" inside it.
  std::unique_ptr<Scope> onescope =
      std::make_unique<Scope>(static_cast<const Settings*>(nullptr));
  onescope->SetValue("one", Value(nullptr, one), nullptr);
  scope.SetValue("onescope", Value(nullptr, std::move(onescope)), nullptr);

  // List called "onelist" with one value that maps to 1.
  Value onelist(nullptr, Value::LIST);
  onelist.list_value().push_back(Value(nullptr, one));
  scope.SetValue("onelist", onelist, nullptr);

  // Construct the string token, which includes the quotes.
  std::string literal_string;
  literal_string.push_back('"');
  literal_string.append(input);
  literal_string.push_back('"');
  Token literal(Location(), Token::STRING, literal_string);

  Value result(nullptr, Value::STRING);
  Err err;
  bool ret = ExpandStringLiteral(&scope, literal, &result, &err);

  // Err and return value should agree.
  EXPECT_NE(ret, err.has_error());

  if (ret != success)
    return false;

  if (!success)
    return true;  // Don't check result on failure.
  printf("%s\n", result.string_value().c_str());
  return result.string_value() == expected;
}

}  // namespace

TEST(StringUtils, ExpandStringLiteralIdentifier) {
  EXPECT_TRUE(CheckExpansionCase("", "", true));
  EXPECT_TRUE(CheckExpansionCase("hello", "hello", true));
  EXPECT_TRUE(CheckExpansionCase("hello #$one", "hello #1", true));
  EXPECT_TRUE(CheckExpansionCase("hello #$one/two", "hello #1/two", true));
  EXPECT_TRUE(CheckExpansionCase("hello #${one}", "hello #1", true));
  EXPECT_TRUE(CheckExpansionCase("hello #${one}one", "hello #1one", true));
  EXPECT_TRUE(CheckExpansionCase("hello #${one}$one", "hello #11", true));
  EXPECT_TRUE(CheckExpansionCase("$onestring${one}$one", "one11", true));
  EXPECT_TRUE(CheckExpansionCase("$onescope", "{\n  one = 1\n}", true));
  EXPECT_TRUE(CheckExpansionCase("$onelist", "[1]", true));

  // Hex values
  EXPECT_TRUE(CheckExpansionCase("$0x0AA", "\x0A""A", true));
  EXPECT_TRUE(CheckExpansionCase("$0x0a$0xfF", "\x0A\xFF", true));

  // Errors
  EXPECT_TRUE(CheckExpansionCase("hello #$", nullptr, false));
  EXPECT_TRUE(CheckExpansionCase("hello #$%", nullptr, false));
  EXPECT_TRUE(CheckExpansionCase("hello #${", nullptr, false));
  EXPECT_TRUE(CheckExpansionCase("hello #${}", nullptr, false));
  EXPECT_TRUE(CheckExpansionCase("hello #$nonexistant", nullptr, false));
  EXPECT_TRUE(CheckExpansionCase("hello #${unterminated", nullptr, false));
  EXPECT_TRUE(CheckExpansionCase("hex truncated: $0", nullptr, false));
  EXPECT_TRUE(CheckExpansionCase("hex truncated: $0x", nullptr, false));
  EXPECT_TRUE(CheckExpansionCase("hex truncated: $0x0", nullptr, false));
  EXPECT_TRUE(CheckExpansionCase("hex with bad char: $0a", nullptr, false));
  EXPECT_TRUE(CheckExpansionCase("hex with bad char: $0x1z", nullptr, false));
  EXPECT_TRUE(CheckExpansionCase("hex with bad char: $0xz1", nullptr, false));

  // Unknown backslash values aren't special.
  EXPECT_TRUE(CheckExpansionCase("\\", "\\", true));
  EXPECT_TRUE(CheckExpansionCase("\\b", "\\b", true));

  // Backslashes escape some special things. \"\$\\ -> "$\  Note that gtest
  // doesn't like this escape sequence so we have to put it out-of-line.
  const char* in = "\\\"\\$\\\\";
  const char* out = "\"$\\";
  EXPECT_TRUE(CheckExpansionCase(in, out, true));
}

TEST(StringUtils, ExpandStringLiteralExpression) {
  // Accessing the scope.
  EXPECT_TRUE(CheckExpansionCase("hello #${onescope.one}", "hello #1", true));
  EXPECT_TRUE(CheckExpansionCase("hello #${onescope.two}", nullptr, false));

  // Accessing the list.
  EXPECT_TRUE(CheckExpansionCase("hello #${onelist[0]}", "hello #1", true));
  EXPECT_TRUE(CheckExpansionCase("hello #${onelist[1]}", nullptr, false));

  // Trying some other (otherwise valid) expressions should fail.
  EXPECT_TRUE(CheckExpansionCase("${1 + 2}", nullptr, false));
  EXPECT_TRUE(CheckExpansionCase("${print(1)}", nullptr, false));
}

TEST(StringUtils, EditDistance) {
  EXPECT_EQ(3u, EditDistance("doom melon", "dune melon", 100));
  EXPECT_EQ(2u, EditDistance("doom melon", "dune melon", 1));

  EXPECT_EQ(2u, EditDistance("ab", "ba", 100));
  EXPECT_EQ(2u, EditDistance("ba", "ab", 100));

  EXPECT_EQ(2u, EditDistance("ananas", "banana", 100));
  EXPECT_EQ(2u, EditDistance("banana", "ananas", 100));

  EXPECT_EQ(2u, EditDistance("unclear", "nuclear", 100));
  EXPECT_EQ(2u, EditDistance("nuclear", "unclear", 100));

  EXPECT_EQ(3u, EditDistance("chrome", "chromium", 100));
  EXPECT_EQ(3u, EditDistance("chromium", "chrome", 100));

  EXPECT_EQ(4u, EditDistance("", "abcd", 100));
  EXPECT_EQ(4u, EditDistance("abcd", "", 100));

  EXPECT_EQ(4u, EditDistance("xxx", "xxxxxxx", 100));
  EXPECT_EQ(4u, EditDistance("xxxxxxx", "xxx", 100));

  EXPECT_EQ(7u, EditDistance("yyy", "xxxxxxx", 100));
  EXPECT_EQ(7u, EditDistance("xxxxxxx", "yyy", 100));
}

TEST(StringUtils, SpellcheckString) {
  std::vector<base::StringPiece> words;
  words.push_back("your");
  words.push_back("bravado");
  words.push_back("won\'t");
  words.push_back("help");
  words.push_back("you");
  words.push_back("now");

  EXPECT_EQ("help", SpellcheckString("halp", words));

  // barbados has an edit distance of 4 from bravado, so there's no suggestion.
  EXPECT_TRUE(SpellcheckString("barbados", words).empty());
}
