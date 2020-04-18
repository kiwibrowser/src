// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/phone_number_i18n.h"

#include <stddef.h>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/field_types.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/libphonenumber/phonenumber_api.h"

using base::ASCIIToUTF16;
using base::UTF8ToUTF16;

namespace autofill {

using i18n::NormalizePhoneNumber;
using i18n::ParsePhoneNumber;
using i18n::ConstructPhoneNumber;
using i18n::PhoneNumbersMatch;

TEST(PhoneNumberI18NTest, NormalizePhoneNumber) {
  // "Large" digits.
  base::string16 phone1(
      UTF8ToUTF16("\xEF\xBC\x91\xEF\xBC\x96\xEF\xBC\x95\xEF\xBC\x90"
                  "\xEF\xBC\x97\xEF\xBC\x94\xEF\xBC\x99\xEF\xBC\x98"
                  "\xEF\xBC\x93\xEF\xBC\x92\xEF\xBC\x93"));
  EXPECT_EQ(NormalizePhoneNumber(phone1, "US"), ASCIIToUTF16("16507498323"));

  // Devanagari script digits.
  base::string16 phone2(
      UTF8ToUTF16("\xD9\xA1\xD9\xA6\xD9\xA5\xD9\xA0\xD9\xA8\xD9\xA3"
                  "\xD9\xA2\xD9\xA3\xD9\xA7\xD9\xA4\xD9\xA9"));
  EXPECT_EQ(NormalizePhoneNumber(phone2, "US"), ASCIIToUTF16("16508323749"));

  base::string16 phone3(UTF8ToUTF16("16503334\xef\xbc\x92\x35\xd9\xa5"));
  EXPECT_EQ(NormalizePhoneNumber(phone3, "US"), ASCIIToUTF16("16503334255"));

  base::string16 phone4(UTF8ToUTF16("+1(650)2346789"));
  EXPECT_EQ(NormalizePhoneNumber(phone4, "US"), ASCIIToUTF16("16502346789"));

  base::string16 phone5(UTF8ToUTF16("6502346789"));
  EXPECT_EQ(NormalizePhoneNumber(phone5, "US"), ASCIIToUTF16("6502346789"));
}

struct ParseNumberTestCase {
  // Expected parsing result.
  bool isPossibleNumber;
  // Inputs.
  std::string input;
  std::string assumed_region;
  // Further expectations.
  std::string number;
  std::string city_code;
  std::string country_code;
  std::string deduced_region;
};

class ParseNumberTest : public testing::TestWithParam<ParseNumberTestCase> {};

TEST_P(ParseNumberTest, ParsePhoneNumber) {
  auto test_case = GetParam();
  SCOPED_TRACE("Testing phone number " + test_case.input);

  base::string16 country_code, city_code, number;
  std::string deduced_region;
  ::i18n::phonenumbers::PhoneNumber unused_i18n_number;
  EXPECT_EQ(
      test_case.isPossibleNumber,
      ParsePhoneNumber(ASCIIToUTF16(test_case.input), test_case.assumed_region,
                       &country_code, &city_code, &number, &deduced_region,
                       &unused_i18n_number));
  EXPECT_EQ(ASCIIToUTF16(test_case.number), number);
  EXPECT_EQ(ASCIIToUTF16(test_case.city_code), city_code);
  EXPECT_EQ(ASCIIToUTF16(test_case.country_code), country_code);
  EXPECT_EQ(test_case.deduced_region, deduced_region);
}

INSTANTIATE_TEST_CASE_P(
    PhoneNumberI18NTest,
    ParseNumberTest,
    testing::Values(
        // Test for empty string.  Should give back empty strings.
        ParseNumberTestCase{false, "", "US"},
        // Test for string with less than 7 digits.  Should give back empty
        // strings.
        ParseNumberTestCase{false, "1234", "US"},
        // Test for string with exactly 7 digits.
        // Still a possible number with unknown("ZZ") deduced region.
        ParseNumberTestCase{true, "17134567", "US", "7134567", "", "1", "ZZ"},
        // Does not have area code, but still a possible number with
        // unknown("ZZ") deduced region.
        ParseNumberTestCase{true, "7134567", "US", "7134567", "", "", "ZZ"},
        // Valid Canadian toll-free number.
        ParseNumberTestCase{true, "3101234", "US", "3101234", "", "", "CA"},
        // Test for string with greater than 7 digits but less than 10 digits.
        // Should fail parsing in US.
        ParseNumberTestCase{false, "123456789", "US"},
        // Test for string with greater than 7 digits but less than 10 digits
        // and
        // separators.
        // Should fail parsing in US.
        ParseNumberTestCase{false, "12.345-6789", "US"},
        // Test for string with exactly 10 digits.
        // Should give back phone number and city code.
        // This one going to fail because of the incorrect area code.
        ParseNumberTestCase{false, "1234567890", "US"},
        // This is actually not a valid number because the first number after
        // area code is 1. But it's still a possible number, just with deduced
        // country set to unknown("ZZ").
        ParseNumberTestCase{true, "6501567890", "US", "1567890", "650", "",
                            "ZZ"},
        ParseNumberTestCase{true, "6504567890", "US", "4567890", "650", "",
                            "US"},
        // Test for string with exactly 10 digits and separators.
        // Should give back phone number and city code.
        ParseNumberTestCase{true, "(650) 456-7890", "US", "4567890", "650", "",
                            "US"},
        // Tests for string with over 10 digits.
        // 01 is incorrect prefix in the USA, we interpret 011 as prefix, and
        // rest is parsed as a Singapore number(country code "SG").
        ParseNumberTestCase{true, "0116504567890", "US", "04567890", "", "65",
                            "SG"},
        // 011 is a correct "dial out" prefix in the USA - the parsing should
        // succeed.
        ParseNumberTestCase{true, "01116504567890", "US", "4567890", "650", "1",
                            "US"},
        // 011 is a correct "dial out" prefix in the USA but the rest of the
        // number
        // can't parse as a US number.
        ParseNumberTestCase{true, "01178124567890", "US", "4567890", "812", "7",
                            "RU"},
        // Test for string with over 10 digits with separator characters.
        // Should give back phone number, city code, and country code. "011" is
        // US "dial out" code, which is discarded.
        ParseNumberTestCase{true, "(0111) 650-456.7890", "US", "4567890", "650",
                            "1", "US"},
        // Now try phone from Czech republic - it has 00 dial out code, 420
        // country
        // code and variable length area codes.
        ParseNumberTestCase{true, "+420 27-89.10.112", "US", "910112", "278",
                            "420", "CZ"},
        ParseNumberTestCase{false, "27-89.10.112", "US"},
        ParseNumberTestCase{true, "27-89.10.112", "CZ", "910112", "278", "",
                            "CZ"},
        ParseNumberTestCase{false, "420 57-89.10.112", "US"},
        ParseNumberTestCase{true, "420 57-89.10.112", "CZ", "910112", "578",
                            "420", "CZ"},
        // Parses vanity numbers.
        ParseNumberTestCase{true, "1-650-FLOWERS", "US", "3569377", "650", "1",
                            "US"},
        // 800 is not an area code, but the destination code. In our library
        // these
        // codes should be treated the same as area codes.
        ParseNumberTestCase{true, "1-800-FLOWERS", "US", "3569377", "800", "1",
                            "US"},
        // Don't add a country code where there was none.
        ParseNumberTestCase{true, "(08) 450 777 7777", "DE", "7777777", "8450",
                            "", "DE"}));

TEST(PhoneNumberI18NTest, ConstructPhoneNumber) {
  base::string16 number;
  EXPECT_TRUE(ConstructPhoneNumber(ASCIIToUTF16("1"), ASCIIToUTF16("650"),
                                   ASCIIToUTF16("2345678"), "US", &number));
  EXPECT_EQ(ASCIIToUTF16("1 650-234-5678"), number);
  EXPECT_TRUE(ConstructPhoneNumber(base::string16(), ASCIIToUTF16("650"),
                                   ASCIIToUTF16("2345678"), "US", &number));
  EXPECT_EQ(ASCIIToUTF16("(650) 234-5678"), number);
  EXPECT_TRUE(ConstructPhoneNumber(ASCIIToUTF16("1"), base::string16(),
                                   ASCIIToUTF16("6502345678"), "US", &number));
  EXPECT_EQ(ASCIIToUTF16("1 650-234-5678"), number);
  EXPECT_TRUE(ConstructPhoneNumber(base::string16(), base::string16(),
                                   ASCIIToUTF16("6502345678"), "US", &number));
  EXPECT_EQ(ASCIIToUTF16("(650) 234-5678"), number);

  EXPECT_FALSE(ConstructPhoneNumber(base::string16(), ASCIIToUTF16("650"),
                                    ASCIIToUTF16("234567890"), "US", &number));
  EXPECT_EQ(base::string16(), number);
  // Italian number
  EXPECT_TRUE(ConstructPhoneNumber(ASCIIToUTF16("39"), ASCIIToUTF16("347"),
                                   ASCIIToUTF16("2345678"), "IT", &number));
  EXPECT_EQ(ASCIIToUTF16("+39 347 234 5678"), number);
  EXPECT_TRUE(ConstructPhoneNumber(base::string16(), ASCIIToUTF16("347"),
                                   ASCIIToUTF16("2345678"), "IT", &number));
  EXPECT_EQ(ASCIIToUTF16("347 234 5678"), number);
  // German number.
  EXPECT_TRUE(ConstructPhoneNumber(ASCIIToUTF16("49"), ASCIIToUTF16("024"),
                                   ASCIIToUTF16("2345678901"), "DE", &number));
  EXPECT_EQ(ASCIIToUTF16("+49 2423 45678901"), number);
  EXPECT_TRUE(ConstructPhoneNumber(base::string16(), ASCIIToUTF16("024"),
                                   ASCIIToUTF16("2345678901"), "DE", &number));
  EXPECT_EQ(ASCIIToUTF16("02423 45678901"), number);
}

TEST(PhoneNumberI18NTest, PhoneNumbersMatch) {
  // Same numbers, defined country code.
  EXPECT_TRUE(PhoneNumbersMatch(ASCIIToUTF16("4158889999"),
                                ASCIIToUTF16("4158889999"), "US", "en-US"));
  // Same numbers, undefined country code.
  EXPECT_TRUE(PhoneNumbersMatch(ASCIIToUTF16("4158889999"),
                                ASCIIToUTF16("4158889999"), std::string(),
                                "en-US"));

  // Numbers differ by country code only.
  EXPECT_TRUE(PhoneNumbersMatch(ASCIIToUTF16("14158889999"),
                                ASCIIToUTF16("4158889999"), "US", "en-US"));

  // Same numbers, different formats.
  EXPECT_TRUE(PhoneNumbersMatch(ASCIIToUTF16("4158889999"),
                                ASCIIToUTF16("415-888-9999"), "US", "en-US"));
  EXPECT_TRUE(PhoneNumbersMatch(ASCIIToUTF16("4158889999"),
                                ASCIIToUTF16("(415)888-9999"), "US", "en-US"));
  EXPECT_TRUE(PhoneNumbersMatch(ASCIIToUTF16("4158889999"),
                                ASCIIToUTF16("415 888 9999"), "US", "en-US"));
  EXPECT_TRUE(PhoneNumbersMatch(ASCIIToUTF16("4158889999"),
                                ASCIIToUTF16("415 TUV WXYZ"), "US", "en-US"));
  EXPECT_TRUE(PhoneNumbersMatch(ASCIIToUTF16("1(415)888-99-99"),
                                ASCIIToUTF16("+14158889999"), "US", "en-US"));

  // Partial matches don't count.
  EXPECT_FALSE(PhoneNumbersMatch(ASCIIToUTF16("14158889999"),
                                 ASCIIToUTF16("8889999"), "US", "en-US"));

  // Different numbers don't match.
  EXPECT_FALSE(PhoneNumbersMatch(ASCIIToUTF16("14158889999"),
                                 ASCIIToUTF16("1415888"), "US", "en-US"));
}

// Tests that the phone numbers are correctly formatted for the Payment
// Response.
TEST(PhoneNumberUtilTest, FormatPhoneForResponse) {
  EXPECT_EQ("+15152231234",
            i18n::FormatPhoneForResponse("(515) 223-1234", "US"));
  EXPECT_EQ("+15152231234",
            i18n::FormatPhoneForResponse("(1) 515-223-1234", "US"));
  EXPECT_EQ("+33142685300",
            i18n::FormatPhoneForResponse("1 42 68 53 00", "FR"));

  // Invalid numbers are not formatted.
  EXPECT_EQ("(515) 123-1234",
            i18n::FormatPhoneForResponse("(515) 123-1234", "US"));
  EXPECT_EQ("(1) 515-123-1234",
            i18n::FormatPhoneForResponse("(1) 515-123-1234", "US"));
}

// Tests that the phone numbers are correctly formatted to display to the user.
TEST(PhoneNumberUtilTest, FormatPhoneForDisplay) {
  // Invalid number is not formatted.
  EXPECT_EQ("5151231234", i18n::FormatPhoneForDisplay("5151231234", "US"));
  // Valid number is formatted.
  EXPECT_EQ("+1 515-223-1234", i18n::FormatPhoneForDisplay("5152231234", "US"));
  EXPECT_EQ("+33 1 42 68 53 00",
            i18n::FormatPhoneForDisplay("142685300", "FR"));
}

// Test for the GetFormattedPhoneNumberForDisplay method.
struct PhoneNumberFormatCase {
  PhoneNumberFormatCase(const char* phone,
                        const char* country,
                        const char* expected_format,
                        const char* locale = "")
      : phone(phone),
        country(country),
        expected_format(expected_format),
        locale(locale) {}

  const char* phone;
  const char* country;
  const char* expected_format;
  const char* locale;
};

class GetFormattedPhoneNumberForDisplayTest
    : public testing::TestWithParam<PhoneNumberFormatCase> {};

TEST_P(GetFormattedPhoneNumberForDisplayTest,
       GetFormattedPhoneNumberForDisplay) {
  AutofillProfile profile;
  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::UTF8ToUTF16(GetParam().phone));
  profile.SetRawInfo(ADDRESS_HOME_COUNTRY,
                     base::UTF8ToUTF16(GetParam().country));
  EXPECT_EQ(GetParam().expected_format,
            base::UTF16ToUTF8(i18n::GetFormattedPhoneNumberForDisplay(
                profile, GetParam().locale)));
}

INSTANTIATE_TEST_CASE_P(
    GetFormattedPhoneNumberForDisplay,
    GetFormattedPhoneNumberForDisplayTest,
    testing::Values(
        //////////////////////////
        // US phone in US.
        //////////////////////////
        // Formatted phone numbers.
        PhoneNumberFormatCase("+1 415-555-5555", "US", "+1 415-555-5555"),
        PhoneNumberFormatCase("1 415-555-5555", "US", "+1 415-555-5555"),
        PhoneNumberFormatCase("415-555-5555", "US", "+1 415-555-5555"),
        // Raw phone numbers.
        PhoneNumberFormatCase("+14155555555", "US", "+1 415-555-5555"),
        PhoneNumberFormatCase("14155555555", "US", "+1 415-555-5555"),
        PhoneNumberFormatCase("4155555555", "US", "+1 415-555-5555"),

        //////////////////////////
        // US phone in CA.
        //////////////////////////
        // Formatted phone numbers.
        PhoneNumberFormatCase("+1 415-555-5555", "CA", "+1 415-555-5555"),
        PhoneNumberFormatCase("1 415-555-5555", "CA", "+1 415-555-5555"),
        PhoneNumberFormatCase("415-555-5555", "CA", "+1 415-555-5555"),
        // Raw phone numbers.
        PhoneNumberFormatCase("+14155555555", "CA", "+1 415-555-5555"),
        PhoneNumberFormatCase("14155555555", "CA", "+1 415-555-5555"),
        PhoneNumberFormatCase("4155555555", "CA", "+1 415-555-5555"),

        //////////////////////////
        // US phone in AU.
        //////////////////////////
        // A US phone with the country code is correctly formatted as an US
        // number.
        PhoneNumberFormatCase("+1 415-555-5555", "AU", "+1 415-555-5555"),
        PhoneNumberFormatCase("1 415-555-5555", "AU", "+1 415-555-5555"),
        // Without a country code, the phone is formatted for the profile's
        // country, if it's valid.
        PhoneNumberFormatCase("2 9374 4000", "AU", "+61 2 9374 4000"),
        // Without a country code, formatting returns the number as entered by
        // user, if it's invalid.
        PhoneNumberFormatCase("415-555-5555", "AU", "4155555555"),

        //////////////////////////
        // US phone in MX.
        //////////////////////////
        // A US phone with the country code is correctly formatted as an US
        // number.
        PhoneNumberFormatCase("+1 415-555-5555", "MX", "+1 415-555-5555"),
        // "+52 1 415 555 5555" is a valid number for Mexico,
        PhoneNumberFormatCase("1 415-555-5555", "MX", "+52 1 415 555 5555"),
        // Without a country code, the phone is formatted for the profile's
        // country.
        PhoneNumberFormatCase("415-555-5555", "MX", "+52 415 555 5555"),

        //////////////////////////
        // AU phone in AU.
        //////////////////////////
        // Formatted phone numbers.
        PhoneNumberFormatCase("+61 2 9374 4000", "AU", "+61 2 9374 4000"),
        PhoneNumberFormatCase("61 2 9374 4000", "AU", "+61 2 9374 4000"),
        PhoneNumberFormatCase("02 9374 4000", "AU", "+61 2 9374 4000"),
        PhoneNumberFormatCase("2 9374 4000", "AU", "+61 2 9374 4000"),
        // Raw phone numbers.
        PhoneNumberFormatCase("+61293744000", "AU", "+61 2 9374 4000"),
        PhoneNumberFormatCase("61293744000", "AU", "+61 2 9374 4000"),
        PhoneNumberFormatCase("0293744000", "AU", "+61 2 9374 4000"),
        PhoneNumberFormatCase("293744000", "AU", "+61 2 9374 4000"),

        //////////////////////////
        // AU phone in US.
        //////////////////////////
        // An AU phone with the country code is correctly formatted as an AU
        // number.
        PhoneNumberFormatCase("+61 2 9374 4000", "US", "+61 2 9374 4000"),
        PhoneNumberFormatCase("61 2 9374 4000", "US", "+61 2 9374 4000"),
        // Without a country code, the phone is formatted for the profile's
        // country.
        // This local AU number is associated with US profile, the number is
        // not a valid US number, therefore formatting will just return what
        // user entered.
        PhoneNumberFormatCase("02 9374 4000", "US", "0293744000"),
        // This local GR(Greece) number is formatted as an US number, if it's
        // valid US number.
        PhoneNumberFormatCase("22 6800 0090", "US", "+1 226-800-0090"),

        //////////////////////////
        // MX phone in MX.
        //////////////////////////
        // Formatted phone numbers.
        PhoneNumberFormatCase("+52 55 5342 8400", "MX", "+52 55 5342 8400"),
        PhoneNumberFormatCase("52 55 5342 8400", "MX", "+52 55 5342 8400"),
        PhoneNumberFormatCase("55 5342 8400", "MX", "+52 55 5342 8400"),
        // Raw phone numbers.
        PhoneNumberFormatCase("+525553428400", "MX", "+52 55 5342 8400"),
        PhoneNumberFormatCase("525553428400", "MX", "+52 55 5342 8400"),
        PhoneNumberFormatCase("5553428400", "MX", "+52 55 5342 8400"),

        //////////////////////////
        // MX phone in US.
        //////////////////////////
        // A MX phone with the country code is correctly formatted as a MX
        // number.
        PhoneNumberFormatCase("+52 55 5342 8400", "US", "+52 55 5342 8400"),
        PhoneNumberFormatCase("52 55 5342 8400", "US", "+52 55 5342 8400"),
        // This number is not a valid US number, we won't try to format.
        PhoneNumberFormatCase("55 5342 8400", "US", "5553428400")));

INSTANTIATE_TEST_CASE_P(
    GetFormattedPhoneNumberForDisplay_EdgeCases,
    GetFormattedPhoneNumberForDisplayTest,
    testing::Values(
        //////////////////////////
        // No country.
        //////////////////////////
        // Fallback to locale if no country is set.
        PhoneNumberFormatCase("52 55 5342 8400",
                              "",
                              "+52 55 5342 8400",
                              "es_MX"),
        PhoneNumberFormatCase("55 5342 8400", "", "+52 55 5342 8400", "es_MX"),
        PhoneNumberFormatCase("61 2 9374 4000", "", "+61 2 9374 4000", "en_AU"),
        PhoneNumberFormatCase("02 9374 4000", "", "+61 2 9374 4000", "en_AU"),

        // Numbers in local format yet are invalid with user locale, user might
        // be trying to enter a foreign number, calling formatting will just
        // return what the user entered.
        PhoneNumberFormatCase("55 5342 8400", "", "5553428400", "en_US"),
        PhoneNumberFormatCase("55 5342 8400", "", "5553428400"),
        PhoneNumberFormatCase("226 123 1234", "", "2261231234", "en_US"),
        PhoneNumberFormatCase("293744000", "", "293744000"),
        PhoneNumberFormatCase("02 9374 4000", "", "0293744000"),

        //////////////////////////
        // No country or locale.
        //////////////////////////
        // Format according to the country code.
        PhoneNumberFormatCase("61 2 9374 4000", "", "+61 2 9374 4000"),
        PhoneNumberFormatCase("52 55 5342 8400", "", "+52 55 5342 8400"),
        PhoneNumberFormatCase("1 415 555 5555", "", "+1 415-555-5555"),
        // If no country code is found, formats for US.
        PhoneNumberFormatCase("415-555-5555", "", "+1 415-555-5555")));

}  // namespace autofill
