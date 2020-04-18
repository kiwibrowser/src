// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/update_client/request_sender.h"

#include <algorithm>
#include <utility>

#include "base/base64.h"
#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/client_update_protocol/ecdsa.h"
#include "components/update_client/configurator.h"
#include "components/update_client/utils.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_status.h"

namespace update_client {

namespace {

// This is an ECDSA prime256v1 named-curve key.
constexpr int kKeyVersion = 8;
const char kKeyPubBytesBase64[] =
    "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE+J2iCpfk8lThcuKUPzTaVcUjhNR3"
    "AYHK+tTelGdHvyGGx7RP7BphYSPmpH6P4Vr72ak0W1a0bW55O9HW2oz3rQ==";

// The ETag header carries the ECSDA signature of the protocol response, if
// signing has been used.
constexpr const char* kHeaderEtag = "ETag";

// The server uses the optional X-Retry-After header to indicate that the
// current request should not be attempted again. Any response received along
// with the X-Retry-After header should be interpreted as it would have been
// without the X-Retry-After header.
//
// In addition to the presence of the header, the value of the header is
// used as a signal for when to do future update checks, but only when the
// response is over https. Values over http are not trusted and are ignored.
//
// The value of the header is the number of seconds to wait before trying to do
// a subsequent update check. The upper bound for the number of seconds to wait
// before trying to do a subsequent update check is capped at 24 hours.
constexpr const char* kHeaderXRetryAfter = "X-Retry-After";
constexpr int64_t kMaxRetryAfterSec = 24 * 60 * 60;

}  // namespace

RequestSender::RequestSender(scoped_refptr<Configurator> config)
    : config_(config), use_signing_(false) {}

RequestSender::~RequestSender() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void RequestSender::Send(
    const std::vector<GURL>& urls,
    const std::map<std::string, std::string>& request_extra_headers,
    const std::string& request_body,
    bool use_signing,
    RequestSenderCallback request_sender_callback) {
  DCHECK(thread_checker_.CalledOnValidThread());

  urls_ = urls;
  request_extra_headers_ = request_extra_headers;
  request_body_ = request_body;
  use_signing_ = use_signing;
  request_sender_callback_ = std::move(request_sender_callback);

  if (urls_.empty()) {
    return HandleSendError(-1, 0);
  }

  cur_url_ = urls_.begin();

  if (use_signing_) {
    public_key_ = GetKey(kKeyPubBytesBase64);
    if (public_key_.empty())
      return HandleSendError(-1, 0);
  }

  SendInternal();
}

void RequestSender::SendInternal() {
  DCHECK(cur_url_ != urls_.end());
  DCHECK(cur_url_->is_valid());
  DCHECK(thread_checker_.CalledOnValidThread());

  GURL url(*cur_url_);

  if (use_signing_) {
    DCHECK(!public_key_.empty());
    signer_ = client_update_protocol::Ecdsa::Create(kKeyVersion, public_key_);
    std::string request_query_string;
    signer_->SignRequest(request_body_, &request_query_string);

    url = BuildUpdateUrl(url, request_query_string);
  }

  url_fetcher_ = SendProtocolRequest(url, request_extra_headers_, request_body_,
                                     this, config_->RequestContext());
  if (!url_fetcher_.get())
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&RequestSender::SendInternalComplete,
                                  base::Unretained(this), -1, std::string(),
                                  std::string(), 0));
}

void RequestSender::SendInternalComplete(int error,
                                         const std::string& response_body,
                                         const std::string& response_etag,
                                         int retry_after_sec) {
  if (!error) {
    if (!use_signing_) {
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::BindOnce(std::move(request_sender_callback_), 0,
                                    response_body, retry_after_sec));
      return;
    }

    DCHECK(use_signing_);
    DCHECK(signer_.get());
    if (signer_->ValidateResponse(response_body, response_etag)) {
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::BindOnce(std::move(request_sender_callback_), 0,
                                    response_body, retry_after_sec));
      return;
    }

    error = kErrorResponseNotTrusted;
  }

  DCHECK(error);

  // A positive |retry_after_sec| is a hint from the server that the client
  // should not send further request until the cooldown has expired.
  if (retry_after_sec <= 0 && ++cur_url_ != urls_.end() &&
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::BindOnce(&RequestSender::SendInternal,
                                    base::Unretained(this)))) {
    return;
  }

  HandleSendError(error, retry_after_sec);
}

void RequestSender::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(source);

  const GURL original_url(source->GetOriginalURL());
  VLOG(1) << "request completed from url: " << original_url.spec();

  const int fetch_error(GetFetchError(*source));
  std::string response_body;
  CHECK(source->GetResponseAsString(&response_body));

  int64_t retry_after_sec(-1);
  const auto status(source->GetStatus().status());
  if (original_url.SchemeIsCryptographic() &&
      status == net::URLRequestStatus::SUCCESS) {
    retry_after_sec = GetInt64HeaderValue(source, kHeaderXRetryAfter);
    retry_after_sec = std::min(retry_after_sec, kMaxRetryAfterSec);
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&RequestSender::SendInternalComplete,
                     base::Unretained(this), fetch_error, response_body,
                     GetStringHeaderValue(source, kHeaderEtag),
                     static_cast<int>(retry_after_sec)));
}

void RequestSender::HandleSendError(int error, int retry_after_sec) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(request_sender_callback_), error,
                                std::string(), retry_after_sec));
}

std::string RequestSender::GetKey(const char* key_bytes_base64) {
  std::string result;
  return base::Base64Decode(std::string(key_bytes_base64), &result)
             ? result
             : std::string();
}

GURL RequestSender::BuildUpdateUrl(const GURL& url,
                                   const std::string& query_params) {
  const std::string query_string(
      url.has_query() ? base::StringPrintf("%s&%s", url.query().c_str(),
                                           query_params.c_str())
                      : query_params);
  GURL::Replacements replacements;
  replacements.SetQueryStr(query_string);

  return url.ReplaceComponents(replacements);
}

std::string RequestSender::GetStringHeaderValue(const net::URLFetcher* source,
                                                const char* header_name) {
  auto* response_headers(source->GetResponseHeaders());
  if (!response_headers)
    return std::string();

  std::string etag;
  return response_headers->EnumerateHeader(nullptr, header_name, &etag)
             ? etag
             : std::string();
}

int64_t RequestSender::GetInt64HeaderValue(const net::URLFetcher* source,
                                           const char* header_name) {
  auto* response_headers(source->GetResponseHeaders());
  return response_headers ? response_headers->GetInt64HeaderValue(header_name)
                          : -1;
}

}  // namespace update_client
