// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/sync/browser/sync_credentials_filter.h"

#include <stddef.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/user_action_tester.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/fake_form_fetcher.h"
#include "components/password_manager/core/browser/mock_password_store.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "components/password_manager/core/browser/password_manager_test_utils.h"
#include "components/password_manager/core/browser/stub_form_saver.h"
#include "components/password_manager/core/browser/stub_password_manager_client.h"
#include "components/password_manager/core/browser/stub_password_manager_driver.h"
#include "components/password_manager/core/common/password_manager_features.h"
#include "components/password_manager/sync/browser/sync_username_test_base.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using autofill::PasswordForm;

namespace password_manager {

namespace {

const char kFilledAndLoginActionName[] =
    "PasswordManager_SyncCredentialFilledAndLoginSuccessfull";

class FakePasswordManagerClient : public StubPasswordManagerClient {
 public:
  FakePasswordManagerClient()
      : password_store_(new testing::NiceMock<MockPasswordStore>) {}

  ~FakePasswordManagerClient() override {
    password_store_->ShutdownOnUIThread();
  }

  // PasswordManagerClient:
  const GURL& GetLastCommittedEntryURL() const override {
    return last_committed_entry_url_;
  }
  MockPasswordStore* GetPasswordStore() const override {
    return password_store_.get();
  }

  void set_last_committed_entry_url(const char* url_spec) {
    last_committed_entry_url_ = GURL(url_spec);
  }

 private:
  base::MessageLoop message_loop_;  // For |password_store_|.
  GURL last_committed_entry_url_;
  scoped_refptr<testing::NiceMock<MockPasswordStore>> password_store_;

  DISALLOW_COPY_AND_ASSIGN(FakePasswordManagerClient);
};

bool IsFormFiltered(const CredentialsFilter* filter, const PasswordForm& form) {
  std::vector<std::unique_ptr<PasswordForm>> vector;
  vector.push_back(std::make_unique<PasswordForm>(form));
  vector = filter->FilterResults(std::move(vector));
  return vector.empty();
}

}  // namespace

class CredentialsFilterTest : public SyncUsernameTestBase {
 public:
  struct TestCase {
    enum { SYNCING_PASSWORDS, NOT_SYNCING_PASSWORDS } password_sync;
    PasswordForm form;
    std::string fake_sync_username;
    const char* const last_committed_entry_url;
    enum { FORM_FILTERED, FORM_NOT_FILTERED } is_form_filtered;
    enum { NO_HISTOGRAM, HISTOGRAM_REPORTED } histogram_reported;
  };

  // Flag for creating a PasswordFormManager, deciding its IsNewLogin() value.
  enum class LoginState { NEW, EXISTING };

  CredentialsFilterTest()
      : password_manager_(&client_),
        pending_(SimpleGaiaForm("user@gmail.com")),
        form_manager_(&password_manager_,
                      &client_,
                      driver_.AsWeakPtr(),
                      pending_,
                      std::make_unique<StubFormSaver>(),
                      &fetcher_),
        filter_(&client_,
                base::Bind(&SyncUsernameTestBase::sync_service,
                           base::Unretained(this)),
                base::Bind(&SyncUsernameTestBase::signin_manager,
                           base::Unretained(this))) {
    form_manager_.Init(nullptr);
    fetcher_.Fetch();
  }

  void CheckFilterResultsTestCase(const TestCase& test_case) {
    SetSyncingPasswords(test_case.password_sync == TestCase::SYNCING_PASSWORDS);
    FakeSigninAs(test_case.fake_sync_username);
    client_.set_last_committed_entry_url(test_case.last_committed_entry_url);
    base::HistogramTester tester;
    const bool expected_is_form_filtered =
        test_case.is_form_filtered == TestCase::FORM_FILTERED;
    EXPECT_EQ(expected_is_form_filtered,
              IsFormFiltered(&filter_, test_case.form));
    if (test_case.histogram_reported == TestCase::HISTOGRAM_REPORTED) {
      tester.ExpectUniqueSample("PasswordManager.SyncCredentialFiltered",
                                expected_is_form_filtered, 1);
    } else {
      tester.ExpectTotalCount("PasswordManager.SyncCredentialFiltered", 0);
    }
    FakeSignout();
  }

  // Makes |form_manager_| provisionally save |pending_|. Depending on
  // |login_state| being NEW or EXISTING, prepares |form_manager_| in a state in
  // which |pending_| looks like a new or existing credential, respectively.
  void SavePending(LoginState login_state) {
    std::vector<const PasswordForm*> matches;
    if (login_state == LoginState::EXISTING) {
      matches.push_back(&pending_);
    }
    fetcher_.SetNonFederated(matches, 0u);

    form_manager_.ProvisionallySave(pending_);
  }

 protected:
  FakePasswordManagerClient client_;
  PasswordManager password_manager_;
  StubPasswordManagerDriver driver_;
  PasswordForm pending_;
  FakeFormFetcher fetcher_;
  PasswordFormManager form_manager_;

  SyncCredentialsFilter filter_;
};

TEST_F(CredentialsFilterTest, FilterResults_AllowAll) {
  // By default, sync username is not filtered at all.
  const TestCase kTestCases[] = {
      // Reauth URL, not sync username.
      {TestCase::SYNCING_PASSWORDS, SimpleGaiaForm("user@example.org"),
       "another_user@example.org",
       "https://accounts.google.com/login?rart=123&continue=blah",
       TestCase::FORM_NOT_FILTERED, TestCase::NO_HISTOGRAM},

      // Reauth URL, sync username.
      {TestCase::SYNCING_PASSWORDS, SimpleGaiaForm("user@example.org"),
       "user@example.org",
       "https://accounts.google.com/login?rart=123&continue=blah",
       TestCase::FORM_NOT_FILTERED, TestCase::NO_HISTOGRAM},

      // Slightly invalid reauth URL, sync username.
      {TestCase::SYNCING_PASSWORDS, SimpleGaiaForm("user@example.org"),
       "user@example.org",
       "https://accounts.google.com/addlogin?rart",  // Missing rart value.
       TestCase::FORM_NOT_FILTERED, TestCase::NO_HISTOGRAM},

      // Non-reauth URL, sync username.
      {TestCase::SYNCING_PASSWORDS, SimpleGaiaForm("user@example.org"),
       "user@example.org", "https://accounts.google.com/login?param=123",
       TestCase::FORM_NOT_FILTERED, TestCase::NO_HISTOGRAM},

      // Non-GAIA "reauth" URL, sync username.
      {TestCase::SYNCING_PASSWORDS, SimpleNonGaiaForm("user@example.org"),
       "user@example.org", "https://site.com/login?rart=678",
       TestCase::FORM_NOT_FILTERED, TestCase::NO_HISTOGRAM},
  };

  for (size_t i = 0; i < arraysize(kTestCases); ++i) {
    SCOPED_TRACE(testing::Message() << "i=" << i);
    CheckFilterResultsTestCase(kTestCases[i]);
  }
}

TEST_F(CredentialsFilterTest, FilterResults_DisallowSyncOnReauth) {
  // Only 'ProtectSyncCredentialOnReauth' feature is kept enabled, fill the
  // sync credential everywhere but on reauth.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitFromCommandLine(
      features::kProtectSyncCredentialOnReauth.name,
      features::kProtectSyncCredential.name);

  const TestCase kTestCases[] = {
      // Reauth URL, not sync username.
      {TestCase::SYNCING_PASSWORDS, SimpleGaiaForm("user@example.org"),
       "another_user@example.org",
       "https://accounts.google.com/login?rart=123&continue=blah",
       TestCase::FORM_NOT_FILTERED, TestCase::HISTOGRAM_REPORTED},

      // Reauth URL, sync username.
      {TestCase::SYNCING_PASSWORDS, SimpleGaiaForm("user@example.org"),
       "user@example.org",
       "https://accounts.google.com/login?rart=123&continue=blah",
       TestCase::FORM_FILTERED, TestCase::HISTOGRAM_REPORTED},

      // Slightly invalid reauth URL, sync username.
      {TestCase::SYNCING_PASSWORDS, SimpleGaiaForm("user@example.org"),
       "user@example.org",
       "https://accounts.google.com/addlogin?rart",  // Missing rart value.
       TestCase::FORM_FILTERED, TestCase::HISTOGRAM_REPORTED},

      // Non-reauth URL, sync username.
      {TestCase::SYNCING_PASSWORDS, SimpleGaiaForm("user@example.org"),
       "user@example.org", "https://accounts.google.com/login?param=123",
       TestCase::FORM_NOT_FILTERED, TestCase::NO_HISTOGRAM},

      // Non-GAIA "reauth" URL, sync username.
      {TestCase::SYNCING_PASSWORDS, SimpleNonGaiaForm("user@example.org"),
       "user@example.org", "https://site.com/login?rart=678",
       TestCase::FORM_NOT_FILTERED, TestCase::NO_HISTOGRAM},
  };

  for (size_t i = 0; i < arraysize(kTestCases); ++i) {
    SCOPED_TRACE(testing::Message() << "i=" << i);
    CheckFilterResultsTestCase(kTestCases[i]);
  }
}

TEST_F(CredentialsFilterTest, FilterResults_DisallowSync) {
  // Both features are kept enabled, should cause sync credential to be
  // filtered.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitFromCommandLine(
      features::kProtectSyncCredential.name + std::string(",") +
          features::kProtectSyncCredentialOnReauth.name,
      std::string());

  const TestCase kTestCases[] = {
      // Reauth URL, not sync username.
      {TestCase::SYNCING_PASSWORDS, SimpleGaiaForm("user@example.org"),
       "another_user@example.org",
       "https://accounts.google.com/login?rart=123&continue=blah",
       TestCase::FORM_NOT_FILTERED, TestCase::HISTOGRAM_REPORTED},

      // Reauth URL, sync username.
      {TestCase::SYNCING_PASSWORDS, SimpleGaiaForm("user@example.org"),
       "user@example.org",
       "https://accounts.google.com/login?rart=123&continue=blah",
       TestCase::FORM_FILTERED, TestCase::HISTOGRAM_REPORTED},

      // Slightly invalid reauth URL, sync username.
      {TestCase::SYNCING_PASSWORDS, SimpleGaiaForm("user@example.org"),
       "user@example.org",
       "https://accounts.google.com/addlogin?rart",  // Missing rart value.
       TestCase::FORM_FILTERED, TestCase::HISTOGRAM_REPORTED},

      // Non-reauth URL, sync username.
      {TestCase::SYNCING_PASSWORDS, SimpleGaiaForm("user@example.org"),
       "user@example.org", "https://accounts.google.com/login?param=123",
       TestCase::FORM_FILTERED, TestCase::HISTOGRAM_REPORTED},

      // Non-GAIA "reauth" URL, sync username.
      {TestCase::SYNCING_PASSWORDS, SimpleNonGaiaForm("user@example.org"),
       "user@example.org", "https://site.com/login?rart=678",
       TestCase::FORM_NOT_FILTERED, TestCase::HISTOGRAM_REPORTED},
  };

  for (size_t i = 0; i < arraysize(kTestCases); ++i) {
    SCOPED_TRACE(testing::Message() << "i=" << i);
    CheckFilterResultsTestCase(kTestCases[i]);
  }
}

TEST_F(CredentialsFilterTest, ReportFormLoginSuccess_ExistingSyncCredentials) {
  FakeSigninAs("user@gmail.com");
  SetSyncingPasswords(true);

  base::UserActionTester tester;
  SavePending(LoginState::EXISTING);
  filter_.ReportFormLoginSuccess(form_manager_);
  EXPECT_EQ(1, tester.GetActionCount(kFilledAndLoginActionName));
}

TEST_F(CredentialsFilterTest, ReportFormLoginSuccess_NewSyncCredentials) {
  FakeSigninAs("user@gmail.com");
  SetSyncingPasswords(true);

  base::UserActionTester tester;
  SavePending(LoginState::NEW);
  filter_.ReportFormLoginSuccess(form_manager_);
  EXPECT_EQ(0, tester.GetActionCount(kFilledAndLoginActionName));
}

TEST_F(CredentialsFilterTest, ReportFormLoginSuccess_GAIANotSyncCredentials) {
  const char kOtherUsername[] = "other_user@gmail.com";
  FakeSigninAs(kOtherUsername);
  ASSERT_NE(pending_.username_value, base::ASCIIToUTF16(kOtherUsername));
  SetSyncingPasswords(true);

  base::UserActionTester tester;
  SavePending(LoginState::EXISTING);
  filter_.ReportFormLoginSuccess(form_manager_);
  EXPECT_EQ(0, tester.GetActionCount(kFilledAndLoginActionName));
}

TEST_F(CredentialsFilterTest, ReportFormLoginSuccess_NotGAIACredentials) {
  pending_ = SimpleNonGaiaForm("user@gmail.com");
  FakeSigninAs("user@gmail.com");
  SetSyncingPasswords(true);

  base::UserActionTester tester;
  SavePending(LoginState::EXISTING);
  filter_.ReportFormLoginSuccess(form_manager_);
  EXPECT_EQ(0, tester.GetActionCount(kFilledAndLoginActionName));
}

TEST_F(CredentialsFilterTest, ReportFormLoginSuccess_NotSyncing) {
  FakeSigninAs("user@gmail.com");
  SetSyncingPasswords(false);

  base::UserActionTester tester;
  SavePending(LoginState::EXISTING);
  filter_.ReportFormLoginSuccess(form_manager_);
  EXPECT_EQ(0, tester.GetActionCount(kFilledAndLoginActionName));
}

TEST_F(CredentialsFilterTest, ShouldSave_NotSyncCredential) {
  PasswordForm form = SimpleGaiaForm("user@example.org");

  ASSERT_NE("user@example.org",
            signin_manager()->GetAuthenticatedAccountInfo().email);
  SetSyncingPasswords(true);
  EXPECT_TRUE(filter_.ShouldSave(form));
}

TEST_F(CredentialsFilterTest, ShouldSave_SyncCredential) {
  PasswordForm form = SimpleGaiaForm("user@example.org");

  FakeSigninAs("user@example.org");
  SetSyncingPasswords(true);
  EXPECT_FALSE(filter_.ShouldSave(form));
}

TEST_F(CredentialsFilterTest, ShouldSave_SyncCredential_NotSyncingPasswords) {
  PasswordForm form = SimpleGaiaForm("user@example.org");

  FakeSigninAs("user@example.org");
  SetSyncingPasswords(false);
  EXPECT_TRUE(filter_.ShouldSave(form));
}

TEST_F(CredentialsFilterTest, ShouldFilterOneForm) {
  // Both features are kept enabled, should cause sync credential to be
  // filtered.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitFromCommandLine(
      features::kProtectSyncCredential.name + std::string(",") +
          features::kProtectSyncCredentialOnReauth.name,
      std::string());

  std::vector<std::unique_ptr<PasswordForm>> results;
  results.push_back(
      std::make_unique<PasswordForm>(SimpleGaiaForm("test1@gmail.com")));
  results.push_back(
      std::make_unique<PasswordForm>(SimpleGaiaForm("test2@gmail.com")));

  FakeSigninAs("test1@gmail.com");

  results = filter_.FilterResults(std::move(results));

  ASSERT_EQ(1u, results.size());
  EXPECT_EQ(SimpleGaiaForm("test2@gmail.com"), *results[0]);
}

}  // namespace password_manager
