// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/timezone/timezone_request.h"

#include <stddef.h>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "google_apis/google_api_keys.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"

namespace chromeos {

namespace {

const char kDefaultTimezoneProviderUrl[] =
    "https://maps.googleapis.com/maps/api/timezone/json?";

const char kKeyString[] = "key";
// Language parameter is unsupported for now.
// const char kLanguageString[] = "language";
const char kLocationString[] = "location";
const char kSensorString[] = "sensor";
const char kTimestampString[] = "timestamp";

const char kDstOffsetString[] = "dstOffset";
const char kRawOffsetString[] = "rawOffset";
const char kTimeZoneIdString[] = "timeZoneId";
const char kTimeZoneNameString[] = "timeZoneName";
const char kStatusString[] = "status";
const char kErrorMessageString[] = "error_message";

// Sleep between timezone request retry on HTTP error.
const unsigned int kResolveTimeZoneRetrySleepOnServerErrorSeconds = 5;

// Sleep between timezone request retry on bad server response.
const unsigned int kResolveTimeZoneRetrySleepBadResponseSeconds = 10;

struct StatusString2Enum {
  const char* string;
  TimeZoneResponseData::Status value;
};

const StatusString2Enum statusString2Enum[] = {
    {"OK", TimeZoneResponseData::OK},
    {"INVALID_REQUEST", TimeZoneResponseData::INVALID_REQUEST},
    {"OVER_QUERY_LIMIT", TimeZoneResponseData::OVER_QUERY_LIMIT},
    {"REQUEST_DENIED", TimeZoneResponseData::REQUEST_DENIED},
    {"UNKNOWN_ERROR", TimeZoneResponseData::UNKNOWN_ERROR},
    {"ZERO_RESULTS", TimeZoneResponseData::ZERO_RESULTS}, };

enum TimeZoneRequestEvent {
  // NOTE: Do not renumber these as that would confuse interpretation of
  // previously logged data. When making changes, also update the enum list
  // in tools/metrics/histograms/histograms.xml to keep it in sync.
  TIMEZONE_REQUEST_EVENT_REQUEST_START = 0,
  TIMEZONE_REQUEST_EVENT_RESPONSE_SUCCESS = 1,
  TIMEZONE_REQUEST_EVENT_RESPONSE_NOT_OK = 2,
  TIMEZONE_REQUEST_EVENT_RESPONSE_EMPTY = 3,
  TIMEZONE_REQUEST_EVENT_RESPONSE_MALFORMED = 4,

  // NOTE: Add entries only immediately above this line.
  TIMEZONE_REQUEST_EVENT_COUNT = 5
};

enum TimeZoneRequestResult {
  // NOTE: Do not renumber these as that would confuse interpretation of
  // previously logged data. When making changes, also update the enum list
  // in tools/metrics/histograms/histograms.xml to keep it in sync.
  TIMEZONE_REQUEST_RESULT_SUCCESS = 0,
  TIMEZONE_REQUEST_RESULT_FAILURE = 1,
  TIMEZONE_REQUEST_RESULT_SERVER_ERROR = 2,
  TIMEZONE_REQUEST_RESULT_CANCELLED = 3,

  // NOTE: Add entries only immediately above this line.
  TIMEZONE_REQUEST_RESULT_COUNT = 4
};

// Too many requests (more than 1) mean there is a problem in implementation.
void RecordUmaEvent(TimeZoneRequestEvent event) {
  UMA_HISTOGRAM_ENUMERATION(
      "TimeZone.TimeZoneRequest.Event", event, TIMEZONE_REQUEST_EVENT_COUNT);
}

void RecordUmaResponseCode(int code) {
  base::UmaHistogramSparse("TimeZone.TimeZoneRequest.ResponseCode", code);
}

// Slow timezone resolve leads to bad user experience.
void RecordUmaResponseTime(base::TimeDelta elapsed, bool success) {
  if (success) {
    UMA_HISTOGRAM_TIMES("TimeZone.TimeZoneRequest.ResponseSuccessTime",
                        elapsed);
  } else {
    UMA_HISTOGRAM_TIMES("TimeZone.TimeZoneRequest.ResponseFailureTime",
                        elapsed);
  }
}

void RecordUmaResult(TimeZoneRequestResult result, unsigned retries) {
  UMA_HISTOGRAM_ENUMERATION(
      "TimeZone.TimeZoneRequest.Result", result, TIMEZONE_REQUEST_RESULT_COUNT);
  base::UmaHistogramSparse("TimeZone.TimeZoneRequest.Retries", retries);
}

// Creates the request url to send to the server.
// |sensor| if this location was determined using hardware sensor.
GURL TimeZoneRequestURL(const GURL& url,
                        const Geoposition& geoposition,
                        bool sensor) {
  std::string query(url.query());
  query += base::StringPrintf(
      "%s=%f,%f", kLocationString, geoposition.latitude, geoposition.longitude);
  if (url == DefaultTimezoneProviderURL()) {
    std::string api_key = google_apis::GetAPIKey();
    if (!api_key.empty()) {
      query += "&";
      query += kKeyString;
      query += "=";
      query += net::EscapeQueryParamValue(api_key, true);
    }
  }
  if (!geoposition.timestamp.is_null()) {
    query += base::StringPrintf(
        "&%s=%ld", kTimestampString, geoposition.timestamp.ToTimeT());
  }
  query += "&";
  query += kSensorString;
  query += "=";
  query += (sensor ? "true" : "false");

  GURL::Replacements replacements;
  replacements.SetQueryStr(query);
  return url.ReplaceComponents(replacements);
}

void PrintTimeZoneError(const GURL& server_url,
                        const std::string& message,
                        TimeZoneResponseData* timezone) {
  timezone->status = TimeZoneResponseData::REQUEST_ERROR;
  timezone->error_message =
      base::StringPrintf("TimeZone provider at '%s' : %s.",
                         server_url.GetOrigin().spec().c_str(),
                         message.c_str());
  LOG(WARNING) << "TimeZoneRequest::GetTimeZoneFromResponse() : "
               << timezone->error_message;
}

// Parses the server response body. Returns true if parsing was successful.
// Sets |*timezone| to the parsed TimeZone if a valid timezone was received,
// otherwise leaves it unchanged.
bool ParseServerResponse(const GURL& server_url,
                         const std::string& response_body,
                         TimeZoneResponseData* timezone) {
  DCHECK(timezone);

  if (response_body.empty()) {
    PrintTimeZoneError(server_url, "Server returned empty response", timezone);
    RecordUmaEvent(TIMEZONE_REQUEST_EVENT_RESPONSE_EMPTY);
    return false;
  }
  VLOG(1) << "TimeZoneRequest::ParseServerResponse() : Parsing response "
          << response_body;

  // Parse the response, ignoring comments.
  std::string error_msg;
  std::unique_ptr<base::Value> response_value =
      base::JSONReader::ReadAndReturnError(response_body, base::JSON_PARSE_RFC,
                                           NULL, &error_msg);
  if (response_value == NULL) {
    PrintTimeZoneError(server_url, "JSONReader failed: " + error_msg, timezone);
    RecordUmaEvent(TIMEZONE_REQUEST_EVENT_RESPONSE_MALFORMED);
    return false;
  }

  const base::DictionaryValue* response_object = NULL;
  if (!response_value->GetAsDictionary(&response_object)) {
    PrintTimeZoneError(
        server_url,
        "Unexpected response type : " +
            base::StringPrintf(
                "%u", static_cast<unsigned int>(response_value->type())),
        timezone);
    RecordUmaEvent(TIMEZONE_REQUEST_EVENT_RESPONSE_MALFORMED);
    return false;
  }

  std::string status;

  if (!response_object->GetStringWithoutPathExpansion(kStatusString, &status)) {
    PrintTimeZoneError(server_url, "Missing status attribute.", timezone);
    RecordUmaEvent(TIMEZONE_REQUEST_EVENT_RESPONSE_MALFORMED);
    return false;
  }

  bool found = false;
  for (size_t i = 0; i < arraysize(statusString2Enum); ++i) {
    if (status != statusString2Enum[i].string)
      continue;

    timezone->status = statusString2Enum[i].value;
    found = true;
    break;
  }

  if (!found) {
    PrintTimeZoneError(
        server_url, "Bad status attribute value: '" + status + "'", timezone);
    RecordUmaEvent(TIMEZONE_REQUEST_EVENT_RESPONSE_MALFORMED);
    return false;
  }

  const bool status_ok = (timezone->status == TimeZoneResponseData::OK);

  if (!response_object->GetDoubleWithoutPathExpansion(kDstOffsetString,
                                                      &timezone->dstOffset) &&
      status_ok) {
    PrintTimeZoneError(server_url, "Missing dstOffset attribute.", timezone);
    RecordUmaEvent(TIMEZONE_REQUEST_EVENT_RESPONSE_MALFORMED);
    return false;
  }

  if (!response_object->GetDoubleWithoutPathExpansion(kRawOffsetString,
                                                      &timezone->rawOffset) &&
      status_ok) {
    PrintTimeZoneError(server_url, "Missing rawOffset attribute.", timezone);
    RecordUmaEvent(TIMEZONE_REQUEST_EVENT_RESPONSE_MALFORMED);
    return false;
  }

  if (!response_object->GetStringWithoutPathExpansion(kTimeZoneIdString,
                                                      &timezone->timeZoneId) &&
      status_ok) {
    PrintTimeZoneError(server_url, "Missing timeZoneId attribute.", timezone);
    RecordUmaEvent(TIMEZONE_REQUEST_EVENT_RESPONSE_MALFORMED);
    return false;
  }

  if (!response_object->GetStringWithoutPathExpansion(
          kTimeZoneNameString, &timezone->timeZoneName) &&
      status_ok) {
    PrintTimeZoneError(server_url, "Missing timeZoneName attribute.", timezone);
    RecordUmaEvent(TIMEZONE_REQUEST_EVENT_RESPONSE_MALFORMED);
    return false;
  }

  // "error_message" field is optional. Ignore result.
  response_object->GetStringWithoutPathExpansion(kErrorMessageString,
                                                 &timezone->error_message);

  return true;
}

// Attempts to extract a position from the response. Detects and indicates
// various failure cases.
std::unique_ptr<TimeZoneResponseData> GetTimeZoneFromResponse(
    bool http_success,
    int status_code,
    const std::string& response_body,
    const GURL& server_url) {
  std::unique_ptr<TimeZoneResponseData> timezone(new TimeZoneResponseData);

  // HttpPost can fail for a number of reasons. Most likely this is because
  // we're offline, or there was no response.
  if (!http_success) {
    PrintTimeZoneError(server_url, "No response received", timezone.get());
    RecordUmaEvent(TIMEZONE_REQUEST_EVENT_RESPONSE_EMPTY);
    return timezone;
  }
  if (status_code != net::HTTP_OK) {
    std::string message = "Returned error code ";
    message += base::IntToString(status_code);
    PrintTimeZoneError(server_url, message, timezone.get());
    RecordUmaEvent(TIMEZONE_REQUEST_EVENT_RESPONSE_NOT_OK);
    return timezone;
  }

  if (!ParseServerResponse(server_url, response_body, timezone.get()))
    return timezone;

  RecordUmaEvent(TIMEZONE_REQUEST_EVENT_RESPONSE_SUCCESS);
  return timezone;
}

}  // namespace

TimeZoneResponseData::TimeZoneResponseData()
    : dstOffset(0), rawOffset(0), status(ZERO_RESULTS) {
}

GURL DefaultTimezoneProviderURL() {
  return GURL(kDefaultTimezoneProviderUrl);
}

TimeZoneRequest::TimeZoneRequest(
    net::URLRequestContextGetter* url_context_getter,
    const GURL& service_url,
    const Geoposition& geoposition,
    base::TimeDelta retry_timeout)
    : url_context_getter_(url_context_getter),
      service_url_(service_url),
      geoposition_(geoposition),
      retry_timeout_abs_(base::Time::Now() + retry_timeout),
      retry_sleep_on_server_error_(base::TimeDelta::FromSeconds(
          kResolveTimeZoneRetrySleepOnServerErrorSeconds)),
      retry_sleep_on_bad_response_(base::TimeDelta::FromSeconds(
          kResolveTimeZoneRetrySleepBadResponseSeconds)),
      retries_(0) {
}

TimeZoneRequest::~TimeZoneRequest() {
  DCHECK(thread_checker_.CalledOnValidThread());

  // If callback is not empty, request is cancelled.
  if (!callback_.is_null()) {
    RecordUmaResponseTime(base::Time::Now() - request_started_at_, false);
    RecordUmaResult(TIMEZONE_REQUEST_RESULT_CANCELLED, retries_);
  }
}

void TimeZoneRequest::StartRequest() {
  DCHECK(thread_checker_.CalledOnValidThread());
  RecordUmaEvent(TIMEZONE_REQUEST_EVENT_REQUEST_START);
  request_started_at_ = base::Time::Now();
  ++retries_;

  url_fetcher_ =
      net::URLFetcher::Create(request_url_, net::URLFetcher::GET, this);
  url_fetcher_->SetRequestContext(url_context_getter_.get());
  url_fetcher_->SetLoadFlags(net::LOAD_BYPASS_CACHE |
                             net::LOAD_DISABLE_CACHE |
                             net::LOAD_DO_NOT_SAVE_COOKIES |
                             net::LOAD_DO_NOT_SEND_COOKIES |
                             net::LOAD_DO_NOT_SEND_AUTH_DATA);
  url_fetcher_->Start();
}

void TimeZoneRequest::MakeRequest(TimeZoneResponseCallback callback) {
  callback_ = callback;
  request_url_ =
      TimeZoneRequestURL(service_url_, geoposition_, false /* sensor */);
  StartRequest();
}

void TimeZoneRequest::Retry(bool server_error) {
  const base::TimeDelta delay(server_error ? retry_sleep_on_server_error_
                                           : retry_sleep_on_bad_response_);
  timezone_request_scheduled_.Start(
      FROM_HERE, delay, this, &TimeZoneRequest::StartRequest);
}

void TimeZoneRequest::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK_EQ(url_fetcher_.get(), source);

  net::URLRequestStatus status = source->GetStatus();
  int response_code = source->GetResponseCode();
  RecordUmaResponseCode(response_code);

  std::string data;
  source->GetResponseAsString(&data);
  std::unique_ptr<TimeZoneResponseData> timezone = GetTimeZoneFromResponse(
      status.is_success(), response_code, data, source->GetURL());
  const bool server_error =
      !status.is_success() || (response_code >= 500 && response_code < 600);
  url_fetcher_.reset();

  DVLOG(1) << "TimeZoneRequest::OnURLFetchComplete(): timezone={"
           << timezone->ToStringForDebug() << "}";

  const base::Time now = base::Time::Now();
  const bool retry_timeout = (now >= retry_timeout_abs_);

  const bool success = (timezone->status == TimeZoneResponseData::OK);
  if (!success && !retry_timeout) {
    Retry(server_error);
    return;
  }
  RecordUmaResponseTime(base::Time::Now() - request_started_at_, success);

  const TimeZoneRequestResult result =
      (server_error ? TIMEZONE_REQUEST_RESULT_SERVER_ERROR
                    : (success ? TIMEZONE_REQUEST_RESULT_SUCCESS
                               : TIMEZONE_REQUEST_RESULT_FAILURE));
  RecordUmaResult(result, retries_);

  TimeZoneResponseCallback callback = callback_;

  // Empty callback is used to identify "completed or not yet started request".
  callback_.Reset();

  // callback.Run() usually destroys TimeZoneRequest, because this is the way
  // callback is implemented in TimeZoneProvider.
  callback.Run(std::move(timezone), server_error);
  // "this" is already destroyed here.
}

std::string TimeZoneResponseData::ToStringForDebug() const {
  static const char* const status2string[] = {
      "OK",
      "INVALID_REQUEST",
      "OVER_QUERY_LIMIT",
      "REQUEST_DENIED",
      "UNKNOWN_ERROR",
      "ZERO_RESULTS",
      "REQUEST_ERROR"
  };

  return base::StringPrintf(
      "dstOffset=%f, rawOffset=%f, timeZoneId='%s', timeZoneName='%s', "
      "error_message='%s', status=%u (%s)",
      dstOffset,
      rawOffset,
      timeZoneId.c_str(),
      timeZoneName.c_str(),
      error_message.c_str(),
      (unsigned)status,
      (status < arraysize(status2string) ? status2string[status] : "unknown"));
}

}  // namespace chromeos
