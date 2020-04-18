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

#include <libaddressinput/localization.h>

#include <libaddressinput/address_data.h>
#include <libaddressinput/address_field.h>
#include <libaddressinput/address_problem.h>

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "grit.h"
#include "messages.h"

namespace {

using i18n::addressinput::AddressData;
using i18n::addressinput::AddressField;
using i18n::addressinput::INVALID_MESSAGE_ID;
using i18n::addressinput::Localization;

using i18n::addressinput::COUNTRY;
using i18n::addressinput::ADMIN_AREA;
using i18n::addressinput::LOCALITY;
using i18n::addressinput::DEPENDENT_LOCALITY;
using i18n::addressinput::SORTING_CODE;
using i18n::addressinput::POSTAL_CODE;
using i18n::addressinput::STREET_ADDRESS;
using i18n::addressinput::ORGANIZATION;
using i18n::addressinput::RECIPIENT;

using i18n::addressinput::MISSING_REQUIRED_FIELD;
using i18n::addressinput::UNKNOWN_VALUE;
using i18n::addressinput::INVALID_FORMAT;
using i18n::addressinput::MISMATCHING_VALUE;
using i18n::addressinput::USES_P_O_BOX;

// Tests for Localization object.
class LocalizationTest : public testing::TestWithParam<int> {
 public:
  LocalizationTest(const LocalizationTest&) = delete;
  LocalizationTest& operator=(const LocalizationTest&) = delete;

 protected:
  LocalizationTest() {}
  Localization localization_;
};

// Verifies that a custom message getter can be used.
static const char kValidMessage[] = "Data";
std::string GetValidMessage(int message_id) { return kValidMessage; }
TEST_P(LocalizationTest, ValidStringGetterCanBeUsed) {
  localization_.SetGetter(&GetValidMessage);
  EXPECT_EQ(kValidMessage, localization_.GetString(GetParam()));
}

// Verifies that the default language for messages does not have empty strings.
TEST_P(LocalizationTest, DefaultStringIsNotEmpty) {
  EXPECT_FALSE(localization_.GetString(GetParam()).empty());
}

// Verifies that the messages do not have newlines.
TEST_P(LocalizationTest, NoNewline) {
  EXPECT_EQ(std::string::npos, localization_.GetString(GetParam()).find('\n'));
}

// Verifies that the messages do not have double spaces.
TEST_P(LocalizationTest, NoDoubleSpace) {
  EXPECT_EQ(std::string::npos,
            localization_.GetString(GetParam()).find(std::string(2U, ' ')));
}

// Tests all message identifiers.
INSTANTIATE_TEST_CASE_P(
    AllMessages, LocalizationTest,
    testing::Values(
        IDS_LIBADDRESSINPUT_COUNTRY_OR_REGION_LABEL,
        IDS_LIBADDRESSINPUT_LOCALITY_LABEL,
        IDS_LIBADDRESSINPUT_ADDRESS_LINE_1_LABEL,
        IDS_LIBADDRESSINPUT_PIN_CODE_LABEL,
        IDS_LIBADDRESSINPUT_POSTAL_CODE_LABEL,
        IDS_LIBADDRESSINPUT_ZIP_CODE_LABEL,
        IDS_LIBADDRESSINPUT_AREA,
        IDS_LIBADDRESSINPUT_COUNTY,
        IDS_LIBADDRESSINPUT_DEPARTMENT,
        IDS_LIBADDRESSINPUT_DISTRICT,
        IDS_LIBADDRESSINPUT_DO_SI,
        IDS_LIBADDRESSINPUT_EMIRATE,
        IDS_LIBADDRESSINPUT_ISLAND,
        IDS_LIBADDRESSINPUT_PARISH,
        IDS_LIBADDRESSINPUT_PREFECTURE,
        IDS_LIBADDRESSINPUT_PROVINCE,
        IDS_LIBADDRESSINPUT_STATE,
        IDS_LIBADDRESSINPUT_ORGANIZATION_LABEL,
        IDS_LIBADDRESSINPUT_RECIPIENT_LABEL,
        IDS_LIBADDRESSINPUT_MISSING_REQUIRED_FIELD,
        IDS_LIBADDRESSINPUT_MISSING_REQUIRED_POSTAL_CODE_EXAMPLE_AND_URL,
        IDS_LIBADDRESSINPUT_MISSING_REQUIRED_POSTAL_CODE_EXAMPLE,
        IDS_LIBADDRESSINPUT_MISSING_REQUIRED_ZIP_CODE_EXAMPLE_AND_URL,
        IDS_LIBADDRESSINPUT_MISSING_REQUIRED_ZIP_CODE_EXAMPLE,
        IDS_LIBADDRESSINPUT_UNKNOWN_VALUE,
        IDS_LIBADDRESSINPUT_UNRECOGNIZED_FORMAT_POSTAL_CODE_EXAMPLE_AND_URL,
        IDS_LIBADDRESSINPUT_UNRECOGNIZED_FORMAT_POSTAL_CODE_EXAMPLE,
        IDS_LIBADDRESSINPUT_UNRECOGNIZED_FORMAT_POSTAL_CODE,
        IDS_LIBADDRESSINPUT_UNRECOGNIZED_FORMAT_ZIP_CODE_EXAMPLE_AND_URL,
        IDS_LIBADDRESSINPUT_UNRECOGNIZED_FORMAT_ZIP_CODE_EXAMPLE,
        IDS_LIBADDRESSINPUT_UNRECOGNIZED_FORMAT_ZIP,
        IDS_LIBADDRESSINPUT_MISMATCHING_VALUE_POSTAL_CODE_URL,
        IDS_LIBADDRESSINPUT_MISMATCHING_VALUE_POSTAL_CODE,
        IDS_LIBADDRESSINPUT_MISMATCHING_VALUE_ZIP_URL,
        IDS_LIBADDRESSINPUT_MISMATCHING_VALUE_ZIP,
        IDS_LIBADDRESSINPUT_PO_BOX_FORBIDDEN_VALUE));

// Verifies that an invalid message identifier results in an empty string in the
// default configuration.
TEST_F(LocalizationTest, InvalidMessageIsEmptyString) {
  EXPECT_TRUE(localization_.GetString(INVALID_MESSAGE_ID).empty());
}

TEST(LocalizationGetErrorMessageTest, MissingRequiredPostalCode) {
  Localization localization;
  AddressData address;
  address.region_code = "CH";
  EXPECT_EQ("You must provide a postal code, for example 2544."
            " Don't know your postal code? Find it out"
            " <a href=\"http://www.post.ch/db/owa/pv_plz_pack/pr_main\">"
            "here</a>.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         MISSING_REQUIRED_FIELD, true, true));
  EXPECT_EQ("You must provide a postal code, for example 2544.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         MISSING_REQUIRED_FIELD, true, false));
  EXPECT_EQ("You can't leave this empty.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         MISSING_REQUIRED_FIELD, false, false));
  EXPECT_EQ("You can't leave this empty.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         MISSING_REQUIRED_FIELD, false, true));
}

TEST(LocalizationGetErrorMessageTest, MissingRequiredZipCode) {
  Localization localization;
  AddressData address;
  address.region_code = "US";
  EXPECT_EQ("You must provide a ZIP code, for example 95014."
            " Don't know your ZIP code? Find it out"
            " <a href=\"https://tools.usps.com/go/ZipLookupAction!"
            "input.action\">here</a>.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         MISSING_REQUIRED_FIELD, true, true));
  EXPECT_EQ("You must provide a ZIP code, for example 95014.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         MISSING_REQUIRED_FIELD, true, false));
  EXPECT_EQ("You can't leave this empty.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         MISSING_REQUIRED_FIELD, false, false));
  EXPECT_EQ("You can't leave this empty.",
            localization.GetErrorMessage(address, POSTAL_CODE,
            MISSING_REQUIRED_FIELD, false, true));
}

TEST(LocalizationGetErrorMessageTest, MissingRequiredOtherFields) {
  Localization localization;
  AddressData address;
  address.region_code = "US";
  std::vector<AddressField> other_fields;
  other_fields.push_back(COUNTRY);
  other_fields.push_back(ADMIN_AREA);
  other_fields.push_back(LOCALITY);
  other_fields.push_back(DEPENDENT_LOCALITY);
  other_fields.push_back(SORTING_CODE);
  other_fields.push_back(STREET_ADDRESS);
  other_fields.push_back(ORGANIZATION);
  other_fields.push_back(RECIPIENT);
  for (std::vector<AddressField>::iterator it = other_fields.begin();
       it != other_fields.end(); it++) {
    EXPECT_EQ("You can't leave this empty.",
              localization.GetErrorMessage(
                  address, *it, MISSING_REQUIRED_FIELD, true, true));
    EXPECT_EQ("You can't leave this empty.",
              localization.GetErrorMessage(
                  address, *it, MISSING_REQUIRED_FIELD, true, false));
    EXPECT_EQ("You can't leave this empty.",
              localization.GetErrorMessage(
                  address, *it, MISSING_REQUIRED_FIELD, false, false));
    EXPECT_EQ("You can't leave this empty.",
              localization.GetErrorMessage(
                  address, *it, MISSING_REQUIRED_FIELD, false, true));
  }
}

TEST(LocalizationGetErrorMessageTest, UnknownValueOtherFields) {
  Localization localization;
  AddressData address;
  address.region_code = "US";
  address.administrative_area = "bad admin area";
  address.locality = "bad locality";
  address.dependent_locality = "bad dependent locality";
  address.sorting_code = "bad sorting code";
  std::vector<std::string> address_line;
  address_line.push_back("bad address line 1");
  address_line.push_back("bad address line 2");
  address.address_line = address_line;
  address.organization = "bad organization";
  address.recipient = "bad recipient";
  EXPECT_EQ("US "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, COUNTRY, UNKNOWN_VALUE, true, true));
  EXPECT_EQ("US "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, COUNTRY, UNKNOWN_VALUE, true, false));
  EXPECT_EQ("US "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, COUNTRY, UNKNOWN_VALUE, false, false));
  EXPECT_EQ("US "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, COUNTRY, UNKNOWN_VALUE, false, true));
  EXPECT_EQ("bad admin area "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, ADMIN_AREA, UNKNOWN_VALUE, true, true));
  EXPECT_EQ("bad admin area "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, ADMIN_AREA, UNKNOWN_VALUE, true, false));
  EXPECT_EQ("bad admin area "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, ADMIN_AREA, UNKNOWN_VALUE, false, false));
  EXPECT_EQ("bad admin area "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, ADMIN_AREA, UNKNOWN_VALUE, false, true));
  EXPECT_EQ("bad locality "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, LOCALITY, UNKNOWN_VALUE, true, true));
  EXPECT_EQ("bad locality "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, LOCALITY, UNKNOWN_VALUE, true, false));
  EXPECT_EQ("bad locality "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, LOCALITY, UNKNOWN_VALUE, false, false));
  EXPECT_EQ("bad locality "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, LOCALITY, UNKNOWN_VALUE, false, true));
  EXPECT_EQ("bad dependent locality "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, DEPENDENT_LOCALITY, UNKNOWN_VALUE, true, true));
  EXPECT_EQ("bad dependent locality "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, DEPENDENT_LOCALITY, UNKNOWN_VALUE, true, false));
  EXPECT_EQ("bad dependent locality "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, DEPENDENT_LOCALITY, UNKNOWN_VALUE, false, false));
  EXPECT_EQ("bad dependent locality "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, DEPENDENT_LOCALITY, UNKNOWN_VALUE, false, true));
  EXPECT_EQ("bad sorting code "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, SORTING_CODE, UNKNOWN_VALUE, true, true));
  EXPECT_EQ("bad sorting code "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, SORTING_CODE, UNKNOWN_VALUE, true, false));
  EXPECT_EQ("bad sorting code "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, SORTING_CODE, UNKNOWN_VALUE, false, false));
  EXPECT_EQ("bad sorting code "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, SORTING_CODE, UNKNOWN_VALUE, false, true));
  EXPECT_EQ("bad address line 1 "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, STREET_ADDRESS, UNKNOWN_VALUE, true, true));
  EXPECT_EQ("bad address line 1 "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, STREET_ADDRESS, UNKNOWN_VALUE, true, false));
  EXPECT_EQ("bad address line 1 "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, STREET_ADDRESS, UNKNOWN_VALUE, false, false));
  EXPECT_EQ("bad address line 1 "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, STREET_ADDRESS, UNKNOWN_VALUE, false, true));
  EXPECT_EQ("bad organization "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, ORGANIZATION, UNKNOWN_VALUE, true, true));
  EXPECT_EQ("bad organization "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, ORGANIZATION, UNKNOWN_VALUE, true, false));
  EXPECT_EQ("bad organization "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, ORGANIZATION, UNKNOWN_VALUE, false, false));
  EXPECT_EQ("bad organization "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, ORGANIZATION, UNKNOWN_VALUE, false, true));
  EXPECT_EQ("bad recipient "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, RECIPIENT, UNKNOWN_VALUE, true, true));
  EXPECT_EQ("bad recipient "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, RECIPIENT, UNKNOWN_VALUE, true, false));
  EXPECT_EQ("bad recipient "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, RECIPIENT, UNKNOWN_VALUE, false, false));
  EXPECT_EQ("bad recipient "
            "is not recognized as a known value for this field.",
            localization.GetErrorMessage(
                address, RECIPIENT, UNKNOWN_VALUE, false, true));
}

TEST(LocalizationGetErrorMessageTest, InvalidFormatPostalCode) {
  Localization localization;
  AddressData address;
  address.region_code = "CH";
  EXPECT_EQ("This postal code format is not recognized. Example "
            "of a valid postal code: 2544."
            " Don't know your postal code? Find it out"
            " <a href=\"http://www.post.ch/db/owa/pv_plz_pack/pr_main\">"
            "here</a>.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         INVALID_FORMAT, true, true));
  EXPECT_EQ("This postal code format is not recognized. Example "
            "of a valid postal code: 2544.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         INVALID_FORMAT, true, false));
  EXPECT_EQ("This postal code format is not recognized.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         INVALID_FORMAT, false, false));
  EXPECT_EQ("This postal code format is not recognized.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         INVALID_FORMAT, false, true));
}

TEST(LocalizationGetErrorMessageTest, InvalidFormatZipCode) {
  Localization localization;
  AddressData address;
  address.region_code = "US";
  EXPECT_EQ("This ZIP code format is not recognized. Example of "
            "a valid ZIP code: 95014."
            " Don't know your ZIP code? Find it out"
            " <a href=\"https://tools.usps.com/go/ZipLookupAction!"
            "input.action\">here</a>.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         INVALID_FORMAT, true, true));
  EXPECT_EQ("This ZIP code format is not recognized. Example of "
            "a valid ZIP code: 95014.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         INVALID_FORMAT, true, false));
  EXPECT_EQ("This ZIP code format is not recognized.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         INVALID_FORMAT, false, false));
  EXPECT_EQ("This ZIP code format is not recognized.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         INVALID_FORMAT, false, true));
}

TEST(LocalizationGetErrorMessageTest, MismatchingValuePostalCode) {
  Localization localization;
  AddressData address;
  address.region_code = "CH";
  EXPECT_EQ("This postal code does not appear to match the rest "
            "of this address."
            " Don't know your postal code? Find it out"
            " <a href=\"http://www.post.ch/db/owa/pv_plz_pack/pr_main\">"
            "here</a>.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         MISMATCHING_VALUE, true, true));
  EXPECT_EQ("This postal code does not appear to match the rest "
            "of this address.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         MISMATCHING_VALUE, true, false));
  EXPECT_EQ("This postal code does not appear to match the rest "
            "of this address.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         MISMATCHING_VALUE, false, false));
  EXPECT_EQ("This postal code does not appear to match the rest "
            "of this address."
            " Don't know your postal code? Find it out"
            " <a href=\"http://www.post.ch/db/owa/pv_plz_pack/pr_main\">"
            "here</a>.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         MISMATCHING_VALUE, false, true));
}

TEST(LocalizationGetErrorMessageTest, MismatchingValueZipCode) {
  Localization localization;
  AddressData address;
  address.region_code = "US";
  EXPECT_EQ("This ZIP code does not appear to match the rest of "
            "this address."
            " Don't know your ZIP code? Find it out"
            " <a href=\"https://tools.usps.com/go/ZipLookupAction!"
            "input.action\">here</a>.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         MISMATCHING_VALUE, true, true));
  EXPECT_EQ("This ZIP code does not appear to match the rest of "
            "this address.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         MISMATCHING_VALUE, true, false));
  EXPECT_EQ("This ZIP code does not appear to match the rest of "
            "this address.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         MISMATCHING_VALUE, false, false));
  EXPECT_EQ("This ZIP code does not appear to match the rest of "
            "this address."
            " Don't know your ZIP code? Find it out"
            " <a href=\"https://tools.usps.com/go/ZipLookupAction!"
            "input.action\">here</a>.",
            localization.GetErrorMessage(address, POSTAL_CODE,
                                         MISMATCHING_VALUE, false, true));
}

TEST(LocalizationGetErrorMessageTest, UsesPOBoxOtherFields) {
  Localization localization;
  AddressData address;
  address.region_code = "US";
  std::vector<AddressField> other_fields;
  other_fields.push_back(COUNTRY);
  other_fields.push_back(ADMIN_AREA);
  other_fields.push_back(LOCALITY);
  other_fields.push_back(DEPENDENT_LOCALITY);
  other_fields.push_back(SORTING_CODE);
  other_fields.push_back(STREET_ADDRESS);
  other_fields.push_back(ORGANIZATION);
  other_fields.push_back(RECIPIENT);
  for (std::vector<AddressField>::iterator it = other_fields.begin();
       it != other_fields.end(); it++) {
    EXPECT_EQ("This address line appears to contain a post "
              "office box. Please use a street"
              " or building address.",
              localization.GetErrorMessage(
                  address, *it, USES_P_O_BOX, true, true));
    EXPECT_EQ("This address line appears to contain a post "
              "office box. Please use a street"
              " or building address.",
              localization.GetErrorMessage(
                  address, *it, USES_P_O_BOX, true, false));
    EXPECT_EQ("This address line appears to contain a post "
              "office box. Please use a street"
              " or building address.",
              localization.GetErrorMessage(
                  address, *it, USES_P_O_BOX, false, false));
    EXPECT_EQ("This address line appears to contain a post "
              "office box. Please use a street"
              " or building address.",
              localization.GetErrorMessage(
                  address, *it, USES_P_O_BOX, false, true));
  }
}

}  // namespace
