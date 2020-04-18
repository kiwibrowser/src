// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gcm/engine/unregistration_request.h"

#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "google_apis/gcm/base/gcm_util.h"
#include "google_apis/gcm/monitoring/gcm_stats_recorder.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"

namespace gcm {

namespace {

const char kRequestContentType[] = "application/x-www-form-urlencoded";

// Request constants.
const char kCategoryKey[] = "app";
const char kSubtypeKey[] = "X-subtype";
const char kDeleteKey[] = "delete";
const char kDeleteValue[] = "true";
const char kDeviceIdKey[] = "device";
const char kLoginHeader[] = "AidLogin";

// Response constants.
const char kErrorPrefix[] = "Error=";
const char kInvalidParameters[] = "INVALID_PARAMETERS";
const char kInternalServerError[] = "InternalServerError";
const char kDeviceRegistrationError[] = "PHONE_REGISTRATION_ERROR";

// Gets correct status from the error message.
UnregistrationRequest::Status GetStatusFromError(const std::string& error) {
  if (error.find(kInvalidParameters) != std::string::npos)
    return UnregistrationRequest::INVALID_PARAMETERS;
  if (error.find(kInternalServerError) != std::string::npos)
    return UnregistrationRequest::INTERNAL_SERVER_ERROR;
  if (error.find(kDeviceRegistrationError) != std::string::npos)
    return UnregistrationRequest::DEVICE_REGISTRATION_ERROR;
  // Should not be reached, unless the server adds new error types.
  return UnregistrationRequest::UNKNOWN_ERROR;
}

// Determines whether to retry based on the status of the last request.
bool ShouldRetryWithStatus(UnregistrationRequest::Status status) {
  switch (status) {
    case UnregistrationRequest::URL_FETCHING_FAILED:
    case UnregistrationRequest::NO_RESPONSE_BODY:
    case UnregistrationRequest::RESPONSE_PARSING_FAILED:
    case UnregistrationRequest::INCORRECT_APP_ID:
    case UnregistrationRequest::SERVICE_UNAVAILABLE:
    case UnregistrationRequest::INTERNAL_SERVER_ERROR:
    case UnregistrationRequest::HTTP_NOT_OK:
      return true;
    case UnregistrationRequest::SUCCESS:
    case UnregistrationRequest::INVALID_PARAMETERS:
    case UnregistrationRequest::DEVICE_REGISTRATION_ERROR:
    case UnregistrationRequest::UNKNOWN_ERROR:
    case UnregistrationRequest::REACHED_MAX_RETRIES:
      return false;
    case UnregistrationRequest::UNREGISTRATION_STATUS_COUNT:
      NOTREACHED();
      break;
  }
  return false;
}

}  // namespace

UnregistrationRequest::RequestInfo::RequestInfo(uint64_t android_id,
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

UnregistrationRequest::RequestInfo::~RequestInfo() {}

UnregistrationRequest::CustomRequestHandler::CustomRequestHandler() {}

UnregistrationRequest::CustomRequestHandler::~CustomRequestHandler() {}

UnregistrationRequest::UnregistrationRequest(
    const GURL& registration_url,
    const RequestInfo& request_info,
    std::unique_ptr<CustomRequestHandler> custom_request_handler,
    const net::BackoffEntry::Policy& backoff_policy,
    const UnregistrationCallback& callback,
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

UnregistrationRequest::~UnregistrationRequest() {}

void UnregistrationRequest::Start() {
  DCHECK(!callback_.is_null());
  DCHECK(!url_fetcher_.get());
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("gcm_unregistration", R"(
        semantics {
          sender: "GCM Driver"
          description:
            "Chromium interacts with Google Cloud Messaging to receive push "
            "messages for various browser features, as well as on behalf of "
            "websites and extensions. This requests Google Cloud Messaging to "
            "invalidate the included registration so that it can no longer be "
            "used to distribute messages to Chromium."
          trigger:
            "Immediately after a feature, website or extension removes a "
            "registration they previously created with the GCM Driver."
          data:
            "The profile-bound Android ID and associated secret, and the "
            "identifiers for the feature, website or extension that is "
            "removing the registration."
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

  DVLOG(1) << "Unregistration request: " << body;
  url_fetcher_->SetUploadData(kRequestContentType, body);

  DVLOG(1) << "Performing unregistration for: " << request_info_.app_id();
  recorder_->RecordUnregistrationSent(request_info_.app_id(),
                                      source_to_record_);
  request_start_time_ = base::TimeTicks::Now();
  url_fetcher_->Start();
}

void UnregistrationRequest::BuildRequestHeaders(std::string* extra_headers) {
  net::HttpRequestHeaders headers;
  headers.SetHeader(net::HttpRequestHeaders::kAuthorization,
                    std::string(kLoginHeader) + " " +
                        base::NumberToString(request_info_.android_id) + ":" +
                        base::NumberToString(request_info_.security_token));
  *extra_headers = headers.ToString();
}

void UnregistrationRequest::BuildRequestBody(std::string* body) {
  BuildFormEncoding(kCategoryKey, request_info_.category, body);
  if (!request_info_.subtype.empty())
    BuildFormEncoding(kSubtypeKey, request_info_.subtype, body);

  BuildFormEncoding(kDeviceIdKey,
                    base::NumberToString(request_info_.android_id), body);
  BuildFormEncoding(kDeleteKey, kDeleteValue, body);

  DCHECK(custom_request_handler_.get());
  custom_request_handler_->BuildRequestBody(body);
}

UnregistrationRequest::Status UnregistrationRequest::ParseResponse(
    const net::URLFetcher* source) {
  if (!source->GetStatus().is_success()) {
    DVLOG(1) << "Unregistration URL fetching failed.";
    return URL_FETCHING_FAILED;
  }

  std::string response;
  if (!source->GetResponseAsString(&response)) {
    DVLOG(1) << "Failed to get unregistration response body.";
    return NO_RESPONSE_BODY;
  }

  // If we are able to parse a meaningful known error, let's do so. Note that
  // some errors will have HTTP_OK response code!
  if (response.find(kErrorPrefix) != std::string::npos) {
    std::string error = response.substr(response.find(kErrorPrefix) +
                                        arraysize(kErrorPrefix) - 1);
    DVLOG(1) << "Unregistration response error message: " << error;
    return GetStatusFromError(error);
  }

  net::HttpStatusCode response_status = static_cast<net::HttpStatusCode>(
      source->GetResponseCode());
  if (response_status != net::HTTP_OK) {
    DVLOG(1) << "Unregistration HTTP response code not OK: " << response_status;
    if (response_status == net::HTTP_SERVICE_UNAVAILABLE)
      return SERVICE_UNAVAILABLE;
    if (response_status == net::HTTP_INTERNAL_SERVER_ERROR)
      return INTERNAL_SERVER_ERROR;
    return HTTP_NOT_OK;
  }

  DCHECK(custom_request_handler_.get());
  return custom_request_handler_->ParseResponse(response);
}

void UnregistrationRequest::RetryWithBackoff() {
  DCHECK_GT(retries_left_, 0);
  --retries_left_;
  url_fetcher_.reset();
  backoff_entry_.InformOfRequest(false);

  DVLOG(1) << "Delaying GCM unregistration of app: " << request_info_.app_id()
           << ", for " << backoff_entry_.GetTimeUntilRelease().InMilliseconds()
           << " milliseconds.";
  recorder_->RecordUnregistrationRetryDelayed(
      request_info_.app_id(), source_to_record_,
      backoff_entry_.GetTimeUntilRelease().InMilliseconds(), retries_left_ + 1);
  DCHECK(!weak_ptr_factory_.HasWeakPtrs());
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&UnregistrationRequest::Start, weak_ptr_factory_.GetWeakPtr()),
      backoff_entry_.GetTimeUntilRelease());
}

void UnregistrationRequest::OnURLFetchComplete(const net::URLFetcher* source) {
  UnregistrationRequest::Status status = ParseResponse(source);

  DVLOG(1) << "UnregistrationRequestStatus: " << status;

  DCHECK(custom_request_handler_.get());
  custom_request_handler_->ReportUMAs(
      status,
      backoff_entry_.failure_count(),
      base::TimeTicks::Now() - request_start_time_);

  recorder_->RecordUnregistrationResponse(request_info_.app_id(),
                                          source_to_record_, status);

  if (ShouldRetryWithStatus(status)) {
    if (retries_left_ > 0) {
      RetryWithBackoff();
      return;
    }

    status = REACHED_MAX_RETRIES;
    recorder_->RecordUnregistrationResponse(request_info_.app_id(),
                                            source_to_record_, status);

    // Only REACHED_MAX_RETRIES is reported because the function will skip
    // reporting count and time when status is not SUCCESS.
    DCHECK(custom_request_handler_.get());
    custom_request_handler_->ReportUMAs(status, 0, base::TimeDelta());
  }

  callback_.Run(status);
}

}  // namespace gcm
