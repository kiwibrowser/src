// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/functions.h"
#include "tools/gn/test_with_scope.h"

TEST(FunctionProcessFileTemplates, SingleString) {
  TestWithScope setup;

  std::vector<Value> args;

  Value sources(nullptr, Value::LIST);
  sources.list_value().push_back(Value(nullptr, "//src/foo.txt"));
  args.push_back(sources);

  Value expansion(nullptr, "1234{{source_name_part}}5678");
  args.push_back(expansion);

  Err err;
  Value result =
      functions::RunProcessFileTemplate(setup.scope(), nullptr, args, &err);
  EXPECT_FALSE(err.has_error());

  ASSERT_TRUE(result.type() == Value::LIST);
  ASSERT_EQ(1u, result.list_value().size());
  ASSERT_TRUE(result.list_value()[0].type() == Value::STRING);
  ASSERT_EQ("1234foo5678", result.list_value()[0].string_value());
}

TEST(FunctionProcessFileTemplates, MultipleStrings) {
  TestWithScope setup;

  std::vector<Value> args;

  Value sources(nullptr, Value::LIST);
  sources.list_value().push_back(Value(nullptr, "//src/one.txt"));
  sources.list_value().push_back(Value(nullptr, "//src/two.txt"));
  args.push_back(sources);

  Value expansions(nullptr, Value::LIST);
  expansions.list_value().push_back(
      Value(nullptr, "1234{{source_name_part}}5678"));
  expansions.list_value().push_back(
      Value(nullptr, "ABCD{{source_file_part}}EFGH"));
  args.push_back(expansions);

  Err err;
  Value result =
      functions::RunProcessFileTemplate(setup.scope(), nullptr, args, &err);
  EXPECT_FALSE(err.has_error());

  ASSERT_TRUE(result.type() == Value::LIST);
  ASSERT_EQ(4u, result.list_value().size());
  ASSERT_TRUE(result.list_value()[0].type() == Value::STRING);
  ASSERT_TRUE(result.list_value()[1].type() == Value::STRING);
  ASSERT_TRUE(result.list_value()[2].type() == Value::STRING);
  ASSERT_TRUE(result.list_value()[3].type() == Value::STRING);
  ASSERT_EQ("1234one5678", result.list_value()[0].string_value());
  ASSERT_EQ("ABCDone.txtEFGH", result.list_value()[1].string_value());
  ASSERT_EQ("1234two5678", result.list_value()[2].string_value());
  ASSERT_EQ("ABCDtwo.txtEFGH", result.list_value()[3].string_value());
}
