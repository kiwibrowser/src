// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/login/auth/mock_url_fetchers.h"

#include <errno.h>

#include "base/bind.h"
#include "base/location.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

ExpectCanceledFetcher::ExpectCanceledFetcher(
    bool success,
    const GURL& url,
    const std::string& results,
    net::URLFetcher::RequestType request_type,
    net::URLFetcherDelegate* d)
    : net::TestURLFetcher(0, url, d), weak_factory_(this) {
}

ExpectCanceledFetcher::~ExpectCanceledFetcher() = default;

void ExpectCanceledFetcher::Start() {
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&ExpectCanceledFetcher::CompleteFetch,
                     weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromMilliseconds(100));
}

void ExpectCanceledFetcher::CompleteFetch() {
  ADD_FAILURE() << "Fetch completed in ExpectCanceledFetcher!";

  // Allow exiting even if we mess up.
  base::RunLoop::QuitCurrentWhenIdleDeprecated();
}

GotCanceledFetcher::GotCanceledFetcher(
    bool success,
    const GURL& url,
    const std::string& results,
    net::URLFetcher::RequestType request_type,
    net::URLFetcherDelegate* d)
    : net::TestURLFetcher(0, url, d) {
  set_url(url);
  set_status(net::URLRequestStatus::FromError(net::ERR_ABORTED));
  set_response_code(net::HTTP_FORBIDDEN);
}

GotCanceledFetcher::~GotCanceledFetcher() = default;

void GotCanceledFetcher::Start() {
  delegate()->OnURLFetchComplete(this);
}

SuccessFetcher::SuccessFetcher(bool success,
                               const GURL& url,
                               const std::string& results,
                               net::URLFetcher::RequestType request_type,
                               net::URLFetcherDelegate* d)
    : net::TestURLFetcher(0, url, d) {
  set_url(url);
  set_status(net::URLRequestStatus(net::URLRequestStatus::SUCCESS, 0));
  set_response_code(net::HTTP_OK);
}

SuccessFetcher::~SuccessFetcher() = default;

void SuccessFetcher::Start() {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&SuccessFetcher::RunDelegate, weak_factory_.GetWeakPtr()));
}

void SuccessFetcher::RunDelegate() {
  delegate()->OnURLFetchComplete(this);
}

FailFetcher::FailFetcher(bool success,
                         const GURL& url,
                         const std::string& results,
                         net::URLFetcher::RequestType request_type,
                         net::URLFetcherDelegate* d)
    : net::TestURLFetcher(0, url, d) {
  set_url(url);
  set_status(net::URLRequestStatus::FromError(net::ERR_CONNECTION_RESET));
  set_response_code(net::HTTP_OK);
}

FailFetcher::~FailFetcher() = default;

void FailFetcher::Start() {
  delegate()->OnURLFetchComplete(this);
}

// static
const char CaptchaFetcher::kCaptchaToken[] = "token";
// static
const char CaptchaFetcher::kCaptchaUrlBase[] = "http://accounts.google.com/";
// static
const char CaptchaFetcher::kCaptchaUrlFragment[] = "fragment";
// static
const char CaptchaFetcher::kUnlockUrl[] = "http://what.ever";

CaptchaFetcher::CaptchaFetcher(bool success,
                               const GURL& url,
                               const std::string& results,
                               net::URLFetcher::RequestType request_type,
                               net::URLFetcherDelegate* d)
    : net::TestURLFetcher(0, url, d) {
  set_url(url);
  set_status(net::URLRequestStatus(net::URLRequestStatus::SUCCESS, 0));
  set_response_code(net::HTTP_FORBIDDEN);
  SetResponseString(base::StringPrintf(
      "Error=%s\n"
      "Url=%s\n"
      "CaptchaUrl=%s\n"
      "CaptchaToken=%s\n",
      "CaptchaRequired",
      kUnlockUrl,
      kCaptchaUrlFragment,
      kCaptchaToken));
}

CaptchaFetcher::~CaptchaFetcher() = default;

// static
std::string CaptchaFetcher::GetCaptchaToken() {
  return kCaptchaToken;
}

// static
std::string CaptchaFetcher::GetCaptchaUrl() {
  return std::string(kCaptchaUrlBase).append(kCaptchaUrlFragment);
}

// static
std::string CaptchaFetcher::GetUnlockUrl() {
  return kUnlockUrl;
}

void CaptchaFetcher::Start() {
  delegate()->OnURLFetchComplete(this);
}

HostedFetcher::HostedFetcher(bool success,
                             const GURL& url,
                             const std::string& results,
                             net::URLFetcher::RequestType request_type,
                             net::URLFetcherDelegate* d)
    : net::TestURLFetcher(0, url, d) {
  set_url(url);
  set_status(net::URLRequestStatus(net::URLRequestStatus::SUCCESS, 0));
  set_response_code(net::HTTP_OK);
}

HostedFetcher::~HostedFetcher() = default;

void HostedFetcher::Start() {
  VLOG(1) << upload_data();
  if (upload_data().find("HOSTED") == std::string::npos) {
    VLOG(1) << "HostedFetcher failing request";
    set_response_code(net::HTTP_FORBIDDEN);
    SetResponseString("Error=BadAuthentication");
  }
  delegate()->OnURLFetchComplete(this);
}

}  // namespace chromeos
