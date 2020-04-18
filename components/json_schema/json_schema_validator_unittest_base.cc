// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/json_schema/json_schema_validator_unittest_base.h"

#include <cfloat>
#include <cmath>
#include <limits>
#include <memory>

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/json/json_file_value_serializer.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "components/json_schema/json_schema_constants.h"
#include "components/json_schema/json_schema_validator.h"

namespace schema = json_schema_constants;

namespace {

#define TEST_SOURCE base::StringPrintf("%s:%i", __FILE__, __LINE__)

base::Value* LoadValue(const std::string& filename) {
  base::FilePath path;
  base::PathService::Get(base::DIR_SOURCE_ROOT, &path);
  path = path.AppendASCII("components")
             .AppendASCII("test")
             .AppendASCII("data")
             .AppendASCII("json_schema")
             .AppendASCII(filename);
  EXPECT_TRUE(base::PathExists(path));

  std::string error_message;
  JSONFileValueDeserializer deserializer(path);
  base::Value* result =
      deserializer.Deserialize(nullptr, &error_message).release();
  if (!result)
    ADD_FAILURE() << "Could not parse JSON: " << error_message;
  return result;
}

base::Value* LoadValue(const std::string& filename, base::Value::Type type) {
  std::unique_ptr<base::Value> result(LoadValue(filename));
  if (!result)
    return nullptr;
  if (result->type() != type) {
    ADD_FAILURE() << "Expected type " << type << ", got: " << result->type();
    return nullptr;
  }
  return result.release();
}

base::ListValue* LoadList(const std::string& filename) {
  return static_cast<base::ListValue*>(
      LoadValue(filename, base::Value::Type::LIST));
}

base::DictionaryValue* LoadDictionary(const std::string& filename) {
  return static_cast<base::DictionaryValue*>(
      LoadValue(filename, base::Value::Type::DICTIONARY));
}

}  // namespace


JSONSchemaValidatorTestBase::JSONSchemaValidatorTestBase() {
}

void JSONSchemaValidatorTestBase::RunTests() {
  TestComplex();
  TestStringPattern();
  TestEnum();
  TestChoices();
  TestExtends();
  TestObject();
  TestTypeReference();
  TestArrayTuple();
  TestArrayNonTuple();
  TestString();
  TestNumber();
  TestTypeClassifier();
  TestTypes();
}

void JSONSchemaValidatorTestBase::TestComplex() {
  std::unique_ptr<base::DictionaryValue> schema(
      LoadDictionary("complex_schema.json"));
  std::unique_ptr<base::ListValue> instance(LoadList("complex_instance.json"));

  ASSERT_TRUE(schema.get());
  ASSERT_TRUE(instance.get());

  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), nullptr);
  instance->Remove(instance->GetSize() - 1, nullptr);
  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), nullptr);
  instance->Append(std::make_unique<base::DictionaryValue>());
  ExpectNotValid(
      TEST_SOURCE, instance.get(), schema.get(), nullptr, "1",
      JSONSchemaValidator::FormatErrorMessage(
          JSONSchemaValidator::kInvalidType, schema::kNumber, schema::kObject));
  instance->Remove(instance->GetSize() - 1, nullptr);

  base::DictionaryValue* item = nullptr;
  ASSERT_TRUE(instance->GetDictionary(0, &item));
  item->SetString("url", "xxxxxxxxxxx");

  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), nullptr, "0.url",
                 JSONSchemaValidator::FormatErrorMessage(
                     JSONSchemaValidator::kStringMaxLength, "10"));
}

void JSONSchemaValidatorTestBase::TestStringPattern() {
  std::unique_ptr<base::DictionaryValue> schema(new base::DictionaryValue());
  schema->SetString(schema::kType, schema::kString);
  schema->SetString(schema::kPattern, "foo+");

  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value("foo")).get(),
              schema.get(), nullptr);
  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value("foooooo")).get(),
              schema.get(), nullptr);
  ExpectNotValid(TEST_SOURCE,
                 std::unique_ptr<base::Value>(new base::Value("bar")).get(),
                 schema.get(), nullptr, std::string(),
                 JSONSchemaValidator::FormatErrorMessage(
                     JSONSchemaValidator::kStringPattern, "foo+"));
}

void JSONSchemaValidatorTestBase::TestEnum() {
  std::unique_ptr<base::DictionaryValue> schema(
      LoadDictionary("enum_schema.json"));

  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value("foo")).get(),
              schema.get(), nullptr);
  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value(42)).get(),
              schema.get(), nullptr);
  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value(false)).get(),
              schema.get(), nullptr);

  ExpectNotValid(
      TEST_SOURCE, std::unique_ptr<base::Value>(new base::Value("42")).get(),
      schema.get(), nullptr, std::string(), JSONSchemaValidator::kInvalidEnum);
  ExpectNotValid(TEST_SOURCE, std::make_unique<base::Value>().get(),
                 schema.get(), nullptr, std::string(),
                 JSONSchemaValidator::kInvalidEnum);
}

void JSONSchemaValidatorTestBase::TestChoices() {
  std::unique_ptr<base::DictionaryValue> schema(
      LoadDictionary("choices_schema.json"));

  ExpectValid(TEST_SOURCE, std::make_unique<base::Value>().get(), schema.get(),
              nullptr);
  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value(42)).get(),
              schema.get(), nullptr);

  std::unique_ptr<base::DictionaryValue> instance(new base::DictionaryValue());
  instance->SetString("foo", "bar");
  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), nullptr);

  ExpectNotValid(TEST_SOURCE,
                 std::unique_ptr<base::Value>(new base::Value("foo")).get(),
                 schema.get(), nullptr, std::string(),
                 JSONSchemaValidator::kInvalidChoice);
  ExpectNotValid(TEST_SOURCE,
                 std::unique_ptr<base::Value>(new base::ListValue()).get(),
                 schema.get(), nullptr, std::string(),
                 JSONSchemaValidator::kInvalidChoice);

  instance->SetInteger("foo", 42);
  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), nullptr,
                 std::string(), JSONSchemaValidator::kInvalidChoice);
}

void JSONSchemaValidatorTestBase::TestExtends() {
  // TODO(aa): JS only
}

void JSONSchemaValidatorTestBase::TestObject() {
  std::unique_ptr<base::DictionaryValue> schema(new base::DictionaryValue());
  schema->SetString(schema::kType, schema::kObject);
  schema->SetString("properties.foo.type", schema::kString);
  schema->SetString("properties.bar.type", schema::kInteger);

  std::unique_ptr<base::DictionaryValue> instance(new base::DictionaryValue());
  instance->SetString("foo", "foo");
  instance->SetInteger("bar", 42);

  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), nullptr);

  instance->SetBoolean("extra", true);
  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), nullptr, "extra",
                 JSONSchemaValidator::kUnexpectedProperty);
  instance->Remove("extra", nullptr);

  instance->Remove("bar", nullptr);
  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), nullptr, "bar",
                 JSONSchemaValidator::kObjectPropertyIsRequired);

  instance->SetString("bar", "42");
  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), nullptr, "bar",
                 JSONSchemaValidator::FormatErrorMessage(
                     JSONSchemaValidator::kInvalidType, schema::kInteger,
                     schema::kString));
  instance->SetInteger("bar", 42);

  // Test "patternProperties".
  instance->SetInteger("extra", 42);
  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), nullptr, "extra",
                 JSONSchemaValidator::kUnexpectedProperty);
  schema->SetString("patternProperties.extra+.type",
                    schema::kInteger);
  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), nullptr);
  instance->Remove("extra", nullptr);
  instance->SetInteger("extraaa", 42);
  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), nullptr);
  instance->Remove("extraaa", nullptr);
  instance->SetInteger("extr", 42);
  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), nullptr, "extr",
                 JSONSchemaValidator::kUnexpectedProperty);
  instance->Remove("extr", nullptr);
  schema->Remove(schema::kPatternProperties, nullptr);

  // Test "patternProperties" and "properties" schemas are both checked if
  // applicable.
  schema->SetString("patternProperties.fo+.type", schema::kInteger);
  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), nullptr, "foo",
                 JSONSchemaValidator::FormatErrorMessage(
                     JSONSchemaValidator::kInvalidType, schema::kInteger,
                     schema::kString));
  instance->SetInteger("foo", 123);
  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), nullptr, "foo",
                 JSONSchemaValidator::FormatErrorMessage(
                     JSONSchemaValidator::kInvalidType, schema::kString,
                     schema::kInteger));
  instance->SetString("foo", "foo");
  schema->Remove(schema::kPatternProperties, nullptr);

  // Test additional properties.
  base::DictionaryValue* additional_properties = schema->SetDictionary(
      schema::kAdditionalProperties, std::make_unique<base::DictionaryValue>());
  additional_properties->SetString(schema::kType, schema::kAny);

  instance->SetBoolean("extra", true);
  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), nullptr);

  instance->SetString("extra", "foo");
  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), nullptr);

  additional_properties->SetString(schema::kType, schema::kBoolean);
  instance->SetBoolean("extra", true);
  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), nullptr);

  instance->SetString("extra", "foo");
  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), nullptr, "extra",
                 JSONSchemaValidator::FormatErrorMessage(
                     JSONSchemaValidator::kInvalidType, schema::kBoolean,
                     schema::kString));
  instance->Remove("extra", nullptr);

  base::DictionaryValue* properties = nullptr;
  base::DictionaryValue* bar_property = nullptr;
  ASSERT_TRUE(schema->GetDictionary(schema::kProperties, &properties));
  ASSERT_TRUE(properties->GetDictionary("bar", &bar_property));

  bar_property->SetBoolean(schema::kOptional, true);
  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), nullptr);
  instance->Remove("bar", nullptr);
  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), nullptr);
  instance->Set("bar", std::make_unique<base::Value>());
  ExpectNotValid(
      TEST_SOURCE, instance.get(), schema.get(), nullptr, "bar",
      JSONSchemaValidator::FormatErrorMessage(JSONSchemaValidator::kInvalidType,
                                              schema::kInteger, schema::kNull));
  instance->SetString("bar", "42");
  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), nullptr, "bar",
                 JSONSchemaValidator::FormatErrorMessage(
                     JSONSchemaValidator::kInvalidType, schema::kInteger,
                     schema::kString));

  // Verify that JSON parser handles dot in "patternProperties" well.
  schema.reset(LoadDictionary("pattern_properties_dot.json"));
  ASSERT_TRUE(schema->GetDictionary(schema::kPatternProperties, &properties));
  ASSERT_TRUE(properties->HasKey("^.$"));

  instance.reset(new base::DictionaryValue());
  instance->SetString("a", "whatever");
  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), nullptr);
  instance->SetString("foo", "bar");
  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), nullptr, "foo",
                 JSONSchemaValidator::kUnexpectedProperty);
}

void JSONSchemaValidatorTestBase::TestTypeReference() {
  std::unique_ptr<base::ListValue> types(LoadList("reference_types.json"));
  ASSERT_TRUE(types.get());

  std::unique_ptr<base::DictionaryValue> schema(new base::DictionaryValue());
  schema->SetString(schema::kType, schema::kObject);
  schema->SetString("properties.foo.type", schema::kString);
  schema->SetString("properties.bar.$ref", "Max10Int");
  schema->SetString("properties.baz.$ref", "MinLengthString");

  std::unique_ptr<base::DictionaryValue> schema_inline(
      new base::DictionaryValue());
  schema_inline->SetString(schema::kType, schema::kObject);
  schema_inline->SetString("properties.foo.type", schema::kString);
  schema_inline->SetString("properties.bar.id", "NegativeInt");
  schema_inline->SetString("properties.bar.type", schema::kInteger);
  schema_inline->SetInteger("properties.bar.maximum", 0);
  schema_inline->SetString("properties.baz.$ref", "NegativeInt");

  std::unique_ptr<base::DictionaryValue> instance(new base::DictionaryValue());
  instance->SetString("foo", "foo");
  instance->SetInteger("bar", 4);
  instance->SetString("baz", "ab");

  std::unique_ptr<base::DictionaryValue> instance_inline(
      new base::DictionaryValue());
  instance_inline->SetString("foo", "foo");
  instance_inline->SetInteger("bar", -4);
  instance_inline->SetInteger("baz", -2);

  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), types.get());
  ExpectValid(TEST_SOURCE, instance_inline.get(), schema_inline.get(), nullptr);

  // Validation failure, but successful schema reference.
  instance->SetString("baz", "a");
  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), types.get(),
                 "baz", JSONSchemaValidator::FormatErrorMessage(
                     JSONSchemaValidator::kStringMinLength, "2"));

  instance_inline->SetInteger("bar", 20);
  ExpectNotValid(TEST_SOURCE, instance_inline.get(), schema_inline.get(),
                 nullptr, "bar",
                 JSONSchemaValidator::FormatErrorMessage(
                     JSONSchemaValidator::kNumberMaximum, "0"));

  // Remove MinLengthString type.
  types->Remove(types->GetSize() - 1, nullptr);
  instance->SetString("baz", "ab");
  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), types.get(),
                 "bar", JSONSchemaValidator::FormatErrorMessage(
                     JSONSchemaValidator::kUnknownTypeReference,
                     "Max10Int"));

  // Remove internal type "NegativeInt".
  schema_inline->Remove("properties.bar", nullptr);
  instance_inline->Remove("bar", nullptr);
  ExpectNotValid(
      TEST_SOURCE, instance_inline.get(), schema_inline.get(), nullptr, "baz",
      JSONSchemaValidator::FormatErrorMessage(
          JSONSchemaValidator::kUnknownTypeReference, "NegativeInt"));
}

void JSONSchemaValidatorTestBase::TestArrayTuple() {
  std::unique_ptr<base::DictionaryValue> schema(
      LoadDictionary("array_tuple_schema.json"));
  ASSERT_TRUE(schema.get());

  std::unique_ptr<base::ListValue> instance(new base::ListValue());
  instance->AppendString("42");
  instance->AppendInteger(42);

  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), nullptr);

  instance->AppendString("anything");
  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), nullptr,
                 std::string(),
                 JSONSchemaValidator::FormatErrorMessage(
                     JSONSchemaValidator::kArrayMaxItems, "2"));

  instance->Remove(1, nullptr);
  instance->Remove(1, nullptr);
  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), nullptr, "1",
                 JSONSchemaValidator::kArrayItemRequired);

  instance->Set(0, std::make_unique<base::Value>(42));
  instance->AppendInteger(42);
  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), nullptr, "0",
                 JSONSchemaValidator::FormatErrorMessage(
                     JSONSchemaValidator::kInvalidType, schema::kString,
                     schema::kInteger));

  base::DictionaryValue* additional_properties = schema->SetDictionary(
      schema::kAdditionalProperties, std::make_unique<base::DictionaryValue>());
  additional_properties->SetString(schema::kType, schema::kAny);
  instance->Set(0, std::make_unique<base::Value>("42"));
  instance->AppendString("anything");
  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), nullptr);
  instance->Set(2, std::make_unique<base::ListValue>());
  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), nullptr);

  additional_properties->SetString(schema::kType, schema::kBoolean);
  ExpectNotValid(
      TEST_SOURCE, instance.get(), schema.get(), nullptr, "2",
      JSONSchemaValidator::FormatErrorMessage(
          JSONSchemaValidator::kInvalidType, schema::kBoolean, schema::kArray));
  instance->Set(2, std::make_unique<base::Value>(false));
  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), nullptr);

  base::ListValue* items_schema = nullptr;
  base::DictionaryValue* item0_schema = nullptr;
  ASSERT_TRUE(schema->GetList(schema::kItems, &items_schema));
  ASSERT_TRUE(items_schema->GetDictionary(0, &item0_schema));
  item0_schema->SetBoolean(schema::kOptional, true);
  instance->Remove(2, nullptr);
  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), nullptr);
  // TODO(aa): I think this is inconsistent with the handling of NULL+optional
  // for objects.
  instance->Set(0, std::make_unique<base::Value>());
  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), nullptr);
  instance->Set(0, std::make_unique<base::Value>(42));
  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), nullptr, "0",
                 JSONSchemaValidator::FormatErrorMessage(
                     JSONSchemaValidator::kInvalidType, schema::kString,
                     schema::kInteger));
}

void JSONSchemaValidatorTestBase::TestArrayNonTuple() {
  std::unique_ptr<base::DictionaryValue> schema(new base::DictionaryValue());
  schema->SetString(schema::kType, schema::kArray);
  schema->SetString("items.type", schema::kString);
  schema->SetInteger(schema::kMinItems, 2);
  schema->SetInteger(schema::kMaxItems, 3);

  std::unique_ptr<base::ListValue> instance(new base::ListValue());
  instance->AppendString("x");
  instance->AppendString("x");

  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), nullptr);
  instance->AppendString("x");
  ExpectValid(TEST_SOURCE, instance.get(), schema.get(), nullptr);

  instance->AppendString("x");
  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), nullptr,
                 std::string(),
                 JSONSchemaValidator::FormatErrorMessage(
                     JSONSchemaValidator::kArrayMaxItems, "3"));
  instance->Remove(1, nullptr);
  instance->Remove(1, nullptr);
  instance->Remove(1, nullptr);
  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), nullptr,
                 std::string(),
                 JSONSchemaValidator::FormatErrorMessage(
                     JSONSchemaValidator::kArrayMinItems, "2"));

  instance->Remove(1, nullptr);
  instance->AppendInteger(42);
  ExpectNotValid(TEST_SOURCE, instance.get(), schema.get(), nullptr, "1",
                 JSONSchemaValidator::FormatErrorMessage(
                     JSONSchemaValidator::kInvalidType, schema::kString,
                     schema::kInteger));
}

void JSONSchemaValidatorTestBase::TestString() {
  std::unique_ptr<base::DictionaryValue> schema(new base::DictionaryValue());
  schema->SetString(schema::kType, schema::kString);
  schema->SetInteger(schema::kMinLength, 1);
  schema->SetInteger(schema::kMaxLength, 10);

  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value("x")).get(),
              schema.get(), nullptr);
  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value("xxxxxxxxxx")).get(),
              schema.get(), nullptr);

  ExpectNotValid(
      TEST_SOURCE,
      std::unique_ptr<base::Value>(new base::Value(std::string())).get(),
      schema.get(), nullptr, std::string(),
      JSONSchemaValidator::FormatErrorMessage(
          JSONSchemaValidator::kStringMinLength, "1"));
  ExpectNotValid(
      TEST_SOURCE,
      std::unique_ptr<base::Value>(new base::Value("xxxxxxxxxxx")).get(),
      schema.get(), nullptr, std::string(),
      JSONSchemaValidator::FormatErrorMessage(
          JSONSchemaValidator::kStringMaxLength, "10"));
}

void JSONSchemaValidatorTestBase::TestNumber() {
  std::unique_ptr<base::DictionaryValue> schema(new base::DictionaryValue());
  schema->SetString(schema::kType, schema::kNumber);
  schema->SetInteger(schema::kMinimum, 1);
  schema->SetInteger(schema::kMaximum, 100);
  schema->SetInteger("maxDecimal", 2);

  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value(1)).get(),
              schema.get(), nullptr);
  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value(50)).get(),
              schema.get(), nullptr);
  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value(100)).get(),
              schema.get(), nullptr);
  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value(88.88)).get(),
              schema.get(), nullptr);

  ExpectNotValid(TEST_SOURCE,
                 std::unique_ptr<base::Value>(new base::Value(0.5)).get(),
                 schema.get(), nullptr, std::string(),
                 JSONSchemaValidator::FormatErrorMessage(
                     JSONSchemaValidator::kNumberMinimum, "1"));
  ExpectNotValid(TEST_SOURCE,
                 std::unique_ptr<base::Value>(new base::Value(100.1)).get(),
                 schema.get(), nullptr, std::string(),
                 JSONSchemaValidator::FormatErrorMessage(
                     JSONSchemaValidator::kNumberMaximum, "100"));
}

void JSONSchemaValidatorTestBase::TestTypeClassifier() {
  EXPECT_EQ(std::string(schema::kBoolean),
            JSONSchemaValidator::GetJSONSchemaType(
                std::unique_ptr<base::Value>(new base::Value(true)).get()));
  EXPECT_EQ(std::string(schema::kBoolean),
            JSONSchemaValidator::GetJSONSchemaType(
                std::unique_ptr<base::Value>(new base::Value(false)).get()));

  // It doesn't matter whether the C++ type is 'integer' or 'real'. If the
  // number is integral and within the representable range of integers in
  // double, it's classified as 'integer'.
  EXPECT_EQ(std::string(schema::kInteger),
            JSONSchemaValidator::GetJSONSchemaType(
                std::unique_ptr<base::Value>(new base::Value(42)).get()));
  EXPECT_EQ(std::string(schema::kInteger),
            JSONSchemaValidator::GetJSONSchemaType(
                std::unique_ptr<base::Value>(new base::Value(0)).get()));
  EXPECT_EQ(std::string(schema::kInteger),
            JSONSchemaValidator::GetJSONSchemaType(
                std::unique_ptr<base::Value>(new base::Value(42)).get()));
  EXPECT_EQ(
      std::string(schema::kInteger),
      JSONSchemaValidator::GetJSONSchemaType(
          std::unique_ptr<base::Value>(new base::Value(pow(2.0, DBL_MANT_DIG)))
              .get()));
  EXPECT_EQ(
      std::string(schema::kInteger),
      JSONSchemaValidator::GetJSONSchemaType(
          std::unique_ptr<base::Value>(new base::Value(pow(-2.0, DBL_MANT_DIG)))
              .get()));

  // "number" is only used for non-integral numbers, or numbers beyond what
  // double can accurately represent.
  EXPECT_EQ(std::string(schema::kNumber),
            JSONSchemaValidator::GetJSONSchemaType(
                std::unique_ptr<base::Value>(new base::Value(88.8)).get()));
  EXPECT_EQ(std::string(schema::kNumber),
            JSONSchemaValidator::GetJSONSchemaType(
                std::unique_ptr<base::Value>(
                    new base::Value(pow(2.0, DBL_MANT_DIG) * 2))
                    .get()));
  EXPECT_EQ(std::string(schema::kNumber),
            JSONSchemaValidator::GetJSONSchemaType(
                std::unique_ptr<base::Value>(
                    new base::Value(pow(-2.0, DBL_MANT_DIG) * 2))
                    .get()));

  EXPECT_EQ(std::string(schema::kString),
            JSONSchemaValidator::GetJSONSchemaType(
                std::unique_ptr<base::Value>(new base::Value("foo")).get()));
  EXPECT_EQ(std::string(schema::kArray),
            JSONSchemaValidator::GetJSONSchemaType(
                std::unique_ptr<base::Value>(new base::ListValue()).get()));
  EXPECT_EQ(
      std::string(schema::kObject),
      JSONSchemaValidator::GetJSONSchemaType(
          std::unique_ptr<base::Value>(new base::DictionaryValue()).get()));
  EXPECT_EQ(std::string(schema::kNull),
            JSONSchemaValidator::GetJSONSchemaType(
                std::make_unique<base::Value>().get()));
}

void JSONSchemaValidatorTestBase::TestTypes() {
  std::unique_ptr<base::DictionaryValue> schema(new base::DictionaryValue());

  // valid
  schema->SetString(schema::kType, schema::kObject);
  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::DictionaryValue()).get(),
              schema.get(), nullptr);

  schema->SetString(schema::kType, schema::kArray);
  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::ListValue()).get(),
              schema.get(), nullptr);

  schema->SetString(schema::kType, schema::kString);
  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value("foobar")).get(),
              schema.get(), nullptr);

  schema->SetString(schema::kType, schema::kNumber);
  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value(88.8)).get(),
              schema.get(), nullptr);
  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value(42)).get(),
              schema.get(), nullptr);
  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value(42)).get(),
              schema.get(), nullptr);
  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value(0)).get(),
              schema.get(), nullptr);

  schema->SetString(schema::kType, schema::kInteger);
  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value(42)).get(),
              schema.get(), nullptr);
  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value(42)).get(),
              schema.get(), nullptr);
  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value(0)).get(),
              schema.get(), nullptr);
  ExpectValid(
      TEST_SOURCE,
      std::unique_ptr<base::Value>(new base::Value(pow(2.0, DBL_MANT_DIG)))
          .get(),
      schema.get(), nullptr);
  ExpectValid(
      TEST_SOURCE,
      std::unique_ptr<base::Value>(new base::Value(pow(-2.0, DBL_MANT_DIG)))
          .get(),
      schema.get(), nullptr);

  schema->SetString(schema::kType, schema::kBoolean);
  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value(false)).get(),
              schema.get(), nullptr);
  ExpectValid(TEST_SOURCE,
              std::unique_ptr<base::Value>(new base::Value(true)).get(),
              schema.get(), nullptr);

  schema->SetString(schema::kType, schema::kNull);
  ExpectValid(TEST_SOURCE, std::make_unique<base::Value>().get(), schema.get(),
              nullptr);

  // not valid
  schema->SetString(schema::kType, schema::kObject);
  ExpectNotValid(
      TEST_SOURCE, std::unique_ptr<base::Value>(new base::ListValue()).get(),
      schema.get(), nullptr, std::string(),
      JSONSchemaValidator::FormatErrorMessage(JSONSchemaValidator::kInvalidType,
                                              schema::kObject, schema::kArray));

  schema->SetString(schema::kType, schema::kObject);
  ExpectNotValid(
      TEST_SOURCE, std::make_unique<base::Value>().get(), schema.get(), nullptr,
      std::string(),
      JSONSchemaValidator::FormatErrorMessage(JSONSchemaValidator::kInvalidType,
                                              schema::kObject, schema::kNull));

  schema->SetString(schema::kType, schema::kArray);
  ExpectNotValid(
      TEST_SOURCE, std::unique_ptr<base::Value>(new base::Value(42)).get(),
      schema.get(), nullptr, std::string(),
      JSONSchemaValidator::FormatErrorMessage(
          JSONSchemaValidator::kInvalidType, schema::kArray, schema::kInteger));

  schema->SetString(schema::kType, schema::kString);
  ExpectNotValid(TEST_SOURCE,
                 std::unique_ptr<base::Value>(new base::Value(42)).get(),
                 schema.get(), nullptr, std::string(),
                 JSONSchemaValidator::FormatErrorMessage(
                     JSONSchemaValidator::kInvalidType, schema::kString,
                     schema::kInteger));

  schema->SetString(schema::kType, schema::kNumber);
  ExpectNotValid(
      TEST_SOURCE, std::unique_ptr<base::Value>(new base::Value("42")).get(),
      schema.get(), nullptr, std::string(),
      JSONSchemaValidator::FormatErrorMessage(
          JSONSchemaValidator::kInvalidType, schema::kNumber, schema::kString));

  schema->SetString(schema::kType, schema::kInteger);
  ExpectNotValid(TEST_SOURCE,
                 std::unique_ptr<base::Value>(new base::Value(88.8)).get(),
                 schema.get(), nullptr, std::string(),
                 JSONSchemaValidator::kInvalidTypeIntegerNumber);

  schema->SetString(schema::kType, schema::kBoolean);
  ExpectNotValid(TEST_SOURCE,
                 std::unique_ptr<base::Value>(new base::Value(1)).get(),
                 schema.get(), nullptr, std::string(),
                 JSONSchemaValidator::FormatErrorMessage(
                     JSONSchemaValidator::kInvalidType, schema::kBoolean,
                     schema::kInteger));

  schema->SetString(schema::kType, schema::kNull);
  ExpectNotValid(
      TEST_SOURCE, std::unique_ptr<base::Value>(new base::Value(false)).get(),
      schema.get(), nullptr, std::string(),
      JSONSchemaValidator::FormatErrorMessage(JSONSchemaValidator::kInvalidType,
                                              schema::kNull, schema::kBoolean));
}
