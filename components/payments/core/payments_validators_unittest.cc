// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/payments_validators.h"

#include <ostream>  // NOLINT
#include "testing/gtest/include/gtest/gtest.h"

namespace payments {
namespace {

struct CurrencyCodeTestCase {
  CurrencyCodeTestCase(const char* code, bool expected_valid)
      : code(code), expected_valid(expected_valid) {}
  ~CurrencyCodeTestCase() {}

  const char* code;
  bool expected_valid;
};

class PaymentsCurrencyValidatorTest
    : public testing::TestWithParam<CurrencyCodeTestCase> {};

const char* longString2048() {
  static char long_string[2049];
  for (int i = 0; i < 2048; i++)
    long_string[i] = 'a';
  long_string[2048] = '\0';
  return long_string;
}

TEST_P(PaymentsCurrencyValidatorTest, IsValidCurrencyCodeFormat) {
  std::string error_message;
  EXPECT_EQ(GetParam().expected_valid,
            payments::PaymentsValidators::IsValidCurrencyCodeFormat(
                GetParam().code, &error_message))
      << error_message;
  EXPECT_EQ(GetParam().expected_valid, error_message.empty()) << error_message;

  EXPECT_EQ(GetParam().expected_valid,
            payments::PaymentsValidators::IsValidCurrencyCodeFormat(
                GetParam().code, nullptr));
}

INSTANTIATE_TEST_CASE_P(
    CurrencyCodes,
    PaymentsCurrencyValidatorTest,
    testing::Values(
        // The most common identifiers are three-letter alphabetic codes as
        // defined by [ISO4217] (for example, "USD" for US Dollars).
        // |system| is a URL that indicates the currency system that the
        // currency identifier belongs to. By default,
        // the value is urn:iso:std:iso:4217 indicating that currency is defined
        // by [[ISO4217]], however any string of at most 2048
        // characters is considered valid in other currencySystem. Returns false
        // if currency |code| is too long (greater than 2048).
        CurrencyCodeTestCase("USD", true),
        CurrencyCodeTestCase("US1", false),
        CurrencyCodeTestCase("US", false),
        CurrencyCodeTestCase("USDO", false),
        CurrencyCodeTestCase("usd", false),
        CurrencyCodeTestCase("ANYSTRING", false),
        CurrencyCodeTestCase("", false),
        CurrencyCodeTestCase(longString2048(), false)));

struct TestCase {
  TestCase(const char* input, bool expected_valid)
      : input(input), expected_valid(expected_valid) {}
  ~TestCase() {}

  const char* input;
  bool expected_valid;
};

std::ostream& operator<<(std::ostream& out, const TestCase& test_case) {
  out << "'" << test_case.input << "' is expected to be "
      << (test_case.expected_valid ? "valid" : "invalid");
  return out;
}

class PaymentsAmountValidatorTest : public testing::TestWithParam<TestCase> {};

TEST_P(PaymentsAmountValidatorTest, IsValidAmountFormat) {
  std::string error_message;
  EXPECT_EQ(GetParam().expected_valid,
            payments::PaymentsValidators::IsValidAmountFormat(GetParam().input,
                                                              &error_message))
      << error_message;
  EXPECT_EQ(GetParam().expected_valid, error_message.empty()) << error_message;

  EXPECT_EQ(GetParam().expected_valid,
            payments::PaymentsValidators::IsValidAmountFormat(GetParam().input,
                                                              nullptr));
}

INSTANTIATE_TEST_CASE_P(
    Amounts,
    PaymentsAmountValidatorTest,
    testing::Values(TestCase("0", true),
                    TestCase("-0", true),
                    TestCase("1", true),
                    TestCase("10", true),
                    TestCase("-3", true),
                    TestCase("10.99", true),
                    TestCase("-3.00", true),
                    TestCase("01234567890123456789.0123456789", true),
                    TestCase("01234567890123456789012345678.9", true),
                    TestCase("012345678901234567890123456789", true),
                    TestCase("-01234567890123456789.0123456789", true),
                    TestCase("-01234567890123456789012345678.9", true),
                    TestCase("-012345678901234567890123456789", true),
                    // Invalid amount formats
                    TestCase("", false),
                    TestCase("-", false),
                    TestCase("notdigits", false),
                    TestCase("ALSONOTDIGITS", false),
                    TestCase("10.", false),
                    TestCase(".99", false),
                    TestCase("-10.", false),
                    TestCase("-.99", false),
                    TestCase("10-", false),
                    TestCase("1-0", false),
                    TestCase("1.0.0", false),
                    TestCase("1/3", false)));

class PaymentsRegionValidatorTest : public testing::TestWithParam<TestCase> {};

TEST_P(PaymentsRegionValidatorTest, IsValidCountryCodeFormat) {
  std::string error_message;
  EXPECT_EQ(GetParam().expected_valid,
            payments::PaymentsValidators::IsValidCountryCodeFormat(
                GetParam().input, &error_message))
      << error_message;
  EXPECT_EQ(GetParam().expected_valid, error_message.empty()) << error_message;

  EXPECT_EQ(GetParam().expected_valid,
            payments::PaymentsValidators::IsValidCountryCodeFormat(
                GetParam().input, nullptr));
}

INSTANTIATE_TEST_CASE_P(CountryCodes,
                        PaymentsRegionValidatorTest,
                        testing::Values(TestCase("US", true),
                                        // Invalid country code formats
                                        TestCase("U1", false),
                                        TestCase("U", false),
                                        TestCase("us", false),
                                        TestCase("USA", false),
                                        TestCase("", false)));

class PaymentsLanguageValidatorTest : public testing::TestWithParam<TestCase> {
};

TEST_P(PaymentsLanguageValidatorTest, IsValidLanguageCodeFormat) {
  std::string error_message;
  EXPECT_EQ(GetParam().expected_valid,
            payments::PaymentsValidators::IsValidLanguageCodeFormat(
                GetParam().input, &error_message))
      << error_message;
  EXPECT_EQ(GetParam().expected_valid, error_message.empty()) << error_message;

  EXPECT_EQ(GetParam().expected_valid,
            payments::PaymentsValidators::IsValidLanguageCodeFormat(
                GetParam().input, nullptr));
}

INSTANTIATE_TEST_CASE_P(LanguageCodes,
                        PaymentsLanguageValidatorTest,
                        testing::Values(TestCase("", true),
                                        TestCase("en", true),
                                        TestCase("eng", true),
                                        // Invalid language code formats
                                        TestCase("e1", false),
                                        TestCase("en1", false),
                                        TestCase("e", false),
                                        TestCase("engl", false),
                                        TestCase("EN", false)));

class PaymentsScriptValidatorTest : public testing::TestWithParam<TestCase> {};

TEST_P(PaymentsScriptValidatorTest, IsValidScriptCodeFormat) {
  std::string error_message;
  EXPECT_EQ(GetParam().expected_valid,
            payments::PaymentsValidators::IsValidScriptCodeFormat(
                GetParam().input, &error_message))
      << error_message;
  EXPECT_EQ(GetParam().expected_valid, error_message.empty()) << error_message;

  EXPECT_EQ(GetParam().expected_valid,
            payments::PaymentsValidators::IsValidScriptCodeFormat(
                GetParam().input, nullptr));
}

INSTANTIATE_TEST_CASE_P(ScriptCodes,
                        PaymentsScriptValidatorTest,
                        testing::Values(TestCase("", true),
                                        TestCase("Latn", true),
                                        // Invalid script code formats
                                        TestCase("Lat1", false),
                                        TestCase("1lat", false),
                                        TestCase("Latin", false),
                                        TestCase("Lat", false),
                                        TestCase("latn", false),
                                        TestCase("LATN", false)));

struct LanguageTagTestCase {
  LanguageTagTestCase(const char* language_tag,
                      const char* expected_language_code,
                      const char* expected_script_code)
      : language_tag(language_tag),
        expected_language_code(expected_language_code),
        expected_script_code(expected_script_code) {}
  ~LanguageTagTestCase() {}

  const char* language_tag;
  const char* expected_language_code;
  const char* expected_script_code;
};

class PaymentsLanguageTagSplitTest
    : public testing::TestWithParam<LanguageTagTestCase> {};

TEST_P(PaymentsLanguageTagSplitTest, Test) {
  std::string language_code;
  std::string script_code;

  PaymentsValidators::SplitLanguageTag(GetParam().language_tag, &language_code,
                                       &script_code);

  EXPECT_EQ(GetParam().expected_language_code, language_code);
  EXPECT_EQ(GetParam().expected_script_code, script_code);
  EXPECT_TRUE(
      PaymentsValidators::IsValidLanguageCodeFormat(language_code, nullptr));
  EXPECT_TRUE(
      PaymentsValidators::IsValidScriptCodeFormat(script_code, nullptr));
}

INSTANTIATE_TEST_CASE_P(
    LanguageTags,
    PaymentsLanguageTagSplitTest,
    testing::Values(LanguageTagTestCase("", "", ""),
                    LanguageTagTestCase("ja", "ja", ""),
                    LanguageTagTestCase("ja-Latn", "ja", "Latn"),
                    LanguageTagTestCase("ja-Latn-JP", "ja", "Latn"),
                    LanguageTagTestCase("ja-JP", "ja", ""),
                    LanguageTagTestCase("Latn", "", ""),
                    LanguageTagTestCase("JP", "", ""),
                    LanguageTagTestCase("en", "en", ""),
                    LanguageTagTestCase("en-Latn", "en", "Latn"),
                    LanguageTagTestCase("en-Latn-US", "en", "Latn"),
                    LanguageTagTestCase("en-US", "en", "")));

}  // namespace
}  // namespace payments
