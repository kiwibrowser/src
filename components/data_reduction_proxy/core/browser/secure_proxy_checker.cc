// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/secure_proxy_checker.h"

#include "base/metrics/histogram_macros.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"

namespace {

// Key of the UMA DataReductionProxy.SecureProxyCheck.Latency histogram.
const char kUMAProxySecureProxyCheckLatency[] =
    "DataReductionProxy.SecureProxyCheck.Latency";

}  // namespace

namespace data_reduction_proxy {

SecureProxyChecker::SecureProxyChecker(
    const scoped_refptr<net::URLRequestContextGetter>&
        url_request_context_getter)
    : url_request_context_getter_(url_request_context_getter) {}

void SecureProxyChecker::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK_EQ(source, fetcher_.get());
  net::URLRequestStatus status = source->GetStatus();

  std::string response;
  source->GetResponseAsString(&response);

  base::TimeDelta secure_proxy_check_latency =
      base::Time::Now() - secure_proxy_check_start_time_;
  if (secure_proxy_check_latency >= base::TimeDelta()) {
    UMA_HISTOGRAM_MEDIUM_TIMES(kUMAProxySecureProxyCheckLatency,
                               secure_proxy_check_latency);
  }

  fetcher_callback_.Run(response, status, source->GetResponseCode());
}

void SecureProxyChecker::CheckIfSecureProxyIsAllowed(
    SecureProxyCheckerCallback fetcher_callback) {
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation(
          "data_reduction_proxy_secure_proxy_check", R"(
            semantics {
              sender: "Data Reduction Proxy"
              description:
                "Sends a request to the Data Reduction Proxy server. Proceeds "
                "with using a secure connection to the proxy only if the "
                "response is not blocked or modified by an intermediary."
              trigger:
                "A request can be sent whenever the browser is determining how "
                "to configure its connection to the data reduction proxy. This "
                "happens on startup and network changes."
              data: "A specific URL, not related to user data."
              destination: GOOGLE_OWNED_SERVICE
            }
            policy {
              cookies_allowed: NO
              setting:
                "Users can control Data Saver on Android via the 'Data Saver' "
                "setting. Data Saver is not available on iOS, and on desktop "
                "it is enabled by installing the Data Saver extension."
              policy_exception_justification: "Not implemented."
            })");
  fetcher_ =
      net::URLFetcher::Create(params::GetSecureProxyCheckURL(),
                              net::URLFetcher::GET, this, traffic_annotation);
  data_use_measurement::DataUseUserData::AttachToFetcher(
      fetcher_.get(),
      data_use_measurement::DataUseUserData::DATA_REDUCTION_PROXY);
  fetcher_->SetLoadFlags(net::LOAD_DISABLE_CACHE | net::LOAD_BYPASS_PROXY |
                         net::LOAD_DO_NOT_SEND_COOKIES |
                         net::LOAD_DO_NOT_SAVE_COOKIES);
  fetcher_->SetRequestContext(url_request_context_getter_.get());
  // Configure max retries to be at most kMaxRetries times for 5xx errors.
  static const int kMaxRetries = 5;
  fetcher_->SetMaxRetriesOn5xx(kMaxRetries);
  fetcher_->SetAutomaticallyRetryOnNetworkChanges(kMaxRetries);
  // The secure proxy check should not be redirected. Since the secure proxy
  // check will inevitably fail if it gets redirected somewhere else (e.g. by
  // a captive portal), short circuit that by giving up on the secure proxy
  // check if it gets redirected.
  fetcher_->SetStopOnRedirect(true);

  fetcher_callback_ = fetcher_callback;

  secure_proxy_check_start_time_ = base::Time::Now();
  fetcher_->Start();
}

SecureProxyChecker::~SecureProxyChecker() {}

}  // namespace data_reduction_proxy