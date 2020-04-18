// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/operators.h"

#include <stdint.h>

#include <memory>
#include <utility>

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/parse_tree.h"
#include "tools/gn/pattern.h"
#include "tools/gn/test_with_scope.h"

namespace {

bool IsValueIntegerEqualing(const Value& v, int64_t i) {
  if (v.type() != Value::INTEGER)
    return false;
  return v.int_value() == i;
}

bool IsValueStringEqualing(const Value& v, const char* s) {
  if (v.type() != Value::STRING)
    return false;
  return v.string_value() == s;
}

// This parse node is for passing to tests. It returns a canned value for
// Execute().
class TestParseNode : public ParseNode {
 public:
  TestParseNode(const Value& v) : value_(v) {
  }

  Value Execute(Scope* scope, Err* err) const override {
    return value_;
  }
  LocationRange GetRange() const override {
    return LocationRange();
  }
  Err MakeErrorDescribing(const std::string& msg,
                          const std::string& help) const override {
    return Err(this, msg);
  }
  void Print(std::ostream& out, int indent) const override {
  }

 private:
  Value value_;
};

// Sets up a BinaryOpNode for testing.
class TestBinaryOpNode : public BinaryOpNode {
 public:
  // Input token value string must outlive class.
  TestBinaryOpNode(Token::Type op_token_type,
                   const char* op_token_value)
      : BinaryOpNode(),
        op_token_ownership_(Location(), op_token_type, op_token_value) {
    set_op(op_token_ownership_);
  }

  void SetLeftToValue(const Value& value) {
    set_left(std::make_unique<TestParseNode>(value));
  }

  // Sets the left-hand side of the operator to an identifier node, this is
  // used for testing assignments. Input string must outlive class.
  void SetLeftToIdentifier(const char* identifier) {
    left_identifier_token_ownership_ =
        Token(Location(), Token::IDENTIFIER, identifier);
    set_left(
        std::make_unique<IdentifierNode>(left_identifier_token_ownership_));
  }

  void SetRightToValue(const Value& value) {
    set_right(std::make_unique<TestParseNode>(value));
  }
  void SetRightToListOfValue(const Value& value) {
    Value list(nullptr, Value::LIST);
    list.list_value().push_back(value);
    set_right(std::make_unique<TestParseNode>(list));
  }
  void SetRightToListOfValue(const Value& value1, const Value& value2) {
    Value list(nullptr, Value::LIST);
    list.list_value().push_back(value1);
    list.list_value().push_back(value2);
    set_right(std::make_unique<TestParseNode>(list));
  }

 private:
  // The base class takes the Token by reference, this manages the lifetime.
  Token op_token_ownership_;

  // When setting the left to an identifier, this manages the lifetime of
  // the identifier token.
  Token left_identifier_token_ownership_;
};

}  // namespace

TEST(Operators, SourcesAppend) {
  Err err;
  TestWithScope setup;

  // Set up "sources" with an empty list.
  const char sources[] = "sources";
  setup.scope()->SetValue(sources, Value(nullptr, Value::LIST), nullptr);

  // Set up the operator.
  TestBinaryOpNode node(Token::PLUS_EQUALS, "+=");
  node.SetLeftToIdentifier(sources);

  // Set up the filter on the scope to remove everything ending with "rm"
  std::unique_ptr<PatternList> pattern_list = std::make_unique<PatternList>();
  pattern_list->Append(Pattern("*rm"));
  setup.scope()->set_sources_assignment_filter(std::move(pattern_list));

  // Append an integer.
  node.SetRightToListOfValue(Value(nullptr, static_cast<int64_t>(5)));
  node.Execute(setup.scope(), &err);
  EXPECT_FALSE(err.has_error());

  // Append a string that doesn't match the pattern, it should get appended.
  const char string1[] = "good";
  node.SetRightToListOfValue(Value(nullptr, string1));
  node.Execute(setup.scope(), &err);
  EXPECT_FALSE(err.has_error());

  // Append a string that does match the pattern, it should be a no-op.
  const char string2[] = "foo-rm";
  node.SetRightToListOfValue(Value(nullptr, string2));
  node.Execute(setup.scope(), &err);
  EXPECT_FALSE(err.has_error());

  // Append a list with the two strings from above.
  node.SetRightToListOfValue(Value(nullptr, string1), Value(nullptr, string2));
  node.Execute(setup.scope(), &err);
  EXPECT_FALSE(err.has_error());

  // The sources variable in the scope should now have: [ 5, "good", "good" ]
  const Value* value = setup.scope()->GetValue(sources);
  ASSERT_TRUE(value);
  ASSERT_EQ(Value::LIST, value->type());
  ASSERT_EQ(3u, value->list_value().size());
  EXPECT_TRUE(IsValueIntegerEqualing(value->list_value()[0], 5));
  EXPECT_TRUE(IsValueStringEqualing(value->list_value()[1], "good"));
  EXPECT_TRUE(IsValueStringEqualing(value->list_value()[2], "good"));
}

// Note that the SourcesAppend test above tests the basic list + list features,
// this test handles the other cases.
TEST(Operators, ListAppend) {
  Err err;
  TestWithScope setup;

  // Set up "foo" with an empty list.
  const char foo[] = "foo";
  setup.scope()->SetValue(foo, Value(nullptr, Value::LIST), nullptr);

  // Set up the operator to append to "foo".
  TestBinaryOpNode node(Token::PLUS_EQUALS, "+=");
  node.SetLeftToIdentifier(foo);

  // Append a list with a list, the result should be a nested list.
  Value inner_list(nullptr, Value::LIST);
  inner_list.list_value().push_back(Value(nullptr, static_cast<int64_t>(12)));
  node.SetRightToListOfValue(inner_list);

  Value ret = ExecuteBinaryOperator(setup.scope(), &node, node.left(),
                                    node.right(), &err);
  EXPECT_FALSE(err.has_error());

  // Return from the operator should always be "none", it should update the
  // value only.
  EXPECT_EQ(Value::NONE, ret.type());

  // The value should be updated with "[ [ 12 ] ]"
  Value result = *setup.scope()->GetValue(foo);
  ASSERT_EQ(Value::LIST, result.type());
  ASSERT_EQ(1u, result.list_value().size());
  ASSERT_EQ(Value::LIST, result.list_value()[0].type());
  ASSERT_EQ(1u, result.list_value()[0].list_value().size());
  ASSERT_EQ(Value::INTEGER, result.list_value()[0].list_value()[0].type());
  ASSERT_EQ(12, result.list_value()[0].list_value()[0].int_value());

  // Try to append an integer and a string directly (e.g. foo += "hi").
  // This should fail.
  const char str_str[] = "\"hi\"";
  Token str(Location(), Token::STRING, str_str);
  node.set_right(std::make_unique<LiteralNode>(str));
  ExecuteBinaryOperator(setup.scope(), &node, node.left(), node.right(), &err);
  EXPECT_TRUE(err.has_error());
  err = Err();

  node.SetRightToValue(Value(nullptr, static_cast<int64_t>(12)));
  ExecuteBinaryOperator(setup.scope(), &node, node.left(), node.right(), &err);
  EXPECT_TRUE(err.has_error());
}

TEST(Operators, ListRemove) {
  Err err;
  TestWithScope setup;

  const char foo_str[] = "foo";
  const char bar_str[] = "bar";
  Value test_list(nullptr, Value::LIST);
  test_list.list_value().push_back(Value(nullptr, foo_str));
  test_list.list_value().push_back(Value(nullptr, bar_str));
  test_list.list_value().push_back(Value(nullptr, foo_str));

  // Set up "var" with an the test list.
  const char var[] = "var";
  setup.scope()->SetValue(var, test_list, nullptr);

  TestBinaryOpNode node(Token::MINUS_EQUALS, "-=");
  node.SetLeftToIdentifier(var);

  // Subtract a list consisting of "foo".
  node.SetRightToListOfValue(Value(nullptr, foo_str));
  Value result = ExecuteBinaryOperator(
      setup.scope(), &node, node.left(), node.right(), &err);
  EXPECT_FALSE(err.has_error());

  // -= returns an empty value to reduce the possibility of writing confusing
  // cases like foo = bar += 1.
  EXPECT_EQ(Value::NONE, result.type());

  // The "var" variable should have been updated. Both instances of "foo" are
  // deleted.
  const Value* new_value = setup.scope()->GetValue(var);
  ASSERT_TRUE(new_value);
  ASSERT_EQ(Value::LIST, new_value->type());
  ASSERT_EQ(1u, new_value->list_value().size());
  ASSERT_EQ(Value::STRING, new_value->list_value()[0].type());
  EXPECT_EQ("bar", new_value->list_value()[0].string_value());
}

TEST(Operators, IntegerAdd) {
  Err err;
  TestWithScope setup;

  TestBinaryOpNode node(Token::PLUS, "+");
  node.SetLeftToValue(Value(nullptr, static_cast<int64_t>(123)));
  node.SetRightToValue(Value(nullptr, static_cast<int64_t>(456)));
  Value ret = ExecuteBinaryOperator(setup.scope(), &node, node.left(),
                                    node.right(), &err);
  ASSERT_FALSE(err.has_error());
  ASSERT_EQ(Value::INTEGER, ret.type());
  EXPECT_EQ(579, ret.int_value());
}

TEST(Operators, IntegerSubtract) {
  Err err;
  TestWithScope setup;

  TestBinaryOpNode node(Token::MINUS, "-");
  node.SetLeftToValue(Value(nullptr, static_cast<int64_t>(123)));
  node.SetRightToValue(Value(nullptr, static_cast<int64_t>(456)));
  Value ret = ExecuteBinaryOperator(setup.scope(), &node, node.left(),
                                    node.right(), &err);
  ASSERT_FALSE(err.has_error());
  ASSERT_EQ(Value::INTEGER, ret.type());
  EXPECT_EQ(-333, ret.int_value());
}

TEST(Operators, ShortCircuitAnd) {
  Err err;
  TestWithScope setup;

  // Set a && operator with the left to false.
  TestBinaryOpNode node(Token::BOOLEAN_AND, "&&");
  node.SetLeftToValue(Value(nullptr, false));

  // Set right as foo, but don't define a value for it.
  const char foo[] = "foo";
  Token identifier_token(Location(), Token::IDENTIFIER, foo);
  node.set_right(std::make_unique<IdentifierNode>(identifier_token));

  Value ret = ExecuteBinaryOperator(setup.scope(), &node, node.left(),
                                    node.right(), &err);
  EXPECT_FALSE(err.has_error());
}

TEST(Operators, ShortCircuitOr) {
  Err err;
  TestWithScope setup;

  // Set a || operator with the left to true.
  TestBinaryOpNode node(Token::BOOLEAN_OR, "||");
  node.SetLeftToValue(Value(nullptr, true));

  // Set right as foo, but don't define a value for it.
  const char foo[] = "foo";
  Token identifier_token(Location(), Token::IDENTIFIER, foo);
  node.set_right(std::make_unique<IdentifierNode>(identifier_token));

  Value ret = ExecuteBinaryOperator(setup.scope(), &node, node.left(),
                                    node.right(), &err);
  EXPECT_FALSE(err.has_error());
}

// Overwriting nonempty lists and scopes with other nonempty lists and scopes
// should be disallowed.
TEST(Operators, NonemptyOverwriting) {
  Err err;
  TestWithScope setup;

  // Set up "foo" with a nonempty list.
  const char foo[] = "foo";
  Value old_value(nullptr, Value::LIST);
  old_value.list_value().push_back(Value(nullptr, "string"));
  setup.scope()->SetValue(foo, old_value, nullptr);

  TestBinaryOpNode node(Token::EQUAL, "=");
  node.SetLeftToIdentifier(foo);

  // Assigning a nonempty list should fail.
  node.SetRightToListOfValue(Value(nullptr, "string"));
  node.Execute(setup.scope(), &err);
  ASSERT_TRUE(err.has_error());
  EXPECT_EQ("Replacing nonempty list.", err.message());
  err = Err();

  // Assigning an empty list should succeed.
  node.SetRightToValue(Value(nullptr, Value::LIST));
  node.Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error());
  const Value* new_value = setup.scope()->GetValue(foo);
  ASSERT_TRUE(new_value);
  ASSERT_EQ(Value::LIST, new_value->type());
  ASSERT_TRUE(new_value->list_value().empty());

  // Set up "foo" with a nonempty scope.
  const char bar[] = "bar";
  old_value = Value(nullptr, std::make_unique<Scope>(setup.settings()));
  old_value.scope_value()->SetValue(bar, Value(nullptr, "bar"), nullptr);
  setup.scope()->SetValue(foo, old_value, nullptr);

  // Assigning a nonempty scope should fail (re-use old_value copy).
  node.SetRightToValue(old_value);
  node.Execute(setup.scope(), &err);
  ASSERT_TRUE(err.has_error());
  EXPECT_EQ("Replacing nonempty scope.", err.message());
  err = Err();

  // Assigning an empty list should succeed.
  node.SetRightToValue(
      Value(nullptr, std::make_unique<Scope>(setup.settings())));
  node.Execute(setup.scope(), &err);
  ASSERT_FALSE(err.has_error());
  new_value = setup.scope()->GetValue(foo);
  ASSERT_TRUE(new_value);
  ASSERT_EQ(Value::SCOPE, new_value->type());
  ASSERT_FALSE(new_value->scope_value()->HasValues(Scope::SEARCH_CURRENT));
}

// Tests this case:
//  foo = 1
//  target(...) {
//    foo += 1
//
// This should mark the outer "foo" as used, and the inner "foo" as unused.
TEST(Operators, PlusEqualsUsed) {
  Err err;
  TestWithScope setup;

  // Outer "foo" definition, it should be unused.
  const char foo[] = "foo";
  Value old_value(nullptr, static_cast<int64_t>(1));
  setup.scope()->SetValue(foo, old_value, nullptr);
  EXPECT_TRUE(setup.scope()->IsSetButUnused(foo));

  // Nested scope.
  Scope nested(setup.scope());

  // Run "foo += 1".
  TestBinaryOpNode node(Token::PLUS_EQUALS, "+=");
  node.SetLeftToIdentifier(foo);
  node.SetRightToValue(Value(nullptr, static_cast<int64_t>(1)));
  node.Execute(&nested, &err);
  ASSERT_FALSE(err.has_error());

  // Outer foo should be used, inner foo should not be.
  EXPECT_FALSE(setup.scope()->IsSetButUnused(foo));
  EXPECT_TRUE(nested.IsSetButUnused(foo));
}
