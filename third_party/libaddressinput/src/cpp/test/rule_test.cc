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

#include "rule.h"

#include <libaddressinput/address_field.h>
#include <libaddressinput/localization.h>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "format_element.h"
#include "grit.h"
#include "messages.h"
#include "region_data_constants.h"
#include "util/json.h"

namespace {

using i18n::addressinput::AddressField;
using i18n::addressinput::ADMIN_AREA;
using i18n::addressinput::FormatElement;
using i18n::addressinput::INVALID_MESSAGE_ID;
using i18n::addressinput::Json;
using i18n::addressinput::LOCALITY;
using i18n::addressinput::Localization;
using i18n::addressinput::RegionDataConstants;
using i18n::addressinput::Rule;
using i18n::addressinput::STREET_ADDRESS;

TEST(RuleTest, CopyOverwritesRule) {
  Rule rule;
  ASSERT_TRUE(rule.ParseSerializedRule(
      R"({)"
      R"("fmt":"%S%Z",)"
      R"("lfmt":"%Z%S",)"
      R"("id":"data/XA",)"
      R"("name":"Le Test",)"
      R"("lname":"Testistan",)"
      R"("require":"AC",)"
      R"("sub_keys":"aa~bb~cc",)"
      R"("languages":"en~fr",)"
      R"("zip":"\\d{3}",)"
      R"("state_name_type":"area",)"
      R"("locality_name_type":"post_town",)"
      R"("sublocality_name_type":"neighborhood",)"
      R"("zip_name_type":"postal",)"
      R"("zipex":"1234",)"
      R"("posturl":"http://www.testpost.com")"
      R"(})"));

  Rule copy;
  EXPECT_NE(rule.GetFormat(), copy.GetFormat());
  EXPECT_NE(rule.GetLatinFormat(), copy.GetLatinFormat());
  EXPECT_NE(rule.GetId(), copy.GetId());
  EXPECT_NE(rule.GetRequired(), copy.GetRequired());
  EXPECT_NE(rule.GetSubKeys(), copy.GetSubKeys());
  EXPECT_NE(rule.GetLanguages(), copy.GetLanguages());
  EXPECT_NE(rule.GetAdminAreaNameMessageId(),
            copy.GetAdminAreaNameMessageId());
  EXPECT_NE(rule.GetPostalCodeNameMessageId(),
            copy.GetPostalCodeNameMessageId());
  EXPECT_NE(rule.GetLocalityNameMessageId(),
            copy.GetLocalityNameMessageId());
  EXPECT_NE(rule.GetSublocalityNameMessageId(),
            copy.GetSublocalityNameMessageId());
  EXPECT_NE(rule.GetName(), copy.GetName());
  EXPECT_NE(rule.GetLatinName(), copy.GetLatinName());
  EXPECT_NE(rule.GetPostalCodeExample(), copy.GetPostalCodeExample());
  EXPECT_NE(rule.GetPostServiceUrl(), copy.GetPostServiceUrl());

  EXPECT_TRUE(rule.GetPostalCodeMatcher() != nullptr);
  EXPECT_TRUE(copy.GetPostalCodeMatcher() == nullptr);

  copy.CopyFrom(rule);
  EXPECT_EQ(rule.GetFormat(), copy.GetFormat());
  EXPECT_EQ(rule.GetLatinFormat(), copy.GetLatinFormat());
  EXPECT_EQ(rule.GetId(), copy.GetId());
  EXPECT_EQ(rule.GetRequired(), copy.GetRequired());
  EXPECT_EQ(rule.GetSubKeys(), copy.GetSubKeys());
  EXPECT_EQ(rule.GetLanguages(), copy.GetLanguages());
  EXPECT_EQ(rule.GetAdminAreaNameMessageId(),
            copy.GetAdminAreaNameMessageId());
  EXPECT_EQ(rule.GetPostalCodeNameMessageId(),
            copy.GetPostalCodeNameMessageId());
  EXPECT_EQ(rule.GetSublocalityNameMessageId(),
            copy.GetSublocalityNameMessageId());
  EXPECT_EQ(rule.GetLocalityNameMessageId(),
            copy.GetLocalityNameMessageId());
  EXPECT_EQ(rule.GetName(), copy.GetName());
  EXPECT_EQ(rule.GetLatinName(), copy.GetLatinName());
  EXPECT_EQ(rule.GetPostalCodeExample(), copy.GetPostalCodeExample());
  EXPECT_EQ(rule.GetPostServiceUrl(), copy.GetPostServiceUrl());

  EXPECT_TRUE(copy.GetPostalCodeMatcher() != nullptr);
}

TEST(RuleTest, ParseOverwritesRule) {
  Rule rule;
  ASSERT_TRUE(rule.ParseSerializedRule("{"
                                       "\"fmt\":\"%S%Z\","
                                       "\"state_name_type\":\"area\","
                                       "\"zip\":\"1234\","
                                       "\"zip_name_type\":\"postal\","
                                       "\"zipex\":\"1234\","
                                       "\"posturl\":\"http://www.testpost.com\""
                                       "}"));
  EXPECT_FALSE(rule.GetFormat().empty());
  EXPECT_EQ(IDS_LIBADDRESSINPUT_AREA,
            rule.GetAdminAreaNameMessageId());
  EXPECT_EQ(IDS_LIBADDRESSINPUT_POSTAL_CODE_LABEL,
            rule.GetPostalCodeNameMessageId());
  EXPECT_EQ("1234", rule.GetSolePostalCode());
  EXPECT_EQ("1234", rule.GetPostalCodeExample());
  EXPECT_EQ("http://www.testpost.com", rule.GetPostServiceUrl());

  ASSERT_TRUE(rule.ParseSerializedRule("{"
                                       "\"fmt\":\"\","
                                       "\"state_name_type\":\"do_si\","
                                       "\"zip_name_type\":\"zip\","
                                       "\"zipex\":\"5678\","
                                       "\"posturl\":\"http://www.fakepost.com\""
                                       "}"));
  EXPECT_TRUE(rule.GetFormat().empty());
  EXPECT_EQ(IDS_LIBADDRESSINPUT_DO_SI,
            rule.GetAdminAreaNameMessageId());
  EXPECT_EQ(IDS_LIBADDRESSINPUT_ZIP_CODE_LABEL,
            rule.GetPostalCodeNameMessageId());
  EXPECT_TRUE(rule.GetSolePostalCode().empty());
  EXPECT_EQ("5678", rule.GetPostalCodeExample());
  EXPECT_EQ("http://www.fakepost.com", rule.GetPostServiceUrl());
}

TEST(RuleTest, ParsesFormatCorrectly) {
  std::vector<FormatElement> expected;
  expected.push_back(FormatElement(ADMIN_AREA));
  expected.push_back(FormatElement(LOCALITY));
  Rule rule;
  ASSERT_TRUE(rule.ParseSerializedRule("{\"fmt\":\"%S%C\"}"));
  EXPECT_EQ(expected, rule.GetFormat());
}

TEST(RuleTest, ParsesNameCorrectly) {
  Rule rule;
  ASSERT_TRUE(rule.ParseSerializedRule("{\"name\":\"Le Test\"}"));
  EXPECT_EQ("Le Test", rule.GetName());
}

TEST(RuleTest, ParsesLatinNameCorrectly) {
  Rule rule;
  ASSERT_TRUE(rule.ParseSerializedRule("{\"lname\":\"Testistan\"}"));
  EXPECT_EQ("Testistan", rule.GetLatinName());
}

TEST(RuleTest, ParsesLatinFormatCorrectly) {
  std::vector<FormatElement> expected;
  expected.push_back(FormatElement(LOCALITY));
  expected.push_back(FormatElement(ADMIN_AREA));
  Rule rule;
  ASSERT_TRUE(rule.ParseSerializedRule("{\"lfmt\":\"%C%S\"}"));
  EXPECT_EQ(expected, rule.GetLatinFormat());
}

TEST(RuleTest, ParsesRequiredCorrectly) {
  std::vector<AddressField> expected;
  expected.push_back(STREET_ADDRESS);
  expected.push_back(LOCALITY);
  Rule rule;
  ASSERT_TRUE(rule.ParseSerializedRule("{\"require\":\"AC\"}"));
  EXPECT_EQ(expected, rule.GetRequired());
}

TEST(RuleTest, ParsesSubKeysCorrectly) {
  std::vector<std::string> expected;
  expected.push_back("aa");
  expected.push_back("bb");
  expected.push_back("cc");
  Rule rule;
  ASSERT_TRUE(rule.ParseSerializedRule("{\"sub_keys\":\"aa~bb~cc\"}"));
  EXPECT_EQ(expected, rule.GetSubKeys());
}

TEST(RuleTest, ParsesLanguagesCorrectly) {
  std::vector<std::string> expected;
  expected.push_back("de");
  expected.push_back("fr");
  expected.push_back("it");
  Rule rule;
  ASSERT_TRUE(rule.ParseSerializedRule("{\"languages\":\"de~fr~it\"}"));
  EXPECT_EQ(expected, rule.GetLanguages());
}

TEST(RuleTest, ParsesPostalCodeExampleCorrectly) {
  Rule rule;
  ASSERT_TRUE(rule.ParseSerializedRule("{\"zipex\":\"1234,12345-6789\"}"));
  EXPECT_EQ("1234,12345-6789", rule.GetPostalCodeExample());
}

TEST(RuleTest, ParsesPostServiceUrlCorrectly) {
  Rule rule;
  ASSERT_TRUE(
      rule.ParseSerializedRule("{\"posturl\":\"http://www.testpost.com\"}"));
  EXPECT_EQ("http://www.testpost.com", rule.GetPostServiceUrl());
}

TEST(RuleTest, PostalCodeMatcher) {
  Rule rule;
  ASSERT_TRUE(rule.ParseSerializedRule(R"({"zip":"\\d{3}"})"));
  EXPECT_TRUE(rule.GetPostalCodeMatcher() != nullptr);
}

TEST(RuleTest, PostalCodeMatcherInvalidRegExp) {
  Rule rule;
  ASSERT_TRUE(rule.ParseSerializedRule(R"({"zip":"("})"));
  EXPECT_TRUE(rule.GetPostalCodeMatcher() == nullptr);
}

TEST(RuleTest, ParsesJsonRuleCorrectly) {
  Json json;
  ASSERT_TRUE(json.ParseObject(R"({"zip":"\\d{3}"})"));
  Rule rule;
  rule.ParseJsonRule(json);
  EXPECT_TRUE(rule.GetPostalCodeMatcher() != nullptr);
}

TEST(RuleTest, EmptyStringIsNotValid) {
  Rule rule;
  EXPECT_FALSE(rule.ParseSerializedRule(std::string()));
}

TEST(RuleTest, EmptyDictionaryIsValid) {
  Rule rule;
  EXPECT_TRUE(rule.ParseSerializedRule("{}"));
}

// Tests for parsing the postal code name.
class PostalCodeNameParseTest
    : public testing::TestWithParam<std::pair<std::string, int> > {
 public:
  PostalCodeNameParseTest(const PostalCodeNameParseTest&) = delete;
  PostalCodeNameParseTest& operator=(const PostalCodeNameParseTest&) = delete;

 protected:
  PostalCodeNameParseTest() {}
  Rule rule_;
};

// Verifies that a postal code name is parsed correctly.
TEST_P(PostalCodeNameParseTest, ParsedCorrectly) {
  ASSERT_TRUE(rule_.ParseSerializedRule(GetParam().first));
  EXPECT_EQ(GetParam().second, rule_.GetPostalCodeNameMessageId());
}

// Test parsing all postal code names.
INSTANTIATE_TEST_CASE_P(
    AllPostalCodeNames, PostalCodeNameParseTest,
    testing::Values(
        std::make_pair("{\"zip_name_type\":\"pin\"}",
                       IDS_LIBADDRESSINPUT_PIN_CODE_LABEL),
        std::make_pair("{\"zip_name_type\":\"postal\"}",
                       IDS_LIBADDRESSINPUT_POSTAL_CODE_LABEL),
        std::make_pair("{\"zip_name_type\":\"zip\"}",
                       IDS_LIBADDRESSINPUT_ZIP_CODE_LABEL)));

// Tests for parsing the locality name.
class LocalityNameParseTest
    : public testing::TestWithParam<std::pair<std::string, int> > {
 public:
  LocalityNameParseTest(const LocalityNameParseTest&) = delete;
  LocalityNameParseTest& operator=(const LocalityNameParseTest&) = delete;

 protected:
  LocalityNameParseTest() {}
  Rule rule_;
};

// Verifies that a locality name is parsed correctly.
TEST_P(LocalityNameParseTest, ParsedCorrectly) {
  ASSERT_TRUE(rule_.ParseSerializedRule(GetParam().first));
  EXPECT_EQ(GetParam().second, rule_.GetLocalityNameMessageId());
}

// Test parsing all locality names.
INSTANTIATE_TEST_CASE_P(
    AllLocalityNames, LocalityNameParseTest,
    testing::Values(
        std::make_pair("{\"locality_name_type\":\"post_town\"}",
                       IDS_LIBADDRESSINPUT_POST_TOWN),
        std::make_pair("{\"locality_name_type\":\"city\"}",
                       IDS_LIBADDRESSINPUT_LOCALITY_LABEL),
        std::make_pair("{\"locality_name_type\":\"district\"}",
                       IDS_LIBADDRESSINPUT_DISTRICT)));

// Tests for parsing the locality name.
class SublocalityNameParseTest
    : public testing::TestWithParam<std::pair<std::string, int> > {
 public:
  SublocalityNameParseTest(const SublocalityNameParseTest&) = delete;
  SublocalityNameParseTest& operator=(const SublocalityNameParseTest&) = delete;

 protected:
  SublocalityNameParseTest() {}
  Rule rule_;
};

// Verifies that a sublocality name is parsed correctly.
TEST_P(SublocalityNameParseTest, ParsedCorrectly) {
  ASSERT_TRUE(rule_.ParseSerializedRule(GetParam().first));
  EXPECT_EQ(GetParam().second, rule_.GetSublocalityNameMessageId());
}

// Test parsing all sublocality names.
INSTANTIATE_TEST_CASE_P(
    AllSublocalityNames, SublocalityNameParseTest,
    testing::Values(
        std::make_pair("{\"sublocality_name_type\":\"village_township\"}",
                       IDS_LIBADDRESSINPUT_VILLAGE_TOWNSHIP),
        std::make_pair("{\"sublocality_name_type\":\"neighborhood\"}",
                       IDS_LIBADDRESSINPUT_NEIGHBORHOOD),
        std::make_pair("{\"sublocality_name_type\":\"suburb\"}",
                       IDS_LIBADDRESSINPUT_SUBURB),
        std::make_pair("{\"sublocality_name_type\":\"district\"}",
                       IDS_LIBADDRESSINPUT_DISTRICT)));

// Tests for parsing the administrative area name.
class AdminAreaNameParseTest
    : public testing::TestWithParam<std::pair<std::string, int> > {
 public:
  AdminAreaNameParseTest(const AdminAreaNameParseTest&) = delete;
  AdminAreaNameParseTest& operator=(const AdminAreaNameParseTest&) = delete;

 protected:
  AdminAreaNameParseTest() {}
  Rule rule_;
};

// Verifies that an administrative area name is parsed correctly.
TEST_P(AdminAreaNameParseTest, ParsedCorrectly) {
  ASSERT_TRUE(rule_.ParseSerializedRule(GetParam().first));
  EXPECT_EQ(GetParam().second, rule_.GetAdminAreaNameMessageId());
}

// Test parsing all administrative area names.
INSTANTIATE_TEST_CASE_P(
    AllAdminAreaNames, AdminAreaNameParseTest,
    testing::Values(
        std::make_pair("{\"state_name_type\":\"area\"}",
                       IDS_LIBADDRESSINPUT_AREA),
        std::make_pair("{\"state_name_type\":\"county\"}",
                       IDS_LIBADDRESSINPUT_COUNTY),
        std::make_pair("{\"state_name_type\":\"department\"}",
                       IDS_LIBADDRESSINPUT_DEPARTMENT),
        std::make_pair("{\"state_name_type\":\"district\"}",
                       IDS_LIBADDRESSINPUT_DISTRICT),
        std::make_pair("{\"state_name_type\":\"do_si\"}",
                       IDS_LIBADDRESSINPUT_DO_SI),
        std::make_pair("{\"state_name_type\":\"emirate\"}",
                       IDS_LIBADDRESSINPUT_EMIRATE),
        std::make_pair("{\"state_name_type\":\"island\"}",
                       IDS_LIBADDRESSINPUT_ISLAND),
        std::make_pair("{\"state_name_type\":\"parish\"}",
                       IDS_LIBADDRESSINPUT_PARISH),
        std::make_pair("{\"state_name_type\":\"prefecture\"}",
                       IDS_LIBADDRESSINPUT_PREFECTURE),
        std::make_pair("{\"state_name_type\":\"province\"}",
                       IDS_LIBADDRESSINPUT_PROVINCE),
        std::make_pair("{\"state_name_type\":\"state\"}",
                       IDS_LIBADDRESSINPUT_STATE)));

// Tests for rule parsing.
class RuleParseTest : public testing::TestWithParam<std::string> {
 public:
  RuleParseTest(const RuleParseTest&) = delete;
  RuleParseTest& operator=(const RuleParseTest&) = delete;

 protected:
  RuleParseTest() {}

  const std::string& GetRegionData() const {
    // GetParam() is either a region code or the region data itself.
    // RegionDataContants::GetRegionData() returns an empty string for anything
    // that's not a region code.
    const std::string& data = RegionDataConstants::GetRegionData(GetParam());
    return !data.empty() ? data : GetParam();
  }

  Rule rule_;
  Localization localization_;
};

// Verifies that a region data can be parsed successfully.
TEST_P(RuleParseTest, RegionDataParsedSuccessfully) {
  EXPECT_TRUE(rule_.ParseSerializedRule(GetRegionData()));
}

// Verifies that the admin area name type corresponds to a UI string.
TEST_P(RuleParseTest, AdminAreaNameTypeHasUiString) {
  const std::string& region_data = GetRegionData();
  rule_.ParseSerializedRule(region_data);
  if (region_data.find("state_name_type") != std::string::npos) {
    EXPECT_NE(INVALID_MESSAGE_ID, rule_.GetAdminAreaNameMessageId());
    EXPECT_FALSE(
        localization_.GetString(rule_.GetAdminAreaNameMessageId()).empty());
  }
}

// Verifies that the postal code name type corresponds to a UI string.
TEST_P(RuleParseTest, PostalCodeNameTypeHasUiString) {
  const std::string& region_data = GetRegionData();
  rule_.ParseSerializedRule(region_data);
  if (region_data.find("zip_name_type") != std::string::npos) {
    EXPECT_NE(INVALID_MESSAGE_ID, rule_.GetPostalCodeNameMessageId());
    EXPECT_FALSE(
        localization_.GetString(rule_.GetPostalCodeNameMessageId()).empty());
  }
}

// Verifies that the locality name type corresponds to a UI string.
TEST_P(RuleParseTest, LocalityNameTypeHasUiString) {
  const std::string& region_data = GetRegionData();
  rule_.ParseSerializedRule(region_data);
  // The leading quote here ensures we don't match against sublocality_name_type
  // in the data.
  if (region_data.find("\"locality_name_type") != std::string::npos) {
    EXPECT_NE(INVALID_MESSAGE_ID, rule_.GetLocalityNameMessageId());
    EXPECT_FALSE(
        localization_.GetString(rule_.GetLocalityNameMessageId()).empty());
  }
}

// Verifies that the sublocality name type corresponds to a UI string.
TEST_P(RuleParseTest, SublocalityNameTypeHasUiString) {
  const std::string& region_data = GetRegionData();
  rule_.ParseSerializedRule(region_data);
  if (region_data.find("sublocality_name_type") != std::string::npos) {
    EXPECT_NE(INVALID_MESSAGE_ID, rule_.GetSublocalityNameMessageId());
    EXPECT_FALSE(
        localization_.GetString(rule_.GetSublocalityNameMessageId()).empty());
  }
}

// Verifies that the sole postal code is correctly recognized and copied.
TEST_P(RuleParseTest, SolePostalCode) {
  Rule rule;
  ASSERT_TRUE(rule.ParseSerializedRule("{\"zip\":\"1234\"}"));
  EXPECT_TRUE(rule.GetPostalCodeMatcher() != nullptr);
  EXPECT_EQ(rule.GetSolePostalCode(), "1234");

  Rule copy;
  EXPECT_TRUE(copy.GetPostalCodeMatcher() == nullptr);
  EXPECT_TRUE(copy.GetSolePostalCode().empty());

  copy.CopyFrom(rule);
  EXPECT_TRUE(copy.GetPostalCodeMatcher() != nullptr);
  EXPECT_EQ(rule.GetSolePostalCode(), copy.GetSolePostalCode());
}

// Test parsing all region data.
INSTANTIATE_TEST_CASE_P(
    AllRulesTest, RuleParseTest,
    testing::ValuesIn(RegionDataConstants::GetRegionCodes()));

// Test parsing the default rule.
INSTANTIATE_TEST_CASE_P(
    DefaultRuleTest, RuleParseTest,
    testing::Values(RegionDataConstants::GetDefaultRegionData()));

}  // namespace
