// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/strings_util.h"

#include <string>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace payments {
namespace {

using CardType = ::autofill::CreditCard::CardType;

constexpr CardType CREDIT = ::autofill::CreditCard::CARD_TYPE_CREDIT;
constexpr CardType DEBIT = ::autofill::CreditCard::CARD_TYPE_DEBIT;
constexpr CardType PREPAID = ::autofill::CreditCard::CARD_TYPE_PREPAID;
constexpr CardType UNKNOWN = ::autofill::CreditCard::CARD_TYPE_UNKNOWN;

}  // namespace

#if defined(OS_MACOSX)
TEST(StringsUtilTest, GetAcceptedCardTypesText) {
  static const struct {
    std::vector<CardType> card_types;
    const char* const expected_text;
  } kTestCases[] = {
      {std::vector<CardType>(), "Accepted Cards"},
      {{UNKNOWN}, "Accepted Cards"},
      {{CREDIT}, "Accepted Credit Cards"},
      {{DEBIT}, "Accepted Debit Cards"},
      {{PREPAID}, "Accepted Prepaid Cards"},
      {{CREDIT, DEBIT}, "Accepted Credit and Debit Cards"},
      {{CREDIT, PREPAID}, "Accepted Credit and Prepaid Cards"},
      {{DEBIT, PREPAID}, "Accepted Debit and Prepaid Cards"},
      {{CREDIT, DEBIT, PREPAID}, "Accepted Cards"},
  };
  for (size_t i = 0; i < arraysize(kTestCases); ++i) {
    EXPECT_EQ(
        base::UTF8ToUTF16(kTestCases[i].expected_text),
        GetAcceptedCardTypesText(std::set<CardType>(
            kTestCases[i].card_types.begin(), kTestCases[i].card_types.end())));
  }
}
#else
TEST(StringsUtilTest, GetAcceptedCardTypesText) {
  static const struct {
    std::vector<CardType> card_types;
    const char* const expected_text;
  } kTestCases[] = {
      {std::vector<CardType>(), "Accepted cards"},
      {{UNKNOWN}, "Accepted cards"},
      {{CREDIT}, "Accepted credit cards"},
      {{DEBIT}, "Accepted debit cards"},
      {{PREPAID}, "Accepted prepaid cards"},
      {{CREDIT, DEBIT}, "Accepted credit and debit cards"},
      {{CREDIT, PREPAID}, "Accepted credit and prepaid cards"},
      {{DEBIT, PREPAID}, "Accepted debit and prepaid cards"},
      {{CREDIT, DEBIT, PREPAID}, "Accepted cards"},
  };
  for (size_t i = 0; i < arraysize(kTestCases); ++i) {
    EXPECT_EQ(
        base::UTF8ToUTF16(kTestCases[i].expected_text),
        GetAcceptedCardTypesText(std::set<CardType>(
            kTestCases[i].card_types.begin(), kTestCases[i].card_types.end())));
  }
}
#endif

TEST(StringsUtilTest, GetCardTypesAreAcceptedText) {
  static const struct {
    std::vector<CardType> card_types;
    const char* const expected_text;
  } kTestCases[] = {
      {std::vector<CardType>(), ""},
      {{UNKNOWN}, ""},
      {{CREDIT}, "Credit cards are accepted."},
      {{DEBIT}, "Debit cards are accepted."},
      {{PREPAID}, "Prepaid cards are accepted."},
      {{CREDIT, DEBIT}, "Credit and debit cards are accepted."},
      {{CREDIT, PREPAID}, "Credit and prepaid cards are accepted."},
      {{DEBIT, PREPAID}, "Debit and prepaid cards are accepted."},
      {{CREDIT, DEBIT, PREPAID}, ""},
  };
  for (size_t i = 0; i < arraysize(kTestCases); ++i) {
    EXPECT_EQ(
        base::UTF8ToUTF16(kTestCases[i].expected_text),
        GetCardTypesAreAcceptedText(std::set<CardType>(
            kTestCases[i].card_types.begin(), kTestCases[i].card_types.end())));
  }
}

}  // namespace payments
