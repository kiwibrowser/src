// Copyright (C) 2013 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "util/json.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace {

using i18n::addressinput::Json;

TEST(JsonTest, EmptyStringIsNotValid) {
  Json json;
  EXPECT_FALSE(json.ParseObject(std::string()));
}

TEST(JsonTest, EmptyDictionaryContainsNoKeys) {
  Json json;
  ASSERT_TRUE(json.ParseObject("{}"));
  std::string not_checked;
  EXPECT_FALSE(json.GetStringValueForKey("key", &not_checked));
  EXPECT_FALSE(json.GetStringValueForKey(std::string(), &not_checked));
}

TEST(JsonTest, InvalidJsonIsNotValid) {
  Json json;
  EXPECT_FALSE(json.ParseObject("{"));
}

TEST(JsonTest, OneKeyIsValid) {
  Json json;
  ASSERT_TRUE(json.ParseObject(R"({"key": "value"})"));
  std::string value;
  EXPECT_TRUE(json.GetStringValueForKey("key", &value));
  EXPECT_EQ("value", value);
}

TEST(JsonTest, EmptyStringKeyIsNotInObject) {
  Json json;
  ASSERT_TRUE(json.ParseObject(R"({"key": "value"})"));
  std::string not_checked;
  EXPECT_FALSE(json.GetStringValueForKey(std::string(), &not_checked));
}

TEST(JsonTest, EmptyKeyIsValid) {
  Json json;
  ASSERT_TRUE(json.ParseObject(R"({"": "value"})"));
  std::string value;
  EXPECT_TRUE(json.GetStringValueForKey(std::string(), &value));
  EXPECT_EQ("value", value);
}

TEST(JsonTest, EmptyValueIsValid) {
  Json json;
  ASSERT_TRUE(json.ParseObject(R"({"key": ""})"));
  std::string value;
  EXPECT_TRUE(json.GetStringValueForKey("key", &value));
  EXPECT_TRUE(value.empty());
}

TEST(JsonTest, Utf8EncodingIsValid) {
  Json json;
  ASSERT_TRUE(json.ParseObject(u8R"({"key": "Ü"})"));
  std::string value;
  EXPECT_TRUE(json.GetStringValueForKey("key", &value));
  EXPECT_EQ(u8"Ü", value);
}

TEST(JsonTest, InvalidUtf8IsNotValid) {
  Json json;
  EXPECT_FALSE(json.ParseObject("{\"key\": \"\xC3\x28\"}"));
}

TEST(JsonTest, NullInMiddleIsNotValid) {
  Json json;
  static const char kJson[] = "{\"key\": \"val\0ue\"}";
  EXPECT_FALSE(json.ParseObject(std::string(kJson, sizeof kJson - 1)));
}

TEST(JsonTest, TwoKeysAreValid) {
  Json json;
  ASSERT_TRUE(json.ParseObject(R"({"key1": "value1", "key2": "value2"})"));
  std::string value;
  EXPECT_TRUE(json.GetStringValueForKey("key1", &value));
  EXPECT_EQ("value1", value);

  EXPECT_TRUE(json.GetStringValueForKey("key2", &value));
  EXPECT_EQ("value2", value);
}

TEST(JsonTest, ListIsNotValid) {
  Json json;
  EXPECT_FALSE(json.ParseObject("[]"));
}

TEST(JsonTest, StringIsNotValid) {
  Json json;
  EXPECT_FALSE(json.ParseObject(R"("value")"));
}

TEST(JsonTest, NumberIsNotValid) {
  Json json;
  EXPECT_FALSE(json.ParseObject("3"));
}

TEST(JsonTest, NoDictionaryFound) {
  Json json;
  ASSERT_TRUE(json.ParseObject(R"({"key":"value"})"));
  EXPECT_TRUE(json.GetSubDictionaries().empty());
}

TEST(JsonTest, DictionaryFound) {
  Json json;
  ASSERT_TRUE(json.ParseObject(R"({"key":{"inner_key":"value"}})"));
  const std::vector<const Json*>& sub_dicts = json.GetSubDictionaries();
  ASSERT_EQ(1U, sub_dicts.size());

  std::string value;
  EXPECT_TRUE(sub_dicts.front()->GetStringValueForKey("inner_key", &value));
  EXPECT_EQ("value", value);
}

TEST(JsonTest, DictionariesHaveSubDictionaries) {
  Json json;
  ASSERT_TRUE(json.ParseObject(
      R"({"key":{"inner_key":{"inner_inner_key":"value"}}})"));
  const std::vector<const Json*>& sub_dicts = json.GetSubDictionaries();
  ASSERT_EQ(1U, sub_dicts.size());

  const std::vector<const Json*>& sub_sub_dicts =
      sub_dicts.front()->GetSubDictionaries();
  ASSERT_EQ(1U, sub_sub_dicts.size());

  std::string value;
  EXPECT_TRUE(
      sub_sub_dicts.front()->GetStringValueForKey("inner_inner_key", &value));
  EXPECT_EQ("value", value);
}

}  // namespace
