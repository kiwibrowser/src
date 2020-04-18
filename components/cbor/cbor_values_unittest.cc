// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cbor/cbor_values.h"

#include <string>
#include <utility>

#include "testing/gtest/include/gtest/gtest.h"

namespace cbor {

TEST(CBORValuesTest, TestNothrow) {
  static_assert(std::is_nothrow_move_constructible<CBORValue>::value,
                "IsNothrowMoveConstructible");
  static_assert(std::is_nothrow_default_constructible<CBORValue>::value,
                "IsNothrowDefaultConstructible");
  static_assert(std::is_nothrow_constructible<CBORValue, std::string&&>::value,
                "IsNothrowMoveConstructibleFromString");
  static_assert(
      std::is_nothrow_constructible<CBORValue, CBORValue::BinaryValue&&>::value,
      "IsNothrowMoveConstructibleFromBytestring");
  static_assert(
      std::is_nothrow_constructible<CBORValue, CBORValue::ArrayValue&&>::value,
      "IsNothrowMoveConstructibleFromArray");
  static_assert(std::is_nothrow_move_assignable<CBORValue>::value,
                "IsNothrowMoveAssignable");
}

// Test constructors
TEST(CBORValuesTest, ConstructUnsigned) {
  CBORValue value(37);
  ASSERT_EQ(CBORValue::Type::UNSIGNED, value.type());
  EXPECT_EQ(37u, value.GetInteger());
}

TEST(CBORValuesTest, ConstructNegative) {
  CBORValue value(-1);
  ASSERT_EQ(CBORValue::Type::NEGATIVE, value.type());
  EXPECT_EQ(-1, value.GetInteger());
}

TEST(CBORValuesTest, ConstructStringFromConstCharPtr) {
  const char* str = "foobar";
  CBORValue value(str);
  ASSERT_EQ(CBORValue::Type::STRING, value.type());
  EXPECT_EQ("foobar", value.GetString());
}

TEST(CBORValuesTest, ConstructStringFromStdStringConstRef) {
  std::string str = "foobar";
  CBORValue value(str);
  ASSERT_EQ(CBORValue::Type::STRING, value.type());
  EXPECT_EQ("foobar", value.GetString());
}

TEST(CBORValuesTest, ConstructStringFromStdStringRefRef) {
  std::string str = "foobar";
  CBORValue value(std::move(str));
  ASSERT_EQ(CBORValue::Type::STRING, value.type());
  EXPECT_EQ("foobar", value.GetString());
}

TEST(CBORValuesTest, ConstructBytestring) {
  CBORValue value(CBORValue::BinaryValue({0xF, 0x0, 0x0, 0xB, 0xA, 0x2}));
  ASSERT_EQ(CBORValue::Type::BYTE_STRING, value.type());
  EXPECT_EQ(CBORValue::BinaryValue({0xF, 0x0, 0x0, 0xB, 0xA, 0x2}),
            value.GetBytestring());
}

TEST(CBORValuesTest, ConstructBytestringFromString) {
  CBORValue value(CBORValue("hello", CBORValue::Type::BYTE_STRING));
  ASSERT_EQ(CBORValue::Type::BYTE_STRING, value.type());
  EXPECT_EQ(CBORValue::BinaryValue({'h', 'e', 'l', 'l', 'o'}),
            value.GetBytestring());
  EXPECT_EQ("hello", value.GetBytestringAsString());
}

TEST(CBORValuesTest, ConstructArray) {
  CBORValue::ArrayValue array;
  array.emplace_back(CBORValue("foo"));
  {
    CBORValue value(array);
    ASSERT_EQ(CBORValue::Type::ARRAY, value.type());
    ASSERT_EQ(1u, value.GetArray().size());
    ASSERT_EQ(CBORValue::Type::STRING, value.GetArray()[0].type());
    EXPECT_EQ("foo", value.GetArray()[0].GetString());
  }

  array.back() = CBORValue("bar");
  {
    CBORValue value(std::move(array));
    ASSERT_EQ(CBORValue::Type::ARRAY, value.type());
    ASSERT_EQ(1u, value.GetArray().size());
    ASSERT_EQ(CBORValue::Type::STRING, value.GetArray()[0].type());
    EXPECT_EQ("bar", value.GetArray()[0].GetString());
  }
}

TEST(CBORValuesTest, ConstructMap) {
  CBORValue::MapValue map;
  const CBORValue key_foo("foo");
  map[CBORValue("foo")] = CBORValue("bar");
  {
    CBORValue value(map);
    ASSERT_EQ(CBORValue::Type::MAP, value.type());
    ASSERT_EQ(value.GetMap().count(key_foo), 1u);
    ASSERT_EQ(CBORValue::Type::STRING,
              value.GetMap().find(key_foo)->second.type());
    EXPECT_EQ("bar", value.GetMap().find(key_foo)->second.GetString());
  }

  map[CBORValue("foo")] = CBORValue("baz");
  {
    CBORValue value(std::move(map));
    ASSERT_EQ(CBORValue::Type::MAP, value.type());
    ASSERT_EQ(value.GetMap().count(key_foo), 1u);
    ASSERT_EQ(CBORValue::Type::STRING,
              value.GetMap().find(key_foo)->second.type());
    EXPECT_EQ("baz", value.GetMap().find(key_foo)->second.GetString());
  }
}

TEST(CBORValuesTest, ConstructSimpleValue) {
  CBORValue false_value(CBORValue::SimpleValue::FALSE_VALUE);
  ASSERT_EQ(CBORValue::Type::SIMPLE_VALUE, false_value.type());
  EXPECT_EQ(CBORValue::SimpleValue::FALSE_VALUE, false_value.GetSimpleValue());

  CBORValue true_value(CBORValue::SimpleValue::TRUE_VALUE);
  ASSERT_EQ(CBORValue::Type::SIMPLE_VALUE, true_value.type());
  EXPECT_EQ(CBORValue::SimpleValue::TRUE_VALUE, true_value.GetSimpleValue());

  CBORValue null_value(CBORValue::SimpleValue::NULL_VALUE);
  ASSERT_EQ(CBORValue::Type::SIMPLE_VALUE, null_value.type());
  EXPECT_EQ(CBORValue::SimpleValue::NULL_VALUE, null_value.GetSimpleValue());

  CBORValue undefined_value(CBORValue::SimpleValue::UNDEFINED);
  ASSERT_EQ(CBORValue::Type::SIMPLE_VALUE, undefined_value.type());
  EXPECT_EQ(CBORValue::SimpleValue::UNDEFINED,
            undefined_value.GetSimpleValue());
}

TEST(CBORValuesTest, ConstructSimpleBooleanValue) {
  CBORValue true_value(true);
  ASSERT_EQ(CBORValue::Type::SIMPLE_VALUE, true_value.type());
  EXPECT_TRUE(true_value.GetBool());

  CBORValue false_value(false);
  ASSERT_EQ(CBORValue::Type::SIMPLE_VALUE, false_value.type());
  EXPECT_FALSE(false_value.GetBool());
}

// Test copy constructors
TEST(CBORValuesTest, CopyUnsigned) {
  CBORValue value(74);
  CBORValue copied_value(value.Clone());
  ASSERT_EQ(value.type(), copied_value.type());
  EXPECT_EQ(value.GetInteger(), copied_value.GetInteger());

  CBORValue blank;

  blank = value.Clone();
  ASSERT_EQ(value.type(), blank.type());
  EXPECT_EQ(value.GetInteger(), blank.GetInteger());
}

TEST(CBORValuesTest, CopyNegativeInt) {
  CBORValue value(-74);
  CBORValue copied_value(value.Clone());
  ASSERT_EQ(value.type(), copied_value.type());
  EXPECT_EQ(value.GetInteger(), copied_value.GetInteger());

  CBORValue blank;

  blank = value.Clone();
  ASSERT_EQ(value.type(), blank.type());
  EXPECT_EQ(value.GetInteger(), blank.GetInteger());
}

TEST(CBORValuesTest, CopyString) {
  CBORValue value("foobar");
  CBORValue copied_value(value.Clone());
  ASSERT_EQ(value.type(), copied_value.type());
  EXPECT_EQ(value.GetString(), copied_value.GetString());

  CBORValue blank;

  blank = value.Clone();
  ASSERT_EQ(value.type(), blank.type());
  EXPECT_EQ(value.GetString(), blank.GetString());
}

TEST(CBORValuesTest, CopyBytestring) {
  CBORValue value(CBORValue::BinaryValue({0xF, 0x0, 0x0, 0xB, 0xA, 0x2}));
  CBORValue copied_value(value.Clone());
  ASSERT_EQ(value.type(), copied_value.type());
  EXPECT_EQ(value.GetBytestring(), copied_value.GetBytestring());

  CBORValue blank;

  blank = value.Clone();
  ASSERT_EQ(value.type(), blank.type());
  EXPECT_EQ(value.GetBytestring(), blank.GetBytestring());
}

TEST(CBORValuesTest, CopyArray) {
  CBORValue::ArrayValue array;
  array.emplace_back(123);
  CBORValue value(std::move(array));

  CBORValue copied_value(value.Clone());
  ASSERT_EQ(1u, copied_value.GetArray().size());
  ASSERT_TRUE(copied_value.GetArray()[0].is_unsigned());
  EXPECT_EQ(value.GetArray()[0].GetInteger(),
            copied_value.GetArray()[0].GetInteger());

  CBORValue blank;
  blank = value.Clone();
  EXPECT_EQ(1u, blank.GetArray().size());
}

TEST(CBORValuesTest, CopyMap) {
  CBORValue::MapValue map;
  CBORValue key_a("a");
  map[CBORValue("a")] = CBORValue(123);
  CBORValue value(std::move(map));

  CBORValue copied_value(value.Clone());
  EXPECT_EQ(1u, copied_value.GetMap().size());
  ASSERT_EQ(value.GetMap().count(key_a), 1u);
  ASSERT_EQ(copied_value.GetMap().count(key_a), 1u);
  ASSERT_TRUE(copied_value.GetMap().find(key_a)->second.is_unsigned());
  EXPECT_EQ(value.GetMap().find(key_a)->second.GetInteger(),
            copied_value.GetMap().find(key_a)->second.GetInteger());

  CBORValue blank;
  blank = value.Clone();
  EXPECT_EQ(1u, blank.GetMap().size());
  ASSERT_EQ(blank.GetMap().count(key_a), 1u);
  ASSERT_TRUE(blank.GetMap().find(key_a)->second.is_unsigned());
  EXPECT_EQ(value.GetMap().find(key_a)->second.GetInteger(),
            blank.GetMap().find(key_a)->second.GetInteger());
}

TEST(CBORValuesTest, CopySimpleValue) {
  CBORValue value(CBORValue::SimpleValue::TRUE_VALUE);
  CBORValue copied_value(value.Clone());
  EXPECT_EQ(value.type(), copied_value.type());
  EXPECT_EQ(value.GetSimpleValue(), copied_value.GetSimpleValue());

  CBORValue blank;

  blank = value.Clone();
  EXPECT_EQ(value.type(), blank.type());
  EXPECT_EQ(value.GetSimpleValue(), blank.GetSimpleValue());
}

// Test move constructors and move-assignment
TEST(CBORValuesTest, MoveUnsigned) {
  CBORValue value(74);
  CBORValue moved_value(std::move(value));
  EXPECT_EQ(CBORValue::Type::UNSIGNED, moved_value.type());
  EXPECT_EQ(74u, moved_value.GetInteger());

  CBORValue blank;

  blank = CBORValue(654);
  EXPECT_EQ(CBORValue::Type::UNSIGNED, blank.type());
  EXPECT_EQ(654u, blank.GetInteger());
}

TEST(CBORValuesTest, MoveNegativeInteger) {
  CBORValue value(-74);
  CBORValue moved_value(std::move(value));
  EXPECT_EQ(CBORValue::Type::NEGATIVE, moved_value.type());
  EXPECT_EQ(-74, moved_value.GetInteger());

  CBORValue blank;

  blank = CBORValue(-654);
  EXPECT_EQ(CBORValue::Type::NEGATIVE, blank.type());
  EXPECT_EQ(-654, blank.GetInteger());
}

TEST(CBORValuesTest, MoveString) {
  CBORValue value("foobar");
  CBORValue moved_value(std::move(value));
  EXPECT_EQ(CBORValue::Type::STRING, moved_value.type());
  EXPECT_EQ("foobar", moved_value.GetString());

  CBORValue blank;

  blank = CBORValue("foobar");
  EXPECT_EQ(CBORValue::Type::STRING, blank.type());
  EXPECT_EQ("foobar", blank.GetString());
}

TEST(CBORValuesTest, MoveBytestring) {
  const CBORValue::BinaryValue bytes({0xF, 0x0, 0x0, 0xB, 0xA, 0x2});
  CBORValue value(bytes);
  CBORValue moved_value(std::move(value));
  EXPECT_EQ(CBORValue::Type::BYTE_STRING, moved_value.type());
  EXPECT_EQ(bytes, moved_value.GetBytestring());

  CBORValue blank;

  blank = CBORValue(bytes);
  EXPECT_EQ(CBORValue::Type::BYTE_STRING, blank.type());
  EXPECT_EQ(bytes, blank.GetBytestring());
}

TEST(CBORValuesTest, MoveConstructMap) {
  CBORValue::MapValue map;
  const CBORValue key_a("a");
  map[CBORValue("a")] = CBORValue(123);

  CBORValue value(std::move(map));
  CBORValue moved_value(std::move(value));
  ASSERT_EQ(CBORValue::Type::MAP, moved_value.type());
  ASSERT_EQ(moved_value.GetMap().count(key_a), 1u);
  ASSERT_TRUE(moved_value.GetMap().find(key_a)->second.is_unsigned());
  EXPECT_EQ(123u, moved_value.GetMap().find(key_a)->second.GetInteger());
}

TEST(CBORValuesTest, MoveAssignMap) {
  CBORValue::MapValue map;
  const CBORValue key_a("a");
  map[CBORValue("a")] = CBORValue(123);

  CBORValue blank;
  blank = CBORValue(std::move(map));
  ASSERT_TRUE(blank.is_map());
  ASSERT_EQ(blank.GetMap().count(key_a), 1u);
  ASSERT_TRUE(blank.GetMap().find(key_a)->second.is_unsigned());
  EXPECT_EQ(123u, blank.GetMap().find(key_a)->second.GetInteger());
}

TEST(CBORValuesTest, MoveArray) {
  CBORValue::ArrayValue array;
  array.emplace_back(123);
  CBORValue value(array);
  CBORValue moved_value(std::move(value));
  EXPECT_EQ(CBORValue::Type::ARRAY, moved_value.type());
  EXPECT_EQ(123u, moved_value.GetArray().back().GetInteger());

  CBORValue blank;
  blank = CBORValue(std::move(array));
  EXPECT_EQ(CBORValue::Type::ARRAY, blank.type());
  EXPECT_EQ(123u, blank.GetArray().back().GetInteger());
}

TEST(CBORValuesTest, MoveSimpleValue) {
  CBORValue value(CBORValue::SimpleValue::UNDEFINED);
  CBORValue moved_value(std::move(value));
  EXPECT_EQ(CBORValue::Type::SIMPLE_VALUE, moved_value.type());
  EXPECT_EQ(CBORValue::SimpleValue::UNDEFINED, moved_value.GetSimpleValue());

  CBORValue blank;

  blank = CBORValue(CBORValue::SimpleValue::UNDEFINED);
  EXPECT_EQ(CBORValue::Type::SIMPLE_VALUE, blank.type());
  EXPECT_EQ(CBORValue::SimpleValue::UNDEFINED, blank.GetSimpleValue());
}

TEST(CBORValuesTest, SelfSwap) {
  CBORValue test(1);
  std::swap(test, test);
  EXPECT_EQ(test.GetInteger(), 1u);
}

}  // namespace cbor
