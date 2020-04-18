// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/input_conversion.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/err.h"
#include "tools/gn/input_file.h"
#include "tools/gn/parse_tree.h"
#include "tools/gn/scheduler.h"
#include "tools/gn/test_with_scheduler.h"
#include "tools/gn/test_with_scope.h"
#include "tools/gn/value.h"

namespace {

// InputConversion needs a global scheduler object.
class InputConversionTest : public TestWithScheduler {
 public:
  InputConversionTest() = default;

  const Settings* settings() { return setup_.settings(); }

 private:
  TestWithScope setup_;
};

}  // namespace

TEST_F(InputConversionTest, String) {
  Err err;
  std::string input("\nfoo bar  \n");
  Value result = ConvertInputToValue(settings(), input, nullptr,
                                     Value(nullptr, "string"), &err);
  EXPECT_FALSE(err.has_error());
  EXPECT_EQ(Value::STRING, result.type());
  EXPECT_EQ(input, result.string_value());

  // Test with trimming.
  result = ConvertInputToValue(settings(), input, nullptr,
                               Value(nullptr, "trim string"), &err);
  EXPECT_FALSE(err.has_error());
  EXPECT_EQ(Value::STRING, result.type());
  EXPECT_EQ("foo bar", result.string_value());
}

TEST_F(InputConversionTest, ListLines) {
  Err err;
  std::string input("\nfoo\nbar  \n\n");
  Value result = ConvertInputToValue(settings(), input, nullptr,
                                     Value(nullptr, "list lines"), &err);
  EXPECT_FALSE(err.has_error());
  EXPECT_EQ(Value::LIST, result.type());
  ASSERT_EQ(4u, result.list_value().size());
  EXPECT_EQ("",    result.list_value()[0].string_value());
  EXPECT_EQ("foo", result.list_value()[1].string_value());
  EXPECT_EQ("bar", result.list_value()[2].string_value());
  EXPECT_EQ("",    result.list_value()[3].string_value());

  // Test with trimming.
  result = ConvertInputToValue(settings(), input, nullptr,
                               Value(nullptr, "trim list lines"), &err);
  EXPECT_FALSE(err.has_error());
  EXPECT_EQ(Value::LIST, result.type());
  ASSERT_EQ(2u, result.list_value().size());
  EXPECT_EQ("foo", result.list_value()[0].string_value());
  EXPECT_EQ("bar", result.list_value()[1].string_value());
}

TEST_F(InputConversionTest, ValueString) {
  Err err;
  std::string input("\"str\"");
  Value result = ConvertInputToValue(settings(), input, nullptr,
                                     Value(nullptr, "value"), &err);
  EXPECT_FALSE(err.has_error());
  EXPECT_EQ(Value::STRING, result.type());
  EXPECT_EQ("str", result.string_value());
}

TEST_F(InputConversionTest, ValueInt) {
  Err err;
  std::string input("\n\n  6 \n ");
  Value result = ConvertInputToValue(settings(), input, nullptr,
                                     Value(nullptr, "value"), &err);
  EXPECT_FALSE(err.has_error());
  EXPECT_EQ(Value::INTEGER, result.type());
  EXPECT_EQ(6, result.int_value());
}

TEST_F(InputConversionTest, ValueList) {
  Err err;
  std::string input("\n [ \"a\", 5]");
  Value result = ConvertInputToValue(settings(), input, nullptr,
                                     Value(nullptr, "value"), &err);
  EXPECT_FALSE(err.has_error());
  ASSERT_EQ(Value::LIST, result.type());
  ASSERT_EQ(2u, result.list_value().size());
  EXPECT_EQ("a", result.list_value()[0].string_value());
  EXPECT_EQ(5,   result.list_value()[1].int_value());
}

TEST_F(InputConversionTest, ValueDict) {
  Err err;
  std::string input("\n a = 5 b = \"foo\" c = a + 2");
  Value result = ConvertInputToValue(settings(), input, nullptr,
                                     Value(nullptr, "scope"), &err);
  EXPECT_FALSE(err.has_error());
  ASSERT_EQ(Value::SCOPE, result.type());

  const Value* a_value = result.scope_value()->GetValue("a");
  ASSERT_TRUE(a_value);
  EXPECT_EQ(5, a_value->int_value());

  const Value* b_value = result.scope_value()->GetValue("b");
  ASSERT_TRUE(b_value);
  EXPECT_EQ("foo", b_value->string_value());

  const Value* c_value = result.scope_value()->GetValue("c");
  ASSERT_TRUE(c_value);
  EXPECT_EQ(7, c_value->int_value());

  // Tests that when we get Values out of the input conversion, the resulting
  // values have an origin set to something corresponding to the input.
  const ParseNode* a_origin = a_value->origin();
  ASSERT_TRUE(a_origin);
  LocationRange a_range = a_origin->GetRange();
  EXPECT_EQ(2, a_range.begin().line_number());
  EXPECT_EQ(6, a_range.begin().column_number());

  const InputFile* a_file = a_range.begin().file();
  EXPECT_EQ(input, a_file->contents());
}

TEST_F(InputConversionTest, ValueJSON) {
  Err err;
  std::string input(R"*({
  "a": 5,
  "b": "foo",
  "c": {
    "d": true,
    "e": [
      {
        "f": "bar"
      }
    ]
  }
})*");
  Value result = ConvertInputToValue(settings(), input, nullptr,
                                     Value(nullptr, "json"), &err);
  EXPECT_FALSE(err.has_error());
  ASSERT_EQ(Value::SCOPE, result.type());

  const Value* a_value = result.scope_value()->GetValue("a");
  ASSERT_TRUE(a_value);
  EXPECT_EQ(5, a_value->int_value());

  const Value* b_value = result.scope_value()->GetValue("b");
  ASSERT_TRUE(b_value);
  EXPECT_EQ("foo", b_value->string_value());

  const Value* c_value = result.scope_value()->GetValue("c");
  ASSERT_TRUE(c_value);
  ASSERT_EQ(Value::SCOPE, c_value->type());

  const Value* d_value = c_value->scope_value()->GetValue("d");
  ASSERT_TRUE(d_value);
  EXPECT_EQ(true, d_value->boolean_value());

  const Value* e_value = c_value->scope_value()->GetValue("e");
  ASSERT_TRUE(e_value);
  ASSERT_EQ(Value::LIST, e_value->type());

  EXPECT_EQ(1u, e_value->list_value().size());
  ASSERT_EQ(Value::SCOPE, e_value->list_value()[0].type());
  const Value* f_value = e_value->list_value()[0].scope_value()->GetValue("f");
  ASSERT_TRUE(f_value);
  EXPECT_EQ("bar", f_value->string_value());
}

TEST_F(InputConversionTest, ValueJSONInvalidInput) {
  Err err;
  std::string input(R"*({
  "a": 5,
  "b":
})*");
  Value result = ConvertInputToValue(settings(), input, nullptr,
                                     Value(nullptr, "json"), &err);
  EXPECT_TRUE(err.has_error());
  EXPECT_EQ("Input is not a valid JSON: Line: 4, column: 2, Unexpected token.",
            err.message());
}

TEST_F(InputConversionTest, ValueJSONUnsupportedValue) {
  Err err;
  std::string input(R"*({
  "a": null
})*");
  Value result = ConvertInputToValue(settings(), input, nullptr,
                                     Value(nullptr, "json"), &err);
  EXPECT_TRUE(err.has_error());
  EXPECT_EQ("Null values are not supported.", err.message());
}

TEST_F(InputConversionTest, ValueJSONInvalidVariable) {
  Err err;
  std::string input(R"*({
  "a\\x0001b": 5
})*");
  Value result = ConvertInputToValue(settings(), input, nullptr,
                                     Value(nullptr, "json"), &err);
  EXPECT_TRUE(err.has_error());
  EXPECT_EQ("Invalid identifier \"a\\x0001b\".", err.message());
}

TEST_F(InputConversionTest, ValueJSONUnsupported) {
  Err err;
  std::string input(R"*({
  "d": 0.0
})*");
  Value result = ConvertInputToValue(settings(), input, nullptr,
                                     Value(nullptr, "json"), &err);
  EXPECT_TRUE(err.has_error());
  EXPECT_EQ("Floating point values are not supported.", err.message());
}

TEST_F(InputConversionTest, ValueEmpty) {
  Err err;
  Value result = ConvertInputToValue(settings(), "", nullptr,
                                     Value(nullptr, "value"), &err);
  EXPECT_FALSE(err.has_error());
  EXPECT_EQ(Value::NONE, result.type());
}

TEST_F(InputConversionTest, ValueError) {
  static const char* const kTests[] = {
      "\n [ \"a\", 5\nfoo bar",

      // Blocks not allowed.
      "{ foo = 5 }",

      // Function calls not allowed.
      "print(5)",

      // Trailing junk not allowed.
      "233105-1",

      // Non-literals hidden in arrays are not allowed.
      "[233105 - 1]",
      "[rebase_path(\"//\")]",
  };

  for (auto* test : kTests) {
    Err err;
    std::string input(test);
    Value result = ConvertInputToValue(settings(), input, nullptr,
                                       Value(nullptr, "value"), &err);
    EXPECT_TRUE(err.has_error()) << test;
  }
}

// Passing none or the empty string for input conversion should ignore the
// result.
TEST_F(InputConversionTest, Ignore) {
  Err err;
  Value result = ConvertInputToValue(settings(), "foo", nullptr, Value(), &err);
  EXPECT_FALSE(err.has_error());
  EXPECT_EQ(Value::NONE, result.type());

  result =
      ConvertInputToValue(settings(), "foo", nullptr, Value(nullptr, ""), &err);
  EXPECT_FALSE(err.has_error());
  EXPECT_EQ(Value::NONE, result.type());
}
