// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gaia/oauth2_api_call_flow.h"

#include <string>
#include <vector>

#include "base/strings/stringprintf.h"
#include "google_apis/gaia/gaia_auth_util.h"
#include "google_apis/gaia/gaia_urls.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"

using net::URLFetcher;
using net::URLFetcherDelegate;
using net::URLRequestContextGetter;
using net::URLRequestStatus;

namespace {
static const char kAuthorizationHeaderFormat[] =
    "Authorization: Bearer %s";

static std::string MakeAuthorizationHeader(const std::string& auth_token) {
  return base::StringPrintf(kAuthorizationHeaderFormat, auth_token.c_str());
}
}  // namespace

OAuth2ApiCallFlow::OAuth2ApiCallFlow() : state_(INITIAL) {
}

OAuth2ApiCallFlow::~OAuth2ApiCallFlow() {}

void OAuth2ApiCallFlow::Start(net::URLRequestContextGetter* context,
                              const std::string& access_token) {
  CHECK(state_ == INITIAL);
  state_ = API_CALL_STARTED;

  url_fetcher_ = CreateURLFetcher(context, access_token);
  url_fetcher_->Start();  // OnURLFetchComplete will be called.
}

void OAuth2ApiCallFlow::EndApiCall(const net::URLFetcher* source) {
  CHECK_EQ(API_CALL_STARTED, state_);

  URLRequestStatus status = source->GetStatus();
  int status_code = source->GetResponseCode();
  if (!status.is_success() ||
      (status_code != net::HTTP_OK && status_code != net::HTTP_NO_CONTENT)) {
    state_ = ERROR_STATE;
    ProcessApiCallFailure(source);
  } else {
    state_ = API_CALL_DONE;
    ProcessApiCallSuccess(source);
  }
}

std::string OAuth2ApiCallFlow::CreateApiCallBodyContentType() {
  return "application/x-www-form-urlencoded";
}

net::URLFetcher::RequestType OAuth2ApiCallFlow::GetRequestTypeForBody(
    const std::string& body) {
  return body.empty() ? URLFetcher::GET : URLFetcher::POST;
}

void OAuth2ApiCallFlow::OnURLFetchComplete(const net::URLFetcher* source) {
  CHECK(source);
  CHECK_EQ(API_CALL_STARTED, state_);
  EndApiCall(source);
}

std::unique_ptr<URLFetcher> OAuth2ApiCallFlow::CreateURLFetcher(
    net::URLRequestContextGetter* context,
    const std::string& access_token) {
  std::string body = CreateApiCallBody();
  net::URLFetcher::RequestType request_type = GetRequestTypeForBody(body);
  net::NetworkTrafficAnnotationTag traffic_annotation =
      CompleteNetworkTrafficAnnotation("oauth2_api_call_flow",
                                       GetNetworkTrafficAnnotationTag(), R"(
          policy {
            cookies_allowed: NO
          })");
  std::unique_ptr<URLFetcher> result = net::URLFetcher::Create(
      0, CreateApiCallUrl(), request_type, this, traffic_annotation);

  gaia::MarkURLFetcherAsGaia(result.get());
  result->SetRequestContext(context);
  result->SetLoadFlags(net::LOAD_DO_NOT_SEND_COOKIES |
                       net::LOAD_DO_NOT_SAVE_COOKIES);
  result->AddExtraRequestHeader(MakeAuthorizationHeader(access_token));
  // Fetchers are sometimes cancelled because a network change was detected,
  // especially at startup and after sign-in on ChromeOS. Retrying once should
  // be enough in those cases; let the fetcher retry up to 3 times just in case.
  // http://crbug.com/163710
  result->SetAutomaticallyRetryOnNetworkChanges(3);

  // Even if the the body is empty, we still set the Content-Type because an
  // empty string may be a meaningful value. For example, a Protocol Buffer
  // message with only default values will be serialized as an empty string.
  if (request_type != net::URLFetcher::GET)
    result->SetUploadData(CreateApiCallBodyContentType(), body);

  return result;
}
