// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/schema.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "components/policy/core/common/schema_internal.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {

namespace {

#define TestSchemaValidation(a, b, c, d) \
    TestSchemaValidationHelper(          \
        base::StringPrintf("%s:%i", __FILE__, __LINE__), a, b, c, d)

const char kTestSchema[] =
    "{"
    "  \"type\": \"object\","
    "  \"properties\": {"
    "    \"Boolean\": { \"type\": \"boolean\" },"
    "    \"Integer\": { \"type\": \"integer\" },"
    "    \"Null\": { \"type\": \"null\" },"
    "    \"Number\": { \"type\": \"number\" },"
    "    \"String\": { \"type\": \"string\" },"
    "    \"Array\": {"
    "      \"type\": \"array\","
    "      \"items\": { \"type\": \"string\" }"
    "    },"
    "    \"ArrayOfObjects\": {"
    "      \"type\": \"array\","
    "      \"items\": {"
    "        \"type\": \"object\","
    "        \"properties\": {"
    "          \"one\": { \"type\": \"string\" },"
    "          \"two\": { \"type\": \"integer\" }"
    "        }"
    "      }"
    "    },"
    "    \"ArrayOfArray\": {"
    "      \"type\": \"array\","
    "      \"items\": {"
    "        \"type\": \"array\","
    "        \"items\": { \"type\": \"string\" }"
    "      }"
    "    },"
    "    \"Object\": {"
    "      \"type\": \"object\","
    "      \"properties\": {"
    "        \"one\": { \"type\": \"boolean\" },"
    "        \"two\": { \"type\": \"integer\" }"
    "      },"
    "      \"additionalProperties\": { \"type\": \"string\" }"
    "    },"
    "    \"ObjectOfObject\": {"
    "      \"type\": \"object\","
    "      \"properties\": {"
    "        \"Object\": {"
    "          \"type\": \"object\","
    "          \"properties\": {"
    "            \"one\": { \"type\": \"string\" },"
    "            \"two\": { \"type\": \"integer\" }"
    "          }"
    "        }"
    "      }"
    "    },"
    "    \"IntegerWithEnums\": {"
    "      \"type\": \"integer\","
    "      \"enum\": [1, 2, 3]"
    "    },"
    "    \"IntegerWithEnumsGaps\": {"
    "      \"type\": \"integer\","
    "      \"enum\": [10, 20, 30]"
    "    },"
    "    \"StringWithEnums\": {"
    "      \"type\": \"string\","
    "      \"enum\": [\"one\", \"two\", \"three\"]"
    "    },"
    "    \"IntegerWithRange\": {"
    "      \"type\": \"integer\","
    "      \"minimum\": 1,"
    "      \"maximum\": 3"
    "    },"
    "    \"ObjectOfArray\": {"
    "      \"type\": \"object\","
    "      \"properties\": {"
    "        \"List\": {"
    "          \"type\": \"array\","
    "          \"items\": { \"type\": \"integer\" }"
    "        }"
    "      }"
    "    },"
    "    \"ArrayOfObjectOfArray\": {"
    "      \"type\": \"array\","
    "      \"items\": {"
    "        \"type\": \"object\","
    "        \"properties\": {"
    "          \"List\": {"
    "            \"type\": \"array\","
    "            \"items\": { \"type\": \"string\" }"
    "          }"
    "        }"
    "      }"
    "    },"
    "    \"StringWithPattern\": {"
    "      \"type\": \"string\","
    "      \"pattern\": \"^foo+$\""
    "    },"
    "    \"ObjectWithPatternProperties\": {"
    "      \"type\": \"object\","
    "      \"patternProperties\": {"
    "        \"^foo+$\": { \"type\": \"integer\" },"
    "        \"^bar+$\": {"
    "          \"type\": \"string\","
    "          \"enum\": [\"one\", \"two\"]"
    "        }"
    "      },"
    "      \"properties\": {"
    "        \"bar\": {"
    "          \"type\": \"string\","
    "          \"enum\": [\"one\", \"three\"]"
    "        }"
    "      }"
    "    }"
    "  }"
    "}";

bool ParseFails(const std::string& content) {
  std::string error;
  Schema schema = Schema::Parse(content, &error);
  if (schema.valid())
    return false;
  EXPECT_FALSE(error.empty());
  return true;
}

void TestSchemaValidationHelper(const std::string& source,
                                Schema schema,
                                const base::Value& value,
                                SchemaOnErrorStrategy strategy,
                                bool expected_return_value) {
  std::string error;
  static const char kNoErrorReturned[] = "No error returned.";

  // Test that Schema::Validate() works as expected.
  error = kNoErrorReturned;
  bool returned = schema.Validate(value, strategy, nullptr, &error);
  ASSERT_EQ(expected_return_value, returned) << source << ": " << error;

  // Test that Schema::Normalize() will return the same value as
  // Schema::Validate().
  error = kNoErrorReturned;
  std::unique_ptr<base::Value> cloned_value(value.DeepCopy());
  bool touched = false;
  returned =
      schema.Normalize(cloned_value.get(), strategy, nullptr, &error, &touched);
  EXPECT_EQ(expected_return_value, returned) << source << ": " << error;

  bool strictly_valid = schema.Validate(value, SCHEMA_STRICT, nullptr, &error);
  EXPECT_EQ(touched, !strictly_valid && returned) << source;

  // Test that Schema::Normalize() have actually dropped invalid and unknown
  // properties.
  if (expected_return_value) {
    EXPECT_TRUE(schema.Validate(*cloned_value, SCHEMA_STRICT, nullptr, &error))
        << source;
    EXPECT_TRUE(schema.Normalize(cloned_value.get(), SCHEMA_STRICT, nullptr,
                                 &error, nullptr))
        << source;
  }
}

void TestSchemaValidationWithPath(Schema schema,
                                  const base::Value& value,
                                  const std::string& expected_failure_path) {
  std::string error_path = "NOT_SET";
  std::string error;

  bool returned = schema.Validate(value, SCHEMA_STRICT, &error_path, &error);
  ASSERT_FALSE(returned) << error_path;
  EXPECT_EQ(error_path, expected_failure_path);
}

std::string SchemaObjectWrapper(const std::string& subschema) {
  return "{"
         "  \"type\": \"object\","
         "  \"properties\": {"
         "    \"SomePropertyName\":" + subschema +
         "  }"
         "}";
}

}  // namespace

TEST(SchemaTest, MinimalSchema) {
  EXPECT_FALSE(ParseFails("{ \"type\": \"object\" }"));
}

TEST(SchemaTest, InvalidSchemas) {
  EXPECT_TRUE(ParseFails(""));
  EXPECT_TRUE(ParseFails("omg"));
  EXPECT_TRUE(ParseFails("\"omg\""));
  EXPECT_TRUE(ParseFails("123"));
  EXPECT_TRUE(ParseFails("[]"));
  EXPECT_TRUE(ParseFails("null"));
  EXPECT_TRUE(ParseFails("{}"));

  EXPECT_TRUE(ParseFails(
      "{"
      "  \"type\": \"object\","
        "\"additionalProperties\": { \"type\":\"object\" }"
      "}"));

  EXPECT_TRUE(ParseFails(
      "{"
      "  \"type\": \"object\","
      "  \"patternProperties\": { \"a+b*\": { \"type\": \"object\" } }"
      "}"));

  EXPECT_TRUE(ParseFails(
      "{"
      "  \"type\": \"object\","
      "  \"properties\": { \"Policy\": { \"type\": \"bogus\" } }"
      "}"));

  EXPECT_TRUE(ParseFails(
      "{"
      "  \"type\": \"object\","
      "  \"properties\": { \"Policy\": { \"type\": [\"string\", \"number\"] } }"
      "}"));

  EXPECT_TRUE(ParseFails(
      "{"
      "  \"type\": \"object\","
      "  \"properties\": { \"Policy\": { \"type\": \"any\" } }"
      "}"));

  EXPECT_TRUE(ParseFails(
      "{"
      "  \"type\": \"object\","
      "  \"properties\": { \"Policy\": 123 }"
      "}"));

  EXPECT_FALSE(ParseFails(
      "{"
      "  \"type\": \"object\","
      "  \"unknown attribute\": \"is ignored\""
      "}"));
}

TEST(SchemaTest, Ownership) {
  std::string error;
  Schema schema = Schema::Parse(
      "{"
      "  \"type\": \"object\","
      "  \"properties\": {"
      "    \"sub\": {"
      "      \"type\": \"object\","
      "      \"properties\": {"
      "        \"subsub\": { \"type\": \"string\" }"
      "      }"
      "    }"
      "  }"
      "}", &error);
  ASSERT_TRUE(schema.valid()) << error;
  ASSERT_EQ(base::Value::Type::DICTIONARY, schema.type());

  schema = schema.GetKnownProperty("sub");
  ASSERT_TRUE(schema.valid());
  ASSERT_EQ(base::Value::Type::DICTIONARY, schema.type());

  {
    Schema::Iterator it = schema.GetPropertiesIterator();
    ASSERT_FALSE(it.IsAtEnd());
    EXPECT_STREQ("subsub", it.key());

    schema = it.schema();
    it.Advance();
    EXPECT_TRUE(it.IsAtEnd());
  }

  ASSERT_TRUE(schema.valid());
  EXPECT_EQ(base::Value::Type::STRING, schema.type());

  // This test shouldn't leak nor use invalid memory.
}

TEST(SchemaTest, ValidSchema) {
  std::string error;
  Schema schema = Schema::Parse(kTestSchema, &error);
  ASSERT_TRUE(schema.valid()) << error;

  ASSERT_EQ(base::Value::Type::DICTIONARY, schema.type());
  EXPECT_FALSE(schema.GetProperty("invalid").valid());

  Schema sub = schema.GetProperty("Boolean");
  ASSERT_TRUE(sub.valid());
  EXPECT_EQ(base::Value::Type::BOOLEAN, sub.type());

  sub = schema.GetProperty("Integer");
  ASSERT_TRUE(sub.valid());
  EXPECT_EQ(base::Value::Type::INTEGER, sub.type());

  sub = schema.GetProperty("Null");
  ASSERT_TRUE(sub.valid());
  EXPECT_EQ(base::Value::Type::NONE, sub.type());

  sub = schema.GetProperty("Number");
  ASSERT_TRUE(sub.valid());
  EXPECT_EQ(base::Value::Type::DOUBLE, sub.type());

  sub = schema.GetProperty("String");
  ASSERT_TRUE(sub.valid());
  EXPECT_EQ(base::Value::Type::STRING, sub.type());

  sub = schema.GetProperty("Array");
  ASSERT_TRUE(sub.valid());
  ASSERT_EQ(base::Value::Type::LIST, sub.type());
  sub = sub.GetItems();
  ASSERT_TRUE(sub.valid());
  EXPECT_EQ(base::Value::Type::STRING, sub.type());

  sub = schema.GetProperty("ArrayOfObjects");
  ASSERT_TRUE(sub.valid());
  ASSERT_EQ(base::Value::Type::LIST, sub.type());
  sub = sub.GetItems();
  ASSERT_TRUE(sub.valid());
  EXPECT_EQ(base::Value::Type::DICTIONARY, sub.type());
  Schema subsub = sub.GetProperty("one");
  ASSERT_TRUE(subsub.valid());
  EXPECT_EQ(base::Value::Type::STRING, subsub.type());
  subsub = sub.GetProperty("two");
  ASSERT_TRUE(subsub.valid());
  EXPECT_EQ(base::Value::Type::INTEGER, subsub.type());
  subsub = sub.GetProperty("invalid");
  EXPECT_FALSE(subsub.valid());

  sub = schema.GetProperty("ArrayOfArray");
  ASSERT_TRUE(sub.valid());
  ASSERT_EQ(base::Value::Type::LIST, sub.type());
  sub = sub.GetItems();
  ASSERT_TRUE(sub.valid());
  ASSERT_EQ(base::Value::Type::LIST, sub.type());
  sub = sub.GetItems();
  ASSERT_TRUE(sub.valid());
  EXPECT_EQ(base::Value::Type::STRING, sub.type());

  sub = schema.GetProperty("Object");
  ASSERT_TRUE(sub.valid());
  ASSERT_EQ(base::Value::Type::DICTIONARY, sub.type());
  subsub = sub.GetProperty("one");
  ASSERT_TRUE(subsub.valid());
  EXPECT_EQ(base::Value::Type::BOOLEAN, subsub.type());
  subsub = sub.GetProperty("two");
  ASSERT_TRUE(subsub.valid());
  EXPECT_EQ(base::Value::Type::INTEGER, subsub.type());
  subsub = sub.GetProperty("undeclared");
  ASSERT_TRUE(subsub.valid());
  EXPECT_EQ(base::Value::Type::STRING, subsub.type());

  sub = schema.GetProperty("IntegerWithEnums");
  ASSERT_TRUE(sub.valid());
  ASSERT_EQ(base::Value::Type::INTEGER, sub.type());

  sub = schema.GetProperty("IntegerWithEnumsGaps");
  ASSERT_TRUE(sub.valid());
  ASSERT_EQ(base::Value::Type::INTEGER, sub.type());

  sub = schema.GetProperty("StringWithEnums");
  ASSERT_TRUE(sub.valid());
  ASSERT_EQ(base::Value::Type::STRING, sub.type());

  sub = schema.GetProperty("IntegerWithRange");
  ASSERT_TRUE(sub.valid());
  ASSERT_EQ(base::Value::Type::INTEGER, sub.type());

  sub = schema.GetProperty("StringWithPattern");
  ASSERT_TRUE(sub.valid());
  ASSERT_EQ(base::Value::Type::STRING, sub.type());

  sub = schema.GetProperty("ObjectWithPatternProperties");
  ASSERT_TRUE(sub.valid());
  ASSERT_EQ(base::Value::Type::DICTIONARY, sub.type());

  struct {
    const char* expected_key;
    base::Value::Type expected_type;
  } kExpectedProperties[] = {
    { "Array",                       base::Value::Type::LIST },
    { "ArrayOfArray",                base::Value::Type::LIST },
    { "ArrayOfObjectOfArray",        base::Value::Type::LIST },
    { "ArrayOfObjects",              base::Value::Type::LIST },
    { "Boolean",                     base::Value::Type::BOOLEAN },
    { "Integer",                     base::Value::Type::INTEGER },
    { "IntegerWithEnums",            base::Value::Type::INTEGER },
    { "IntegerWithEnumsGaps",        base::Value::Type::INTEGER },
    { "IntegerWithRange",            base::Value::Type::INTEGER },
    { "Null",                        base::Value::Type::NONE },
    { "Number",                      base::Value::Type::DOUBLE },
    { "Object",                      base::Value::Type::DICTIONARY },
    { "ObjectOfArray",               base::Value::Type::DICTIONARY },
    { "ObjectOfObject",              base::Value::Type::DICTIONARY },
    { "ObjectWithPatternProperties", base::Value::Type::DICTIONARY },
    { "String",                      base::Value::Type::STRING },
    { "StringWithEnums",             base::Value::Type::STRING },
    { "StringWithPattern",           base::Value::Type::STRING },
  };
  Schema::Iterator it = schema.GetPropertiesIterator();
  for (size_t i = 0; i < arraysize(kExpectedProperties); ++i) {
    ASSERT_FALSE(it.IsAtEnd());
    EXPECT_STREQ(kExpectedProperties[i].expected_key, it.key());
    ASSERT_TRUE(it.schema().valid());
    EXPECT_EQ(kExpectedProperties[i].expected_type, it.schema().type());
    it.Advance();
  }
  EXPECT_TRUE(it.IsAtEnd());
}

TEST(SchemaTest, Lookups) {
  std::string error;

  Schema schema = Schema::Parse("{ \"type\": \"object\" }", &error);
  ASSERT_TRUE(schema.valid()) << error;
  ASSERT_EQ(base::Value::Type::DICTIONARY, schema.type());

  // This empty schema should never find named properties.
  EXPECT_FALSE(schema.GetKnownProperty("").valid());
  EXPECT_FALSE(schema.GetKnownProperty("xyz").valid());
  EXPECT_TRUE(schema.GetPropertiesIterator().IsAtEnd());

  schema = Schema::Parse(
      "{"
      "  \"type\": \"object\","
      "  \"properties\": {"
      "    \"Boolean\": { \"type\": \"boolean\" }"
      "  }"
      "}", &error);
  ASSERT_TRUE(schema.valid()) << error;
  ASSERT_EQ(base::Value::Type::DICTIONARY, schema.type());

  EXPECT_FALSE(schema.GetKnownProperty("").valid());
  EXPECT_FALSE(schema.GetKnownProperty("xyz").valid());
  EXPECT_TRUE(schema.GetKnownProperty("Boolean").valid());

  schema = Schema::Parse(
      "{"
      "  \"type\": \"object\","
      "  \"properties\": {"
      "    \"bb\" : { \"type\": \"null\" },"
      "    \"aa\" : { \"type\": \"boolean\" },"
      "    \"abab\" : { \"type\": \"string\" },"
      "    \"ab\" : { \"type\": \"number\" },"
      "    \"aba\" : { \"type\": \"integer\" }"
      "  }"
      "}", &error);
  ASSERT_TRUE(schema.valid()) << error;
  ASSERT_EQ(base::Value::Type::DICTIONARY, schema.type());

  EXPECT_FALSE(schema.GetKnownProperty("").valid());
  EXPECT_FALSE(schema.GetKnownProperty("xyz").valid());

  struct {
    const char* expected_key;
    base::Value::Type expected_type;
  } kExpectedKeys[] = {
    { "aa",     base::Value::Type::BOOLEAN },
    { "ab",     base::Value::Type::DOUBLE },
    { "aba",    base::Value::Type::INTEGER },
    { "abab",   base::Value::Type::STRING },
    { "bb",     base::Value::Type::NONE },
  };
  for (size_t i = 0; i < arraysize(kExpectedKeys); ++i) {
    Schema sub = schema.GetKnownProperty(kExpectedKeys[i].expected_key);
    ASSERT_TRUE(sub.valid());
    EXPECT_EQ(kExpectedKeys[i].expected_type, sub.type());
  }
}

TEST(SchemaTest, Wrap) {
  const internal::SchemaNode kSchemas[] = {
    { base::Value::Type::DICTIONARY,   0 },    //  0: root node
    { base::Value::Type::BOOLEAN,      -1 },   //  1
    { base::Value::Type::INTEGER,      -1 },   //  2
    { base::Value::Type::DOUBLE,       -1 },   //  3
    { base::Value::Type::STRING,       -1 },   //  4
    { base::Value::Type::LIST,         4 },    //  5: list of strings.
    { base::Value::Type::LIST,         5 },    //  6: list of lists of strings.
    { base::Value::Type::INTEGER,      0 },    //  7: integer enumerations.
    { base::Value::Type::INTEGER,      1 },    //  8: ranged integers.
    { base::Value::Type::STRING,       2 },    //  9: string enumerations.
    { base::Value::Type::STRING,       3 },    // 10: string with pattern.
  };

  const internal::PropertyNode kPropertyNodes[] = {
    { "Boolean",   1 },  // 0
    { "Integer",   2 },  // 1
    { "Number",    3 },  // 2
    { "String",    4 },  // 3
    { "List",      5 },  // 4
    { "IntEnum",   7 },  // 5
    { "RangedInt", 8 },  // 6
    { "StrEnum",   9 },  // 7
    { "StrPat",   10 },  // 8
    { "bar+$",     4 },  // 9
  };

  const internal::PropertiesNode kProperties[] = {
    // 0 to 9 (exclusive) are the known properties in kPropertyNodes, 9 is
    // patternProperties and 6 is the additionalProperties node.
    { 0, 9, 10, 6 },
  };

  const internal::RestrictionNode kRestriction[] = {
    {{0, 3}},  // 0: [1, 2, 3]
    {{5, 1}},  // 1: minimum = 1, maximum = 5
    {{0, 3}},  // 2: ["one", "two", "three"]
    {{3, 3}},  // 3: pattern "foo+"
  };

  const int kIntEnums[] = {1, 2, 3};

  const char* kStringEnums[] = {
    "one",    // 0
    "two",    // 1
    "three",  // 2
    "foo+",   // 3
  };

  const internal::SchemaData kData = {
    kSchemas,
    kPropertyNodes,
    kProperties,
    kRestriction,
    kIntEnums,
    kStringEnums,
  };

  Schema schema = Schema::Wrap(&kData);
  ASSERT_TRUE(schema.valid());
  EXPECT_EQ(base::Value::Type::DICTIONARY, schema.type());

  struct {
    const char* key;
    base::Value::Type type;
  } kExpectedProperties[] = {
    { "Boolean", base::Value::Type::BOOLEAN },
    { "Integer", base::Value::Type::INTEGER },
    { "Number", base::Value::Type::DOUBLE },
    { "String", base::Value::Type::STRING },
    { "List", base::Value::Type::LIST },
    { "IntEnum", base::Value::Type::INTEGER },
    { "RangedInt", base::Value::Type::INTEGER },
    { "StrEnum", base::Value::Type::STRING },
    { "StrPat", base::Value::Type::STRING },
  };

  Schema::Iterator it = schema.GetPropertiesIterator();
  for (size_t i = 0; i < arraysize(kExpectedProperties); ++i) {
    ASSERT_FALSE(it.IsAtEnd());
    EXPECT_STREQ(kExpectedProperties[i].key, it.key());
    Schema sub = it.schema();
    ASSERT_TRUE(sub.valid());
    EXPECT_EQ(kExpectedProperties[i].type, sub.type());

    if (sub.type() == base::Value::Type::LIST) {
      Schema items = sub.GetItems();
      ASSERT_TRUE(items.valid());
      EXPECT_EQ(base::Value::Type::STRING, items.type());
    }

    it.Advance();
  }
  EXPECT_TRUE(it.IsAtEnd());

  Schema sub = schema.GetAdditionalProperties();
  ASSERT_TRUE(sub.valid());
  ASSERT_EQ(base::Value::Type::LIST, sub.type());
  Schema subsub = sub.GetItems();
  ASSERT_TRUE(subsub.valid());
  ASSERT_EQ(base::Value::Type::LIST, subsub.type());
  Schema subsubsub = subsub.GetItems();
  ASSERT_TRUE(subsubsub.valid());
  ASSERT_EQ(base::Value::Type::STRING, subsubsub.type());

  SchemaList schema_list = schema.GetPatternProperties("barr");
  ASSERT_EQ(1u, schema_list.size());
  sub = schema_list[0];
  ASSERT_TRUE(sub.valid());
  EXPECT_EQ(base::Value::Type::STRING, sub.type());

  EXPECT_TRUE(schema.GetPatternProperties("ba").empty());
  EXPECT_TRUE(schema.GetPatternProperties("bar+$").empty());
}

TEST(SchemaTest, Validate) {
  std::string error;
  Schema schema = Schema::Parse(kTestSchema, &error);
  ASSERT_TRUE(schema.valid()) << error;

  base::DictionaryValue bundle;
  TestSchemaValidation(schema, bundle, SCHEMA_STRICT, true);

  // Wrong type, expected integer.
  bundle.SetBoolean("Integer", true);
  TestSchemaValidation(schema, bundle, SCHEMA_STRICT, false);

  // Wrong type, expected list of strings.
  {
    bundle.Clear();
    base::ListValue list;
    list.AppendInteger(1);
    bundle.SetKey("Array", std::move(list));
    TestSchemaValidation(schema, bundle, SCHEMA_STRICT, false);
  }

  // Wrong type in a sub-object.
  {
    bundle.Clear();
    base::DictionaryValue dict;
    dict.SetString("one", "one");
    bundle.SetKey("Object", std::move(dict));
    TestSchemaValidation(schema, bundle, SCHEMA_STRICT, false);
  }

  // Unknown name.
  bundle.Clear();
  bundle.SetBoolean("Unknown", true);
  TestSchemaValidation(schema, bundle, SCHEMA_STRICT, false);

  // All of these will be valid.
  bundle.Clear();
  bundle.SetBoolean("Boolean", true);
  bundle.SetInteger("Integer", 123);
  bundle.Set("Null", std::make_unique<base::Value>());
  bundle.SetDouble("Number", 3.14);
  bundle.SetString("String", "omg");

  {
    base::ListValue list;
    list.AppendString("a string");
    list.AppendString("another string");
    bundle.SetKey("Array", std::move(list));
  }

  {
    base::DictionaryValue dict;
    dict.SetString("one", "string");
    dict.SetInteger("two", 2);
    base::ListValue list;
    list.GetList().push_back(dict.Clone());
    list.GetList().push_back(std::move(dict));
    bundle.SetKey("ArrayOfObjects", std::move(list));
  }

  {
    base::ListValue list;
    list.AppendString("a string");
    list.AppendString("another string");
    base::ListValue listlist;
    listlist.GetList().push_back(list.Clone());
    listlist.GetList().push_back(std::move(list));
    bundle.SetKey("ArrayOfArray", std::move(listlist));
  }

  {
    base::DictionaryValue dict;
    dict.SetBoolean("one", true);
    dict.SetInteger("two", 2);
    dict.SetString("additionally", "a string");
    dict.SetString("and also", "another string");
    bundle.SetKey("Object", std::move(dict));
  }

  bundle.SetInteger("IntegerWithEnums", 1);
  bundle.SetInteger("IntegerWithEnumsGaps", 20);
  bundle.SetString("StringWithEnums", "two");
  bundle.SetInteger("IntegerWithRange", 3);

  TestSchemaValidation(schema, bundle, SCHEMA_STRICT, true);

  bundle.SetInteger("IntegerWithEnums", 0);
  TestSchemaValidation(schema, bundle, SCHEMA_STRICT, false);
  bundle.SetInteger("IntegerWithEnums", 1);

  bundle.SetInteger("IntegerWithEnumsGaps", 0);
  TestSchemaValidation(schema, bundle, SCHEMA_STRICT, false);
  bundle.SetInteger("IntegerWithEnumsGaps", 9);
  TestSchemaValidation(schema, bundle, SCHEMA_STRICT, false);
  bundle.SetInteger("IntegerWithEnumsGaps", 10);
  TestSchemaValidation(schema, bundle, SCHEMA_STRICT, true);
  bundle.SetInteger("IntegerWithEnumsGaps", 11);
  TestSchemaValidation(schema, bundle, SCHEMA_STRICT, false);
  bundle.SetInteger("IntegerWithEnumsGaps", 19);
  TestSchemaValidation(schema, bundle, SCHEMA_STRICT, false);
  bundle.SetInteger("IntegerWithEnumsGaps", 21);
  TestSchemaValidation(schema, bundle, SCHEMA_STRICT, false);
  bundle.SetInteger("IntegerWithEnumsGaps", 29);
  TestSchemaValidation(schema, bundle, SCHEMA_STRICT, false);
  bundle.SetInteger("IntegerWithEnumsGaps", 30);
  TestSchemaValidation(schema, bundle, SCHEMA_STRICT, true);
  bundle.SetInteger("IntegerWithEnumsGaps", 31);
  TestSchemaValidation(schema, bundle, SCHEMA_STRICT, false);
  bundle.SetInteger("IntegerWithEnumsGaps", 100);
  TestSchemaValidation(schema, bundle, SCHEMA_STRICT, false);
  bundle.SetInteger("IntegerWithEnumsGaps", 20);

  bundle.SetString("StringWithEnums", "FOUR");
  TestSchemaValidation(schema, bundle, SCHEMA_STRICT, false);
  bundle.SetString("StringWithEnums", "two");

  bundle.SetInteger("IntegerWithRange", 4);
  TestSchemaValidation(schema, bundle, SCHEMA_STRICT, false);
  bundle.SetInteger("IntegerWithRange", 3);

  // Unknown top level property.
  bundle.SetString("boom", "bang");
  TestSchemaValidation(schema, bundle, SCHEMA_STRICT, false);
  TestSchemaValidation(schema, bundle, SCHEMA_ALLOW_UNKNOWN_TOPLEVEL, true);
  TestSchemaValidation(schema, bundle, SCHEMA_ALLOW_UNKNOWN, true);
  TestSchemaValidationWithPath(schema, bundle, "");
  bundle.Remove("boom", nullptr);

  // Invalid top level property.
  bundle.SetInteger("Boolean", 12345);
  TestSchemaValidation(schema, bundle, SCHEMA_STRICT, false);
  TestSchemaValidation(schema, bundle, SCHEMA_ALLOW_INVALID_TOPLEVEL, true);
  TestSchemaValidation(schema, bundle, SCHEMA_ALLOW_INVALID, true);
  TestSchemaValidationWithPath(schema, bundle, "Boolean");
  bundle.SetBoolean("Boolean", true);

  // Tests on ObjectOfObject.
  {
    Schema subschema = schema.GetProperty("ObjectOfObject");
    ASSERT_TRUE(subschema.valid());
    base::DictionaryValue root;

    // Unknown property.
    root.SetBoolean("Object.three", false);
    TestSchemaValidation(subschema, root, SCHEMA_STRICT, false);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_UNKNOWN_TOPLEVEL, false);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_UNKNOWN, true);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_INVALID_TOPLEVEL, true);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_INVALID, true);
    TestSchemaValidationWithPath(subschema, root, "Object");
    root.Remove("Object.three", nullptr);

    // Invalid property.
    root.SetInteger("Object.one", 12345);
    TestSchemaValidation(subschema, root, SCHEMA_STRICT, false);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_UNKNOWN_TOPLEVEL, false);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_UNKNOWN, false);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_INVALID_TOPLEVEL, true);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_INVALID, true);
    TestSchemaValidationWithPath(subschema, root, "Object.one");
    root.Remove("Object.one", nullptr);
  }

  // Tests on ArrayOfObjects.
  {
    Schema subschema = schema.GetProperty("ArrayOfObjects");
    ASSERT_TRUE(subschema.valid());
    base::ListValue root;

    // Unknown property.
    std::unique_ptr<base::DictionaryValue> dict_value(
        new base::DictionaryValue());
    dict_value->SetBoolean("three", true);
    root.Append(std::move(dict_value));
    TestSchemaValidation(subschema, root, SCHEMA_STRICT, false);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_UNKNOWN_TOPLEVEL, false);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_UNKNOWN, true);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_INVALID_TOPLEVEL, true);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_INVALID, true);
    TestSchemaValidationWithPath(subschema, root, "items[0]");
    root.Remove(root.GetSize() - 1, nullptr);

    // Invalid property.
    dict_value.reset(new base::DictionaryValue());
    dict_value->SetBoolean("two", true);
    root.Append(std::move(dict_value));
    TestSchemaValidation(subschema, root, SCHEMA_STRICT, false);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_UNKNOWN_TOPLEVEL, false);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_UNKNOWN, false);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_INVALID_TOPLEVEL, true);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_INVALID, true);
    TestSchemaValidationWithPath(subschema, root, "items[0].two");
    root.Remove(root.GetSize() - 1, nullptr);
  }

  // Tests on ObjectOfArray.
  {
    Schema subschema = schema.GetProperty("ObjectOfArray");
    ASSERT_TRUE(subschema.valid());
    base::DictionaryValue root;

    base::ListValue* list_value =
        root.SetList("List", std::make_unique<base::ListValue>());

    // Test that there are not errors here.
    list_value->AppendInteger(12345);
    TestSchemaValidation(subschema, root, SCHEMA_STRICT, true);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_UNKNOWN_TOPLEVEL, true);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_UNKNOWN, true);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_INVALID_TOPLEVEL, true);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_INVALID, true);

    // Invalid list item.
    list_value->AppendString("blabla");
    TestSchemaValidation(subschema, root, SCHEMA_STRICT, false);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_UNKNOWN_TOPLEVEL, false);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_UNKNOWN, false);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_INVALID_TOPLEVEL, true);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_INVALID, true);
    TestSchemaValidationWithPath(subschema, root, "List.items[1]");
  }

  // Tests on ArrayOfObjectOfArray.
  {
    Schema subschema = schema.GetProperty("ArrayOfObjectOfArray");
    ASSERT_TRUE(subschema.valid());
    base::ListValue root;

    auto dict_value = std::make_unique<base::DictionaryValue>();
    base::ListValue* list_value =
        dict_value->SetList("List", std::make_unique<base::ListValue>());
    root.Append(std::move(dict_value));

    // Test that there are not errors here.
    list_value->AppendString("blabla");
    TestSchemaValidation(subschema, root, SCHEMA_STRICT, true);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_UNKNOWN_TOPLEVEL, true);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_UNKNOWN, true);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_INVALID_TOPLEVEL, true);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_INVALID, true);

    // Invalid list item.
    list_value->AppendInteger(12345);
    TestSchemaValidation(subschema, root, SCHEMA_STRICT, false);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_UNKNOWN_TOPLEVEL, false);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_UNKNOWN, false);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_INVALID_TOPLEVEL, true);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_INVALID, true);
    TestSchemaValidationWithPath(subschema, root, "items[0].List.items[1]");
  }

  // Tests on StringWithPattern.
  {
    Schema subschema = schema.GetProperty("StringWithPattern");
    ASSERT_TRUE(subschema.valid());

    TestSchemaValidation(subschema, base::Value("foobar"), SCHEMA_STRICT,
                         false);
    TestSchemaValidation(subschema, base::Value("foo"), SCHEMA_STRICT, true);
    TestSchemaValidation(subschema, base::Value("fo"), SCHEMA_STRICT, false);
    TestSchemaValidation(subschema, base::Value("fooo"), SCHEMA_STRICT, true);
    TestSchemaValidation(subschema, base::Value("^foo+$"), SCHEMA_STRICT,
                         false);
  }

  // Tests on ObjectWithPatternProperties.
  {
    Schema subschema = schema.GetProperty("ObjectWithPatternProperties");
    ASSERT_TRUE(subschema.valid());
    base::DictionaryValue root;

    ASSERT_EQ(1u, subschema.GetPatternProperties("fooo").size());
    ASSERT_EQ(1u, subschema.GetPatternProperties("foo").size());
    ASSERT_EQ(1u, subschema.GetPatternProperties("barr").size());
    ASSERT_EQ(1u, subschema.GetPatternProperties("bar").size());
    ASSERT_EQ(1u, subschema.GetMatchingProperties("fooo").size());
    ASSERT_EQ(1u, subschema.GetMatchingProperties("foo").size());
    ASSERT_EQ(1u, subschema.GetMatchingProperties("barr").size());
    ASSERT_EQ(2u, subschema.GetMatchingProperties("bar").size());
    ASSERT_TRUE(subschema.GetPatternProperties("foobar").empty());

    root.SetInteger("fooo", 123);
    TestSchemaValidation(subschema, root, SCHEMA_STRICT, true);
    root.SetBoolean("fooo", false);
    TestSchemaValidation(subschema, root, SCHEMA_STRICT, false);
    root.Remove("fooo", nullptr);

    root.SetInteger("foo", 123);
    TestSchemaValidation(subschema, root, SCHEMA_STRICT, true);
    root.SetBoolean("foo", false);
    TestSchemaValidation(subschema, root, SCHEMA_STRICT, false);
    root.Remove("foo", nullptr);

    root.SetString("barr", "one");
    TestSchemaValidation(subschema, root, SCHEMA_STRICT, true);
    root.SetString("barr", "three");
    TestSchemaValidation(subschema, root, SCHEMA_STRICT, false);
    root.SetBoolean("barr", false);
    TestSchemaValidation(subschema, root, SCHEMA_STRICT, false);
    root.Remove("barr", nullptr);

    root.SetString("bar", "one");
    TestSchemaValidation(subschema, root, SCHEMA_STRICT, true);
    root.SetString("bar", "two");
    TestSchemaValidation(subschema, root, SCHEMA_STRICT, false);
    root.SetString("bar", "three");
    TestSchemaValidation(subschema, root, SCHEMA_STRICT, false);
    root.Remove("bar", nullptr);

    root.SetInteger("foobar", 123);
    TestSchemaValidation(subschema, root, SCHEMA_STRICT, false);
    TestSchemaValidation(subschema, root, SCHEMA_ALLOW_UNKNOWN, true);
    root.Remove("foobar", nullptr);
  }

  // Test that integer to double promotion is allowed.
  bundle.SetInteger("Number", 31415);
  TestSchemaValidation(schema, bundle, SCHEMA_STRICT, true);
}

TEST(SchemaTest, InvalidReferences) {
  // References to undeclared schemas fail.
  EXPECT_TRUE(ParseFails(
      "{"
      "  \"type\": \"object\","
      "  \"properties\": {"
      "    \"name\": { \"$ref\": \"undeclared\" }"
      "  }"
      "}"));

  // Can't refer to self.
  EXPECT_TRUE(ParseFails(
      "{"
      "  \"type\": \"object\","
      "  \"properties\": {"
      "    \"name\": {"
      "      \"id\": \"self\","
      "      \"$ref\": \"self\""
      "    }"
      "  }"
      "}"));

  // Duplicated IDs are invalid.
  EXPECT_TRUE(ParseFails(
      "{"
      "  \"type\": \"object\","
      "  \"properties\": {"
      "    \"name\": {"
      "      \"id\": \"x\","
      "      \"type\": \"string\""
      "    },"
      "    \"another\": {"
      "      \"id\": \"x\","
      "      \"type\": \"string\""
      "    }"
      "  }"
      "}"));

  // Main object can't be a reference.
  EXPECT_TRUE(ParseFails(
      "{"
      "  \"type\": \"object\","
      "  \"id\": \"main\","
      "  \"$ref\": \"main\""
      "}"));

  EXPECT_TRUE(ParseFails(
      "{"
      "  \"type\": \"object\","
      "  \"$ref\": \"main\""
      "}"));
}

TEST(SchemaTest, RecursiveReferences) {
  // Verifies that references can go to a parent schema, to define a
  // recursive type.
  std::string error;
  Schema schema = Schema::Parse(
      "{"
      "  \"type\": \"object\","
      "  \"properties\": {"
      "    \"bookmarks\": {"
      "      \"type\": \"array\","
      "      \"id\": \"ListOfBookmarks\","
      "      \"items\": {"
      "        \"type\": \"object\","
      "        \"properties\": {"
      "          \"name\": { \"type\": \"string\" },"
      "          \"url\": { \"type\": \"string\" },"
      "          \"children\": { \"$ref\": \"ListOfBookmarks\" }"
      "        }"
      "      }"
      "    }"
      "  }"
      "}", &error);
  ASSERT_TRUE(schema.valid()) << error;
  ASSERT_EQ(base::Value::Type::DICTIONARY, schema.type());

  Schema parent = schema.GetKnownProperty("bookmarks");
  ASSERT_TRUE(parent.valid());
  ASSERT_EQ(base::Value::Type::LIST, parent.type());

  // Check the recursive type a number of times.
  for (int i = 0; i < 10; ++i) {
    Schema items = parent.GetItems();
    ASSERT_TRUE(items.valid());
    ASSERT_EQ(base::Value::Type::DICTIONARY, items.type());

    Schema prop = items.GetKnownProperty("name");
    ASSERT_TRUE(prop.valid());
    ASSERT_EQ(base::Value::Type::STRING, prop.type());

    prop = items.GetKnownProperty("url");
    ASSERT_TRUE(prop.valid());
    ASSERT_EQ(base::Value::Type::STRING, prop.type());

    prop = items.GetKnownProperty("children");
    ASSERT_TRUE(prop.valid());
    ASSERT_EQ(base::Value::Type::LIST, prop.type());

    parent = prop;
  }
}

TEST(SchemaTest, UnorderedReferences) {
  // Verifies that references and IDs can come in any order.
  std::string error;
  Schema schema = Schema::Parse(
      "{"
      "  \"type\": \"object\","
      "  \"properties\": {"
      "    \"a\": { \"$ref\": \"shared\" },"
      "    \"b\": { \"$ref\": \"shared\" },"
      "    \"c\": { \"$ref\": \"shared\" },"
      "    \"d\": { \"$ref\": \"shared\" },"
      "    \"e\": {"
      "      \"type\": \"boolean\","
      "      \"id\": \"shared\""
      "    },"
      "    \"f\": { \"$ref\": \"shared\" },"
      "    \"g\": { \"$ref\": \"shared\" },"
      "    \"h\": { \"$ref\": \"shared\" },"
      "    \"i\": { \"$ref\": \"shared\" }"
      "  }"
      "}", &error);
  ASSERT_TRUE(schema.valid()) << error;
  ASSERT_EQ(base::Value::Type::DICTIONARY, schema.type());

  for (char c = 'a'; c <= 'i'; ++c) {
    Schema sub = schema.GetKnownProperty(std::string(1, c));
    ASSERT_TRUE(sub.valid()) << c;
    ASSERT_EQ(base::Value::Type::BOOLEAN, sub.type()) << c;
  }
}

TEST(SchemaTest, AdditionalPropertiesReference) {
  // Verifies that "additionalProperties" can be a reference.
  std::string error;
  Schema schema = Schema::Parse(
      "{"
      "  \"type\": \"object\","
      "  \"properties\": {"
      "    \"policy\": {"
      "      \"type\": \"object\","
      "      \"properties\": {"
      "        \"foo\": {"
      "          \"type\": \"boolean\","
      "          \"id\": \"FooId\""
      "        }"
      "      },"
      "      \"additionalProperties\": { \"$ref\": \"FooId\" }"
      "    }"
      "  }"
      "}", &error);
  ASSERT_TRUE(schema.valid()) << error;
  ASSERT_EQ(base::Value::Type::DICTIONARY, schema.type());

  Schema policy = schema.GetKnownProperty("policy");
  ASSERT_TRUE(policy.valid());
  ASSERT_EQ(base::Value::Type::DICTIONARY, policy.type());

  Schema foo = policy.GetKnownProperty("foo");
  ASSERT_TRUE(foo.valid());
  EXPECT_EQ(base::Value::Type::BOOLEAN, foo.type());

  Schema additional = policy.GetAdditionalProperties();
  ASSERT_TRUE(additional.valid());
  EXPECT_EQ(base::Value::Type::BOOLEAN, additional.type());

  Schema x = policy.GetProperty("x");
  ASSERT_TRUE(x.valid());
  EXPECT_EQ(base::Value::Type::BOOLEAN, x.type());
}

TEST(SchemaTest, ItemsReference) {
  // Verifies that "items" can be a reference.
  std::string error;
  Schema schema = Schema::Parse(
      "{"
      "  \"type\": \"object\","
      "  \"properties\": {"
      "    \"foo\": {"
      "      \"type\": \"boolean\","
      "      \"id\": \"FooId\""
      "    },"
      "    \"list\": {"
      "      \"type\": \"array\","
      "      \"items\": { \"$ref\": \"FooId\" }"
      "    }"
      "  }"
      "}", &error);
  ASSERT_TRUE(schema.valid()) << error;
  ASSERT_EQ(base::Value::Type::DICTIONARY, schema.type());

  Schema foo = schema.GetKnownProperty("foo");
  ASSERT_TRUE(foo.valid());
  EXPECT_EQ(base::Value::Type::BOOLEAN, foo.type());

  Schema list = schema.GetKnownProperty("list");
  ASSERT_TRUE(list.valid());
  ASSERT_EQ(base::Value::Type::LIST, list.type());

  Schema items = list.GetItems();
  ASSERT_TRUE(items.valid());
  ASSERT_EQ(base::Value::Type::BOOLEAN, items.type());
}

TEST(SchemaTest, EnumerationRestriction) {
  // Enum attribute is a list.
  EXPECT_TRUE(ParseFails(SchemaObjectWrapper(
      "{"
      "  \"type\": \"string\","
      "  \"enum\": 12"
      "}")));

  // Empty enum attributes is not allowed.
  EXPECT_TRUE(ParseFails(SchemaObjectWrapper(
      "{"
      "  \"type\": \"integer\","
      "  \"enum\": []"
      "}")));

  // Enum elements type should be same as stated.
  EXPECT_TRUE(ParseFails(SchemaObjectWrapper(
      "{"
      "  \"type\": \"string\","
      "  \"enum\": [1, 2, 3]"
      "}")));

  EXPECT_FALSE(ParseFails(SchemaObjectWrapper(
      "{"
      "  \"type\": \"integer\","
      "  \"enum\": [1, 2, 3]"
      "}")));

  EXPECT_FALSE(ParseFails(SchemaObjectWrapper(
      "{"
      "  \"type\": \"string\","
      "  \"enum\": [\"1\", \"2\", \"3\"]"
      "}")));
}

TEST(SchemaTest, RangedRestriction) {
  EXPECT_TRUE(ParseFails(SchemaObjectWrapper(
      "{"
      "  \"type\": \"integer\","
      "  \"minimum\": 10,"
      "  \"maximum\": 5"
      "}")));

  EXPECT_FALSE(ParseFails(SchemaObjectWrapper(
      "{"
      "  \"type\": \"integer\","
      "  \"minimum\": 10,"
      "  \"maximum\": 20"
      "}")));
}

}  // namespace policy
