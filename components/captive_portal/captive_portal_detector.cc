// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/captive_portal/captive_portal_detector.h"

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "net/base/load_flags.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"
#include "net/url_request/url_request_status.h"

namespace captive_portal {

const char CaptivePortalDetector::kDefaultURL[] =
    "http://www.gstatic.com/generate_204";

CaptivePortalDetector::CaptivePortalDetector(
    network::mojom::URLLoaderFactory* loader_factory)
    : loader_factory_(loader_factory) {}

CaptivePortalDetector::~CaptivePortalDetector() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void CaptivePortalDetector::DetectCaptivePortal(
    const GURL& url,
    const DetectionCallback& detection_callback,
    const net::NetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!FetchingURL());
  DCHECK(detection_callback_.is_null());

  detection_callback_ = detection_callback;

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = url;

  // Can't safely use net::LOAD_DISABLE_CERT_REVOCATION_CHECKING here,
  // since then the connection may be reused without checking the cert.
  resource_request->load_flags =
      net::LOAD_BYPASS_CACHE | net::LOAD_DO_NOT_SAVE_COOKIES |
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SEND_AUTH_DATA;

  // TODO(jam): switch to using ServiceURLLoader to track data measurement once
  // https://crbug.com/808498 is fixed.
  simple_loader_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                                    traffic_annotation);
  simple_loader_->SetAllowHttpErrorResults(true);
  network::SimpleURLLoader::BodyAsStringCallback callback = base::BindOnce(
      &CaptivePortalDetector::OnSimpleLoaderComplete, base::Unretained(this));
  simple_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory_, std::move(callback));
}

void CaptivePortalDetector::Cancel() {
  simple_loader_.reset();
  detection_callback_.Reset();
}

void CaptivePortalDetector::OnSimpleLoaderComplete(
    std::unique_ptr<std::string> response_body) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(FetchingURL());
  DCHECK(!detection_callback_.is_null());

  int response_code = 0;
  net::HttpResponseHeaders* headers = nullptr;
  if (simple_loader_->ResponseInfo() &&
      simple_loader_->ResponseInfo()->headers) {
    headers = simple_loader_->ResponseInfo()->headers.get();
    response_code = simple_loader_->ResponseInfo()->headers->response_code();
  }
  OnSimpleLoaderCompleteInternal(simple_loader_->NetError(), response_code,
                                 simple_loader_->GetFinalURL(), headers);
}

void CaptivePortalDetector::OnSimpleLoaderCompleteInternal(
    int net_error,
    int response_code,
    const GURL& url,
    net::HttpResponseHeaders* headers) {
  Results results;
  GetCaptivePortalResultFromResponse(net_error, response_code, url, headers,
                                     &results);
  DetectionCallback callback = detection_callback_;
  simple_loader_.reset();
  detection_callback_.Reset();
  callback.Run(results);
}

void CaptivePortalDetector::GetCaptivePortalResultFromResponse(
    int net_error,
    int response_code,
    const GURL& url,
    net::HttpResponseHeaders* headers,
    Results* results) const {
  results->result = captive_portal::RESULT_NO_RESPONSE;
  results->response_code = response_code;
  results->retry_after_delta = base::TimeDelta();
  results->landing_url = url;

  VLOG(1) << "Getting captive portal result"
          << " response code: " << results->response_code
          << " landing_url: " << results->landing_url;

  // If there's a network error of some sort when fetching a file via HTTP,
  // there may be a networking problem, rather than a captive portal.
  // TODO(mmenke):  Consider special handling for redirects that end up at
  //                errors, especially SSL certificate errors.
  if (net_error != net::OK)
    return;

  // In the case of 503 errors, look for the Retry-After header.
  if (results->response_code == 503) {
    std::string retry_after_string;

    // If there's no Retry-After header, nothing else to do.
    if (!headers->EnumerateHeader(nullptr, "Retry-After", &retry_after_string))
      return;

    base::TimeDelta retry_after_delta;
    if (net::HttpUtil::ParseRetryAfterHeader(
            retry_after_string, GetCurrentTime(), &retry_after_delta)) {
      results->retry_after_delta = retry_after_delta;
    }

    return;
  }

  // A 511 response (Network Authentication Required) means that the user needs
  // to login to whatever server issued the response.
  // See:  http://tools.ietf.org/html/rfc6585
  if (results->response_code == 511) {
    results->result = captive_portal::RESULT_BEHIND_CAPTIVE_PORTAL;
    return;
  }

  // Other non-2xx/3xx HTTP responses may indicate server errors.
  if (results->response_code >= 400 || results->response_code < 200)
    return;

  // A 204 response code indicates there's no captive portal.
  if (results->response_code == 204) {
    results->result = captive_portal::RESULT_INTERNET_CONNECTED;
    return;
  }

  // Otherwise, assume it's a captive portal.
  results->result = captive_portal::RESULT_BEHIND_CAPTIVE_PORTAL;
}

base::Time CaptivePortalDetector::GetCurrentTime() const {
  if (time_for_testing_.is_null())
    return base::Time::Now();
  return time_for_testing_;
}

bool CaptivePortalDetector::FetchingURL() const {
  return simple_loader_ != nullptr;
}

}  // namespace captive_portal
