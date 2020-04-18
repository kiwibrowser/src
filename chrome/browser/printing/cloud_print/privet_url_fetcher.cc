// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/cloud_print/privet_url_fetcher.h"

#include <stdint.h>

#include <algorithm>
#include <limits>
#include <map>
#include <memory>

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/lazy_instance.h"
#include "base/location.h"
#include "base/rand_util.h"
#include "base/strings/stringprintf.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/printing/cloud_print/privet_constants.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_status.h"

namespace cloud_print {

namespace {

using TokenMap = std::map<std::string, std::string>;

base::LazyInstance<TokenMap>::Leaky g_tokens = LAZY_INSTANCE_INITIALIZER;

const char kXPrivetTokenHeaderPrefix[] = "X-Privet-Token: ";
const char kRangeHeaderFormat[] = "Range: bytes=%d-%d";
const char kXPrivetEmptyToken[] = "\"\"";
const int kPrivetMaxRetries = 20;
const int kPrivetTimeoutOnError = 5;
const int kHTTPErrorCodeInvalidXPrivetToken = 418;

std::string MakeRangeHeader(int start, int end) {
  DCHECK_GE(start, 0);
  DCHECK_GT(end, 0);
  DCHECK_GT(end, start);
  return base::StringPrintf(kRangeHeaderFormat, start, end);
}

}  // namespace

void PrivetURLFetcher::Delegate::OnNeedPrivetToken(TokenCallback callback) {
  OnError(0, TOKEN_ERROR);
}

bool PrivetURLFetcher::Delegate::OnRawData(bool response_is_file,
                                           const std::string& data_string,
                                           const base::FilePath& data_file) {
  return false;
}

PrivetURLFetcher::PrivetURLFetcher(
    const GURL& url,
    net::URLFetcher::RequestType request_type,
    const scoped_refptr<net::URLRequestContextGetter>& context_getter,
    const net::NetworkTrafficAnnotationTag& traffic_annotation,
    PrivetURLFetcher::Delegate* delegate)
    : url_(url),
      request_type_(request_type),
      context_getter_(context_getter),
      traffic_annotation_(traffic_annotation),
      delegate_(delegate),
      max_retries_(kPrivetMaxRetries),
      weak_factory_(this) {}

PrivetURLFetcher::~PrivetURLFetcher() {
}

// static
void PrivetURLFetcher::SetTokenForHost(const std::string& host,
                                       const std::string& token) {
  g_tokens.Get()[host] = token;
}

// static
void PrivetURLFetcher::ResetTokenMapForTest() {
  g_tokens.Get().clear();
}

void PrivetURLFetcher::SetMaxRetriesForTest(int max_retries) {
  DCHECK_EQ(tries_, 0);
  max_retries_ = max_retries;
}

void PrivetURLFetcher::DoNotRetryOnTransientError() {
  DCHECK_EQ(tries_, 0);
  do_not_retry_on_transient_error_ = true;
}

void PrivetURLFetcher::SendEmptyPrivetToken() {
  DCHECK_EQ(tries_, 0);
  send_empty_privet_token_ = true;
}

std::string PrivetURLFetcher::GetPrivetAccessToken() {
  if (send_empty_privet_token_)
    return std::string();

  TokenMap::iterator it = g_tokens.Get().find(GetHostString());
  return it != g_tokens.Get().end() ? it->second : std::string();
}

std::string PrivetURLFetcher::GetHostString() {
  return url_.GetOrigin().spec();
}

void PrivetURLFetcher::SaveResponseToFile() {
  DCHECK_EQ(tries_, 0);
  make_response_file_ = true;
}

void PrivetURLFetcher::SetByteRange(int start, int end) {
  DCHECK_EQ(tries_, 0);
  byte_range_start_ = start;
  byte_range_end_ = end;
  has_byte_range_ = true;
}

void PrivetURLFetcher::Try() {
  tries_++;
  if (tries_ > max_retries_) {
    delegate_->OnError(0, UNKNOWN_ERROR);
    return;
  }

  DVLOG(1) << "Attempt: " << tries_;
  url_fetcher_ =
      net::URLFetcher::Create(url_, request_type_, this, traffic_annotation_);
  data_use_measurement::DataUseUserData::AttachToFetcher(
      url_fetcher_.get(), data_use_measurement::DataUseUserData::CLOUD_PRINT);

  // Privet requests are relevant to hosts on local network only.
  url_fetcher_->SetLoadFlags(url_fetcher_->GetLoadFlags() |
                             net::LOAD_BYPASS_PROXY | net::LOAD_DISABLE_CACHE |
                             net::LOAD_DO_NOT_SEND_COOKIES);
  url_fetcher_->SetRequestContext(context_getter_.get());

  std::string token = GetPrivetAccessToken();
  if (token.empty())
    token = kXPrivetEmptyToken;

  url_fetcher_->AddExtraRequestHeader(std::string(kXPrivetTokenHeaderPrefix) +
                                      token);

  if (has_byte_range_) {
    url_fetcher_->AddExtraRequestHeader(
        MakeRangeHeader(byte_range_start_, byte_range_end_));
  }

  if (make_response_file_)
    url_fetcher_->SaveResponseToTemporaryFile(GetFileTaskRunner());

  // URLFetcher requires us to set upload data for POST requests.
  if (request_type_ == net::URLFetcher::POST)
    url_fetcher_->SetUploadData(upload_content_type_, upload_data_);
  url_fetcher_->Start();
}

void PrivetURLFetcher::Start() {
  DCHECK_EQ(tries_, 0);  // We haven't called |Start()| yet.

  if (!url_.is_valid())
    return delegate_->OnError(0, UNKNOWN_ERROR);

  if (!send_empty_privet_token_) {
    std::string privet_access_token;
    privet_access_token = GetPrivetAccessToken();
    if (privet_access_token.empty()) {
      RequestTokenRefresh();
      return;
    }
  }

  Try();
}

void PrivetURLFetcher::SetUploadData(const std::string& upload_content_type,
                                     const std::string& upload_data) {
  upload_content_type_ = upload_content_type;
  upload_data_ = upload_data;
}

void PrivetURLFetcher::OnURLFetchComplete(const net::URLFetcher* source) {
  DVLOG(1) << "Status: " << source->GetStatus().status()
           << ", ResponseCode: " << source->GetResponseCode();
  if (source->GetStatus().status() != net::URLRequestStatus::CANCELED &&
      (source->GetResponseCode() == net::HTTP_SERVICE_UNAVAILABLE ||
       source->GetResponseCode() == net::URLFetcher::RESPONSE_CODE_INVALID)) {
    ScheduleRetry(kPrivetTimeoutOnError);
    return;
  }

  if (!OnURLFetchCompleteDoNotParseData(source)) {
    // Byte ranges should only be used when we're not parsing the data
    // as JSON.
    DCHECK(!has_byte_range_);

    // We should only be saving raw data to a file.
    DCHECK(!make_response_file_);

    OnURLFetchCompleteParseData(source);
  }
}

// Note that this function returns "true" in error cases to indicate
// that it has fully handled the responses.
bool PrivetURLFetcher::OnURLFetchCompleteDoNotParseData(
    const net::URLFetcher* source) {
  if (source->GetStatus().status() == net::URLRequestStatus::CANCELED) {
    delegate_->OnError(0, REQUEST_CANCELED);
    return true;
  }

  if (source->GetResponseCode() == kHTTPErrorCodeInvalidXPrivetToken) {
    RequestTokenRefresh();
    return true;
  }

  if (source->GetResponseCode() != net::HTTP_OK &&
      source->GetResponseCode() != net::HTTP_PARTIAL_CONTENT &&
      source->GetResponseCode() != net::HTTP_BAD_REQUEST) {
    delegate_->OnError(response_code(), RESPONSE_CODE_ERROR);
    return true;
  }

  if (make_response_file_) {
    base::FilePath response_file_path;
    if (!source->GetResponseAsFilePath(true, &response_file_path)) {
      delegate_->OnError(0, UNKNOWN_ERROR);
      return true;
    }

    return delegate_->OnRawData(true, std::string(), response_file_path);
  }

  std::string response_str;
  if (!source->GetResponseAsString(&response_str)) {
    delegate_->OnError(0, UNKNOWN_ERROR);
    return true;
  }

  return delegate_->OnRawData(false, response_str, base::FilePath());
}

void PrivetURLFetcher::OnURLFetchCompleteParseData(
    const net::URLFetcher* source) {
  // Response contains error description.
  bool is_error_response = false;
  if (source->GetResponseCode() != net::HTTP_OK) {
    delegate_->OnError(response_code(), RESPONSE_CODE_ERROR);
    return;
  }

  std::string response_str;
  if (!source->GetResponseAsString(&response_str)) {
    delegate_->OnError(0, UNKNOWN_ERROR);
    return;
  }

  base::JSONReader json_reader(base::JSON_ALLOW_TRAILING_COMMAS);
  std::unique_ptr<base::Value> value = json_reader.ReadToValue(response_str);
  if (!value || !value->is_dict()) {
    delegate_->OnError(0, JSON_PARSE_ERROR);
    return;
  }

  const base::Value* error_value =
      value->FindKeyOfType(kPrivetKeyError, base::Value::Type::STRING);
  if (error_value) {
    const std::string& error = error_value->GetString();
    if (error == kPrivetErrorInvalidXPrivetToken) {
      RequestTokenRefresh();
      return;
    }
    if (PrivetErrorTransient(error)) {
      if (!do_not_retry_on_transient_error_) {
        const base::Value* timeout_value =
            value->FindKeyOfType(kPrivetKeyTimeout, base::Value::Type::INTEGER);
        ScheduleRetry(timeout_value ? timeout_value->GetInt()
                                    : kPrivetDefaultTimeout);
        return;
      }
    }
    is_error_response = true;
  }

  delegate_->OnParsedJson(
      response_code(), *static_cast<const base::DictionaryValue*>(value.get()),
      is_error_response);
}

void PrivetURLFetcher::ScheduleRetry(int timeout_seconds) {
  double random_scaling_factor =
      1 + base::RandDouble() * kPrivetMaximumTimeRandomAddition;

  int timeout_seconds_randomized =
      static_cast<int>(timeout_seconds * random_scaling_factor);

  timeout_seconds_randomized =
      std::max(timeout_seconds_randomized, kPrivetMinimumTimeout);

  // Don't wait because only error callback is going to be called.
  if (tries_ >= max_retries_)
    timeout_seconds_randomized = 0;

  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&PrivetURLFetcher::Try, weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(timeout_seconds_randomized));
}

void PrivetURLFetcher::RequestTokenRefresh() {
  delegate_->OnNeedPrivetToken(base::BindOnce(&PrivetURLFetcher::RefreshToken,
                                              weak_factory_.GetWeakPtr()));
}

void PrivetURLFetcher::RefreshToken(const std::string& token) {
  if (token.empty()) {
    delegate_->OnError(0, TOKEN_ERROR);
  } else {
    SetTokenForHost(GetHostString(), token);
    Try();
  }
}

bool PrivetURLFetcher::PrivetErrorTransient(const std::string& error) {
  return error == kPrivetErrorDeviceBusy ||
         error == kPrivetErrorPendingUserAction ||
         error == kPrivetErrorPrinterBusy;
}

scoped_refptr<base::SequencedTaskRunner> PrivetURLFetcher::GetFileTaskRunner() {
  if (!file_task_runner_) {
    file_task_runner_ = base::CreateSequencedTaskRunnerWithTraits(
        {base::TaskPriority::BACKGROUND, base::MayBlock()});
  }

  return file_task_runner_;
}

}  // namespace cloud_print
