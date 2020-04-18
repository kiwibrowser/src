// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_AUTH_ARC_AUTH_CONTEXT_H_
#define CHROME_BROWSER_CHROMEOS_ARC_AUTH_ARC_AUTH_CONTEXT_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "google_apis/gaia/ubertoken_fetcher.h"
#include "net/base/backoff_entry.h"

class Profile;
class ProfileOAuth2TokenService;

namespace net {
class URLRequestContextGetter;
}

namespace arc {

class ArcAuthContext : public UbertokenConsumer,
                       public GaiaAuthConsumer,
                       public OAuth2TokenService::Observer {
 public:
  explicit ArcAuthContext(Profile* profile);
  ~ArcAuthContext() override;

  ProfileOAuth2TokenService* token_service() { return token_service_; }
  const std::string& account_id() const { return account_id_; }

  // Returns full account id, including dots that are removed in CrOS for
  // the default account id.
  const std::string& full_account_id() const { return full_account_id_; }

  // Prepares the context. Calling while an inflight operation exists will
  // cancel the inflight operation.
  // On completion, |context| is passed to the callback. On error, |context|
  // is nullptr.
  using PrepareCallback =
      base::Callback<void(net::URLRequestContextGetter* context)>;
  void Prepare(const PrepareCallback& callback);

  // OAuth2TokenService::Observer:
  void OnRefreshTokenAvailable(const std::string& account_id) override;
  void OnRefreshTokensLoaded() override;

  // UbertokenConsumer:
  void OnUbertokenSuccess(const std::string& token) override;
  void OnUbertokenFailure(const GoogleServiceAuthError& error) override;

  // GaiaAuthConsumer:
  void OnMergeSessionSuccess(const std::string& data) override;
  void OnMergeSessionFailure(const GoogleServiceAuthError& error) override;

  // Skips the merge session, instead calling the callback passed to |Prepare()|
  // once the refresh token is available. Use only in testing.
  void SkipMergeSessionForTesting() { skip_merge_session_for_testing_ = true; }

 private:
  void OnRefreshTokenTimeout();

  void StartFetchers();
  void ResetFetchers();
  void OnFetcherError(const GoogleServiceAuthError& error);

  // Unowned pointer.
  Profile* const profile_;
  ProfileOAuth2TokenService* token_service_;

  std::string account_id_;
  std::string full_account_id_;

  // Whether the merge session should be skipped. Set to true only in testing.
  bool skip_merge_session_for_testing_ = false;

  PrepareCallback callback_;
  bool context_prepared_ = false;

  // Defines retry logic in case of transient error.
  net::BackoffEntry retry_backoff_;

  base::OneShotTimer refresh_token_timeout_;
  base::OneShotTimer retry_timeout_;
  std::unique_ptr<GaiaAuthFetcher> merger_fetcher_;
  std::unique_ptr<UbertokenFetcher> ubertoken_fetcher_;

  DISALLOW_COPY_AND_ASSIGN(ArcAuthContext);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_AUTH_ARC_AUTH_CONTEXT_H_
