// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/password_reuse_detector.h"

#include <memory>
#include <string>
#include <vector>

#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/hash_password_manager.h"
#include "components/password_manager/core/browser/password_manager_test_utils.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/safe_browsing/features.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using autofill::PasswordForm;
using base::ASCIIToUTF16;
using testing::_;

namespace password_manager {

namespace {

using StringVector = std::vector<std::string>;

// Constants to make the tests more readable.
const base::Optional<PasswordHashData> NO_GAIA_OR_ENTERPRISE_REUSE =
    base::nullopt;

std::vector<std::pair<std::string, std::string>> GetTestDomainsPasswords() {
  return {
      {"https://accounts.google.com", "saved_password"},
      {"https://facebook.com", "123456789"},
      {"https://a.appspot.com", "abcdefghi"},
      {"https://twitter.com", "short"},
      {"https://example1.com", "secretword"},
      {"https://example2.com", "secretword"},
  };
}

std::unique_ptr<PasswordForm> GetForm(const std::string& domain,
                                      const std::string& password) {
  std::unique_ptr<PasswordForm> form(new PasswordForm);
  form->signon_realm = domain;
  form->password_value = ASCIIToUTF16(password);
  return form;
}

// Convert a vector of pairs of strings ("domain[,domain...]", "password")
// into a vector of PasswordForms.
std::vector<std::unique_ptr<PasswordForm>> GetForms(
    const std::vector<std::pair<std::string, std::string>>& domains_passwords) {
  std::vector<std::unique_ptr<PasswordForm>> result;
  for (const auto& domains_password : domains_passwords) {
    // Some passwords are used on multiple domains.
    for (const auto& domain :
         base::SplitString(domains_password.first, ",", base::TRIM_WHITESPACE,
                           base::SPLIT_WANT_ALL)) {
      result.push_back(GetForm(domain, domains_password.second));
    }
  }
  return result;
}

PasswordStoreChangeList GetChangeList(
    PasswordStoreChange::Type type,
    const std::vector<std::unique_ptr<PasswordForm>>& forms) {
  PasswordStoreChangeList changes;
  for (const auto& form : forms)
    changes.push_back(PasswordStoreChange(type, *form));

  return changes;
}

std::vector<PasswordHashData> PrepareGaiaPasswordData(
    const std::vector<std::string>& passwords) {
  std::vector<PasswordHashData> result;
  for (const auto& password : passwords) {
    PasswordHashData password_hash("username_" + password,
                                   base::ASCIIToUTF16(password),
                                   /*is_gaia_password=*/true);
    result.push_back(password_hash);
  }
  return result;
}

std::vector<PasswordHashData> PrepareEnterprisePasswordData(
    const std::vector<std::string>& passwords) {
  std::vector<PasswordHashData> result;
  for (const auto& password : passwords) {
    PasswordHashData password_hash("enterprise_username_" + password,
                                   base::ASCIIToUTF16(password),
                                   /*is_gaia_password=*/false);
    result.push_back(password_hash);
  }
  return result;
}

void ConfigureEnterprisePasswordProtection(TestingPrefServiceSimple* prefs) {
  prefs->registry()->RegisterStringPref(
      prefs::kPasswordProtectionChangePasswordURL, "");
  prefs->registry()->RegisterListPref(prefs::kPasswordProtectionLoginURLs);
  prefs->SetString(prefs::kPasswordProtectionChangePasswordURL,
                   "https://changepassword.example.com/");
  base::ListValue login_urls;
  login_urls.AppendString("https://login.example.com");
  prefs->Set(prefs::kPasswordProtectionLoginURLs, login_urls);
}

TEST(PasswordReuseDetectorTest, TypingPasswordOnDifferentSite) {
  PasswordReuseDetector reuse_detector;
  reuse_detector.OnGetPasswordStoreResults(GetForms(GetTestDomainsPasswords()));
  MockPasswordReuseDetectorConsumer mockConsumer;

  EXPECT_CALL(mockConsumer, OnReuseFound(_, _, _, _)).Times(0);
  reuse_detector.CheckReuse(ASCIIToUTF16("123saved_passwo"), "https://evil.com",
                            &mockConsumer);
  reuse_detector.CheckReuse(ASCIIToUTF16("123saved_passwor"),
                            "https://evil.com", &mockConsumer);
  testing::Mock::VerifyAndClearExpectations(&mockConsumer);

  EXPECT_CALL(mockConsumer, OnReuseFound(strlen("saved_password"),
                                         Matches(NO_GAIA_OR_ENTERPRISE_REUSE),
                                         StringVector({"google.com"}), 5));
  reuse_detector.CheckReuse(ASCIIToUTF16("123saved_password"),
                            "https://evil.com", &mockConsumer);
  testing::Mock::VerifyAndClearExpectations(&mockConsumer);

  EXPECT_CALL(mockConsumer, OnReuseFound(strlen("saved_password"),
                                         Matches(NO_GAIA_OR_ENTERPRISE_REUSE),
                                         StringVector({"google.com"}), 5));
  reuse_detector.CheckReuse(ASCIIToUTF16("saved_password"), "https://evil.com",
                            &mockConsumer);

  testing::Mock::VerifyAndClearExpectations(&mockConsumer);

  EXPECT_CALL(
      mockConsumer,
      OnReuseFound(strlen("secretword"), Matches(NO_GAIA_OR_ENTERPRISE_REUSE),
                   StringVector({"example1.com", "example2.com"}), 5));
  reuse_detector.CheckReuse(ASCIIToUTF16("abcdsecretword"), "https://evil.com",
                            &mockConsumer);
}

TEST(PasswordReuseDetectorTest, PSLMatchNoReuseEvent) {
  PasswordReuseDetector reuse_detector;
  reuse_detector.OnGetPasswordStoreResults(GetForms(GetTestDomainsPasswords()));
  MockPasswordReuseDetectorConsumer mockConsumer;

  EXPECT_CALL(mockConsumer, OnReuseFound(_, _, _, _)).Times(0);
  reuse_detector.CheckReuse(ASCIIToUTF16("123456789"), "https://m.facebook.com",
                            &mockConsumer);
}

TEST(PasswordReuseDetectorTest, NoPSLMatchReuseEvent) {
  PasswordReuseDetector reuse_detector;
  reuse_detector.OnGetPasswordStoreResults(GetForms(GetTestDomainsPasswords()));
  MockPasswordReuseDetectorConsumer mockConsumer;

  // a.appspot.com and b.appspot.com are not PSL matches. So reuse event should
  // be raised.
  EXPECT_CALL(mockConsumer, OnReuseFound(strlen("abcdefghi"),
                                         Matches(NO_GAIA_OR_ENTERPRISE_REUSE),
                                         StringVector({"a.appspot.com"}), 5));
  reuse_detector.CheckReuse(ASCIIToUTF16("abcdefghi"), "https://b.appspot.com",
                            &mockConsumer);
}

TEST(PasswordReuseDetectorTest, TooShortPasswordNoReuseEvent) {
  PasswordReuseDetector reuse_detector;
  reuse_detector.OnGetPasswordStoreResults(GetForms(GetTestDomainsPasswords()));
  MockPasswordReuseDetectorConsumer mockConsumer;

  EXPECT_CALL(mockConsumer, OnReuseFound(_, _, _, _)).Times(0);
  reuse_detector.CheckReuse(ASCIIToUTF16("short"), "evil.com", &mockConsumer);
}

TEST(PasswordReuseDetectorTest, PasswordNotInputSuffixNoReuseEvent) {
  PasswordReuseDetector reuse_detector;
  reuse_detector.OnGetPasswordStoreResults(GetForms(GetTestDomainsPasswords()));
  MockPasswordReuseDetectorConsumer mockConsumer;

  EXPECT_CALL(mockConsumer, OnReuseFound(_, _, _, _)).Times(0);
  reuse_detector.CheckReuse(ASCIIToUTF16("password123"), "https://evil.com",
                            &mockConsumer);
  reuse_detector.CheckReuse(ASCIIToUTF16("123password456"), "https://evil.com",
                            &mockConsumer);
}

TEST(PasswordReuseDetectorTest, OnLoginsChanged) {
  for (PasswordStoreChange::Type type :
       {PasswordStoreChange::ADD, PasswordStoreChange::UPDATE,
        PasswordStoreChange::REMOVE}) {
    PasswordReuseDetector reuse_detector;
    PasswordStoreChangeList changes =
        GetChangeList(type, GetForms(GetTestDomainsPasswords()));
    reuse_detector.OnLoginsChanged(changes);
    MockPasswordReuseDetectorConsumer mockConsumer;

    if (type == PasswordStoreChange::REMOVE) {
      EXPECT_CALL(mockConsumer, OnReuseFound(_, _, _, _)).Times(0);
    } else {
      EXPECT_CALL(mockConsumer,
                  OnReuseFound(strlen("saved_password"),
                               Matches(NO_GAIA_OR_ENTERPRISE_REUSE),
                               StringVector({"google.com"}), 5));
    }
    reuse_detector.CheckReuse(ASCIIToUTF16("123saved_password"),
                              "https://evil.com", &mockConsumer);
  }
}

TEST(PasswordReuseDetectorTest, MatchMultiplePasswords) {
  // These all have different length passwods so we can check the
  // returned length.
  const std::vector<std::pair<std::string, std::string>> domain_passwords = {
      {"https://a.com, https://all.com", "34567890"},
      {"https://b.com, https://b2.com, https://all.com", "01234567890"},
      {"https://c.com, https://all.com", "1234567890"},
      {"https://d.com", "123456789"},
  };

  PasswordReuseDetector reuse_detector;
  reuse_detector.OnGetPasswordStoreResults(GetForms(domain_passwords));

  MockPasswordReuseDetectorConsumer mockConsumer;

  EXPECT_CALL(
      mockConsumer,
      OnReuseFound(
          strlen("01234567890"), Matches(NO_GAIA_OR_ENTERPRISE_REUSE),
          StringVector({"a.com", "all.com", "b.com", "b2.com", "c.com"}), 8));
  reuse_detector.CheckReuse(ASCIIToUTF16("abcd01234567890"), "https://evil.com",
                            &mockConsumer);
  testing::Mock::VerifyAndClearExpectations(&mockConsumer);

  EXPECT_CALL(
      mockConsumer,
      OnReuseFound(strlen("1234567890"), Matches(NO_GAIA_OR_ENTERPRISE_REUSE),
                   StringVector({"a.com", "all.com", "c.com"}), 8));
  reuse_detector.CheckReuse(ASCIIToUTF16("1234567890"), "https://evil.com",
                            &mockConsumer);
  testing::Mock::VerifyAndClearExpectations(&mockConsumer);

  EXPECT_CALL(mockConsumer, OnReuseFound(_, _, _, _)).Times(0);
  reuse_detector.CheckReuse(ASCIIToUTF16("4567890"), "https://evil.com",
                            &mockConsumer);
}

TEST(PasswordReuseDetectorTest, GaiaPasswordNoReuse) {
  PasswordReuseDetector reuse_detector;
  reuse_detector.OnGetPasswordStoreResults(GetForms(GetTestDomainsPasswords()));
  MockPasswordReuseDetectorConsumer mockConsumer;

  reuse_detector.UseGaiaPasswordHash(
      PrepareGaiaPasswordData({"gaia_pw1", "gaia_pw2"}));

  EXPECT_CALL(mockConsumer, OnReuseFound(_, _, _, _)).Times(0);
  // Typing gaia password on https://accounts.google.com is OK.
  reuse_detector.CheckReuse(ASCIIToUTF16("gaia_pw1"),
                            "https://accounts.google.com", &mockConsumer);
  reuse_detector.CheckReuse(ASCIIToUTF16("gaia_pw2"),
                            "https://accounts.google.com", &mockConsumer);
  // Only suffixes are verifed.
  reuse_detector.CheckReuse(ASCIIToUTF16("sync_password123"),
                            "https://evil.com", &mockConsumer);
  reuse_detector.CheckReuse(ASCIIToUTF16("other_password"), "https://evil.com",
                            &mockConsumer);
}

TEST(PasswordReuseDetectorTest, GaiaPasswordReuseFound) {
  PasswordReuseDetector reuse_detector;
  reuse_detector.OnGetPasswordStoreResults(GetForms(GetTestDomainsPasswords()));
  MockPasswordReuseDetectorConsumer mockConsumer;

  std::vector<PasswordHashData> gaia_password_hashes =
      PrepareGaiaPasswordData({"gaia_pw1", "gaia_pw2"});
  base::Optional<PasswordHashData> expected_reused_password_hash(
      gaia_password_hashes[0]);
  reuse_detector.UseGaiaPasswordHash(gaia_password_hashes);

  EXPECT_CALL(mockConsumer, OnReuseFound(strlen("gaia_pw1"),
                                         Matches(expected_reused_password_hash),
                                         StringVector(), 5));

  reuse_detector.CheckReuse(ASCIIToUTF16("gaia_pw1"),
                            "https://phishing.example.com", &mockConsumer);
}

TEST(PasswordReuseDetectorTest, EnterprisePasswordNoReuse) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      safe_browsing::kEnterprisePasswordProtectionV1);
  TestingPrefServiceSimple prefs;
  ConfigureEnterprisePasswordProtection(&prefs);

  PasswordReuseDetector reuse_detector;
  reuse_detector.SetPrefs(&prefs);
  reuse_detector.OnGetPasswordStoreResults(GetForms(GetTestDomainsPasswords()));
  MockPasswordReuseDetectorConsumer mockConsumer;

  std::vector<PasswordHashData> enterprise_password_hashes =
      PrepareEnterprisePasswordData({"enterprise_pw1", "enterprise_pw2"});
  base::Optional<PasswordHashData> expected_reused_password_hash(
      enterprise_password_hashes[1]);
  reuse_detector.UseNonGaiaEnterprisePasswordHash(enterprise_password_hashes);

  EXPECT_CALL(mockConsumer, OnReuseFound(_, _, _, _)).Times(0);
  // Typing enterprise password on change password page is OK.
  reuse_detector.CheckReuse(ASCIIToUTF16("enterprise_pw1"),
                            "https://changepassword.example.com/",
                            &mockConsumer);
  reuse_detector.CheckReuse(ASCIIToUTF16("enterprise_pw2"),
                            "https://changepassword.example.com/",
                            &mockConsumer);

  // Suffix match is not reuse.
  reuse_detector.CheckReuse(ASCIIToUTF16("enterprise"), "https://evil.com",
                            &mockConsumer);
  reuse_detector.CheckReuse(ASCIIToUTF16("other_password"), "https://evil.com",
                            &mockConsumer);
}

TEST(PasswordReuseDetectorTest, EnterprisePasswordReuseFound) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      safe_browsing::kEnterprisePasswordProtectionV1);
  TestingPrefServiceSimple prefs;
  ConfigureEnterprisePasswordProtection(&prefs);

  PasswordReuseDetector reuse_detector;
  reuse_detector.SetPrefs(&prefs);
  reuse_detector.OnGetPasswordStoreResults(GetForms(GetTestDomainsPasswords()));
  MockPasswordReuseDetectorConsumer mockConsumer;

  std::vector<PasswordHashData> enterprise_password_hashes =
      PrepareEnterprisePasswordData({"enterprise_pw1", "enterprise_pw2"});
  base::Optional<PasswordHashData> expected_reused_password_hash(
      enterprise_password_hashes[1]);
  reuse_detector.UseNonGaiaEnterprisePasswordHash(enterprise_password_hashes);

  EXPECT_CALL(mockConsumer, OnReuseFound(strlen("enterprise_pw2"),
                                         Matches(expected_reused_password_hash),
                                         StringVector(), 5));
  reuse_detector.CheckReuse(ASCIIToUTF16("enterprise_pw2"),
                            "https://phishing.com", &mockConsumer);
}

TEST(PasswordReuseDetectorTest, MatchGaiaAndMultipleSavedPasswords) {
  const std::vector<std::pair<std::string, std::string>> domain_passwords = {
      {"https://a.com", "34567890"}, {"https://b.com", "01234567890"},
  };
  PasswordReuseDetector reuse_detector;
  reuse_detector.OnGetPasswordStoreResults(GetForms(domain_passwords));

  std::string gaia_password = "1234567890";
  std::vector<PasswordHashData> gaia_password_hashes =
      PrepareGaiaPasswordData({gaia_password});
  ASSERT_EQ(1u, gaia_password_hashes.size());
  base::Optional<PasswordHashData> expected_reused_password_hash(
      gaia_password_hashes[0]);
  reuse_detector.UseGaiaPasswordHash(gaia_password_hashes);

  MockPasswordReuseDetectorConsumer mockConsumer;

  EXPECT_CALL(mockConsumer, OnReuseFound(strlen("01234567890"),
                                         Matches(expected_reused_password_hash),
                                         StringVector({"a.com", "b.com"}), 2));
  reuse_detector.CheckReuse(ASCIIToUTF16("abcd01234567890"), "https://evil.com",
                            &mockConsumer);
  testing::Mock::VerifyAndClearExpectations(&mockConsumer);

  EXPECT_CALL(mockConsumer, OnReuseFound(strlen("1234567890"),
                                         Matches(expected_reused_password_hash),
                                         StringVector(), 2));
  reuse_detector.CheckReuse(ASCIIToUTF16("xyz1234567890"), "https://evil.com",
                            &mockConsumer);
  testing::Mock::VerifyAndClearExpectations(&mockConsumer);

  EXPECT_CALL(mockConsumer, OnReuseFound(_, _, _, _)).Times(0);
  reuse_detector.CheckReuse(ASCIIToUTF16("4567890"), "https://evil.com",
                            &mockConsumer);
}

TEST(PasswordReuseDetectorTest, MatchSavedPasswordButNotGaiaPassword) {
  PasswordReuseDetector reuse_detector;
  reuse_detector.OnGetPasswordStoreResults(GetForms(GetTestDomainsPasswords()));
  MockPasswordReuseDetectorConsumer mockConsumer;

  std::string gaia_password = "gaia_password";
  reuse_detector.UseGaiaPasswordHash(PrepareGaiaPasswordData({gaia_password}));

  EXPECT_CALL(mockConsumer, OnReuseFound(strlen("saved_password"),
                                         Matches(NO_GAIA_OR_ENTERPRISE_REUSE),
                                         StringVector({"google.com"}), 5));
  reuse_detector.CheckReuse(ASCIIToUTF16("saved_password"), "https://evil.com",
                            &mockConsumer);
}

TEST(PasswordReuseDetectorTest, MatchEnterpriseAndMultipleSavedPasswords) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      safe_browsing::kEnterprisePasswordProtectionV1);
  TestingPrefServiceSimple prefs;
  ConfigureEnterprisePasswordProtection(&prefs);

  const std::vector<std::pair<std::string, std::string>> domain_passwords = {
      {"https://a.com", "34567890"}, {"https://b.com", "01234567890"},
  };
  PasswordReuseDetector reuse_detector;
  reuse_detector.SetPrefs(&prefs);
  reuse_detector.OnGetPasswordStoreResults(GetForms(domain_passwords));

  std::string enterprise_password = "1234567890";
  std::vector<PasswordHashData> enterprise_password_hashes =
      PrepareEnterprisePasswordData({enterprise_password});
  ASSERT_EQ(1u, enterprise_password_hashes.size());
  base::Optional<PasswordHashData> expected_reused_password_hash(
      enterprise_password_hashes[0]);
  reuse_detector.UseNonGaiaEnterprisePasswordHash(enterprise_password_hashes);

  MockPasswordReuseDetectorConsumer mockConsumer;

  EXPECT_CALL(mockConsumer, OnReuseFound(strlen("01234567890"),
                                         Matches(expected_reused_password_hash),
                                         StringVector({"a.com", "b.com"}), 2));
  reuse_detector.CheckReuse(ASCIIToUTF16("abcd01234567890"), "https://evil.com",
                            &mockConsumer);
  testing::Mock::VerifyAndClearExpectations(&mockConsumer);

  EXPECT_CALL(mockConsumer, OnReuseFound(strlen("1234567890"),
                                         Matches(expected_reused_password_hash),
                                         StringVector(), 2));
  reuse_detector.CheckReuse(ASCIIToUTF16("xyz1234567890"), "https://evil.com",
                            &mockConsumer);
  testing::Mock::VerifyAndClearExpectations(&mockConsumer);

  EXPECT_CALL(mockConsumer, OnReuseFound(_, _, _, _)).Times(0);
  reuse_detector.CheckReuse(ASCIIToUTF16("4567890"), "https://evil.com",
                            &mockConsumer);
}

TEST(PasswordReuseDetectorTest, MatchSavedPasswordButNotEnterprisePassword) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      safe_browsing::kEnterprisePasswordProtectionV1);
  TestingPrefServiceSimple prefs;
  ConfigureEnterprisePasswordProtection(&prefs);

  PasswordReuseDetector reuse_detector;
  reuse_detector.SetPrefs(&prefs);
  reuse_detector.OnGetPasswordStoreResults(GetForms(GetTestDomainsPasswords()));
  MockPasswordReuseDetectorConsumer mockConsumer;

  std::string enterprise_password = "enterprise_password";
  reuse_detector.UseNonGaiaEnterprisePasswordHash(
      PrepareEnterprisePasswordData({enterprise_password}));

  EXPECT_CALL(mockConsumer, OnReuseFound(strlen("saved_password"),
                                         Matches(NO_GAIA_OR_ENTERPRISE_REUSE),
                                         StringVector({"google.com"}), 5));
  reuse_detector.CheckReuse(ASCIIToUTF16("saved_password"), "https://evil.com",
                            &mockConsumer);
}

TEST(PasswordReuseDetectorTest, MatchGaiaEnterpriseAndSavedPassword) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      safe_browsing::kEnterprisePasswordProtectionV1);
  TestingPrefServiceSimple prefs;
  ConfigureEnterprisePasswordProtection(&prefs);

  const std::vector<std::pair<std::string, std::string>> domain_passwords = {
      {"https://a.com", "34567890"}, {"https://b.com", "01234567890"},
  };
  PasswordReuseDetector reuse_detector;
  reuse_detector.SetPrefs(&prefs);
  reuse_detector.OnGetPasswordStoreResults(GetForms(domain_passwords));

  std::string gaia_password = "123456789";
  reuse_detector.UseGaiaPasswordHash(PrepareGaiaPasswordData({gaia_password}));

  std::string enterprise_password = "1234567890";
  std::vector<PasswordHashData> enterprise_password_hashes =
      PrepareEnterprisePasswordData({enterprise_password});
  ASSERT_EQ(1u, enterprise_password_hashes.size());
  base::Optional<PasswordHashData> expected_reused_password_hash(
      enterprise_password_hashes[0]);
  reuse_detector.UseNonGaiaEnterprisePasswordHash(enterprise_password_hashes);

  MockPasswordReuseDetectorConsumer mockConsumer;

  EXPECT_CALL(mockConsumer, OnReuseFound(strlen("01234567890"),
                                         Matches(expected_reused_password_hash),
                                         StringVector({"a.com", "b.com"}), 2));
  reuse_detector.CheckReuse(ASCIIToUTF16("abcd01234567890"), "https://evil.com",
                            &mockConsumer);
  testing::Mock::VerifyAndClearExpectations(&mockConsumer);

  EXPECT_CALL(mockConsumer, OnReuseFound(strlen("1234567890"),
                                         Matches(expected_reused_password_hash),
                                         StringVector(), 2));
  reuse_detector.CheckReuse(ASCIIToUTF16("xyz1234567890"), "https://evil.com",
                            &mockConsumer);
  testing::Mock::VerifyAndClearExpectations(&mockConsumer);

  EXPECT_CALL(mockConsumer, OnReuseFound(_, _, _, _)).Times(0);
  reuse_detector.CheckReuse(ASCIIToUTF16("4567890"), "https://evil.com",
                            &mockConsumer);
}

TEST(PasswordReuseDetectorTest, ClearGaiaPasswordHash) {
  PasswordReuseDetector reuse_detector;
  reuse_detector.OnGetPasswordStoreResults(GetForms(GetTestDomainsPasswords()));
  MockPasswordReuseDetectorConsumer mockConsumer;

  reuse_detector.UseGaiaPasswordHash(
      PrepareGaiaPasswordData({"gaia_pw1", "gaia_pw12"}));
  EXPECT_CALL(mockConsumer, OnReuseFound(strlen("gaia_pw1"), _, _, _));
  reuse_detector.CheckReuse(ASCIIToUTF16("gaia_pw1"), "https://evil.com",
                            &mockConsumer);
  testing::Mock::VerifyAndClearExpectations(&mockConsumer);

  EXPECT_CALL(mockConsumer, OnReuseFound(strlen("gaia_pw12"), _, _, _));
  reuse_detector.CheckReuse(ASCIIToUTF16("gaia_pw12"), "https://evil.com",
                            &mockConsumer);
  testing::Mock::VerifyAndClearExpectations(&mockConsumer);

  reuse_detector.ClearGaiaPasswordHash("username_gaia_pw1");
  EXPECT_CALL(mockConsumer, OnReuseFound(_, _, _, _)).Times(0);
  reuse_detector.CheckReuse(ASCIIToUTF16("gaia_pw1"), "https://evil.com",
                            &mockConsumer);
  testing::Mock::VerifyAndClearExpectations(&mockConsumer);

  reuse_detector.ClearAllGaiaPasswordHash();
  EXPECT_CALL(mockConsumer, OnReuseFound(_, _, _, _)).Times(0);
  reuse_detector.CheckReuse(ASCIIToUTF16("gaia_pw12"), "https://evil.com",
                            &mockConsumer);
}

}  // namespace

}  // namespace password_manager
