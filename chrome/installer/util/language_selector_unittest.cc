// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "base/macros.h"
#include "chrome/installer/util/language_selector.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const wchar_t* const kExactMatchCandidates[] = {
  L"am", L"ar", L"bg", L"bn", L"ca", L"cs", L"da", L"de", L"el", L"en-gb",
  L"en-us", L"es", L"es-419", L"et", L"fa", L"fi", L"fil", L"fr", L"gu", L"hi",
  L"hr", L"hu", L"id", L"it", L"iw", L"ja", L"kn", L"ko", L"lt", L"lv", L"ml",
  L"mr", L"nl", L"no", L"pl", L"pt-br", L"pt-pt", L"ro", L"ru", L"sk", L"sl",
  L"sr", L"sv", L"sw", L"ta", L"te", L"th", L"tr", L"uk", L"vi", L"zh-cn",
  L"zh-tw"
};

const wchar_t* const kAliasMatchCandidates[] = {
  L"he", L"nb", L"tl", L"zh-chs",  L"zh-cht", L"zh-hans", L"zh-hant", L"zh-hk",
  L"zh-mo"
};

const wchar_t* const kWildcardMatchCandidates[] = {
  L"en-AU",
  L"es-CO", L"pt-AB", L"zh-SG"
};

}  // namespace

// Test that a language is selected from the system.
TEST(LanguageSelectorTest, DefaultSelection) {
  installer::LanguageSelector instance;
  EXPECT_FALSE(instance.matched_candidate().empty());
}

// Test some hypothetical candidate sets.
TEST(LanguageSelectorTest, AssortedSelections) {
  {
    std::wstring candidates[] = {
      L"fr-BE", L"fr", L"en"
    };
    installer::LanguageSelector instance(
        std::vector<std::wstring>(&candidates[0],
                                  &candidates[arraysize(candidates)]));
    // Expect the exact match to win.
    EXPECT_EQ(L"fr", instance.matched_candidate());
  }
  {
    std::wstring candidates[] = {
      L"xx-YY", L"cc-Ssss-RR"
    };
    installer::LanguageSelector instance(
      std::vector<std::wstring>(&candidates[0],
      &candidates[arraysize(candidates)]));
    // Expect the fallback to win.
    EXPECT_EQ(L"en-us", instance.matched_candidate());
  }
  {
    std::wstring candidates[] = {
      L"zh-SG", L"en-GB"
    };
    installer::LanguageSelector instance(
      std::vector<std::wstring>(&candidates[0],
      &candidates[arraysize(candidates)]));
    // Expect the alias match to win.
    EXPECT_EQ(L"zh-SG", instance.matched_candidate());
  }
}

// A fixture for testing sets of single-candidate selections.
class LanguageSelectorMatchCandidateTest
    : public ::testing::TestWithParam<const wchar_t*> {
};

TEST_P(LanguageSelectorMatchCandidateTest, TestMatchCandidate) {
  installer::LanguageSelector instance(
    std::vector<std::wstring>(1, std::wstring(GetParam())));
  EXPECT_EQ(GetParam(), instance.matched_candidate());
}

// Test that all existing translations can be found by exact match.
INSTANTIATE_TEST_CASE_P(
    TestExactMatches,
    LanguageSelectorMatchCandidateTest,
    ::testing::ValuesIn(
        &kExactMatchCandidates[0],
        &kExactMatchCandidates[arraysize(kExactMatchCandidates)]));

// Test the alias matches.
INSTANTIATE_TEST_CASE_P(
    TestAliasMatches,
    LanguageSelectorMatchCandidateTest,
    ::testing::ValuesIn(
        &kAliasMatchCandidates[0],
        &kAliasMatchCandidates[arraysize(kAliasMatchCandidates)]));

// Test a few wildcard matches.
INSTANTIATE_TEST_CASE_P(
    TestWildcardMatches,
    LanguageSelectorMatchCandidateTest,
    ::testing::ValuesIn(
        &kWildcardMatchCandidates[0],
        &kWildcardMatchCandidates[arraysize(kWildcardMatchCandidates)]));

// A fixture for testing aliases that match to an expected translation.  The
// first member of the tuple is the expected translation, the second is a
// candidate that should be aliased to the expectation.
class LanguageSelectorAliasTest
    : public ::testing::TestWithParam<
          std::tuple<const wchar_t*, const wchar_t*>> {};

// Test that the candidate language maps to the aliased translation.
TEST_P(LanguageSelectorAliasTest, AliasesMatch) {
  installer::LanguageSelector instance(
      std::vector<std::wstring>(1, std::get<1>(GetParam())));
  EXPECT_EQ(std::get<0>(GetParam()), instance.selected_translation());
}

INSTANTIATE_TEST_CASE_P(
    EnGbAliases,
    LanguageSelectorAliasTest,
    ::testing::Combine(
        ::testing::Values(L"en-gb"),
        ::testing::Values(L"en-au", L"en-ca", L"en-nz", L"en-za")));

INSTANTIATE_TEST_CASE_P(
    IwAliases,
    LanguageSelectorAliasTest,
    ::testing::Combine(
        ::testing::Values(L"iw"),
        ::testing::Values(L"he")));

INSTANTIATE_TEST_CASE_P(
    NoAliases,
    LanguageSelectorAliasTest,
    ::testing::Combine(
        ::testing::Values(L"no"),
        ::testing::Values(L"nb")));

INSTANTIATE_TEST_CASE_P(
    FilAliases,
    LanguageSelectorAliasTest,
    ::testing::Combine(
        ::testing::Values(L"fil"),
        ::testing::Values(L"tl")));

INSTANTIATE_TEST_CASE_P(
    ZhCnAliases,
    LanguageSelectorAliasTest,
    ::testing::Combine(
        ::testing::Values(L"zh-cn"),
        ::testing::Values(L"zh-chs", L"zh-hans", L"zh-sg")));

INSTANTIATE_TEST_CASE_P(
    ZhTwAliases,
    LanguageSelectorAliasTest,
    ::testing::Combine(
        ::testing::Values(L"zh-tw"),
        ::testing::Values(L"zh-cht", L"zh-hant", L"zh-hk", L"zh-mo")));

// Test that we can get the name of the default language.
TEST(LanguageSelectorTest, DefaultLanguageName) {
  installer::LanguageSelector instance;
  EXPECT_FALSE(instance.GetLanguageName(instance.offset()).empty());
}

