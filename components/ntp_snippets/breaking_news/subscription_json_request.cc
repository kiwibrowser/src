// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/breaking_news/subscription_json_request.h"

#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/variations/net/variations_http_headers.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"

using net::URLFetcher;
using net::URLRequestContextGetter;
using net::HttpRequestHeaders;
using net::URLRequestStatus;

namespace ntp_snippets {

namespace internal {

SubscriptionJsonRequest::SubscriptionJsonRequest() = default;

SubscriptionJsonRequest::~SubscriptionJsonRequest() = default;

void SubscriptionJsonRequest::Start(CompletedCallback callback) {
  DCHECK(request_completed_callback_.is_null()) << "Request already running!";
  request_completed_callback_ = std::move(callback);
  url_fetcher_->Start();
}

////////////////////////////////////////////////////////////////////////////////
// URLFetcherDelegate overrides
void SubscriptionJsonRequest::OnURLFetchComplete(const URLFetcher* source) {
  DCHECK_EQ(url_fetcher_.get(), source);
  const URLRequestStatus& status = url_fetcher_->GetStatus();
  int response = url_fetcher_->GetResponseCode();

  if (!status.is_success()) {
    std::move(request_completed_callback_)
        .Run(Status(StatusCode::TEMPORARY_ERROR,
                    base::StringPrintf("Network Error: %d", status.error())));
  } else if (response != net::HTTP_OK) {
    std::move(request_completed_callback_)
        .Run(Status(StatusCode::PERMANENT_ERROR,
                    base::StringPrintf("HTTP Error: %d", response)));
  } else {
    std::move(request_completed_callback_)
        .Run(Status(StatusCode::SUCCESS, std::string()));
  }
}

SubscriptionJsonRequest::Builder::Builder() = default;
SubscriptionJsonRequest::Builder::Builder(SubscriptionJsonRequest::Builder&&) =
    default;
SubscriptionJsonRequest::Builder::~Builder() = default;

std::unique_ptr<SubscriptionJsonRequest>
SubscriptionJsonRequest::Builder::Build() const {
  DCHECK(!url_.is_empty());
  DCHECK(url_request_context_getter_);
  auto request = base::WrapUnique(new SubscriptionJsonRequest());

  std::string body = BuildBody();
  std::string headers = BuildHeaders();
  request->url_fetcher_ = BuildURLFetcher(request.get(), headers, body);

  // Log the request for debugging network issues.
  DVLOG(1) << "Building a subscription request to " << url_ << ":\n"
           << headers << "\n"
           << body;

  return request;
}

SubscriptionJsonRequest::Builder& SubscriptionJsonRequest::Builder::SetToken(
    const std::string& token) {
  token_ = token;
  return *this;
}

SubscriptionJsonRequest::Builder& SubscriptionJsonRequest::Builder::SetUrl(
    const GURL& url) {
  url_ = url;
  return *this;
}

SubscriptionJsonRequest::Builder&
SubscriptionJsonRequest::Builder::SetUrlRequestContextGetter(
    const scoped_refptr<URLRequestContextGetter>& context_getter) {
  url_request_context_getter_ = context_getter;
  return *this;
}

SubscriptionJsonRequest::Builder&
SubscriptionJsonRequest::Builder::SetAuthenticationHeader(
    const std::string& auth_header) {
  auth_header_ = auth_header;
  return *this;
}

SubscriptionJsonRequest::Builder& SubscriptionJsonRequest::Builder::SetLocale(
    const std::string& locale) {
  locale_ = locale;
  return *this;
}

SubscriptionJsonRequest::Builder&
SubscriptionJsonRequest::Builder::SetCountryCode(
    const std::string& country_code) {
  country_code_ = country_code;
  return *this;
}

std::string SubscriptionJsonRequest::Builder::BuildHeaders() const {
  HttpRequestHeaders headers;
  headers.SetHeader(HttpRequestHeaders::kContentType,
                    "application/json; charset=UTF-8");
  if (!auth_header_.empty()) {
    headers.SetHeader(HttpRequestHeaders::kAuthorization, auth_header_);
  }
  // Add X-Client-Data header with experiment IDs from field trials.
  // Note: It's OK to pass SignedIn::kNo if it's unknown, as it does not affect
  // transmission of experiments coming from the variations server.
  variations::AppendVariationHeaders(url_, variations::InIncognito::kNo,
                                     variations::SignedIn::kNo, &headers);
  return headers.ToString();
}

std::string SubscriptionJsonRequest::Builder::BuildBody() const {
  base::DictionaryValue request;
  request.SetString("token", token_);

  request.SetString("locale", locale_);
  request.SetString("country_code", country_code_);

  std::string request_json;
  bool success = base::JSONWriter::Write(request, &request_json);
  DCHECK(success);
  return request_json;
}

std::unique_ptr<URLFetcher> SubscriptionJsonRequest::Builder::BuildURLFetcher(
    URLFetcherDelegate* delegate,
    const std::string& headers,
    const std::string& body) const {
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("gcm_subscription", R"(
        semantics {
          sender: "Subscribe for breaking news delivered via GCM push messages"
          description:
            "Chromium can receive breaking news via GCM push messages. "
            "This request suscribes the client to receiving them."
          trigger:
            "Subscription takes place only once per profile lifetime. "
          data:
            "The subscription token that identifies this Chromium profile."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled by settings now"
          chrome_policy {
            NTPContentSuggestionsEnabled {
              policy_options {mode: MANDATORY}
              NTPContentSuggestionsEnabled: false
            }
          }
        })");
  std::unique_ptr<URLFetcher> url_fetcher =
      URLFetcher::Create(url_, URLFetcher::POST, delegate, traffic_annotation);
  url_fetcher->SetRequestContext(url_request_context_getter_.get());
  url_fetcher->SetLoadFlags(net::LOAD_DO_NOT_SEND_COOKIES |
                            net::LOAD_DO_NOT_SAVE_COOKIES);
  data_use_measurement::DataUseUserData::AttachToFetcher(
      url_fetcher.get(),
      data_use_measurement::DataUseUserData::NTP_SNIPPETS_SUGGESTIONS);

  url_fetcher->SetExtraRequestHeaders(headers);
  url_fetcher->SetUploadData("application/json", body);

  // Fetchers are sometimes cancelled because a network change was detected.
  url_fetcher->SetAutomaticallyRetryOnNetworkChanges(1);
  return url_fetcher;
}

}  // namespace internal

}  // namespace ntp_snippets
