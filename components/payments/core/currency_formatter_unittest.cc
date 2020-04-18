// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/currency_formatter.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace payments {
namespace {

struct TestCase {
  TestCase(const char* amount,
           const char* currency_code,
           const char* locale_name,
           const std::string& expected_amount,
           const char* expected_currency_code)
      : amount(amount),
        currency_code(currency_code),
        locale_name(locale_name),
        expected_amount(expected_amount),
        expected_currency_code(expected_currency_code) {}
  ~TestCase() {}

  const char* const amount;
  const char* const currency_code;
  const char* const locale_name;
  const std::string expected_amount;
  const char* const expected_currency_code;
};

class PaymentsCurrencyFormatterTest : public testing::TestWithParam<TestCase> {
};

TEST_P(PaymentsCurrencyFormatterTest, IsValidCurrencyFormat) {
  CurrencyFormatter formatter(GetParam().currency_code,
                              GetParam().locale_name);
  base::string16 output_amount = formatter.Format(GetParam().amount);

  // Convenience so the test cases can use regular spaces.
  const base::string16 kSpace(base::ASCIIToUTF16(" "));
  const base::string16 kNonBreakingSpace(base::UTF8ToUTF16("\xC2\xA0"));
  base::string16 converted;
  base::ReplaceChars(base::UTF8ToUTF16(GetParam().expected_amount), kSpace,
                     kNonBreakingSpace, &converted);

  EXPECT_EQ(converted, output_amount)
      << "Failed to convert " << GetParam().amount << " ("
      << GetParam().currency_code << ") in " << GetParam().locale_name;
  EXPECT_EQ(GetParam().expected_currency_code,
            formatter.formatted_currency_code());
}

INSTANTIATE_TEST_CASE_P(
    CurrencyAmounts,
    PaymentsCurrencyFormatterTest,
    testing::Values(
        TestCase("55.00", "USD", "en_US", "$55.00", "USD"),
        TestCase("55.00", "USD", "en_CA", "$55.00", "USD"),
        TestCase("55.00", "USD", "fr_CA", "55,00 $", "USD"),
        TestCase("55.00", "USD", "fr_FR", "55,00 $", "USD"),
        TestCase("1234", "USD", "fr_FR", "1 234,00 $", "USD"),
        // Known oddity about the en_AU formatting in ICU. It will strip the
        // currency symbol in non-AUD currencies. Useful to document in tests.
        // See crbug.com/739812.
        TestCase("55.00", "AUD", "en_AU", "$55.00", "AUD"),
        TestCase("55.00", "USD", "en_AU", "55.00", "USD"),
        TestCase("55.00", "CAD", "en_AU", "55.00", "CAD"),
        TestCase("55.00", "JPY", "en_AU", "55", "JPY"),
        TestCase("55.00", "RUB", "en_AU", "55.00", "RUB"),

        TestCase("55.5", "USD", "en_US", "$55.50", "USD"),
        TestCase("55", "USD", "en_US", "$55.00", "USD"),
        TestCase("123", "USD", "en_US", "$123.00", "USD"),
        TestCase("1234", "USD", "en_US", "$1,234.00", "USD"),
        TestCase("0.1234", "USD", "en_US", "$0.1234", "USD"),

        TestCase("55.00", "EUR", "en_US", "€55.00", "EUR"),
        TestCase("55.00", "EUR", "fr_CA", "55,00 €", "EUR"),
        TestCase("55.00", "EUR", "fr_FR", "55,00 €", "EUR"),

        TestCase("55.00", "CAD", "en_US", "$55.00", "CAD"),
        TestCase("55.00", "CAD", "en_CA", "$55.00", "CAD"),
        TestCase("55.00", "CAD", "fr_CA", "55,00 $", "CAD"),
        TestCase("55.00", "CAD", "fr_FR", "55,00 $", "CAD"),

        TestCase("55.00", "AUD", "en_US", "$55.00", "AUD"),
        TestCase("55.00", "AUD", "en_CA", "$55.00", "AUD"),
        TestCase("55.00", "AUD", "fr_CA", "55,00 $", "AUD"),
        TestCase("55.00", "AUD", "fr_FR", "55,00 $", "AUD"),

        TestCase("55.00", "BRL", "en_US", "R$55.00", "BRL"),
        TestCase("55.00", "BRL", "fr_CA", "55,00 R$", "BRL"),
        TestCase("55.00", "BRL", "pt_BR", "R$ 55,00", "BRL"),

        TestCase("55.00", "RUB", "en_US", "55.00", "RUB"),
        TestCase("55.00", "RUB", "fr_CA", "55,00", "RUB"),
        TestCase("55.00", "RUB", "ru_RU", "55,00 ₽", "RUB"),

        TestCase("55", "JPY", "ja_JP", "￥55", "JPY"),
        TestCase("55.0", "JPY", "ja_JP", "￥55", "JPY"),
        TestCase("55.00", "JPY", "ja_JP", "￥55", "JPY"),
        TestCase("55.12", "JPY", "ja_JP", "￥55.12", "JPY"),
        TestCase("55.49", "JPY", "ja_JP", "￥55.49", "JPY"),
        TestCase("55.50", "JPY", "ja_JP", "￥55.5", "JPY"),
        TestCase("55.9999", "JPY", "ja_JP", "￥55.9999", "JPY"),

        // Unofficial ISO 4217 currency code.
        TestCase("55.00", "BTC", "en_US", "55.00", "BTC"),
        TestCase("-0.0000000001", "BTC", "en_US", "-0.0000000001", "BTC"),
        TestCase("-55.00", "BTC", "fr_FR", "-55,00", "BTC"),

        // Any string of at most 2048 characters can be a valid currency code.
        TestCase("55.00", "", "en_US", "55.00", ""),
        TestCase("55,00", "", "fr_CA", "55,00", ""),
        TestCase("55,00", "", "fr-CA", "55,00", ""),
        TestCase("55.00", "ABCDEF", "en_US", "55.00", "ABCDE\xE2\x80\xA6"),

        // Edge cases.
        TestCase("", "", "", "", ""),
        TestCase("-1", "", "", "- 1.00", ""),
        TestCase("-1.1255", "", "", "- 1.1255", ""),

        // Handles big numbers.
        TestCase(
            "123456789012345678901234567890.123456789012345678901234567890",
            "USD",
            "fr_FR",
            "123 456 789 012 345 678 901 234 567 890,123456789 $",
            "USD")));

}  // namespace
}  // namespace payments
