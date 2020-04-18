// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gcm/engine/registration_request.h"

#include <stddef.h>
#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/metrics/histogram.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "google_apis/gcm/base/gcm_util.h"
#include "google_apis/gcm/monitoring/gcm_stats_recorder.h"
#include "net/base/load_flags.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"
#include "url/gurl.h"

namespace gcm {

namespace {

const char kRegistrationRequestContentType[] =
    "application/x-www-form-urlencoded";

// Request constants.
const char kCategoryKey[] = "app";
const char kSubtypeKey[] = "X-subtype";
const char kDeviceIdKey[] = "device";
const char kLoginHeader[] = "AidLogin";

// Response constants.
const char kErrorPrefix[] = "Error=";
const char kTokenPrefix[] = "token=";
const char kDeviceRegistrationError[] = "PHONE_REGISTRATION_ERROR";
const char kAuthenticationFailed[] = "AUTHENTICATION_FAILED";
const char kInvalidSender[] = "INVALID_SENDER";
const char kInvalidParameters[] = "INVALID_PARAMETERS";
const char kInternalServerError[] = "InternalServerError";
const char kQuotaExceeded[] = "QUOTA_EXCEEDED";
const char kTooManyRegistrations[] = "TOO_MANY_REGISTRATIONS";

// Gets correct status from the error message.
RegistrationRequest::Status GetStatusFromError(const std::string& error) {
  if (error.find(kDeviceRegistrationError) != std::string::npos)
    return RegistrationRequest::DEVICE_REGISTRATION_ERROR;
  if (error.find(kAuthenticationFailed) != std::string::npos)
    return RegistrationRequest::AUTHENTICATION_FAILED;
  if (error.find(kInvalidSender) != std::string::npos)
    return RegistrationRequest::INVALID_SENDER;
  if (error.find(kInvalidParameters) != std::string::npos)
    return RegistrationRequest::INVALID_PARAMETERS;
  if (error.find(kInternalServerError) != std::string::npos)
    return RegistrationRequest::INTERNAL_SERVER_ERROR;
  if (error.find(kQuotaExceeded) != std::string::npos)
    return RegistrationRequest::QUOTA_EXCEEDED;
  if (error.find(kTooManyRegistrations) != std::string::npos)
    return RegistrationRequest::TOO_MANY_REGISTRATIONS;
  // Should not be reached, unless the server adds new error types.
  return RegistrationRequest::UNKNOWN_ERROR;
}

// Determines whether to retry based on the status of the last request.
bool ShouldRetryWithStatus(RegistrationRequest::Status status) {
  switch (status) {
    case RegistrationRequest::AUTHENTICATION_FAILED:
    case RegistrationRequest::DEVICE_REGISTRATION_ERROR:
    case RegistrationRequest::UNKNOWN_ERROR:
    case RegistrationRequest::URL_FETCHING_FAILED:
    case RegistrationRequest::HTTP_NOT_OK:
    case RegistrationRequest::NO_RESPONSE_BODY:
    case RegistrationRequest::RESPONSE_PARSING_FAILED:
    case RegistrationRequest::INTERNAL_SERVER_ERROR:
      return true;
    case RegistrationRequest::SUCCESS:
    case RegistrationRequest::INVALID_PARAMETERS:
    case RegistrationRequest::INVALID_SENDER:
    case RegistrationRequest::QUOTA_EXCEEDED:
    case RegistrationRequest::TOO_MANY_REGISTRATIONS:
    case RegistrationRequest::REACHED_MAX_RETRIES:
      return false;
    case RegistrationRequest::STATUS_COUNT:
      NOTREACHED();
      break;
  }
  return false;
}

}  // namespace

RegistrationRequest::RequestInfo::RequestInfo(uint64_t android_id,
                                              uint64_t security_token,
                                              const std::string& category,
                                              const std::string& subtype)
    : android_id(android_id),
      security_token(security_token),
      category(category),
      subtype(subtype) {
  DCHECK(android_id != 0UL);
  DCHECK(security_token != 0UL);
  DCHECK(!category.empty());
}

RegistrationRequest::RequestInfo::~RequestInfo() {}

RegistrationRequest::CustomRequestHandler::CustomRequestHandler() {}

RegistrationRequest::CustomRequestHandler::~CustomRequestHandler() {}

RegistrationRequest::RegistrationRequest(
    const GURL& registration_url,
    const RequestInfo& request_info,
    std::unique_ptr<CustomRequestHandler> custom_request_handler,
    const net::BackoffEntry::Policy& backoff_policy,
    const RegistrationCallback& callback,
    int max_retry_count,
    scoped_refptr<net::URLRequestContextGetter> request_context_getter,
    GCMStatsRecorder* recorder,
    const std::string& source_to_record)
    : callback_(callback),
      request_info_(request_info),
      custom_request_handler_(std::move(custom_request_handler)),
      registration_url_(registration_url),
      backoff_entry_(&backoff_policy),
      request_context_getter_(request_context_getter),
      retries_left_(max_retry_count),
      recorder_(recorder),
      source_to_record_(source_to_record),
      weak_ptr_factory_(this) {
  DCHECK_GE(max_retry_count, 0);
}

RegistrationRequest::~RegistrationRequest() {}

void RegistrationRequest::Start() {
  DCHECK(!callback_.is_null());
  DCHECK(!url_fetcher_.get());
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("gcm_registration", R"(
        semantics {
          sender: "GCM Driver"
          description:
            "Chromium interacts with Google Cloud Messaging to receive push "
            "messages for various browser features, as well as on behalf of "
            "websites and extensions. This requests Google Cloud Messaging to "
            "create a new subscription through which messages can be sent to "
            "the registering entity, through Chromium."
          trigger:
            "Immediately after a feature, website or extension creates a new "
            "registration with the GCM Driver. Repeated registration requests "
            "will be served from the cache instead."
          data:
            "The profile-bound Android ID and associated secret, and the "
            "identifiers for the feature, website or extension that is "
            "creating the registration."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "Support for interacting with Google Cloud Messaging is enabled by "
            "default, and there is no configuration option to completely "
            "disable it."
          policy_exception_justification:
            "Not implemented, considered not useful."
        })");
  url_fetcher_ = net::URLFetcher::Create(
      registration_url_, net::URLFetcher::POST, this, traffic_annotation);
  url_fetcher_->SetRequestContext(request_context_getter_.get());
  url_fetcher_->SetLoadFlags(net::LOAD_DO_NOT_SEND_COOKIES |
                             net::LOAD_DO_NOT_SAVE_COOKIES);

  std::string extra_headers;
  BuildRequestHeaders(&extra_headers);
  url_fetcher_->SetExtraRequestHeaders(extra_headers);

  std::string body;
  BuildRequestBody(&body);

  DVLOG(1) << "Performing registration for: " << request_info_.app_id();
  DVLOG(1) << "Registration request: " << body;
  url_fetcher_->SetUploadData(kRegistrationRequestContentType, body);
  recorder_->RecordRegistrationSent(request_info_.app_id(), source_to_record_);
  request_start_time_ = base::TimeTicks::Now();
  url_fetcher_->Start();
}

void RegistrationRequest::BuildRequestHeaders(std::string* extra_headers) {
  net::HttpRequestHeaders headers;
  headers.SetHeader(net::HttpRequestHeaders::kAuthorization,
                    std::string(kLoginHeader) + " " +
                        base::NumberToString(request_info_.android_id) + ":" +
                        base::NumberToString(request_info_.security_token));
  *extra_headers = headers.ToString();
}

void RegistrationRequest::BuildRequestBody(std::string* body) {
  BuildFormEncoding(kCategoryKey, request_info_.category, body);
  if (!request_info_.subtype.empty())
    BuildFormEncoding(kSubtypeKey, request_info_.subtype, body);

  BuildFormEncoding(kDeviceIdKey,
                    base::NumberToString(request_info_.android_id), body);

  DCHECK(custom_request_handler_.get());
  custom_request_handler_->BuildRequestBody(body);
}

void RegistrationRequest::RetryWithBackoff() {
  DCHECK_GT(retries_left_, 0);
  --retries_left_;
  url_fetcher_.reset();
  backoff_entry_.InformOfRequest(false);

  DVLOG(1) << "Delaying GCM registration of app: " << request_info_.app_id()
           << ", for " << backoff_entry_.GetTimeUntilRelease().InMilliseconds()
           << " milliseconds.";
  recorder_->RecordRegistrationRetryDelayed(
      request_info_.app_id(), source_to_record_,
      backoff_entry_.GetTimeUntilRelease().InMilliseconds(), retries_left_ + 1);
  DCHECK(!weak_ptr_factory_.HasWeakPtrs());
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&RegistrationRequest::Start, weak_ptr_factory_.GetWeakPtr()),
      backoff_entry_.GetTimeUntilRelease());
}

RegistrationRequest::Status RegistrationRequest::ParseResponse(
    const net::URLFetcher* source, std::string* token) {
  if (!source->GetStatus().is_success()) {
    DVLOG(1) << "Registration URL fetching failed.";
    return URL_FETCHING_FAILED;
  }

  std::string response;
  if (!source->GetResponseAsString(&response)) {
    DVLOG(1) << "Failed to get registration response body.";
    return NO_RESPONSE_BODY;
  }

  // If we are able to parse a meaningful known error, let's do so. Note that
  // some errors will have HTTP_OK response code!
  size_t error_pos = response.find(kErrorPrefix);
  if (error_pos != std::string::npos) {
    std::string error = response.substr(
        error_pos + arraysize(kErrorPrefix) - 1);
    DVLOG(1) << "Registration response error message: " << error;
    return GetStatusFromError(error);
  }

  // If we cannot tell what the error is, but at least we know response code was
  // not OK.
  if (source->GetResponseCode() != net::HTTP_OK) {
    DVLOG(1) << "Registration HTTP response code not OK: "
             << source->GetResponseCode();
    return HTTP_NOT_OK;
  }

  size_t token_pos = response.find(kTokenPrefix);
  if (token_pos != std::string::npos) {
    *token = response.substr(token_pos + arraysize(kTokenPrefix) - 1);
    return SUCCESS;
  }

  return RESPONSE_PARSING_FAILED;
}

void RegistrationRequest::OnURLFetchComplete(const net::URLFetcher* source) {
  std::string token;
  Status status = ParseResponse(source, &token);
  recorder_->RecordRegistrationResponse(request_info_.app_id(),
                                        source_to_record_, status);

  DCHECK(custom_request_handler_.get());
  custom_request_handler_->ReportUMAs(
      status,
      backoff_entry_.failure_count(),
      base::TimeTicks::Now() - request_start_time_);

  if (ShouldRetryWithStatus(status)) {
    if (retries_left_ > 0) {
      RetryWithBackoff();
      return;
    }

    status = REACHED_MAX_RETRIES;
    recorder_->RecordRegistrationResponse(request_info_.app_id(),
                                          source_to_record_, status);

    // Only REACHED_MAX_RETRIES is reported because the function will skip
    // reporting count and time when status is not SUCCESS.
    DCHECK(custom_request_handler_.get());
    custom_request_handler_->ReportUMAs(status, 0, base::TimeDelta());
  }

  callback_.Run(status, token);
}

}  // namespace gcm
