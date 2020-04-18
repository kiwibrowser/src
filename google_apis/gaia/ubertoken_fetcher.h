// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_APIS_GAIA_UBERTOKEN_FETCHER_H_
#define GOOGLE_APIS_GAIA_UBERTOKEN_FETCHER_H_

#include <memory>

#include "base/macros.h"
#include "base/timer/timer.h"
#include "google_apis/gaia/gaia_auth_consumer.h"
#include "google_apis/gaia/oauth2_token_service.h"

// Allow to retrieves an uber-auth token for the user. This class uses the
// |OAuth2TokenService| and considers that the user is already logged in. It
// will use the OAuth2 access token to generate the uber-auth token.
//
// This class should be used on a single thread, but it can be whichever thread
// that you like.
//
// This class can handle one request at a time.

class GaiaAuthFetcher;
class GoogleServiceAuthError;

namespace net {
class URLRequestContextGetter;
}

using GaiaAuthFetcherFactory = base::Callback<std::unique_ptr<GaiaAuthFetcher>(
    GaiaAuthConsumer*,
    const std::string&,
    net::URLRequestContextGetter*)>;

// Callback for the |UbertokenFetcher| class.
class UbertokenConsumer {
 public:
  UbertokenConsumer() {}
  virtual ~UbertokenConsumer() {}
  virtual void OnUbertokenSuccess(const std::string& token) {}
  virtual void OnUbertokenFailure(const GoogleServiceAuthError& error) {}
};

// Allows to retrieve an uber-auth token.
class UbertokenFetcher : public GaiaAuthConsumer,
                         public OAuth2TokenService::Consumer {
 public:
  // Maximum number of retries to get the uber-auth token before giving up.
  static const int kMaxRetries;

  UbertokenFetcher(OAuth2TokenService* token_service,
                   UbertokenConsumer* consumer,
                   const std::string& source,
                   net::URLRequestContextGetter* request_context);
  UbertokenFetcher(OAuth2TokenService* token_service,
                   UbertokenConsumer* consumer,
                   const std::string& source,
                   net::URLRequestContextGetter* request_context,
                   GaiaAuthFetcherFactory factory);
  ~UbertokenFetcher() override;

  void set_is_bound_to_channel_id(bool is_bound_to_channel_id) {
    is_bound_to_channel_id_ = is_bound_to_channel_id;
  }

  // Start fetching the token for |account_id|.
  virtual void StartFetchingToken(const std::string& account_id);
  virtual void StartFetchingTokenWithAccessToken(const std::string& account_id,
      const std::string& access_token);

  // Overriden from GaiaAuthConsumer
  void OnUberAuthTokenSuccess(const std::string& token) override;
  void OnUberAuthTokenFailure(const GoogleServiceAuthError& error) override;

  // Overriden from OAuth2TokenService::Consumer:
  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

 private:
  // Request a login-scoped access token from the token service.
  void RequestAccessToken();

  // Exchanges an oauth2 access token for an uber-auth token.
  void ExchangeTokens();

  OAuth2TokenService* token_service_;
  UbertokenConsumer* consumer_;
  std::string source_;
  net::URLRequestContextGetter* request_context_;
  bool is_bound_to_channel_id_;  // defaults to true
  GaiaAuthFetcherFactory gaia_auth_fetcher_factory_;
  std::unique_ptr<GaiaAuthFetcher> gaia_auth_fetcher_;
  std::unique_ptr<OAuth2TokenService::Request> access_token_request_;
  std::string account_id_;
  std::string access_token_;
  int retry_number_;
  base::OneShotTimer retry_timer_;
  bool second_access_token_request_;

  DISALLOW_COPY_AND_ASSIGN(UbertokenFetcher);
};

#endif  // GOOGLE_APIS_GAIA_UBERTOKEN_FETCHER_H_
