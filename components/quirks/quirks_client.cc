// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/quirks/quirks_client.h"

#include "base/base64.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "base/task_runner_util.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/quirks/quirks_manager.h"
#include "components/version_info/version_info.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"

namespace quirks {

namespace {

const char kQuirksUrlFormat[] =
    "https://chromeosquirksserver-pa.googleapis.com/v2/display/%s/clients"
    "/chromeos/M%d?";

const int kMaxServerFailures = 10;

const net::BackoffEntry::Policy kDefaultBackoffPolicy = {
    1,                // Initial errors before applying backoff
    10000,            // 10 seconds.
    2,                // Factor by which the waiting time will be multiplied.
    0,                // Random fuzzing percentage.
    1000 * 3600 * 6,  // Max wait between requests = 6 hours.
    -1,               // Don't discard entry.
    true,             // Use initial delay after first error.
};

bool WriteIccFile(const base::FilePath file_path, const std::string& data) {
  int bytes_written = base::WriteFile(file_path, data.data(), data.length());
  if (bytes_written == -1)
    LOG(ERROR) << "Write failed: " << file_path.value() << ", err = " << errno;
  else
    VLOG(1) << bytes_written << "bytes written to: " << file_path.value();

  return (bytes_written != -1);
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// QuirksClient

QuirksClient::QuirksClient(int64_t product_id,
                           const std::string& display_name,
                           const RequestFinishedCallback& on_request_finished,
                           QuirksManager* manager)
    : product_id_(product_id),
      display_name_(display_name),
      on_request_finished_(on_request_finished),
      manager_(manager),
      icc_path_(manager->delegate()->GetDisplayProfileDirectory().Append(
          IdToFileName(product_id))),
      backoff_entry_(&kDefaultBackoffPolicy),
      weak_ptr_factory_(this) {}

QuirksClient::~QuirksClient() {}

void QuirksClient::StartDownload() {
  DCHECK(thread_checker_.CalledOnValidThread());

  // URL of icc file on Quirks Server.
  int major_version = atoi(version_info::GetVersionNumber().c_str());
  std::string url = base::StringPrintf(
      kQuirksUrlFormat, IdToHexString(product_id_).c_str(), major_version);

  if (!display_name_.empty()) {
    url +=
        "display_name=" + net::EscapeQueryParamValue(display_name_, true) + "&";
  }

  VLOG(2) << "Preparing to download\n  " << url << "\nto file "
          << icc_path_.value();

  url += "key=" + manager_->delegate()->GetApiKey();

  url_fetcher_ = manager_->CreateURLFetcher(GURL(url), this);
  url_fetcher_->SetRequestContext(manager_->url_context_getter());
  url_fetcher_->SetLoadFlags(net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
                             net::LOAD_DO_NOT_SAVE_COOKIES |
                             net::LOAD_DO_NOT_SEND_COOKIES |
                             net::LOAD_DO_NOT_SEND_AUTH_DATA);
  url_fetcher_->Start();
}

void QuirksClient::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK_EQ(url_fetcher_.get(), source);

  const int HTTP_INTERNAL_SERVER_ERROR_LAST =
      net::HTTP_INTERNAL_SERVER_ERROR + 99;
  const net::URLRequestStatus status = source->GetStatus();
  const int response_code = source->GetResponseCode();
  const bool server_error = !status.is_success() ||
                            (response_code >= net::HTTP_INTERNAL_SERVER_ERROR &&
                             response_code <= HTTP_INTERNAL_SERVER_ERROR_LAST);

  VLOG(2) << "QuirksClient::OnURLFetchComplete():"
          << "  status=" << status.status()
          << ",  response_code=" << response_code
          << ",  server_error=" << server_error;

  if (response_code == net::HTTP_NOT_FOUND) {
    VLOG(1) << IdToFileName(product_id_) << " not found on Quirks server.";
    Shutdown(false);
    return;
  }

  if (server_error) {
    if (backoff_entry_.failure_count() >= kMaxServerFailures) {
      // After 10 retires (5+ hours), give up, and try again in a month.
      VLOG(1) << "Too many retries; Quirks Client shutting down.";
      Shutdown(false);
      return;
    }
    url_fetcher_.reset();
    Retry();
    return;
  }

  std::string response;
  url_fetcher_->GetResponseAsString(&response);
  VLOG(2) << "Quirks server response:\n" << response;

  // Parse response data and write to file on file thread.
  std::string data;
  if (!ParseResult(response, &data)) {
    Shutdown(false);
    return;
  }

  base::PostTaskAndReplyWithResult(
      manager_->task_runner(), FROM_HERE,
      base::Bind(&WriteIccFile, icc_path_, data),
      base::Bind(&QuirksClient::Shutdown, weak_ptr_factory_.GetWeakPtr()));
}

void QuirksClient::Shutdown(bool success) {
  DCHECK(thread_checker_.CalledOnValidThread());
  on_request_finished_.Run(success ? icc_path_ : base::FilePath(), true);
  manager_->ClientFinished(this);
}

void QuirksClient::Retry() {
  DCHECK(thread_checker_.CalledOnValidThread());
  backoff_entry_.InformOfRequest(false);
  const base::TimeDelta delay = backoff_entry_.GetTimeUntilRelease();

  VLOG(1) << "Schedule next Quirks download attempt in " << delay.InSecondsF()
          << " seconds (retry = " << backoff_entry_.failure_count() << ").";
  request_scheduled_.Start(FROM_HERE, delay, this,
                           &QuirksClient::StartDownload);
}

bool QuirksClient::ParseResult(const std::string& result, std::string* data) {
  std::string data64;
  const base::DictionaryValue* dict;
  std::unique_ptr<base::Value> json = base::JSONReader::Read(result);
  if (!json || !json->GetAsDictionary(&dict) ||
      !dict->GetString("icc", &data64)) {
    VLOG(1) << "Failed to parse JSON icc data";
    return false;
  }

  if (!base::Base64Decode(data64, data)) {
    VLOG(1) << "Failed to decode Base64 icc data";
    return false;
  }

  return true;
}

}  // namespace quirks
