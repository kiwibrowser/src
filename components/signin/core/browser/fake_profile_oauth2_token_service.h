// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SIGNIN_CORE_BROWSER_FAKE_PROFILE_OAUTH2_TOKEN_SERVICE_H_
#define COMPONENTS_SIGNIN_CORE_BROWSER_FAKE_PROFILE_OAUTH2_TOKEN_SERVICE_H_

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"

// Helper class to simplify writing unittests that depend on an instance of
// ProfileOAuth2TokenService.
//
// Tests would typically do something like the following:
//
// FakeProfileOAuth2TokenService service;
// ...
// service.IssueRefreshToken("token");  // Issue refresh token/notify observers
// ...
// // Confirm that there is at least one active request.
// EXPECT_GT(0U, service.GetPendingRequests().size());
// ...
// // Make any pending token fetches for a given scope succeed.
// ScopeSet scopes;
// scopes.insert(GaiaConstants::kYourServiceScope);
// IssueTokenForScope(scopes, "access_token", base::Time()::Max());
// ...
// // ...or make them fail...
// IssueErrorForScope(scopes, GoogleServiceAuthError(INVALID_GAIA_CREDENTIALS));
//
class FakeProfileOAuth2TokenService : public ProfileOAuth2TokenService {
 public:
  struct PendingRequest {
    PendingRequest();
    PendingRequest(const PendingRequest& other);
    ~PendingRequest();

    std::string account_id;
    std::string client_id;
    std::string client_secret;
    ScopeSet scopes;
    base::WeakPtr<RequestImpl> request;
  };

  FakeProfileOAuth2TokenService();
  explicit FakeProfileOAuth2TokenService(
      std::unique_ptr<OAuth2TokenServiceDelegate> delegate);
  ~FakeProfileOAuth2TokenService() override;

  // Gets a list of active requests (can be used by tests to validate that the
  // correct request has been issued).
  std::vector<PendingRequest> GetPendingRequests();

  // Helper routines to issue tokens for pending requests.
  void IssueAllTokensForAccount(const std::string& account_id,
                                const std::string& access_token,
                                const base::Time& expiration);

  void IssueErrorForAllPendingRequestsForAccount(
      const std::string& account_id,
      const GoogleServiceAuthError& error);

  void IssueTokenForScope(const ScopeSet& scopes,
                          const std::string& access_token,
                          const base::Time& expiration);

  void IssueErrorForScope(const ScopeSet& scopes,
                          const GoogleServiceAuthError& error);

  void IssueTokenForAllPendingRequests(const std::string& access_token,
                                       const base::Time& expiration);

  void IssueErrorForAllPendingRequests(const GoogleServiceAuthError& error);

  void set_auto_post_fetch_response_on_message_loop(bool auto_post_response) {
    auto_post_fetch_response_on_message_loop_ = auto_post_response;
  }

  // Calls ProfileOAuth2TokenService::UpdateAuthError(). Exposed for testing.
  void UpdateAuthErrorForTesting(const std::string& account_id,
                                 const GoogleServiceAuthError& error);

 protected:
  // OAuth2TokenService overrides.
  void FetchOAuth2Token(RequestImpl* request,
                        const std::string& account_id,
                        net::URLRequestContextGetter* getter,
                        const std::string& client_id,
                        const std::string& client_secret,
                        const ScopeSet& scopes) override;

  void InvalidateAccessTokenImpl(const std::string& account_id,
                                 const std::string& client_id,
                                 const ScopeSet& scopes,
                                 const std::string& access_token) override;

 private:
  // Helper function to complete pending requests - if |all_scopes| is true,
  // then all pending requests are completed, otherwise, only those requests
  // matching |scopes| are completed.  If |account_id| is empty, then pending
  // requests for all accounts are completed, otherwise only requests for the
  // given account.
  void CompleteRequests(const std::string& account_id,
                        bool all_scopes,
                        const ScopeSet& scopes,
                        const GoogleServiceAuthError& error,
                        const std::string& access_token,
                        const base::Time& expiration);

  std::vector<PendingRequest> pending_requests_;

  // If true, then this fake service will post responses to
  // |FetchOAuth2Token| on the current run loop. There is no need to call
  // |IssueTokenForScope| in this case.
  bool auto_post_fetch_response_on_message_loop_;

  base::WeakPtrFactory<FakeProfileOAuth2TokenService> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FakeProfileOAuth2TokenService);
};

#endif  // COMPONENTS_SIGNIN_CORE_BROWSER_FAKE_PROFILE_OAUTH2_TOKEN_SERVICE_H_
