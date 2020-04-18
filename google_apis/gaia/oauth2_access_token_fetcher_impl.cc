// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gaia/oauth2_access_token_fetcher_impl.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/metrics/histogram.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "google_apis/gaia/gaia_auth_util.h"
#include "google_apis/gaia/gaia_urls.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"

using net::URLFetcher;
using net::URLFetcherDelegate;
using net::URLRequestContextGetter;
using net::URLRequestStatus;

namespace {
static const char kGetAccessTokenBodyFormat[] =
    "client_id=%s&"
    "client_secret=%s&"
    "grant_type=refresh_token&"
    "refresh_token=%s";

static const char kGetAccessTokenBodyWithScopeFormat[] =
    "client_id=%s&"
    "client_secret=%s&"
    "grant_type=refresh_token&"
    "refresh_token=%s&"
    "scope=%s";

static const char kAccessTokenKey[] = "access_token";
static const char kExpiresInKey[] = "expires_in";
static const char kErrorKey[] = "error";

// Enumerated constants for logging server responses on 400 errors, matching
// RFC 6749.
enum OAuth2ErrorCodesForHistogram {
  OAUTH2_ACCESS_ERROR_INVALID_REQUEST = 0,
  OAUTH2_ACCESS_ERROR_INVALID_CLIENT,
  OAUTH2_ACCESS_ERROR_INVALID_GRANT,
  OAUTH2_ACCESS_ERROR_UNAUTHORIZED_CLIENT,
  OAUTH2_ACCESS_ERROR_UNSUPPORTED_GRANT_TYPE,
  OAUTH2_ACCESS_ERROR_INVALID_SCOPE,
  OAUTH2_ACCESS_ERROR_UNKNOWN,
  OAUTH2_ACCESS_ERROR_COUNT
};

OAuth2ErrorCodesForHistogram OAuth2ErrorToHistogramValue(
    const std::string& error) {
  if (error == "invalid_request")
    return OAUTH2_ACCESS_ERROR_INVALID_REQUEST;
  else if (error == "invalid_client")
    return OAUTH2_ACCESS_ERROR_INVALID_CLIENT;
  else if (error == "invalid_grant")
    return OAUTH2_ACCESS_ERROR_INVALID_GRANT;
  else if (error == "unauthorized_client")
    return OAUTH2_ACCESS_ERROR_UNAUTHORIZED_CLIENT;
  else if (error == "unsupported_grant_type")
    return OAUTH2_ACCESS_ERROR_UNSUPPORTED_GRANT_TYPE;
  else if (error == "invalid_scope")
    return OAUTH2_ACCESS_ERROR_INVALID_SCOPE;

  return OAUTH2_ACCESS_ERROR_UNKNOWN;
}

static GoogleServiceAuthError CreateAuthError(URLRequestStatus status) {
  CHECK(!status.is_success());
  if (status.status() == URLRequestStatus::CANCELED) {
    return GoogleServiceAuthError(GoogleServiceAuthError::REQUEST_CANCELED);
  } else {
    DLOG(WARNING) << "Could not reach Google Accounts servers: errno "
                  << status.error();
    return GoogleServiceAuthError::FromConnectionError(status.error());
  }
}

static std::unique_ptr<URLFetcher> CreateFetcher(
    URLRequestContextGetter* getter,
    const GURL& url,
    const std::string& body,
    URLFetcherDelegate* delegate) {
  bool empty_body = body.empty();
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("oauth2_access_token_fetcher", R"(
        semantics {
          sender: "OAuth 2.0 Access Token Fetcher"
          description:
            "This request is used by the Token Service to fetch an OAuth 2.0 "
            "access token for a known Google account."
          trigger:
            "This request can be triggered at any moment when any service "
            "requests an OAuth 2.0 access token from the Token Service."
          data:
            "Chrome OAuth 2.0 client id and secret, the set of OAuth 2.0 "
            "scopes and the OAuth 2.0 refresh token."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled in settings, but if user signs "
            "out of Chrome, this request would not be made."
          chrome_policy {
            SigninAllowed {
              policy_options {mode: MANDATORY}
              SigninAllowed: false
            }
          }
        })");
  std::unique_ptr<URLFetcher> result = net::URLFetcher::Create(
      0, url, empty_body ? URLFetcher::GET : URLFetcher::POST, delegate,
      traffic_annotation);

  gaia::MarkURLFetcherAsGaia(result.get());
  result->SetRequestContext(getter);
  result->SetLoadFlags(net::LOAD_DO_NOT_SEND_COOKIES |
                       net::LOAD_DO_NOT_SAVE_COOKIES);
  // Fetchers are sometimes cancelled because a network change was detected,
  // especially at startup and after sign-in on ChromeOS. Retrying once should
  // be enough in those cases; let the fetcher retry up to 3 times just in case.
  // http://crbug.com/163710
  result->SetAutomaticallyRetryOnNetworkChanges(3);

  if (!empty_body)
    result->SetUploadData("application/x-www-form-urlencoded", body);

  return result;
}

std::unique_ptr<base::DictionaryValue> ParseGetAccessTokenResponse(
    const net::URLFetcher* source) {
  CHECK(source);

  std::string data;
  source->GetResponseAsString(&data);
  std::unique_ptr<base::Value> value = base::JSONReader::Read(data);
  if (!value.get() || value->type() != base::Value::Type::DICTIONARY)
    value.reset();

  return std::unique_ptr<base::DictionaryValue>(
      static_cast<base::DictionaryValue*>(value.release()));
}

}  // namespace

OAuth2AccessTokenFetcherImpl::OAuth2AccessTokenFetcherImpl(
    OAuth2AccessTokenConsumer* consumer,
    net::URLRequestContextGetter* getter,
    const std::string& refresh_token)
    : OAuth2AccessTokenFetcher(consumer),
      getter_(getter),
      refresh_token_(refresh_token),
      state_(INITIAL) {}

OAuth2AccessTokenFetcherImpl::~OAuth2AccessTokenFetcherImpl() {}

void OAuth2AccessTokenFetcherImpl::CancelRequest() { fetcher_.reset(); }

void OAuth2AccessTokenFetcherImpl::Start(
    const std::string& client_id,
    const std::string& client_secret,
    const std::vector<std::string>& scopes) {
  client_id_ = client_id;
  client_secret_ = client_secret;
  scopes_ = scopes;
  StartGetAccessToken();
}

void OAuth2AccessTokenFetcherImpl::StartGetAccessToken() {
  CHECK_EQ(INITIAL, state_);
  state_ = GET_ACCESS_TOKEN_STARTED;
  fetcher_ = CreateFetcher(getter_, MakeGetAccessTokenUrl(),
                           MakeGetAccessTokenBody(client_id_, client_secret_,
                                                  refresh_token_, scopes_),
                           this);
  fetcher_->Start();  // OnURLFetchComplete will be called.
}

void OAuth2AccessTokenFetcherImpl::EndGetAccessToken(
    const net::URLFetcher* source) {
  CHECK_EQ(GET_ACCESS_TOKEN_STARTED, state_);
  state_ = GET_ACCESS_TOKEN_DONE;

  URLRequestStatus status = source->GetStatus();
  int histogram_value =
      status.is_success() ? source->GetResponseCode() : status.error();
  base::UmaHistogramSparse("Gaia.ResponseCodesForOAuth2AccessToken",
                           histogram_value);
  if (!status.is_success()) {
    OnGetTokenFailure(CreateAuthError(status));
    return;
  }

  switch (source->GetResponseCode()) {
    case net::HTTP_OK:
      break;
    case net::HTTP_FORBIDDEN:
      // HTTP_FORBIDDEN (403) is treated as temporary error, because it may be
      // '403 Rate Limit Exeeded.'
      OnGetTokenFailure(
          GoogleServiceAuthError(GoogleServiceAuthError::SERVICE_UNAVAILABLE));
      return;
    case net::HTTP_BAD_REQUEST: {
      // HTTP_BAD_REQUEST (400) usually contains error as per
      // http://tools.ietf.org/html/rfc6749#section-5.2.
      std::string gaia_error;
      if (!ParseGetAccessTokenFailureResponse(source, &gaia_error)) {
        OnGetTokenFailure(
            GoogleServiceAuthError(GoogleServiceAuthError::SERVICE_ERROR));
        return;
      }

      OAuth2ErrorCodesForHistogram access_error(
          OAuth2ErrorToHistogramValue(gaia_error));
      UMA_HISTOGRAM_ENUMERATION("Gaia.BadRequestTypeForOAuth2AccessToken",
                                access_error,
                                OAUTH2_ACCESS_ERROR_COUNT);

      OnGetTokenFailure(
          access_error == OAUTH2_ACCESS_ERROR_INVALID_GRANT
              ? GoogleServiceAuthError::FromInvalidGaiaCredentialsReason(
                    GoogleServiceAuthError::InvalidGaiaCredentialsReason::
                        CREDENTIALS_REJECTED_BY_SERVER)
              : GoogleServiceAuthError(GoogleServiceAuthError::SERVICE_ERROR));
      return;
    }
    default: {
      if (source->GetResponseCode() >= net::HTTP_INTERNAL_SERVER_ERROR) {
        // 5xx is always treated as transient.
        OnGetTokenFailure(GoogleServiceAuthError(
            GoogleServiceAuthError::SERVICE_UNAVAILABLE));
      } else {
        // The other errors are treated as permanent error.
        DLOG(ERROR) << "Unexpected persistent error: http_status="
                    << source->GetResponseCode();
        OnGetTokenFailure(
            GoogleServiceAuthError::FromInvalidGaiaCredentialsReason(
                GoogleServiceAuthError::InvalidGaiaCredentialsReason::
                    CREDENTIALS_REJECTED_BY_SERVER));
      }
      return;
    }
  }

  // The request was successfully fetched and it returned OK.
  // Parse out the access token and the expiration time.
  std::string access_token;
  int expires_in;
  if (!ParseGetAccessTokenSuccessResponse(source, &access_token, &expires_in)) {
    DLOG(WARNING) << "Response doesn't match expected format";
    OnGetTokenFailure(
        GoogleServiceAuthError(GoogleServiceAuthError::SERVICE_UNAVAILABLE));
    return;
  }
  // The token will expire in |expires_in| seconds. Take a 10% error margin to
  // prevent reusing a token too close to its expiration date.
  OnGetTokenSuccess(
      access_token,
      base::Time::Now() + base::TimeDelta::FromSeconds(9 * expires_in / 10));
}

void OAuth2AccessTokenFetcherImpl::OnGetTokenSuccess(
    const std::string& access_token,
    const base::Time& expiration_time) {
  FireOnGetTokenSuccess(access_token, expiration_time);
}

void OAuth2AccessTokenFetcherImpl::OnGetTokenFailure(
    const GoogleServiceAuthError& error) {
  state_ = ERROR_STATE;
  FireOnGetTokenFailure(error);
}

void OAuth2AccessTokenFetcherImpl::OnURLFetchComplete(
    const net::URLFetcher* source) {
  CHECK(source);
  CHECK(state_ == GET_ACCESS_TOKEN_STARTED);
  EndGetAccessToken(source);
}

// static
GURL OAuth2AccessTokenFetcherImpl::MakeGetAccessTokenUrl() {
  return GaiaUrls::GetInstance()->oauth2_token_url();
}

// static
std::string OAuth2AccessTokenFetcherImpl::MakeGetAccessTokenBody(
    const std::string& client_id,
    const std::string& client_secret,
    const std::string& refresh_token,
    const std::vector<std::string>& scopes) {
  std::string enc_client_id = net::EscapeUrlEncodedData(client_id, true);
  std::string enc_client_secret =
      net::EscapeUrlEncodedData(client_secret, true);
  std::string enc_refresh_token =
      net::EscapeUrlEncodedData(refresh_token, true);
  if (scopes.empty()) {
    return base::StringPrintf(kGetAccessTokenBodyFormat,
                              enc_client_id.c_str(),
                              enc_client_secret.c_str(),
                              enc_refresh_token.c_str());
  } else {
    std::string scopes_string = base::JoinString(scopes, " ");
    return base::StringPrintf(
        kGetAccessTokenBodyWithScopeFormat,
        enc_client_id.c_str(),
        enc_client_secret.c_str(),
        enc_refresh_token.c_str(),
        net::EscapeUrlEncodedData(scopes_string, true).c_str());
  }
}

// static
bool OAuth2AccessTokenFetcherImpl::ParseGetAccessTokenSuccessResponse(
    const net::URLFetcher* source,
    std::string* access_token,
    int* expires_in) {
  CHECK(access_token);
  std::unique_ptr<base::DictionaryValue> value =
      ParseGetAccessTokenResponse(source);
  if (value.get() == NULL)
    return false;

  return value->GetString(kAccessTokenKey, access_token) &&
         value->GetInteger(kExpiresInKey, expires_in);
}

// static
bool OAuth2AccessTokenFetcherImpl::ParseGetAccessTokenFailureResponse(
    const net::URLFetcher* source,
    std::string* error) {
  CHECK(error);
  std::unique_ptr<base::DictionaryValue> value =
      ParseGetAccessTokenResponse(source);
  if (value.get() == NULL)
    return false;
  return value->GetString(kErrorKey, error);
}
