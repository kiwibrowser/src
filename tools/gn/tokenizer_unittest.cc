// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/input_file.h"
#include "tools/gn/token.h"
#include "tools/gn/tokenizer.h"

namespace {

struct TokenExpectation {
  Token::Type type;
  const char* value;
};

template<size_t len>
bool CheckTokenizer(const char* input, const TokenExpectation (&expect)[len]) {
  InputFile input_file(SourceFile("/test"));
  input_file.SetContents(input);

  Err err;
  std::vector<Token> results = Tokenizer::Tokenize(&input_file, &err);

  if (results.size() != len)
    return false;
  for (size_t i = 0; i < len; i++) {
    if (expect[i].type != results[i].type())
      return false;
    if (expect[i].value != results[i].value())
      return false;
  }
  return true;
}

}  // namespace

TEST(Tokenizer, Empty) {
  InputFile empty_string_input(SourceFile("/test"));
  empty_string_input.SetContents("");

  Err err;
  std::vector<Token> results = Tokenizer::Tokenize(&empty_string_input, &err);
  EXPECT_TRUE(results.empty());

  InputFile whitespace_input(SourceFile("/test"));
  whitespace_input.SetContents("  \r \n \r\n");

  results = Tokenizer::Tokenize(&whitespace_input, &err);
  EXPECT_TRUE(results.empty());
}

TEST(Tokenizer, Identifier) {
  TokenExpectation one_ident[] = {
    { Token::IDENTIFIER, "foo" }
  };
  EXPECT_TRUE(CheckTokenizer("  foo ", one_ident));
}

TEST(Tokenizer, Integer) {
  TokenExpectation integers[] = {
    { Token::INTEGER, "123" },
    { Token::INTEGER, "-123" }
  };
  EXPECT_TRUE(CheckTokenizer("  123 -123 ", integers));
}

TEST(Tokenizer, IntegerNoSpace) {
  TokenExpectation integers[] = {
    { Token::INTEGER, "123" },
    { Token::INTEGER, "-123" }
  };
  EXPECT_TRUE(CheckTokenizer("  123-123 ", integers));
}

TEST(Tokenizer, String) {
  TokenExpectation strings[] = {
    { Token::STRING, "\"foo\"" },
    { Token::STRING, "\"bar\\\"baz\"" },
    { Token::STRING, "\"asdf\\\\\"" }
  };
  EXPECT_TRUE(CheckTokenizer("  \"foo\" \"bar\\\"baz\" \"asdf\\\\\" ",
              strings));
}

TEST(Tokenizer, Operator) {
  TokenExpectation operators[] = {
    { Token::MINUS, "-" },
    { Token::PLUS, "+" },
    { Token::EQUAL, "=" },
    { Token::PLUS_EQUALS, "+=" },
    { Token::MINUS_EQUALS, "-=" },
    { Token::NOT_EQUAL, "!=" },
    { Token::EQUAL_EQUAL, "==" },
    { Token::LESS_THAN, "<" },
    { Token::GREATER_THAN, ">" },
    { Token::LESS_EQUAL, "<=" },
    { Token::GREATER_EQUAL, ">=" },
    { Token::BANG, "!" },
    { Token::BOOLEAN_OR, "||" },
    { Token::BOOLEAN_AND, "&&" },
    { Token::DOT, "." },
    { Token::COMMA, "," },
  };
  EXPECT_TRUE(CheckTokenizer("- + = += -= != ==  < > <= >= ! || && . ,",
              operators));
}

TEST(Tokenizer, Scoper) {
  TokenExpectation scopers[] = {
    { Token::LEFT_BRACE, "{" },
    { Token::LEFT_BRACKET, "[" },
    { Token::RIGHT_BRACKET, "]" },
    { Token::RIGHT_BRACE, "}" },
    { Token::LEFT_PAREN, "(" },
    { Token::RIGHT_PAREN, ")" },
  };
  EXPECT_TRUE(CheckTokenizer("{[ ]} ()", scopers));
}

TEST(Tokenizer, FunctionCall) {
  TokenExpectation fn[] = {
    { Token::IDENTIFIER, "fun" },
    { Token::LEFT_PAREN, "(" },
    { Token::STRING, "\"foo\"" },
    { Token::RIGHT_PAREN, ")" },
    { Token::LEFT_BRACE, "{" },
    { Token::IDENTIFIER, "foo" },
    { Token::EQUAL, "=" },
    { Token::INTEGER, "12" },
    { Token::RIGHT_BRACE, "}" },
  };
  EXPECT_TRUE(CheckTokenizer("fun(\"foo\") {\nfoo = 12}", fn));
}

TEST(Tokenizer, Locations) {
  InputFile input(SourceFile("/test"));
  input.SetContents("1 2 \"three\"\n  4");
  Err err;
  std::vector<Token> results = Tokenizer::Tokenize(&input, &err);

  ASSERT_EQ(4u, results.size());
  ASSERT_TRUE(results[0].location() == Location(&input, 1, 1, 1));
  ASSERT_TRUE(results[1].location() == Location(&input, 1, 3, 3));
  ASSERT_TRUE(results[2].location() == Location(&input, 1, 5, 5));
  ASSERT_TRUE(results[3].location() == Location(&input, 2, 3, 8));
}

TEST(Tokenizer, ByteOffsetOfNthLine) {
  EXPECT_EQ(0u, Tokenizer::ByteOffsetOfNthLine("foo", 1));

  // Windows and Posix have different line endings, so check the byte at the
  // location rather than the offset.
  char input1[] = "aaa\nxaa\n\nya";
  EXPECT_EQ('x', input1[Tokenizer::ByteOffsetOfNthLine(input1, 2)]);
  EXPECT_EQ('y', input1[Tokenizer::ByteOffsetOfNthLine(input1, 4)]);

  char input2[3];
  input2[0] = 'a';
  input2[1] = '\n';  // Manually set to avoid Windows double-byte endings.
  input2[2] = 0;
  EXPECT_EQ(0u, Tokenizer::ByteOffsetOfNthLine(input2, 1));
  EXPECT_EQ(2u, Tokenizer::ByteOffsetOfNthLine(input2, 2));
}

TEST(Tokenizer, Comments) {
  TokenExpectation fn[] = {
    { Token::LINE_COMMENT, "# Stuff" },
    { Token::IDENTIFIER, "fun" },
    { Token::LEFT_PAREN, "(" },
    { Token::STRING, "\"foo\"" },
    { Token::RIGHT_PAREN, ")" },
    { Token::LEFT_BRACE, "{" },
    { Token::SUFFIX_COMMENT, "# Things" },
    { Token::LINE_COMMENT, "#Wee" },
    { Token::IDENTIFIER, "foo" },
    { Token::EQUAL, "=" },
    { Token::INTEGER, "12" },
    { Token::SUFFIX_COMMENT, "#Zip" },
    { Token::RIGHT_BRACE, "}" },
  };
  EXPECT_TRUE(CheckTokenizer(
      "# Stuff\n"
      "fun(\"foo\") {  # Things\n"
      "#Wee\n"
      "foo = 12 #Zip\n"
      "}",
      fn));
}

TEST(Tokenizer, CommentsContinued) {
  // In the first test, the comments aren't horizontally aligned, so they're
  // considered separate. In the second test, they are, so "B" is a
  // continuation of "A" (another SUFFIX comment).
  TokenExpectation fn1[] = {
    { Token::IDENTIFIER, "fun" },
    { Token::LEFT_PAREN, "(" },
    { Token::STRING, "\"foo\"" },
    { Token::RIGHT_PAREN, ")" },
    { Token::LEFT_BRACE, "{" },
    { Token::SUFFIX_COMMENT, "# A" },
    { Token::LINE_COMMENT, "# B" },
    { Token::RIGHT_BRACE, "}" },
  };
  EXPECT_TRUE(CheckTokenizer(
      "fun(\"foo\") {  # A\n"
      "  # B\n"
      "}",
      fn1));

  TokenExpectation fn2[] = {
    { Token::IDENTIFIER, "fun" },
    { Token::LEFT_PAREN, "(" },
    { Token::STRING, "\"foo\"" },
    { Token::RIGHT_PAREN, ")" },
    { Token::LEFT_BRACE, "{" },
    { Token::SUFFIX_COMMENT, "# A" },
    { Token::SUFFIX_COMMENT, "# B" },
    { Token::RIGHT_BRACE, "}" },
  };
  EXPECT_TRUE(CheckTokenizer(
      "fun(\"foo\") {  # A\n"
      "              # B\n"  // Note that these are aligned, the \"s move A out.
      "}",
      fn2));
}
