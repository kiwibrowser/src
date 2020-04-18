// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/remote/remote_suggestions_fetcher_impl.h"

#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/default_clock.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/language/core/browser/url_language_histogram.h"
#include "components/ntp_snippets/category.h"
#include "components/ntp_snippets/features.h"
#include "components/ntp_snippets/ntp_snippets_constants.h"
#include "components/ntp_snippets/user_classifier.h"
#include "components/variations/variations_associated_data.h"
#include "net/base/url_util.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_status.h"
#include "services/identity/public/cpp/identity_manager.h"

using language::UrlLanguageHistogram;
using net::HttpRequestHeaders;
using net::URLFetcher;
using net::URLRequestContextGetter;
using net::URLRequestStatus;

namespace ntp_snippets {

using internal::FetchResult;
using internal::JsonRequest;

namespace {

const char kApiKeyQueryParam[] = "key";
const char kPriorityQueryParam[] = "priority";
const char kInteractivePriority[] = "user_action";
const char kNonInteractivePriority[] = "background_prefetch";
const char kAuthorizationRequestHeaderFormat[] = "Bearer %s";

const int kFetchTimeHistogramResolution = 5;

// Enables appending request priority as a query parameter to the fetch url,
// when fetching article suggestions.
const char kAppendRequestPriorityAsQueryParameterParamName[] =
    "append_request_priority_as_query_parameter";
const bool kAppendRequestPriorityAsQueryParameterParamDefault = true;

bool IsAppendingRequestPriorityAsQueryParameterEnabled() {
  return variations::GetVariationParamByFeatureAsBool(
      ntp_snippets::kArticleSuggestionsFeature,
      kAppendRequestPriorityAsQueryParameterParamName,
      kAppendRequestPriorityAsQueryParameterParamDefault);
}

GURL AppendPriorityQueryParameterIfEnabled(const GURL& url,
                                           bool is_interactive_request) {
  if (IsAppendingRequestPriorityAsQueryParameterEnabled()) {
    return net::AppendQueryParameter(url, kPriorityQueryParam,
                                     is_interactive_request
                                         ? kInteractivePriority
                                         : kNonInteractivePriority);
  }
  return url;
}

std::string FetchResultToString(FetchResult result) {
  switch (result) {
    case FetchResult::SUCCESS:
      return "OK";
    case FetchResult::URL_REQUEST_STATUS_ERROR:
      return "URLRequestStatus error";
    case FetchResult::HTTP_ERROR:
      return "HTTP error";
    case FetchResult::JSON_PARSE_ERROR:
      return "Received invalid JSON";
    case FetchResult::INVALID_SNIPPET_CONTENT_ERROR:
      return "Invalid / empty list.";
    case FetchResult::OAUTH_TOKEN_ERROR:
      return "Error in obtaining an OAuth2 access token.";
    case FetchResult::MISSING_API_KEY:
      return "No API key available.";
    case FetchResult::RESULT_MAX:
      break;
  }
  NOTREACHED();
  return "Unknown error";
}

Status FetchResultToStatus(FetchResult result) {
  switch (result) {
    case FetchResult::SUCCESS:
      return Status::Success();
    // Permanent errors occur if it is more likely that the error originated
    // from the client.
    case FetchResult::OAUTH_TOKEN_ERROR:
    case FetchResult::MISSING_API_KEY:
      return Status(StatusCode::PERMANENT_ERROR, FetchResultToString(result));
    // Temporary errors occur if it's more likely that the client behaved
    // correctly but the server failed to respond as expected.
    // TODO(fhorschig): Revisit HTTP_ERROR once the rescheduling was reworked.
    case FetchResult::HTTP_ERROR:
    case FetchResult::URL_REQUEST_STATUS_ERROR:
    case FetchResult::INVALID_SNIPPET_CONTENT_ERROR:
    case FetchResult::JSON_PARSE_ERROR:
      return Status(StatusCode::TEMPORARY_ERROR, FetchResultToString(result));
    case FetchResult::RESULT_MAX:
      break;
  }
  NOTREACHED();
  return Status(StatusCode::PERMANENT_ERROR, std::string());
}

int GetMinuteOfTheDay(bool local_time,
                      bool reduced_resolution,
                      base::Clock* clock) {
  base::Time now(clock->Now());
  base::Time::Exploded now_exploded{};
  local_time ? now.LocalExplode(&now_exploded) : now.UTCExplode(&now_exploded);
  int now_minute = reduced_resolution
                       ? now_exploded.minute / kFetchTimeHistogramResolution *
                             kFetchTimeHistogramResolution
                       : now_exploded.minute;
  return now_exploded.hour * 60 + now_minute;
}

// The response from the backend might include suggestions from multiple
// categories. If only a single category was requested, this function filters
// all other categories out.
void FilterCategories(FetchedCategoriesVector* categories,
                      base::Optional<Category> exclusive_category) {
  if (!exclusive_category.has_value()) {
    return;
  }
  Category exclusive = exclusive_category.value();
  auto category_it =
      std::find_if(categories->begin(), categories->end(),
                   [&exclusive](const FetchedCategory& c) -> bool {
                     return c.category == exclusive;
                   });
  if (category_it == categories->end()) {
    categories->clear();
    return;
  }
  FetchedCategory category = std::move(*category_it);
  categories->clear();
  categories->push_back(std::move(category));
}

}  // namespace

RemoteSuggestionsFetcherImpl::RemoteSuggestionsFetcherImpl(
    identity::IdentityManager* identity_manager,
    scoped_refptr<URLRequestContextGetter> url_request_context_getter,
    PrefService* pref_service,
    UrlLanguageHistogram* language_histogram,
    const ParseJSONCallback& parse_json_callback,
    const GURL& api_endpoint,
    const std::string& api_key,
    const UserClassifier* user_classifier)
    : identity_manager_(identity_manager),
      url_request_context_getter_(std::move(url_request_context_getter)),
      language_histogram_(language_histogram),
      parse_json_callback_(parse_json_callback),
      fetch_url_(api_endpoint),
      api_key_(api_key),
      clock_(base::DefaultClock::GetInstance()),
      user_classifier_(user_classifier),
      last_fetch_authenticated_(false) {}

RemoteSuggestionsFetcherImpl::~RemoteSuggestionsFetcherImpl() = default;

const std::string& RemoteSuggestionsFetcherImpl::GetLastStatusForDebugging()
    const {
  return last_status_;
}
const std::string& RemoteSuggestionsFetcherImpl::GetLastJsonForDebugging()
    const {
  return last_fetch_json_;
}
bool RemoteSuggestionsFetcherImpl::WasLastFetchAuthenticatedForDebugging()
    const {
  return last_fetch_authenticated_;
}
const GURL& RemoteSuggestionsFetcherImpl::GetFetchUrlForDebugging() const {
  return fetch_url_;
}

void RemoteSuggestionsFetcherImpl::FetchSnippets(
    const RequestParams& params,
    SnippetsAvailableCallback callback) {
  if (!params.interactive_request) {
    base::UmaHistogramSparse(
        "NewTabPage.Snippets.FetchTimeLocal",
        GetMinuteOfTheDay(/*local_time=*/true,
                          /*reduced_resolution=*/true, clock_));
    base::UmaHistogramSparse(
        "NewTabPage.Snippets.FetchTimeUTC",
        GetMinuteOfTheDay(/*local_time=*/false,
                          /*reduced_resolution=*/true, clock_));
  }

  JsonRequest::Builder builder;
  builder.SetLanguageHistogram(language_histogram_)
      .SetParams(params)
      .SetParseJsonCallback(parse_json_callback_)
      .SetClock(clock_)
      .SetUrlRequestContextGetter(url_request_context_getter_)
      .SetUserClassifier(*user_classifier_);

  if (identity_manager_->HasPrimaryAccount()) {
    // Signed-in: get OAuth token --> fetch suggestions.
    pending_requests_.emplace(std::move(builder), std::move(callback));
    StartTokenRequest();
  } else {
    // Not signed in: fetch suggestions (without authentication).
    FetchSnippetsNonAuthenticated(std::move(builder), std::move(callback));
  }
}

void RemoteSuggestionsFetcherImpl::FetchSnippetsNonAuthenticated(
    JsonRequest::Builder builder,
    SnippetsAvailableCallback callback) {
  if (api_key_.empty()) {
    // If we don't have an API key, don't even try.
    FetchFinished(OptionalFetchedCategories(), std::move(callback),
                  FetchResult::MISSING_API_KEY, std::string(),
                  /*is_authenticated=*/false);
    return;
  }
  // When not providing OAuth token, we need to pass the Google API key.
  GURL url = net::AppendQueryParameter(fetch_url_, kApiKeyQueryParam, api_key_);
  url = AppendPriorityQueryParameterIfEnabled(url,
                                              builder.is_interactive_request());

  builder.SetUrl(url);
  StartRequest(std::move(builder), std::move(callback),
               /*is_authenticated=*/false);
}

void RemoteSuggestionsFetcherImpl::FetchSnippetsAuthenticated(
    JsonRequest::Builder builder,
    SnippetsAvailableCallback callback,
    const std::string& oauth_access_token) {
  GURL url = AppendPriorityQueryParameterIfEnabled(
      fetch_url_, builder.is_interactive_request());

  builder.SetUrl(url).SetAuthentication(
      identity_manager_->GetPrimaryAccountInfo().account_id,
      base::StringPrintf(kAuthorizationRequestHeaderFormat,
                         oauth_access_token.c_str()));
  StartRequest(std::move(builder), std::move(callback),
               /*is_authenticated=*/true);
}

void RemoteSuggestionsFetcherImpl::StartRequest(
    JsonRequest::Builder builder,
    SnippetsAvailableCallback callback,
    bool is_authenticated) {
  std::unique_ptr<JsonRequest> request = builder.Build();
  JsonRequest* raw_request = request.get();
  raw_request->Start(base::BindOnce(
      &RemoteSuggestionsFetcherImpl::JsonRequestDone, base::Unretained(this),
      std::move(request), std::move(callback), is_authenticated));
}

void RemoteSuggestionsFetcherImpl::StartTokenRequest() {
  // If there is already an ongoing token request, just wait for that.
  if (token_fetcher_) {
    return;
  }

  OAuth2TokenService::ScopeSet scopes{kContentSuggestionsApiScope};
  token_fetcher_ = identity_manager_->CreateAccessTokenFetcherForPrimaryAccount(
      "ntp_snippets", scopes,
      base::BindOnce(&RemoteSuggestionsFetcherImpl::AccessTokenFetchFinished,
                     base::Unretained(this)),
      identity::PrimaryAccountAccessTokenFetcher::Mode::kWaitUntilAvailable);
}

void RemoteSuggestionsFetcherImpl::AccessTokenFetchFinished(
    const GoogleServiceAuthError& error,
    const std::string& access_token) {
  // Delete the fetcher only after we leave this method (which is called from
  // the fetcher itself).
  DCHECK(token_fetcher_);
  std::unique_ptr<identity::PrimaryAccountAccessTokenFetcher>
      token_fetcher_deleter(std::move(token_fetcher_));

  if (error.state() != GoogleServiceAuthError::NONE) {
    AccessTokenError(error);
    return;
  }

  DCHECK(!access_token.empty());

  while (!pending_requests_.empty()) {
    std::pair<JsonRequest::Builder, SnippetsAvailableCallback>
        builder_and_callback = std::move(pending_requests_.front());
    pending_requests_.pop();
    FetchSnippetsAuthenticated(std::move(builder_and_callback.first),
                               std::move(builder_and_callback.second),
                               access_token);
  }
}

void RemoteSuggestionsFetcherImpl::AccessTokenError(
    const GoogleServiceAuthError& error) {
  DCHECK_NE(error.state(), GoogleServiceAuthError::NONE);

  DLOG(ERROR) << "Unable to get token: " << error.ToString();

  while (!pending_requests_.empty()) {
    std::pair<JsonRequest::Builder, SnippetsAvailableCallback>
        builder_and_callback = std::move(pending_requests_.front());

    FetchFinished(OptionalFetchedCategories(),
                  std::move(builder_and_callback.second),
                  FetchResult::OAUTH_TOKEN_ERROR,
                  /*error_details=*/
                  base::StringPrintf(" (%s)", error.ToString().c_str()),
                  /*is_authenticated=*/true);
    pending_requests_.pop();
  }
}

void RemoteSuggestionsFetcherImpl::JsonRequestDone(
    std::unique_ptr<JsonRequest> request,
    SnippetsAvailableCallback callback,
    bool is_authenticated,
    std::unique_ptr<base::Value> result,
    FetchResult status_code,
    const std::string& error_details) {
  DCHECK(request);
  // Record the time when request for fetching remote content snippets finished.
  const base::Time fetch_time = clock_->Now();

  last_fetch_json_ = request->GetResponseString();

  UMA_HISTOGRAM_TIMES("NewTabPage.Snippets.FetchTime",
                      request->GetFetchDuration());

  if (!result) {
    FetchFinished(OptionalFetchedCategories(), std::move(callback), status_code,
                  error_details, is_authenticated);
    return;
  }

  FetchedCategoriesVector categories;
  if (!JsonToCategories(*result, &categories, fetch_time)) {
    LOG(WARNING) << "Received invalid snippets: " << last_fetch_json_;
    FetchFinished(OptionalFetchedCategories(), std::move(callback),
                  FetchResult::INVALID_SNIPPET_CONTENT_ERROR, std::string(),
                  is_authenticated);
    return;
  }
  // Filter out unwanted categories if necessary.
  // TODO(fhorschig): As soon as the server supports filtering by category,
  // adjust the request instead of over-fetching and filtering here.
  FilterCategories(&categories, request->exclusive_category());

  FetchFinished(std::move(categories), std::move(callback),
                FetchResult::SUCCESS, std::string(), is_authenticated);
}

void RemoteSuggestionsFetcherImpl::FetchFinished(
    OptionalFetchedCategories categories,
    SnippetsAvailableCallback callback,
    FetchResult fetch_result,
    const std::string& error_details,
    bool is_authenticated) {
  DCHECK(fetch_result == FetchResult::SUCCESS || !categories.has_value());

  last_status_ = FetchResultToString(fetch_result) + error_details;
  last_fetch_authenticated_ = is_authenticated;

  UMA_HISTOGRAM_ENUMERATION("NewTabPage.Snippets.FetchResult",
                            static_cast<int>(fetch_result),
                            static_cast<int>(FetchResult::RESULT_MAX));

  DVLOG(1) << "Fetch finished: " << last_status_;

  std::move(callback).Run(FetchResultToStatus(fetch_result),
                          std::move(categories));
}

}  // namespace ntp_snippets
