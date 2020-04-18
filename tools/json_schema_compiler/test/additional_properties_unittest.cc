// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/json_schema_compiler/test/additional_properties.h"

#include <memory>
#include <utility>

#include "testing/gtest/include/gtest/gtest.h"

using namespace test::api::additional_properties;

TEST(JsonSchemaCompilerAdditionalPropertiesTest,
    AdditionalPropertiesTypePopulate) {
  {
    std::unique_ptr<base::ListValue> list_value(new base::ListValue());
    list_value->AppendString("asdf");
    list_value->AppendInteger(4);
    std::unique_ptr<base::DictionaryValue> type_value(
        new base::DictionaryValue());
    type_value->SetString("string", "value");
    type_value->SetInteger("other", 9);
    type_value->Set("another", std::move(list_value));
    std::unique_ptr<AdditionalPropertiesType> type(
        new AdditionalPropertiesType());
    ASSERT_TRUE(AdditionalPropertiesType::Populate(*type_value, type.get()));
    EXPECT_TRUE(type->additional_properties.Equals(type_value.get()));
  }
  {
    std::unique_ptr<base::DictionaryValue> type_value(
        new base::DictionaryValue());
    type_value->SetInteger("string", 3);
    std::unique_ptr<AdditionalPropertiesType> type(
        new AdditionalPropertiesType());
    EXPECT_FALSE(AdditionalPropertiesType::Populate(*type_value, type.get()));
  }
}

TEST(JsonSchemaCompilerAdditionalPropertiesTest,
    AdditionalPropertiesParamsCreate) {
  std::unique_ptr<base::DictionaryValue> param_object_value(
      new base::DictionaryValue());
  param_object_value->SetString("str", "a");
  param_object_value->SetInteger("num", 1);
  std::unique_ptr<base::ListValue> params_value(new base::ListValue());
  params_value->Append(param_object_value->CreateDeepCopy());
  std::unique_ptr<AdditionalProperties::Params> params(
      AdditionalProperties::Params::Create(*params_value));
  EXPECT_TRUE(params.get());
  EXPECT_TRUE(params->param_object.additional_properties.Equals(
      param_object_value.get()));
}

TEST(JsonSchemaCompilerAdditionalPropertiesTest,
    ReturnAdditionalPropertiesResultCreate) {
  ReturnAdditionalProperties::Results::ResultObject result_object;
  result_object.integer = 5;
  result_object.additional_properties["key"] = "value";

  base::ListValue expected;
  {
    std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
    dict->SetInteger("integer", 5);
    dict->SetString("key", "value");
    expected.Append(std::move(dict));
  }

  EXPECT_EQ(expected,
            *ReturnAdditionalProperties::Results::Create(result_object));
}
