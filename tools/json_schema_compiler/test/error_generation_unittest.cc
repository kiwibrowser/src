// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/json_schema_compiler/test/error_generation.h"

#include <memory>

#include "base/json/json_writer.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "tools/json_schema_compiler/test/test_util.h"

using namespace test::api::error_generation;
using base::Value;
using json_schema_compiler::test_util::Dictionary;
using json_schema_compiler::test_util::List;

template <typename T>
base::string16 GetPopulateError(const base::Value& value) {
  base::string16 error;
  T test_type;
  T::Populate(value, &test_type, &error);
  return error;
}

testing::AssertionResult EqualsUtf16(const std::string& expected,
                                     const base::string16& actual) {
  if (base::ASCIIToUTF16(expected) == actual)
    return testing::AssertionSuccess();
  return testing::AssertionFailure() << "\n    actual:     " << actual
                                     << "\n    expected:   " << expected;
}

// GenerateTypePopulate errors

TEST(JsonSchemaCompilerErrorTest, RequiredPropertyPopulate) {
  {
    std::unique_ptr<base::DictionaryValue> value =
        Dictionary("string", std::make_unique<base::Value>("bling"));
    EXPECT_TRUE(EqualsUtf16("", GetPopulateError<TestType>(*value)));
  }
  {
    auto value = std::make_unique<base::Value>(base::Value::Type::BINARY);
    EXPECT_TRUE(EqualsUtf16("expected dictionary, got binary",
        GetPopulateError<TestType>(*value)));
  }
}

TEST(JsonSchemaCompilerErrorTest, UnexpectedTypePopulation) {
  {
    std::unique_ptr<base::ListValue> value(new base::ListValue());
    EXPECT_TRUE(EqualsUtf16("",
        GetPopulateError<ChoiceType::Integers>(*value)));
  }
  {
    auto value = std::make_unique<base::Value>(base::Value::Type::BINARY);
    EXPECT_TRUE(EqualsUtf16("expected integers or integer, got binary",
        GetPopulateError<ChoiceType::Integers>(*value)));
  }
}

// GenerateTypePopulateProperty errors

TEST(JsonSchemaCompilerErrorTest, TypeIsRequired) {
  {
    std::unique_ptr<base::DictionaryValue> value =
        Dictionary("integers", std::make_unique<Value>(5));
    EXPECT_TRUE(EqualsUtf16("", GetPopulateError<ChoiceType>(*value)));
  }
  {
    std::unique_ptr<base::DictionaryValue> value(new base::DictionaryValue());
    EXPECT_TRUE(EqualsUtf16("'integers' is required",
        GetPopulateError<ChoiceType>(*value)));
  }
}

// GenerateParamsCheck errors

TEST(JsonSchemaCompilerErrorTest, TooManyParameters) {
  {
    std::unique_ptr<base::ListValue> params_value =
        List(std::make_unique<Value>(5));
    base::string16 error;
    EXPECT_TRUE(TestFunction::Params::Create(*params_value, &error));
  }
  {
    std::unique_ptr<base::ListValue> params_value =
        List(std::make_unique<Value>(5), std::make_unique<Value>(5));
    base::string16 error;
    EXPECT_FALSE(TestFunction::Params::Create(*params_value, &error));
    EXPECT_TRUE(EqualsUtf16("expected 1 arguments, got 2", error));
  }
}

// GenerateFunctionParamsCreate errors

TEST(JsonSchemaCompilerErrorTest, ParamIsRequired) {
  {
    std::unique_ptr<base::ListValue> params_value =
        List(std::make_unique<Value>(5));
    base::string16 error;
    EXPECT_TRUE(TestFunction::Params::Create(*params_value, &error));
  }
  {
    std::unique_ptr<base::ListValue> params_value =
        List(std::make_unique<Value>());
    base::string16 error;
    EXPECT_FALSE(TestFunction::Params::Create(*params_value, &error));
    EXPECT_TRUE(EqualsUtf16("'num' is required", error));
  }
}

// GeneratePopulateVariableFromValue errors

TEST(JsonSchemaCompilerErrorTest, WrongPropertyValueType) {
  {
    std::unique_ptr<base::DictionaryValue> value =
        Dictionary("string", std::make_unique<base::Value>("yes"));
    EXPECT_TRUE(EqualsUtf16("", GetPopulateError<TestType>(*value)));
  }
  {
    std::unique_ptr<base::DictionaryValue> value =
        Dictionary("string", std::make_unique<Value>(1.1));
    EXPECT_TRUE(EqualsUtf16("'string': expected string, got double",
        GetPopulateError<TestType>(*value)));
  }
}

TEST(JsonSchemaCompilerErrorTest, WrongParameterCreationType) {
  {
    base::string16 error;
    std::unique_ptr<base::ListValue> params_value =
        List(std::make_unique<base::Value>("Yeah!"));
    EXPECT_TRUE(TestString::Params::Create(*params_value, &error));
  }
  {
    std::unique_ptr<base::ListValue> params_value =
        List(std::make_unique<Value>(5));
    base::string16 error;
    EXPECT_FALSE(TestTypeInObject::Params::Create(*params_value, &error));
    EXPECT_TRUE(EqualsUtf16("'paramObject': expected dictionary, got integer",
        error));
  }
}

TEST(JsonSchemaCompilerErrorTest, WrongTypeValueType) {
  {
    std::unique_ptr<base::DictionaryValue> value(new base::DictionaryValue());
    EXPECT_TRUE(EqualsUtf16("", GetPopulateError<ObjectType>(*value)));
  }
  {
    std::unique_ptr<base::DictionaryValue> value =
        Dictionary("otherType", std::make_unique<Value>(1.1));
    ObjectType out;
    base::string16 error;
    EXPECT_TRUE(ObjectType::Populate(*value, &out, &error));
    EXPECT_TRUE(EqualsUtf16("'otherType': expected dictionary, got double",
        error));
    EXPECT_EQ(NULL, out.other_type.get());
  }
}

TEST(JsonSchemaCompilerErrorTest, UnableToPopulateArray) {
  {
    std::unique_ptr<base::ListValue> params_value =
        List(std::make_unique<Value>(5));
    EXPECT_TRUE(EqualsUtf16("",
        GetPopulateError<ChoiceType::Integers>(*params_value)));
  }
  {
    std::unique_ptr<base::ListValue> params_value =
        List(std::make_unique<Value>(5), std::make_unique<Value>(false));
    EXPECT_TRUE(EqualsUtf16(
        "expected integer, got boolean; unable to populate array 'integers'",
        GetPopulateError<ChoiceType::Integers>(*params_value)));
  }
}

TEST(JsonSchemaCompilerErrorTest, BinaryTypeExpected) {
  {
    std::unique_ptr<base::DictionaryValue> value = Dictionary(
        "data", std::make_unique<base::Value>(base::Value::Type::BINARY));
    EXPECT_TRUE(EqualsUtf16("", GetPopulateError<BinaryData>(*value)));
  }
  {
    std::unique_ptr<base::DictionaryValue> value =
        Dictionary("data", std::make_unique<Value>(1.1));
    EXPECT_TRUE(EqualsUtf16("'data': expected binary, got double",
        GetPopulateError<BinaryData>(*value)));
  }
}

TEST(JsonSchemaCompilerErrorTest, ListExpected) {
  {
    std::unique_ptr<base::DictionaryValue> value =
        Dictionary("TheArray", std::make_unique<base::ListValue>());
    EXPECT_TRUE(EqualsUtf16("", GetPopulateError<ArrayObject>(*value)));
  }
  {
    std::unique_ptr<base::DictionaryValue> value =
        Dictionary("TheArray", std::make_unique<Value>(5));
    EXPECT_TRUE(EqualsUtf16("'TheArray': expected list, got integer",
        GetPopulateError<ArrayObject>(*value)));
  }
}

// GenerateStringToEnumConversion errors

TEST(JsonSchemaCompilerErrorTest, BadEnumValue) {
  {
    std::unique_ptr<base::DictionaryValue> value =
        Dictionary("enumeration", std::make_unique<base::Value>("one"));
    EXPECT_TRUE(EqualsUtf16("", GetPopulateError<HasEnumeration>(*value)));
  }
  {
    std::unique_ptr<base::DictionaryValue> value =
        Dictionary("enumeration", std::make_unique<base::Value>("bad sauce"));
    EXPECT_TRUE(EqualsUtf16("'Enumeration': expected \"one\" or \"two\" "
              "or \"three\", got \"bad sauce\"",
        GetPopulateError<HasEnumeration>(*value)));
  }
}

// Warn but don't fail out errors

TEST(JsonSchemaCompilerErrorTest, WarnOnOptionalFailure) {
  {
    std::unique_ptr<base::DictionaryValue> value =
        Dictionary("string", std::make_unique<base::Value>("bling"));
    EXPECT_TRUE(EqualsUtf16("", GetPopulateError<OptionalTestType>(*value)));
  }
  {
    std::unique_ptr<base::DictionaryValue> value =
        Dictionary("string", std::make_unique<base::Value>(1));

    OptionalTestType out;
    base::string16 error;
    EXPECT_TRUE(OptionalTestType::Populate(*value, &out, &error));
    EXPECT_TRUE(EqualsUtf16("'string': expected string, got integer",
        error));
    EXPECT_EQ(NULL, out.string.get());
  }
}

TEST(JsonSchemaCompilerErrorTest, OptionalBinaryTypeFailure) {
  {
    std::unique_ptr<base::DictionaryValue> value = Dictionary(
        "data", std::make_unique<base::Value>(base::Value::Type::BINARY));
    EXPECT_TRUE(EqualsUtf16("", GetPopulateError<OptionalBinaryData>(*value)));
  }
  {
    // There's a bug with silent failures if the key doesn't exist.
    std::unique_ptr<base::DictionaryValue> value =
        Dictionary("data", std::make_unique<base::Value>(1));

    OptionalBinaryData out;
    base::string16 error;
    EXPECT_TRUE(OptionalBinaryData::Populate(*value, &out, &error));
    EXPECT_TRUE(EqualsUtf16("'data': expected binary, got integer",
        error));
    EXPECT_EQ(NULL, out.data.get());
  }
}

TEST(JsonSchemaCompilerErrorTest, OptionalArrayTypeFailure) {
  {
    std::unique_ptr<base::DictionaryValue> value =
        Dictionary("TheArray", std::make_unique<base::ListValue>());
    EXPECT_TRUE(EqualsUtf16("", GetPopulateError<ArrayObject>(*value)));
  }
  {
    std::unique_ptr<base::DictionaryValue> value =
        Dictionary("TheArray", std::make_unique<Value>(5));
    ArrayObject out;
    base::string16 error;
    EXPECT_TRUE(ArrayObject::Populate(*value, &out, &error));
    EXPECT_TRUE(EqualsUtf16("'TheArray': expected list, got integer",
        error));
    EXPECT_EQ(NULL, out.the_array.get());
  }
}

TEST(JsonSchemaCompilerErrorTest, OptionalUnableToPopulateArray) {
  {
    std::unique_ptr<base::ListValue> params_value =
        List(std::make_unique<Value>(5));
    EXPECT_TRUE(EqualsUtf16("",
        GetPopulateError<OptionalChoiceType::Integers>(*params_value)));
  }
  {
    std::unique_ptr<base::ListValue> params_value =
        List(std::make_unique<Value>(5), std::make_unique<Value>(false));
    OptionalChoiceType::Integers out;
    base::string16 error;
    EXPECT_TRUE(OptionalChoiceType::Integers::Populate(*params_value, &out,
        &error));
    EXPECT_TRUE(EqualsUtf16(
        "expected integer, got boolean; unable to populate array 'integers'",
        error));
    EXPECT_EQ(NULL, out.as_integer.get());
  }
}

TEST(JsonSchemaCompilerErrorTest, MultiplePopulationErrors) {
  {
    std::unique_ptr<base::DictionaryValue> value =
        Dictionary("TheArray", std::make_unique<Value>(5));
    ArrayObject out;
    base::string16 error;
    EXPECT_TRUE(ArrayObject::Populate(*value, &out, &error));
    EXPECT_TRUE(EqualsUtf16("'TheArray': expected list, got integer",
        error));
    EXPECT_EQ(NULL, out.the_array.get());

    EXPECT_TRUE(ArrayObject::Populate(*value, &out, &error));
    EXPECT_TRUE(EqualsUtf16("'TheArray': expected list, got integer; "
        "'TheArray': expected list, got integer",
        error));
    EXPECT_EQ(NULL, out.the_array.get());
  }
}

TEST(JsonSchemaCompilerErrorTest, TooManyKeys) {
  {
    std::unique_ptr<base::DictionaryValue> value =
        Dictionary("string", std::make_unique<base::Value>("yes"));
    EXPECT_TRUE(EqualsUtf16("", GetPopulateError<TestType>(*value)));
  }
  {
    std::unique_ptr<base::DictionaryValue> value =
        Dictionary("string", std::make_unique<base::Value>("yes"), "ohno",
                   std::make_unique<base::Value>("many values"));
    EXPECT_TRUE(EqualsUtf16("found unexpected key 'ohno'",
        GetPopulateError<TestType>(*value)));
  }
}
