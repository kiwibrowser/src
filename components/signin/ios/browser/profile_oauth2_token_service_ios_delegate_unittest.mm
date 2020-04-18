// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/ios/browser/profile_oauth2_token_service_ios_delegate.h"

#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_error_controller.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "components/signin/core/browser/test_signin_client.h"
#include "components/signin/ios/browser/fake_profile_oauth2_token_service_ios_provider.h"
#include "google_apis/gaia/gaia_urls.h"
#include "google_apis/gaia/oauth2_access_token_consumer.h"
#include "google_apis/gaia/oauth2_token_service_test_util.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

typedef ProfileOAuth2TokenServiceIOSProvider::AccountInfo ProviderAccount;

class ProfileOAuth2TokenServiceIOSDelegateTest
    : public testing::Test,
      public OAuth2AccessTokenConsumer,
      public OAuth2TokenService::Observer,
      public SigninErrorController::Observer {
 public:
  ProfileOAuth2TokenServiceIOSDelegateTest()
      : factory_(NULL),
        client_(&prefs_),
        signin_error_controller_(
            SigninErrorController::AccountMode::ANY_ACCOUNT),
        token_available_count_(0),
        token_revoked_count_(0),
        tokens_loaded_count_(0),
        access_token_success_(0),
        access_token_failure_(0),
        error_changed_count_(0),
        auth_error_changed_count_(0),
        last_access_token_error_(GoogleServiceAuthError::NONE) {}

  void SetUp() override {
    prefs_.registry()->RegisterListPref(
        AccountTrackerService::kAccountInfoPref);
    prefs_.registry()->RegisterIntegerPref(
        prefs::kAccountIdMigrationState,
        AccountTrackerService::MIGRATION_NOT_STARTED);
    account_tracker_.Initialize(&client_);

    prefs_.registry()->RegisterBooleanPref(
        prefs::kTokenServiceExcludeAllSecondaryAccounts, false);
    prefs_.registry()->RegisterListPref(
        prefs::kTokenServiceExcludedSecondaryAccounts);

    fake_provider_ = new FakeProfileOAuth2TokenServiceIOSProvider();
    factory_.SetFakeResponse(GaiaUrls::GetInstance()->oauth2_revoke_url(), "",
                             net::HTTP_OK, net::URLRequestStatus::SUCCESS);
    oauth2_delegate_.reset(new ProfileOAuth2TokenServiceIOSDelegate(
        &client_, base::WrapUnique(fake_provider_), &account_tracker_,
        &signin_error_controller_));
    oauth2_delegate_->AddObserver(this);
    signin_error_controller_.AddObserver(this);
  }

  void TearDown() override {
    signin_error_controller_.RemoveObserver(this);
    oauth2_delegate_->RemoveObserver(this);
    oauth2_delegate_->Shutdown();
  }

  // OAuth2AccessTokenConsumer implementation.
  void OnGetTokenSuccess(const std::string& access_token,
                         const base::Time& expiration_time) override {
    ++access_token_success_;
  }

  void OnGetTokenFailure(const GoogleServiceAuthError& error) override {
    ++access_token_failure_;
    last_access_token_error_ = error;
  };

  // OAuth2TokenService::Observer implementation.
  void OnRefreshTokenAvailable(const std::string& account_id) override {
    ++token_available_count_;
  }
  void OnRefreshTokenRevoked(const std::string& account_id) override {
    ++token_revoked_count_;
  }
  void OnRefreshTokensLoaded() override { ++tokens_loaded_count_; }
  void OnAuthErrorChanged(const std::string& account_id,
                          const GoogleServiceAuthError& error) override {
    ++auth_error_changed_count_;
  }

  // SigninErrorController::Observer implementation.
  void OnErrorChanged() override { ++error_changed_count_; }

  void ResetObserverCounts() {
    token_available_count_ = 0;
    token_revoked_count_ = 0;
    tokens_loaded_count_ = 0;
    token_available_count_ = 0;
    access_token_failure_ = 0;
    error_changed_count_ = 0;
    auth_error_changed_count_ = 0;
  }

  std::string GetAccountId(const ProviderAccount& provider_account) {
    return account_tracker_.PickAccountIdForAccount(provider_account.gaia,
                                                    provider_account.email);
  }

 protected:
  base::MessageLoop message_loop_;
  net::FakeURLFetcherFactory factory_;
  TestingPrefServiceSimple prefs_;
  TestSigninClient client_;
  AccountTrackerService account_tracker_;
  SigninErrorController signin_error_controller_;
  FakeProfileOAuth2TokenServiceIOSProvider* fake_provider_;
  std::unique_ptr<ProfileOAuth2TokenServiceIOSDelegate> oauth2_delegate_;
  TestingOAuth2TokenServiceConsumer consumer_;
  int token_available_count_;
  int token_revoked_count_;
  int tokens_loaded_count_;
  int access_token_success_;
  int access_token_failure_;
  int error_changed_count_;
  int auth_error_changed_count_;
  GoogleServiceAuthError last_access_token_error_;
};

TEST_F(ProfileOAuth2TokenServiceIOSDelegateTest,
       LoadRevokeCredentialsOneAccount) {
  ProviderAccount account = fake_provider_->AddAccount("gaia_1", "email_1@x");
  oauth2_delegate_->LoadCredentials(GetAccountId(account));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, token_available_count_);
  EXPECT_EQ(1, tokens_loaded_count_);
  EXPECT_EQ(0, token_revoked_count_);
  EXPECT_EQ(1U, oauth2_delegate_->GetAccounts().size());
  EXPECT_TRUE(oauth2_delegate_->RefreshTokenIsAvailable(GetAccountId(account)));

  ResetObserverCounts();
  oauth2_delegate_->RevokeAllCredentials();
  EXPECT_EQ(0, token_available_count_);
  EXPECT_EQ(0, tokens_loaded_count_);
  EXPECT_EQ(1, token_revoked_count_);
  EXPECT_EQ(0U, oauth2_delegate_->GetAccounts().size());
  EXPECT_FALSE(oauth2_delegate_->RefreshTokenIsAvailable("another_account"));
}

TEST_F(ProfileOAuth2TokenServiceIOSDelegateTest,
       LoadRevokeCredentialsMultipleAccounts) {
  ProviderAccount account1 = fake_provider_->AddAccount("gaia_1", "email_1@x");
  ProviderAccount account2 = fake_provider_->AddAccount("gaia_2", "email_2@x");
  ProviderAccount account3 = fake_provider_->AddAccount("gaia_3", "email_3@x");
  oauth2_delegate_->LoadCredentials(GetAccountId(account1));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(3, token_available_count_);
  EXPECT_EQ(1, tokens_loaded_count_);
  EXPECT_EQ(0, token_revoked_count_);
  EXPECT_EQ(3U, oauth2_delegate_->GetAccounts().size());
  EXPECT_TRUE(
      oauth2_delegate_->RefreshTokenIsAvailable(GetAccountId(account1)));
  EXPECT_TRUE(
      oauth2_delegate_->RefreshTokenIsAvailable(GetAccountId(account2)));
  EXPECT_TRUE(
      oauth2_delegate_->RefreshTokenIsAvailable(GetAccountId(account3)));

  ResetObserverCounts();
  oauth2_delegate_->RevokeAllCredentials();
  EXPECT_EQ(0, token_available_count_);
  EXPECT_EQ(0, tokens_loaded_count_);
  EXPECT_EQ(3, token_revoked_count_);
  EXPECT_EQ(0U, oauth2_delegate_->GetAccounts().size());
  EXPECT_FALSE(
      oauth2_delegate_->RefreshTokenIsAvailable(GetAccountId(account1)));
  EXPECT_FALSE(
      oauth2_delegate_->RefreshTokenIsAvailable(GetAccountId(account2)));
  EXPECT_FALSE(
      oauth2_delegate_->RefreshTokenIsAvailable(GetAccountId(account3)));
}

TEST_F(ProfileOAuth2TokenServiceIOSDelegateTest, ReloadCredentials) {
  ProviderAccount account1 = fake_provider_->AddAccount("gaia_1", "email_1@x");
  ProviderAccount account2 = fake_provider_->AddAccount("gaia_2", "email_2@x");
  ProviderAccount account3 = fake_provider_->AddAccount("gaia_3", "email_3@x");
  oauth2_delegate_->LoadCredentials(GetAccountId(account1));
  base::RunLoop().RunUntilIdle();

  // Change the accounts.
  ResetObserverCounts();
  fake_provider_->ClearAccounts();
  fake_provider_->AddAccount(account1.gaia, account1.email);
  ProviderAccount account4 = fake_provider_->AddAccount("gaia_4", "email_4@x");
  oauth2_delegate_->ReloadCredentials();

  EXPECT_EQ(1, token_available_count_);
  EXPECT_EQ(0, tokens_loaded_count_);
  EXPECT_EQ(2, token_revoked_count_);
  EXPECT_EQ(2U, oauth2_delegate_->GetAccounts().size());
  EXPECT_TRUE(
      oauth2_delegate_->RefreshTokenIsAvailable(GetAccountId(account1)));
  EXPECT_FALSE(
      oauth2_delegate_->RefreshTokenIsAvailable(GetAccountId(account2)));
  EXPECT_FALSE(
      oauth2_delegate_->RefreshTokenIsAvailable(GetAccountId(account3)));
  EXPECT_TRUE(
      oauth2_delegate_->RefreshTokenIsAvailable(GetAccountId(account4)));
}

TEST_F(ProfileOAuth2TokenServiceIOSDelegateTest,
       ReloadCredentialsIgnoredIfNoPrimaryAccountId) {
  ProviderAccount account1 = fake_provider_->AddAccount("gaia_1", "email_1@x");
  ProviderAccount account2 = fake_provider_->AddAccount("gaia_2", "email_2@x");
  oauth2_delegate_->ReloadCredentials();

  EXPECT_EQ(0, token_available_count_);
  EXPECT_EQ(0, tokens_loaded_count_);
  EXPECT_EQ(0, token_revoked_count_);
  EXPECT_EQ(0U, oauth2_delegate_->GetAccounts().size());
  EXPECT_FALSE(
      oauth2_delegate_->RefreshTokenIsAvailable(GetAccountId(account1)));
  EXPECT_FALSE(
      oauth2_delegate_->RefreshTokenIsAvailable(GetAccountId(account2)));
}

TEST_F(ProfileOAuth2TokenServiceIOSDelegateTest,
       ReloadCredentialsWithPrimaryAccountId) {
  ProviderAccount account1 = fake_provider_->AddAccount("gaia_1", "email_1@x");
  ProviderAccount account2 = fake_provider_->AddAccount("gaia_2", "email_2@x");
  oauth2_delegate_->ReloadCredentials(GetAccountId(account1));

  EXPECT_EQ(2, token_available_count_);
  EXPECT_EQ(0, tokens_loaded_count_);
  EXPECT_EQ(0, token_revoked_count_);
  EXPECT_EQ(2U, oauth2_delegate_->GetAccounts().size());
  EXPECT_TRUE(
      oauth2_delegate_->RefreshTokenIsAvailable(GetAccountId(account1)));
  EXPECT_TRUE(
      oauth2_delegate_->RefreshTokenIsAvailable(GetAccountId(account2)));
}

TEST_F(ProfileOAuth2TokenServiceIOSDelegateTest, StartRequestSuccess) {
  ProviderAccount account1 = fake_provider_->AddAccount("gaia_1", "email_1@x");
  oauth2_delegate_->LoadCredentials(GetAccountId(account1));
  base::RunLoop().RunUntilIdle();

  // Fetch access tokens.
  ResetObserverCounts();
  std::vector<std::string> scopes;
  scopes.push_back("scope");
  std::unique_ptr<OAuth2AccessTokenFetcher> fetcher1(
      oauth2_delegate_->CreateAccessTokenFetcher(
          GetAccountId(account1), oauth2_delegate_->GetRequestContext(), this));
  fetcher1->Start("foo", "bar", scopes);
  EXPECT_EQ(0, access_token_success_);
  EXPECT_EQ(0, access_token_failure_);

  ResetObserverCounts();
  fake_provider_->IssueAccessTokenForAllRequests();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, access_token_success_);
  EXPECT_EQ(0, access_token_failure_);
}

TEST_F(ProfileOAuth2TokenServiceIOSDelegateTest, StartRequestFailure) {
  ProviderAccount account1 = fake_provider_->AddAccount("gaia_1", "email_1@x");
  oauth2_delegate_->LoadCredentials(GetAccountId(account1));
  base::RunLoop().RunUntilIdle();

  // Fetch access tokens.
  ResetObserverCounts();
  std::vector<std::string> scopes;
  scopes.push_back("scope");
  std::unique_ptr<OAuth2AccessTokenFetcher> fetcher1(
      oauth2_delegate_->CreateAccessTokenFetcher(
          GetAccountId(account1), oauth2_delegate_->GetRequestContext(), this));
  fetcher1->Start("foo", "bar", scopes);
  EXPECT_EQ(0, access_token_success_);
  EXPECT_EQ(0, access_token_failure_);

  ResetObserverCounts();
  fake_provider_->IssueAccessTokenErrorForAllRequests();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, access_token_success_);
  EXPECT_EQ(1, access_token_failure_);
}

// Verifies that UpdateAuthError does nothing after the credentials have been
// revoked.
TEST_F(ProfileOAuth2TokenServiceIOSDelegateTest,
       UpdateAuthErrorAfterRevokeCredentials) {
  ProviderAccount account1 = fake_provider_->AddAccount("gaia_1", "email_1@x");
  oauth2_delegate_->ReloadCredentials(GetAccountId(account1));
  base::RunLoop().RunUntilIdle();

  ResetObserverCounts();
  GoogleServiceAuthError error(GoogleServiceAuthError::SERVICE_ERROR);
  oauth2_delegate_->UpdateAuthError(GetAccountId(account1), error);
  EXPECT_EQ(error, oauth2_delegate_->GetAuthError("gaia_1"));
  EXPECT_EQ(1, auth_error_changed_count_);
  EXPECT_EQ(1, error_changed_count_);

  oauth2_delegate_->RevokeAllCredentials();
  ResetObserverCounts();
  oauth2_delegate_->UpdateAuthError(GetAccountId(account1), error);
  EXPECT_EQ(0, auth_error_changed_count_);
  EXPECT_EQ(0, error_changed_count_);
}

TEST_F(ProfileOAuth2TokenServiceIOSDelegateTest, GetAuthError) {
  // Accounts have no error by default.
  ProviderAccount account1 = fake_provider_->AddAccount("gaia_1", "email_1@x");
  oauth2_delegate_->ReloadCredentials(GetAccountId(account1));
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(GoogleServiceAuthError::AuthErrorNone(),
            oauth2_delegate_->GetAuthError("gaia_1"));
  // Update the error.
  GoogleServiceAuthError error =
      GoogleServiceAuthError::FromInvalidGaiaCredentialsReason(
          GoogleServiceAuthError::InvalidGaiaCredentialsReason::
              CREDENTIALS_REJECTED_BY_SERVER);
  oauth2_delegate_->UpdateAuthError("gaia_1", error);
  EXPECT_EQ(error, oauth2_delegate_->GetAuthError("gaia_1"));
  // Unknown account has no error.
  EXPECT_EQ(GoogleServiceAuthError::AuthErrorNone(),
            oauth2_delegate_->GetAuthError("gaia_2"));
}
