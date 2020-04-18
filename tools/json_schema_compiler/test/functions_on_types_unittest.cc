// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/json_schema_compiler/test/functions_on_types.h"

#include <utility>

#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

using namespace test::api::functions_on_types;

TEST(JsonSchemaCompilerFunctionsOnTypesTest, StorageAreaGetParamsCreate) {
  {
    std::unique_ptr<base::ListValue> params_value(new base::ListValue());
    std::unique_ptr<StorageArea::Get::Params> params(
        StorageArea::Get::Params::Create(*params_value));
    ASSERT_TRUE(params);
    EXPECT_FALSE(params->keys);
  }
  {
    std::unique_ptr<base::ListValue> params_value(new base::ListValue());
    params_value->AppendInteger(9);
    std::unique_ptr<StorageArea::Get::Params> params(
        StorageArea::Get::Params::Create(*params_value));
    EXPECT_FALSE(params);
  }
  {
    std::unique_ptr<base::ListValue> params_value(new base::ListValue());
    params_value->AppendString("test");
    std::unique_ptr<StorageArea::Get::Params> params(
        StorageArea::Get::Params::Create(*params_value));
    ASSERT_TRUE(params);
    ASSERT_TRUE(params->keys);
    EXPECT_EQ("test", *params->keys->as_string);
  }
  {
    std::unique_ptr<base::DictionaryValue> keys_object_value(
        new base::DictionaryValue());
    keys_object_value->SetInteger("integer", 5);
    keys_object_value->SetString("string", "string");
    std::unique_ptr<base::ListValue> params_value(new base::ListValue());
    params_value->Append(keys_object_value->CreateDeepCopy());
    std::unique_ptr<StorageArea::Get::Params> params(
        StorageArea::Get::Params::Create(*params_value));
    ASSERT_TRUE(params);
    ASSERT_TRUE(params->keys);
    EXPECT_TRUE(keys_object_value->Equals(
        &params->keys->as_object->additional_properties));
  }
}

TEST(JsonSchemaCompilerFunctionsOnTypesTest, StorageAreaGetResultCreate) {
  StorageArea::Get::Results::Items items;
  items.additional_properties.SetDouble("asdf", 0.1);
  items.additional_properties.SetString("sdfg", "zxcv");
  std::unique_ptr<base::ListValue> results =
      StorageArea::Get::Results::Create(items);
  base::DictionaryValue* item_result = NULL;
  ASSERT_TRUE(results->GetDictionary(0, &item_result));
  EXPECT_TRUE(item_result->Equals(&items.additional_properties));
}

TEST(JsonSchemaCompilerFunctionsOnTypesTest, ChromeSettingGetParamsCreate) {
  std::unique_ptr<base::DictionaryValue> details_value(
      new base::DictionaryValue());
  details_value->SetBoolean("incognito", true);
  std::unique_ptr<base::ListValue> params_value(new base::ListValue());
  params_value->Append(std::move(details_value));
  std::unique_ptr<ChromeSetting::Get::Params> params(
      ChromeSetting::Get::Params::Create(*params_value));
  EXPECT_TRUE(params.get());
  EXPECT_TRUE(*params->details.incognito);
}
