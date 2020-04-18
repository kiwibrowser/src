// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/json_schema_compiler/test/arrays.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/macros.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "tools/json_schema_compiler/test/enums.h"

using namespace test::api::arrays;

namespace {

// TODO(calamity): Change to AppendString etc once kalman's patch goes through
static std::unique_ptr<base::DictionaryValue> CreateBasicArrayTypeDictionary() {
  auto value = std::make_unique<base::DictionaryValue>();
  auto strings_value = std::make_unique<base::ListValue>();
  strings_value->AppendString("a");
  strings_value->AppendString("b");
  strings_value->AppendString("c");
  strings_value->AppendString("it's easy as");
  auto integers_value = std::make_unique<base::ListValue>();
  integers_value->AppendInteger(1);
  integers_value->AppendInteger(2);
  integers_value->AppendInteger(3);
  auto booleans_value = std::make_unique<base::ListValue>();
  booleans_value->AppendBoolean(false);
  booleans_value->AppendBoolean(true);
  auto numbers_value = std::make_unique<base::ListValue>();
  numbers_value->AppendDouble(6.1);
  value->Set("numbers", std::move(numbers_value));
  value->Set("booleans", std::move(booleans_value));
  value->Set("strings", std::move(strings_value));
  value->Set("integers", std::move(integers_value));
  return value;
}

std::unique_ptr<base::DictionaryValue> CreateItemValue(int val) {
  auto value = std::make_unique<base::DictionaryValue>();
  value->SetInteger("val", val);
  return value;
}

}  // namespace

TEST(JsonSchemaCompilerArrayTest, BasicArrayType) {
  {
    std::unique_ptr<base::DictionaryValue> value =
        CreateBasicArrayTypeDictionary();
    std::unique_ptr<BasicArrayType> basic_array_type(new BasicArrayType());
    ASSERT_TRUE(BasicArrayType::Populate(*value, basic_array_type.get()));
    EXPECT_TRUE(value->Equals(basic_array_type->ToValue().get()));
  }
}

TEST(JsonSchemaCompilerArrayTest, EnumArrayReference) {
  // { "types": ["one", "two", "three"] }
  auto types = std::make_unique<base::ListValue>();
  types->AppendString("one");
  types->AppendString("two");
  types->AppendString("three");
  base::DictionaryValue value;
  value.Set("types", std::move(types));

  EnumArrayReference enum_array_reference;

  // Test Populate.
  ASSERT_TRUE(EnumArrayReference::Populate(value, &enum_array_reference));

  Enumeration expected_types[] = {ENUMERATION_ONE, ENUMERATION_TWO,
                                  ENUMERATION_THREE};
  EXPECT_EQ(std::vector<Enumeration>(
                expected_types, expected_types + arraysize(expected_types)),
            enum_array_reference.types);

  // Test ToValue.
  std::unique_ptr<base::Value> as_value(enum_array_reference.ToValue());
  EXPECT_TRUE(value.Equals(as_value.get())) << value << " != " << *as_value;
}

TEST(JsonSchemaCompilerArrayTest, EnumArrayMixed) {
  // { "types": ["one", "two", "three"] }
  auto infile_enums = std::make_unique<base::ListValue>();
  infile_enums->AppendString("one");
  infile_enums->AppendString("two");
  infile_enums->AppendString("three");

  auto external_enums = std::make_unique<base::ListValue>();
  external_enums->AppendString("one");
  external_enums->AppendString("two");
  external_enums->AppendString("three");

  base::DictionaryValue value;
  value.Set("infile_enums", std::move(infile_enums));
  value.Set("external_enums", std::move(external_enums));

  EnumArrayMixed enum_array_mixed;

  // Test Populate.
  ASSERT_TRUE(EnumArrayMixed::Populate(value, &enum_array_mixed));

  Enumeration expected_infile_types[] = {ENUMERATION_ONE, ENUMERATION_TWO,
                                         ENUMERATION_THREE};
  EXPECT_EQ(std::vector<Enumeration>(
                expected_infile_types,
                expected_infile_types + arraysize(expected_infile_types)),
            enum_array_mixed.infile_enums);

  test::api::enums::Enumeration expected_external_types[] = {
      test::api::enums::ENUMERATION_ONE, test::api::enums::ENUMERATION_TWO,
      test::api::enums::ENUMERATION_THREE};
  EXPECT_EQ(std::vector<test::api::enums::Enumeration>(
                expected_external_types,
                expected_external_types + arraysize(expected_external_types)),
            enum_array_mixed.external_enums);

  // Test ToValue.
  std::unique_ptr<base::Value> as_value(enum_array_mixed.ToValue());
  EXPECT_TRUE(value.Equals(as_value.get())) << value << " != " << *as_value;
}

TEST(JsonSchemaCompilerArrayTest, OptionalEnumArrayType) {
  {
    std::vector<Enumeration> enums;
    enums.push_back(ENUMERATION_ONE);
    enums.push_back(ENUMERATION_TWO);
    enums.push_back(ENUMERATION_THREE);

    auto types = std::make_unique<base::ListValue>();
    for (size_t i = 0; i < enums.size(); ++i)
      types->AppendString(ToString(enums[i]));

    base::DictionaryValue value;
    value.Set("types", std::move(types));

    OptionalEnumArrayType enum_array_type;
    ASSERT_TRUE(OptionalEnumArrayType::Populate(value, &enum_array_type));
    EXPECT_EQ(enums, *enum_array_type.types);
  }
  {
    base::DictionaryValue value;
    auto enum_array = std::make_unique<base::ListValue>();
    enum_array->AppendString("invalid");

    value.Set("types", std::move(enum_array));
    OptionalEnumArrayType enum_array_type;
    ASSERT_FALSE(OptionalEnumArrayType::Populate(value, &enum_array_type));
    EXPECT_TRUE(enum_array_type.types->empty());
  }
}

TEST(JsonSchemaCompilerArrayTest, RefArrayType) {
  {
    auto value = std::make_unique<base::DictionaryValue>();
    auto ref_array = std::make_unique<base::ListValue>();
    ref_array->Append(CreateItemValue(1));
    ref_array->Append(CreateItemValue(2));
    ref_array->Append(CreateItemValue(3));
    value->Set("refs", std::move(ref_array));
    auto ref_array_type = std::make_unique<RefArrayType>();
    EXPECT_TRUE(RefArrayType::Populate(*value, ref_array_type.get()));
    ASSERT_EQ(3u, ref_array_type->refs.size());
    EXPECT_EQ(1, ref_array_type->refs[0].val);
    EXPECT_EQ(2, ref_array_type->refs[1].val);
    EXPECT_EQ(3, ref_array_type->refs[2].val);
  }
  {
    auto value = std::make_unique<base::DictionaryValue>();
    auto not_ref_array = std::make_unique<base::ListValue>();
    not_ref_array->Append(CreateItemValue(1));
    not_ref_array->AppendInteger(3);
    value->Set("refs", std::move(not_ref_array));
    auto ref_array_type = std::make_unique<RefArrayType>();
    EXPECT_FALSE(RefArrayType::Populate(*value, ref_array_type.get()));
  }
}

TEST(JsonSchemaCompilerArrayTest, IntegerArrayParamsCreate) {
  std::unique_ptr<base::ListValue> params_value(new base::ListValue());
  std::unique_ptr<base::ListValue> integer_array(new base::ListValue());
  integer_array->AppendInteger(2);
  integer_array->AppendInteger(4);
  integer_array->AppendInteger(8);
  params_value->Append(std::move(integer_array));
  std::unique_ptr<IntegerArray::Params> params(
      IntegerArray::Params::Create(*params_value));
  EXPECT_TRUE(params.get());
  ASSERT_EQ(3u, params->nums.size());
  EXPECT_EQ(2, params->nums[0]);
  EXPECT_EQ(4, params->nums[1]);
  EXPECT_EQ(8, params->nums[2]);
}

TEST(JsonSchemaCompilerArrayTest, AnyArrayParamsCreate) {
  std::unique_ptr<base::ListValue> params_value(new base::ListValue());
  std::unique_ptr<base::ListValue> any_array(new base::ListValue());
  any_array->AppendInteger(1);
  any_array->AppendString("test");
  any_array->Append(CreateItemValue(2));
  params_value->Append(std::move(any_array));
  std::unique_ptr<AnyArray::Params> params(
      AnyArray::Params::Create(*params_value));
  EXPECT_TRUE(params.get());
  ASSERT_EQ(3u, params->anys.size());
  int int_temp = 0;
  EXPECT_TRUE(params->anys[0]->GetAsInteger(&int_temp));
  EXPECT_EQ(1, int_temp);
}

TEST(JsonSchemaCompilerArrayTest, ObjectArrayParamsCreate) {
  std::unique_ptr<base::ListValue> params_value(new base::ListValue());
  std::unique_ptr<base::ListValue> item_array(new base::ListValue());
  item_array->Append(CreateItemValue(1));
  item_array->Append(CreateItemValue(2));
  params_value->Append(std::move(item_array));
  std::unique_ptr<ObjectArray::Params> params(
      ObjectArray::Params::Create(*params_value));
  EXPECT_TRUE(params.get());
  ASSERT_EQ(2u, params->objects.size());
  EXPECT_EQ(1, params->objects[0].additional_properties["val"]);
  EXPECT_EQ(2, params->objects[1].additional_properties["val"]);
}

TEST(JsonSchemaCompilerArrayTest, RefArrayParamsCreate) {
  std::unique_ptr<base::ListValue> params_value(new base::ListValue());
  std::unique_ptr<base::ListValue> item_array(new base::ListValue());
  item_array->Append(CreateItemValue(1));
  item_array->Append(CreateItemValue(2));
  params_value->Append(std::move(item_array));
  std::unique_ptr<RefArray::Params> params(
      RefArray::Params::Create(*params_value));
  EXPECT_TRUE(params.get());
  ASSERT_EQ(2u, params->refs.size());
  EXPECT_EQ(1, params->refs[0].val);
  EXPECT_EQ(2, params->refs[1].val);
}

TEST(JsonSchemaCompilerArrayTest, ReturnIntegerArrayResultCreate) {
  std::vector<int> integers;
  integers.push_back(1);
  integers.push_back(2);
  std::unique_ptr<base::ListValue> results =
      ReturnIntegerArray::Results::Create(integers);

  base::ListValue expected;
  std::unique_ptr<base::ListValue> expected_argument(new base::ListValue());
  expected_argument->AppendInteger(1);
  expected_argument->AppendInteger(2);
  expected.Append(std::move(expected_argument));
  EXPECT_TRUE(results->Equals(&expected));
}

TEST(JsonSchemaCompilerArrayTest, ReturnRefArrayResultCreate) {
  std::vector<Item> items;
  items.push_back(Item());
  items.push_back(Item());
  items[0].val = 1;
  items[1].val = 2;
  std::unique_ptr<base::ListValue> results =
      ReturnRefArray::Results::Create(items);

  base::ListValue expected;
  std::unique_ptr<base::ListValue> expected_argument(new base::ListValue());
  std::unique_ptr<base::DictionaryValue> first(new base::DictionaryValue());
  first->SetInteger("val", 1);
  expected_argument->Append(std::move(first));
  std::unique_ptr<base::DictionaryValue> second(new base::DictionaryValue());
  second->SetInteger("val", 2);
  expected_argument->Append(std::move(second));
  expected.Append(std::move(expected_argument));
  EXPECT_TRUE(results->Equals(&expected));
}
