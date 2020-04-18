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

#include <libaddressinput/address_ui.h>

#include <libaddressinput/address_field.h>
#include <libaddressinput/address_ui_component.h>
#include <libaddressinput/localization.h>

#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace {

using i18n::addressinput::AddressField;
using i18n::addressinput::AddressUiComponent;
using i18n::addressinput::BuildComponents;
using i18n::addressinput::GetRegionCodes;
using i18n::addressinput::Localization;

using i18n::addressinput::COUNTRY;
using i18n::addressinput::ADMIN_AREA;
using i18n::addressinput::POSTAL_CODE;
using i18n::addressinput::STREET_ADDRESS;
using i18n::addressinput::ORGANIZATION;
using i18n::addressinput::RECIPIENT;

static const char kUiLanguageTag[] = "en";

// Returns testing::AssertionSuccess if the |components| are valid. Uses
// |region_code| in test failure messages.
testing::AssertionResult ComponentsAreValid(
    const std::vector<AddressUiComponent>& components) {
  if (components.empty()) {
    return testing::AssertionFailure() << "no components";
  }

  for (std::vector<AddressUiComponent>::const_iterator
       component_it = components.begin();
       component_it != components.end(); ++component_it) {
    static const AddressField kMinAddressField = COUNTRY;
    static const AddressField kMaxAddressField = RECIPIENT;
    if (component_it->field < kMinAddressField ||
        component_it->field > kMaxAddressField) {
      return testing::AssertionFailure() << "unexpected field "
                                         << component_it->field;
    }

    if (component_it->name.empty()) {
      return testing::AssertionFailure() << "empty field name for field "
                                         << component_it->field;
    }
  }

  return testing::AssertionSuccess();
}

// Tests for address UI functions.
class AddressUiTest : public testing::TestWithParam<std::string> {
 public:
  AddressUiTest(const AddressUiTest&) = delete;
  AddressUiTest& operator=(const AddressUiTest&) = delete;

 protected:
  AddressUiTest() {}
  Localization localization_;
  std::string best_address_language_tag_;
};

// Verifies that a region code consists of two characters, for example "TW".
TEST_P(AddressUiTest, RegionCodeHasTwoCharacters) {
  EXPECT_EQ(2, GetParam().size());
}

// Verifies that BuildComponents() returns valid UI components for a region
// code.
TEST_P(AddressUiTest, ComponentsAreValid) {
  EXPECT_TRUE(ComponentsAreValid(BuildComponents(
      GetParam(), localization_, kUiLanguageTag, &best_address_language_tag_)));
}

// Verifies that BuildComponents() returns at most one input field of each type.
TEST_P(AddressUiTest, UniqueFieldTypes) {
  std::set<AddressField> fields;
  const std::vector<AddressUiComponent>& components =
      BuildComponents(GetParam(), localization_, kUiLanguageTag,
                      &best_address_language_tag_);
  for (std::vector<AddressUiComponent>::const_iterator it = components.begin();
       it != components.end(); ++it) {
    EXPECT_TRUE(fields.insert(it->field).second);
  }
}

// Test all regions codes.
INSTANTIATE_TEST_CASE_P(
    AllRegions, AddressUiTest,
    testing::ValuesIn(GetRegionCodes()));

// Verifies that BuildComponents() returns an empty vector for an invalid region
// code.
TEST_F(AddressUiTest, InvalidRegionCodeReturnsEmptyVector) {
  EXPECT_TRUE(BuildComponents(
      "INVALID-REGION-CODE", localization_, kUiLanguageTag,
      &best_address_language_tag_).empty());
}

// Test data for determining the best language tag and whether the right format
// pattern was used (fmt vs lfmt).
struct LanguageTestCase {
  LanguageTestCase(const std::string& region_code,
                   const std::string& ui_language_tag,
                   const std::string& expected_best_address_language_tag,
                   AddressField expected_first_field)
      : region_code(region_code),
        ui_language_tag(ui_language_tag),
        expected_best_address_language_tag(expected_best_address_language_tag),
        expected_first_field(expected_first_field) {}

  ~LanguageTestCase() {}

  // The CLDR region code to test.
  const std::string region_code;

  // The BCP 47 language tag to test.
  const std::string ui_language_tag;

  // The expected value for the best language tag returned by BuildComponents().
  const std::string expected_best_address_language_tag;

  // The first field expected to be returned from BuildComponents(). Useful for
  // determining whether the returned format is in Latin or default order.
  const AddressField expected_first_field;
};

class BestAddressLanguageTagTest
    : public testing::TestWithParam<LanguageTestCase> {
 public:
  BestAddressLanguageTagTest(const BestAddressLanguageTagTest&) = delete;
  BestAddressLanguageTagTest& operator=(const BestAddressLanguageTagTest&) =
      delete;

 protected:
  BestAddressLanguageTagTest() {}
  Localization localization_;
  std::string best_address_language_tag_;
};

std::string GetterStub(int) { return std::string(); }

TEST_P(BestAddressLanguageTagTest, CorrectBestAddressLanguageTag) {
  localization_.SetGetter(&GetterStub);
  const std::vector<AddressUiComponent>& components = BuildComponents(
      GetParam().region_code, localization_, GetParam().ui_language_tag,
      &best_address_language_tag_);
  EXPECT_EQ(GetParam().expected_best_address_language_tag,
            best_address_language_tag_);
  ASSERT_FALSE(components.empty());
  EXPECT_EQ(GetParam().expected_first_field, components.front().field);
}

INSTANTIATE_TEST_CASE_P(
    LanguageTestCases, BestAddressLanguageTagTest,
    testing::Values(
        // Armenia supports hy and has a Latin format.
        LanguageTestCase("AM", "", "hy", RECIPIENT),
        LanguageTestCase("AM", "hy", "hy", RECIPIENT),
        LanguageTestCase("AM", "en", "hy-Latn", RECIPIENT),

        // P.R. China supports zh and has a Latin format.
        LanguageTestCase("CN", "zh-hans", "zh", POSTAL_CODE),
        LanguageTestCase("CN", "zh-hant", "zh", POSTAL_CODE),
        LanguageTestCase("CN", "zh-hans-CN", "zh", POSTAL_CODE),
        LanguageTestCase("CN", "zh", "zh", POSTAL_CODE),
        LanguageTestCase("CN", "ZH_HANS", "zh", POSTAL_CODE),
        LanguageTestCase("CN", "zh-cmn-Hans-CN", "zh", POSTAL_CODE),
        LanguageTestCase("CN", "zh-Latn", "zh-Latn", RECIPIENT),
        LanguageTestCase("CN", "zh-latn-CN", "zh-Latn", RECIPIENT),
        LanguageTestCase("CN", "en", "zh-Latn", RECIPIENT),
        LanguageTestCase("CN", "ja", "zh-Latn", RECIPIENT),
        LanguageTestCase("CN", "ko", "zh-Latn", RECIPIENT),
        LanguageTestCase("CN", "ZH_LATN", "zh-Latn", RECIPIENT),
        // Libaddressinput does not have information about extended language
        // subtags, so it uses the zh-Latn language tag for all base languages
        // that are not zh, even if it's effectively the same language.
        // Mandarin Chinese, Simplified script, as used in China:
        LanguageTestCase("CN", "cmn-Hans-CN", "zh-Latn", RECIPIENT),

        // Hong Kong supports zh-Hant and en. It has a Latin format.
        LanguageTestCase("HK", "zh", "zh-Hant", ADMIN_AREA),
        LanguageTestCase("HK", "zh-hans", "zh-Hant", ADMIN_AREA),
        LanguageTestCase("HK", "zh-hant", "zh-Hant", ADMIN_AREA),
        LanguageTestCase("HK", "zh-yue-HK", "zh-Hant", ADMIN_AREA),
        LanguageTestCase("HK", "en", "en", ADMIN_AREA),
        LanguageTestCase("HK", "zh-latn", "zh-Latn", RECIPIENT),
        LanguageTestCase("HK", "fr", "zh-Latn", RECIPIENT),
        LanguageTestCase("HK", "ja", "zh-Latn", RECIPIENT),
        LanguageTestCase("HK", "ko", "zh-Latn", RECIPIENT),
        // Libaddressinput does not have information about extended language
        // subtags, so it uses the zh-Latn language tag for all base languages
        // that are not zh or en, even if it's effectively the same language.
        // Cantonese Chinese, as used in Hong Kong:
        LanguageTestCase("HK", "yue-HK", "zh-Latn", RECIPIENT),

        // Macao supports zh-Hant and pt. It has a Latin format.
        LanguageTestCase("MO", "zh", "zh-Hant", STREET_ADDRESS),
        LanguageTestCase("MO", "zh-Hant", "zh-Hant", STREET_ADDRESS),
        LanguageTestCase("MO", "pt", "pt", STREET_ADDRESS),
        LanguageTestCase("MO", "zh-Latn", "zh-Latn", RECIPIENT),
        LanguageTestCase("MO", "en", "zh-Latn", RECIPIENT),

        // Switzerland supports de, fr, and it.
        LanguageTestCase("CH", "de", "de", ORGANIZATION),
        LanguageTestCase("CH", "de-DE", "de", ORGANIZATION),
        LanguageTestCase("CH", "de-Latn-DE", "de", ORGANIZATION),
        LanguageTestCase("CH", "fr", "fr", ORGANIZATION),
        LanguageTestCase("CH", "it", "it", ORGANIZATION),
        LanguageTestCase("CH", "en", "de", ORGANIZATION),

        // Antarctica does not have language information.
        LanguageTestCase("AQ", "en", "en", RECIPIENT),
        LanguageTestCase("AQ", "fr", "fr", RECIPIENT),
        LanguageTestCase("AQ", "es", "es", RECIPIENT),
        LanguageTestCase("AQ", "zh-Hans", "zh-Hans", RECIPIENT),

        // Egypt supports ar and has a Latin format.
        LanguageTestCase("EG", "ar", "ar", RECIPIENT),
        LanguageTestCase("EG", "ar-Arab", "ar", RECIPIENT),
        LanguageTestCase("EG", "ar-Latn", "ar-Latn", RECIPIENT),
        LanguageTestCase("EG", "fr", "ar-Latn", RECIPIENT),
        LanguageTestCase("EG", "fa", "ar-Latn", RECIPIENT),
        // Libaddressinput does not have language-to-script mapping, so it uses
        // the ar-Latn language tag for all base languages that are not ar, even
        // if the script is the same.
        LanguageTestCase("EG", "fa-Arab", "ar-Latn", RECIPIENT)));

}  // namespace
