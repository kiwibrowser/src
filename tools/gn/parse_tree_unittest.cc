// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/parse_tree.h"

#include <stdint.h>

#include <memory>
#include <utility>

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/input_file.h"
#include "tools/gn/scope.h"
#include "tools/gn/test_with_scope.h"

TEST(ParseTree, Accessor) {
  TestWithScope setup;

  // Make a pretend parse node with proper tracking that we can blame for the
  // given value.
  InputFile input_file(SourceFile("//foo"));
  Token base_token(Location(&input_file, 1, 1, 1), Token::IDENTIFIER, "a");
  Token member_token(Location(&input_file, 1, 1, 1), Token::IDENTIFIER, "b");

  AccessorNode accessor;
  accessor.set_base(base_token);

  std::unique_ptr<IdentifierNode> member_identifier =
      std::make_unique<IdentifierNode>(member_token);
  accessor.set_member(std::move(member_identifier));

  // The access should fail because a is not defined.
  Err err;
  Value result = accessor.Execute(setup.scope(), &err);
  EXPECT_TRUE(err.has_error());
  EXPECT_EQ(Value::NONE, result.type());

  // Define a as a Scope. It should still fail because b isn't defined.
  err = Err();
  setup.scope()->SetValue(
      "a", Value(nullptr, std::make_unique<Scope>(setup.scope())), nullptr);
  result = accessor.Execute(setup.scope(), &err);
  EXPECT_TRUE(err.has_error());
  EXPECT_EQ(Value::NONE, result.type());

  // Define b, accessor should succeed now.
  const int64_t kBValue = 42;
  err = Err();
  setup.scope()
      ->GetMutableValue("a", Scope::SEARCH_NESTED, false)
      ->scope_value()
      ->SetValue("b", Value(nullptr, kBValue), nullptr);
  result = accessor.Execute(setup.scope(), &err);
  EXPECT_FALSE(err.has_error());
  ASSERT_EQ(Value::INTEGER, result.type());
  EXPECT_EQ(kBValue, result.int_value());
}

TEST(ParseTree, BlockUnusedVars) {
  TestWithScope setup;

  // Printing both values should be OK.
  //
  // The crazy template definition here is a way to execute a block without
  // defining a target. Templates require that both the target_name and the
  // invoker be used, which is what the assertion statement inside the template
  // does.
  TestParseInput input_all_used(
      "template(\"foo\") { assert(target_name != 0 && invoker != 0) }\n"
      "foo(\"a\") {\n"
      "  a = 12\n"
      "  b = 13\n"
      "  print(\"$a $b\")\n"
      "}");
  EXPECT_FALSE(input_all_used.has_error());

  Err err;
  input_all_used.parsed()->Execute(setup.scope(), &err);
  EXPECT_FALSE(err.has_error());

  // Skipping one should throw an unused var error.
  TestParseInput input_unused(
      "foo(\"a\") {\n"
      "  a = 12\n"
      "  b = 13\n"
      "  print(\"$a\")\n"
      "}");
  EXPECT_FALSE(input_unused.has_error());

  input_unused.parsed()->Execute(setup.scope(), &err);
  EXPECT_TRUE(err.has_error());

  // Also verify that the unused variable has the correct origin set. The
  // origin will point to the value assigned to the variable (in this case, the
  // "13" assigned to "b".
  EXPECT_EQ(3, err.location().line_number());
  EXPECT_EQ(7, err.location().column_number());
}

TEST(ParseTree, OriginForDereference) {
  TestWithScope setup;
  TestParseInput input(
      "a = 6\n"
      "get_target_outputs(a)");
  EXPECT_FALSE(input.has_error());

  Err err;
  input.parsed()->Execute(setup.scope(), &err);
  EXPECT_TRUE(err.has_error());

  // The origin for the "not a string" error message should be where the value
  // was dereferenced (the "a" on the second line).
  EXPECT_EQ(2, err.location().line_number());
  EXPECT_EQ(20, err.location().column_number());
}

TEST(ParseTree, SortRangeExtraction) {
  TestWithScope setup;

  // Ranges are [begin, end).

  {
    TestParseInput input(
        "sources = [\n"
        "  \"a\",\n"
        "  \"b\",\n"
        "  \n"
        "  #\n"
        "  # Block\n"
        "  #\n"
        "  \n"
        "  \"c\","
        "  \"d\","
        "]\n");
    EXPECT_FALSE(input.has_error());
    ASSERT_TRUE(input.parsed()->AsBlock());
    ASSERT_TRUE(input.parsed()->AsBlock()->statements()[0]->AsBinaryOp());
    const BinaryOpNode* binop =
        input.parsed()->AsBlock()->statements()[0]->AsBinaryOp();
    ASSERT_TRUE(binop->right()->AsList());
    const ListNode* list = binop->right()->AsList();
    EXPECT_EQ(5u, list->contents().size());
    auto ranges = list->GetSortRanges();
    ASSERT_EQ(2u, ranges.size());
    EXPECT_EQ(0u, ranges[0].begin);
    EXPECT_EQ(2u, ranges[0].end);
    EXPECT_EQ(3u, ranges[1].begin);
    EXPECT_EQ(5u, ranges[1].end);
  }

  {
    TestParseInput input(
        "sources = [\n"
        "  \"a\",\n"
        "  \"b\",\n"
        "  \n"
        "  # Attached comment.\n"
        "  \"c\","
        "  \"d\","
        "]\n");
    EXPECT_FALSE(input.has_error());
    ASSERT_TRUE(input.parsed()->AsBlock());
    ASSERT_TRUE(input.parsed()->AsBlock()->statements()[0]->AsBinaryOp());
    const BinaryOpNode* binop =
        input.parsed()->AsBlock()->statements()[0]->AsBinaryOp();
    ASSERT_TRUE(binop->right()->AsList());
    const ListNode* list = binop->right()->AsList();
    EXPECT_EQ(4u, list->contents().size());
    auto ranges = list->GetSortRanges();
    ASSERT_EQ(2u, ranges.size());
    EXPECT_EQ(0u, ranges[0].begin);
    EXPECT_EQ(2u, ranges[0].end);
    EXPECT_EQ(2u, ranges[1].begin);
    EXPECT_EQ(4u, ranges[1].end);
  }

  {
    TestParseInput input(
        "sources = [\n"
        "  # At end of list.\n"
        "  \"zzzzzzzzzzz.cc\","
        "]\n");
    EXPECT_FALSE(input.has_error());
    ASSERT_TRUE(input.parsed()->AsBlock());
    ASSERT_TRUE(input.parsed()->AsBlock()->statements()[0]->AsBinaryOp());
    const BinaryOpNode* binop =
        input.parsed()->AsBlock()->statements()[0]->AsBinaryOp();
    ASSERT_TRUE(binop->right()->AsList());
    const ListNode* list = binop->right()->AsList();
    EXPECT_EQ(1u, list->contents().size());
    auto ranges = list->GetSortRanges();
    ASSERT_EQ(1u, ranges.size());
    EXPECT_EQ(0u, ranges[0].begin);
    EXPECT_EQ(1u, ranges[0].end);
  }

  {
    TestParseInput input(
        "sources = [\n"
        "  # Block at start.\n"
        "  \n"
        "  \"z.cc\","
        "  \"y.cc\","
        "]\n");
    EXPECT_FALSE(input.has_error());
    ASSERT_TRUE(input.parsed()->AsBlock());
    ASSERT_TRUE(input.parsed()->AsBlock()->statements()[0]->AsBinaryOp());
    const BinaryOpNode* binop =
        input.parsed()->AsBlock()->statements()[0]->AsBinaryOp();
    ASSERT_TRUE(binop->right()->AsList());
    const ListNode* list = binop->right()->AsList();
    EXPECT_EQ(3u, list->contents().size());
    auto ranges = list->GetSortRanges();
    ASSERT_EQ(1u, ranges.size());
    EXPECT_EQ(1u, ranges[0].begin);
    EXPECT_EQ(3u, ranges[0].end);
  }
}

TEST(ParseTree, Integers) {
  static const char* const kGood[] = {
      "0",
      "10",
      "-54321",
      "9223372036854775807",   // INT64_MAX
      "-9223372036854775808",  // INT64_MIN
  };
  for (auto* s : kGood) {
    TestParseInput input(std::string("x = ") + s);
    EXPECT_FALSE(input.has_error());

    TestWithScope setup;
    Err err;
    input.parsed()->Execute(setup.scope(), &err);
    EXPECT_FALSE(err.has_error());
  }

  static const char* const kBad[] = {
      "-0",
      "010",
      "-010",
      "9223372036854775808",   // INT64_MAX + 1
      "-9223372036854775809",  // INT64_MIN - 1
  };
  for (auto* s : kBad) {
    TestParseInput input(std::string("x = ") + s);
    EXPECT_FALSE(input.has_error());

    TestWithScope setup;
    Err err;
    input.parsed()->Execute(setup.scope(), &err);
    EXPECT_TRUE(err.has_error());
  }
}
