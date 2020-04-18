// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/identity/public/cpp/identity_test_environment.h"

#include "base/run_loop.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/fake_profile_oauth2_token_service.h"
#include "components/signin/core/browser/fake_signin_manager.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/signin/core/browser/test_signin_client.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/identity/public/cpp/identity_test_utils.h"

#if defined(OS_CHROMEOS)
using SigninManagerForTest = FakeSigninManagerBase;
#else
using SigninManagerForTest = FakeSigninManager;
#endif  // OS_CHROMEOS

namespace identity {

// Internal class that abstracts the dependencies out of the public interface.
class IdentityTestEnvironmentInternal {
 public:
  IdentityTestEnvironmentInternal();
  ~IdentityTestEnvironmentInternal();

  // The IdentityManager instance created and owned by this instance.
  IdentityManager* identity_manager();

  SigninManagerForTest* signin_manager();

  FakeProfileOAuth2TokenService* token_service();

 private:
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  AccountTrackerService account_tracker_;
  TestSigninClient signin_client_;
  SigninManagerForTest signin_manager_;
  FakeProfileOAuth2TokenService token_service_;
  std::unique_ptr<IdentityManager> identity_manager_;

  DISALLOW_COPY_AND_ASSIGN(IdentityTestEnvironmentInternal);
};

IdentityTestEnvironmentInternal::IdentityTestEnvironmentInternal()
    : signin_client_(&pref_service_),
#if defined(OS_CHROMEOS)
      signin_manager_(&signin_client_, &account_tracker_)
#else
      signin_manager_(&signin_client_,
                      &token_service_,
                      &account_tracker_,
                      nullptr)
#endif
{
  AccountTrackerService::RegisterPrefs(pref_service_.registry());
  SigninManagerBase::RegisterProfilePrefs(pref_service_.registry());
  SigninManagerBase::RegisterPrefs(pref_service_.registry());
  signin::RegisterAccountConsistencyProfilePrefs(pref_service_.registry());
  signin::SetGaiaOriginIsolatedCallback(
      base::BindRepeating([] { return true; }));

  account_tracker_.Initialize(&signin_client_);

  identity_manager_.reset(
      new IdentityManager(&signin_manager_, &token_service_));
}

IdentityTestEnvironmentInternal::~IdentityTestEnvironmentInternal() {}

IdentityManager* IdentityTestEnvironmentInternal::identity_manager() {
  return identity_manager_.get();
}

SigninManagerForTest* IdentityTestEnvironmentInternal::signin_manager() {
  return &signin_manager_;
}

FakeProfileOAuth2TokenService*
IdentityTestEnvironmentInternal::token_service() {
  return &token_service_;
}

IdentityTestEnvironment::IdentityTestEnvironment()
    : internals_(std::make_unique<IdentityTestEnvironmentInternal>()) {
  internals_->identity_manager()->AddDiagnosticsObserver(this);
}

IdentityTestEnvironment::~IdentityTestEnvironment() {
  internals_->identity_manager()->RemoveDiagnosticsObserver(this);
}

IdentityManager* IdentityTestEnvironment::identity_manager() {
  return internals_->identity_manager();
}

void IdentityTestEnvironment::MakePrimaryAccountAvailable(std::string email) {
  identity::MakePrimaryAccountAvailable(internals_->signin_manager(),
                                        internals_->token_service(),
                                        internals_->identity_manager(), email);
}

void IdentityTestEnvironment::ClearPrimaryAccount() {
  identity::ClearPrimaryAccount(internals_->signin_manager(),
                                internals_->identity_manager());
}

void IdentityTestEnvironment::SetAutomaticIssueOfAccessTokens(bool grant) {
  internals_->token_service()->set_auto_post_fetch_response_on_message_loop(
      grant);
}

void IdentityTestEnvironment::
    WaitForAccessTokenRequestIfNecessaryAndRespondWithToken(
        const std::string& token,
        const base::Time& expiration) {
  WaitForAccessTokenRequestIfNecessary();
  internals_->token_service()->IssueTokenForAllPendingRequests(token,
                                                               expiration);
}

void IdentityTestEnvironment::
    WaitForAccessTokenRequestIfNecessaryAndRespondWithError(
        const GoogleServiceAuthError& error) {
  WaitForAccessTokenRequestIfNecessary();
  internals_->token_service()->IssueErrorForAllPendingRequests(error);
}

void IdentityTestEnvironment::OnAccessTokenRequested(
    const std::string& account_id,
    const std::string& consumer_id,
    const OAuth2TokenService::ScopeSet& scopes) {
  // Post a task to handle this access token request in order to support the
  // case where the access token request is handled synchronously in the
  // production code, in which case this callback could be coming in ahead
  // of an invocation of WaitForAccessTokenRequestIfNecessary() that will be
  // made in this same iteration of the run loop.
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&IdentityTestEnvironment::HandleOnAccessTokenRequested,
                     base::Unretained(this)));
}

void IdentityTestEnvironment::HandleOnAccessTokenRequested() {
  if (on_access_token_requested_callback_)
    std::move(on_access_token_requested_callback_).Run();
}

void IdentityTestEnvironment::WaitForAccessTokenRequestIfNecessary() {
  DCHECK(!on_access_token_requested_callback_);
  base::RunLoop run_loop;
  on_access_token_requested_callback_ = run_loop.QuitClosure();
  run_loop.Run();
}

}  // namespace identity
