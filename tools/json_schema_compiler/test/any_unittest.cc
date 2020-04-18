// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/json_schema_compiler/test/any.h"

using namespace test::api::any;

TEST(JsonSchemaCompilerAnyTest, AnyTypePopulate) {
  {
    AnyType any_type;
    std::unique_ptr<base::DictionaryValue> any_type_value(
        new base::DictionaryValue());
    any_type_value->SetString("any", "value");
    EXPECT_TRUE(AnyType::Populate(*any_type_value, &any_type));
    std::unique_ptr<base::Value> any_type_to_value(any_type.ToValue());
    EXPECT_TRUE(any_type_value->Equals(any_type_to_value.get()));
  }
  {
    AnyType any_type;
    std::unique_ptr<base::DictionaryValue> any_type_value(
        new base::DictionaryValue());
    any_type_value->SetInteger("any", 5);
    EXPECT_TRUE(AnyType::Populate(*any_type_value, &any_type));
    std::unique_ptr<base::Value> any_type_to_value(any_type.ToValue());
    EXPECT_TRUE(any_type_value->Equals(any_type_to_value.get()));
  }
}

TEST(JsonSchemaCompilerAnyTest, OptionalAnyParamsCreate) {
  {
    std::unique_ptr<base::ListValue> params_value(new base::ListValue());
    std::unique_ptr<OptionalAny::Params> params(
        OptionalAny::Params::Create(*params_value));
    EXPECT_TRUE(params.get());
    EXPECT_FALSE(params->any_name.get());
  }
  {
    std::unique_ptr<base::ListValue> params_value(new base::ListValue());
    std::unique_ptr<base::Value> param(new base::Value("asdf"));
    params_value->Append(param->CreateDeepCopy());
    std::unique_ptr<OptionalAny::Params> params(
        OptionalAny::Params::Create(*params_value));
    ASSERT_TRUE(params);
    ASSERT_TRUE(params->any_name);
    EXPECT_TRUE(params->any_name->Equals(param.get()));
  }
  {
    std::unique_ptr<base::ListValue> params_value(new base::ListValue());
    std::unique_ptr<base::Value> param(new base::Value(true));
    params_value->Append(param->CreateDeepCopy());
    std::unique_ptr<OptionalAny::Params> params(
        OptionalAny::Params::Create(*params_value));
    ASSERT_TRUE(params);
    ASSERT_TRUE(params->any_name);
    EXPECT_TRUE(params->any_name->Equals(param.get()));
  }
}
