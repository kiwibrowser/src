// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_REMOTE_REMOTE_SUGGESTIONS_FETCHER_IMPL_H_
#define COMPONENTS_NTP_SNIPPETS_REMOTE_REMOTE_SUGGESTIONS_FETCHER_IMPL_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/containers/queue.h"
#include "base/optional.h"
#include "base/time/clock.h"
#include "components/ntp_snippets/remote/json_request.h"
#include "components/ntp_snippets/remote/json_to_categories.h"
#include "components/ntp_snippets/remote/remote_suggestions_fetcher.h"
#include "components/ntp_snippets/remote/request_params.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/identity/public/cpp/primary_account_access_token_fetcher.h"

class PrefService;

namespace base {
class Value;
}  // namespace base

namespace identity {
class IdentityManager;
class PrimaryAccountAccessTokenFetcher;
}

namespace language {
class UrlLanguageHistogram;
}  // namespace language

namespace ntp_snippets {

class UserClassifier;

class RemoteSuggestionsFetcherImpl : public RemoteSuggestionsFetcher {
 public:
  RemoteSuggestionsFetcherImpl(
      identity::IdentityManager* identity_manager,
      scoped_refptr<net::URLRequestContextGetter> url_request_context_getter,
      PrefService* pref_service,
      language::UrlLanguageHistogram* language_histogram,
      const ParseJSONCallback& parse_json_callback,
      const GURL& api_endpoint,
      const std::string& api_key,
      const UserClassifier* user_classifier);
  ~RemoteSuggestionsFetcherImpl() override;

  void FetchSnippets(const RequestParams& params,
                     SnippetsAvailableCallback callback) override;

  const std::string& GetLastStatusForDebugging() const override;
  const std::string& GetLastJsonForDebugging() const override;
  bool WasLastFetchAuthenticatedForDebugging() const override;
  const GURL& GetFetchUrlForDebugging() const override;

  // Overrides internal clock for testing purposes.
  void SetClockForTesting(base::Clock* clock) { clock_ = clock; }

 private:
  void FetchSnippetsNonAuthenticated(internal::JsonRequest::Builder builder,
                                     SnippetsAvailableCallback callback);
  void FetchSnippetsAuthenticated(internal::JsonRequest::Builder builder,
                                  SnippetsAvailableCallback callback,
                                  const std::string& oauth_access_token);
  void StartRequest(internal::JsonRequest::Builder builder,
                    SnippetsAvailableCallback callback,
                    bool is_authenticated);

  void StartTokenRequest();

  void AccessTokenFetchFinished(const GoogleServiceAuthError& error,
                                const std::string& access_token);
  void AccessTokenError(const GoogleServiceAuthError& error);

  void JsonRequestDone(std::unique_ptr<internal::JsonRequest> request,
                       SnippetsAvailableCallback callback,
                       bool is_authenticated,
                       std::unique_ptr<base::Value> result,
                       internal::FetchResult status_code,
                       const std::string& error_details);
  void FetchFinished(OptionalFetchedCategories categories,
                     SnippetsAvailableCallback callback,
                     internal::FetchResult status_code,
                     const std::string& error_details,
                     bool is_authenticated);

  // Authentication for signed-in users.
  identity::IdentityManager* identity_manager_;

  std::unique_ptr<identity::PrimaryAccountAccessTokenFetcher> token_fetcher_;

  // Holds the URL request context.
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;

  // Stores requests that wait for an access token.
  base::queue<
      std::pair<internal::JsonRequest::Builder, SnippetsAvailableCallback>>
      pending_requests_;

  // Weak reference, not owned.
  language::UrlLanguageHistogram* const language_histogram_;

  const ParseJSONCallback parse_json_callback_;

  // API endpoint for fetching suggestions.
  const GURL fetch_url_;

  // API key to use for non-authenticated requests.
  const std::string api_key_;

  // Allow for an injectable clock for testing.
  base::Clock* clock_;

  // Classifier that tells us how active the user is. Not owned.
  const UserClassifier* user_classifier_;

  // Info on the last finished fetch.
  std::string last_status_;
  std::string last_fetch_json_;
  bool last_fetch_authenticated_;

  DISALLOW_COPY_AND_ASSIGN(RemoteSuggestionsFetcherImpl);
};

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_REMOTE_REMOTE_SUGGESTIONS_FETCHER_IMPL_H_
