// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/warmup_url_fetcher.h"

#include "base/callback.h"
#include "base/guid.h"
#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_util.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_features.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_headers.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "net/nqe/network_quality_estimator.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"

namespace data_reduction_proxy {

WarmupURLFetcher::WarmupURLFetcher(
    const scoped_refptr<net::URLRequestContextGetter>&
        url_request_context_getter,
    WarmupURLFetcherCallback callback)
    : is_fetch_in_flight_(false),
      previous_attempt_counts_(0),
      url_request_context_getter_(url_request_context_getter),
      callback_(callback) {
  DCHECK(url_request_context_getter_);
  DCHECK(url_request_context_getter_->GetURLRequestContext()
             ->network_quality_estimator());
}

WarmupURLFetcher::~WarmupURLFetcher() {}

void WarmupURLFetcher::FetchWarmupURL(size_t previous_attempt_counts) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  previous_attempt_counts_ = previous_attempt_counts;

  DCHECK_LE(0u, previous_attempt_counts_);
  DCHECK_GE(2u, previous_attempt_counts_);

  // There can be at most one pending fetch at any time.
  fetch_delay_timer_.Stop();

  if (previous_attempt_counts_ == 0) {
    FetchWarmupURLNow();
    return;
  }
  fetch_delay_timer_.Start(FROM_HERE, GetFetchWaitTime(), this,
                           &WarmupURLFetcher::FetchWarmupURLNow);
}

base::TimeDelta WarmupURLFetcher::GetFetchWaitTime() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_LT(0u, previous_attempt_counts_);
  DCHECK_GE(2u, previous_attempt_counts_);

  if (previous_attempt_counts_ == 1)
    return base::TimeDelta::FromSeconds(30);

  return base::TimeDelta::FromSeconds(60);
}

void WarmupURLFetcher::FetchWarmupURLNow() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  UMA_HISTOGRAM_EXACT_LINEAR("DataReductionProxy.WarmupURL.FetchInitiated", 1,
                             2);
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("data_reduction_proxy_warmup", R"(
          semantics {
            sender: "Data Reduction Proxy"
            description:
              "Sends a request to the Data Reduction Proxy server to warm up "
              "the connection to the proxy."
            trigger:
              "A network change while the data reduction proxy is enabled will "
              "trigger this request."
            data: "A specific URL, not related to user data."
            destination: GOOGLE_OWNED_SERVICE
          }
          policy {
            cookies_allowed: NO
            setting:
              "Users can control Data Saver on Android via the 'Data Saver' "
              "setting. Data Saver is not available on iOS, and on desktop it "
              "is enabled by installing the Data Saver extension."
            policy_exception_justification: "Not implemented."
          })");

  GURL warmup_url_with_query_params;
  GetWarmupURLWithQueryParam(&warmup_url_with_query_params);

  fetcher_.reset();
  fetch_timeout_timer_.Stop();
  is_fetch_in_flight_ = true;

  fetcher_ =
      net::URLFetcher::Create(warmup_url_with_query_params,
                              net::URLFetcher::GET, this, traffic_annotation);
  data_use_measurement::DataUseUserData::AttachToFetcher(
      fetcher_.get(),
      data_use_measurement::DataUseUserData::DATA_REDUCTION_PROXY);
  // Do not disable cookies. This allows the warmup connection to be reused
  // for fetching user initiated requests.
  fetcher_->SetLoadFlags(net::LOAD_BYPASS_CACHE);
  fetcher_->SetRequestContext(url_request_context_getter_.get());
  // |fetcher| should not retry on 5xx errors. |fetcher_| should retry on
  // network changes since the network stack may receive the connection change
  // event later than |this|.
  static const int kMaxRetries = 5;
  fetcher_->SetAutomaticallyRetryOn5xx(false);
  fetcher_->SetAutomaticallyRetryOnNetworkChanges(kMaxRetries);
  fetch_timeout_timer_.Start(FROM_HERE, GetFetchTimeout(), this,
                             &WarmupURLFetcher::OnFetchTimeout);
  fetcher_->Start();
}

void WarmupURLFetcher::GetWarmupURLWithQueryParam(
    GURL* warmup_url_with_query_params) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Set the query param to a random string to prevent intermediate middleboxes
  // from returning cached content.
  const std::string query = "q=" + base::GenerateGUID();
  GURL::Replacements replacements;
  replacements.SetQuery(query.c_str(), url::Component(0, query.length()));

  *warmup_url_with_query_params =
      params::GetWarmupURL().ReplaceComponents(replacements);

  DCHECK(warmup_url_with_query_params->is_valid() &&
         warmup_url_with_query_params->has_query());
}

void WarmupURLFetcher::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(source, fetcher_.get());
  DCHECK(is_fetch_in_flight_);

  UMA_HISTOGRAM_BOOLEAN(
      "DataReductionProxy.WarmupURL.FetchSuccessful",
      source->GetStatus().status() == net::URLRequestStatus::SUCCESS);

  base::UmaHistogramSparse("DataReductionProxy.WarmupURL.NetError",
                           std::abs(source->GetStatus().error()));

  base::UmaHistogramSparse("DataReductionProxy.WarmupURL.HttpResponseCode",
                           std::abs(source->GetResponseCode()));

  if (source->GetResponseHeaders()) {
    UMA_HISTOGRAM_BOOLEAN(
        "DataReductionProxy.WarmupURL.HasViaHeader",
        HasDataReductionProxyViaHeader(*source->GetResponseHeaders(),
                                       nullptr /* has_intermediary */));

    UMA_HISTOGRAM_ENUMERATION("DataReductionProxy.WarmupURL.ProxySchemeUsed",
                              util::ConvertNetProxySchemeToProxyScheme(
                                  source->ProxyServerUsed().scheme()),
                              PROXY_SCHEME_MAX);
  }

  if (!GetFieldTrialParamByFeatureAsBool(
          features::kDataReductionProxyRobustConnection,
          params::GetWarmupCallbackParamName(), false)) {
    CleanupAfterFetch();
    return;
  }

  if (!source->GetStatus().is_success() &&
      source->GetStatus().error() == net::ERR_INTERNET_DISCONNECTED) {
    // Fetching failed due to Internet unavailability, and not due to some
    // error. No need to run the callback.
    CleanupAfterFetch();
    return;
  }

  bool success_response =
      source->GetStatus().status() == net::URLRequestStatus::SUCCESS &&
      params::IsWhitelistedHttpResponseCodeForProbes(
          source->GetResponseCode()) &&
      source->GetResponseHeaders() &&
      HasDataReductionProxyViaHeader(*(source->GetResponseHeaders()),
                                     nullptr /* has_intermediary */);
  callback_.Run(source->ProxyServerUsed(), success_response
                                               ? FetchResult::kSuccessful
                                               : FetchResult::kFailed);
  CleanupAfterFetch();
}

bool WarmupURLFetcher::IsFetchInFlight() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  return is_fetch_in_flight_;
}

base::TimeDelta WarmupURLFetcher::GetFetchTimeout() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_LE(0u, previous_attempt_counts_);
  DCHECK_GE(2u, previous_attempt_counts_);

  // The timeout value should always be between |min_timeout| and |max_timeout|
  // (both inclusive).
  const base::TimeDelta min_timeout =
      base::TimeDelta::FromSeconds(GetFieldTrialParamByFeatureAsInt(
          features::kDataReductionProxyRobustConnection,
          "warmup_url_fetch_min_timeout_seconds", 8));
  const base::TimeDelta max_timeout =
      base::TimeDelta::FromSeconds(GetFieldTrialParamByFeatureAsInt(
          features::kDataReductionProxyRobustConnection,
          "warmup_url_fetch_max_timeout_seconds", 60));
  DCHECK_LT(base::TimeDelta::FromSeconds(0), min_timeout);
  DCHECK_LT(base::TimeDelta::FromSeconds(0), max_timeout);
  DCHECK_LE(min_timeout, max_timeout);

  // Set the timeout based on how many times the fetching of the warmup URL
  // has been tried.
  size_t http_rtt_multiplier = GetFieldTrialParamByFeatureAsInt(
      features::kDataReductionProxyRobustConnection,
      "warmup_url_fetch_init_http_rtt_multiplier", 5);
  if (previous_attempt_counts_ == 1) {
    http_rtt_multiplier = GetFieldTrialParamByFeatureAsInt(
                              features::kDataReductionProxyRobustConnection,
                              "warmup_url_fetch_init_http_rtt_multiplier", 5) *
                          2;
  } else if (previous_attempt_counts_ == 2) {
    http_rtt_multiplier = GetFieldTrialParamByFeatureAsInt(
                              features::kDataReductionProxyRobustConnection,
                              "warmup_url_fetch_init_http_rtt_multiplier", 5) *
                          4;
  }
  // Sanity checks.
  DCHECK_LT(0u, http_rtt_multiplier);
  DCHECK_GE(1000u, http_rtt_multiplier);

  const net::NetworkQualityEstimator* network_quality_estimator =
      url_request_context_getter_->GetURLRequestContext()
          ->network_quality_estimator();

  base::Optional<base::TimeDelta> http_rtt_estimate =
      network_quality_estimator->GetHttpRTT();
  if (!http_rtt_estimate)
    return max_timeout;

  base::TimeDelta timeout = http_rtt_multiplier * http_rtt_estimate.value();
  if (timeout > max_timeout)
    return max_timeout;

  if (timeout < min_timeout)
    return min_timeout;

  return timeout;
}

void WarmupURLFetcher::OnFetchTimeout() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(is_fetch_in_flight_);
  DCHECK(fetcher_);

  const net::ProxyServer proxy_server = fetcher_->ProxyServerUsed();
  DCHECK_LE(1, proxy_server.scheme());

  UMA_HISTOGRAM_BOOLEAN("DataReductionProxy.WarmupURL.FetchSuccessful", false);
  base::UmaHistogramSparse("DataReductionProxy.WarmupURL.NetError",
                           net::ERR_ABORTED);
  base::UmaHistogramSparse("DataReductionProxy.WarmupURL.HttpResponseCode",
                           std::abs(net::URLFetcher::RESPONSE_CODE_INVALID));

  if (!GetFieldTrialParamByFeatureAsBool(
          features::kDataReductionProxyRobustConnection,
          params::GetWarmupCallbackParamName(), false)) {
    // Running the callback is not enabled.
    CleanupAfterFetch();
    return;
  }

  callback_.Run(proxy_server, FetchResult::kTimedOut);
  CleanupAfterFetch();
}

void WarmupURLFetcher::CleanupAfterFetch() {
  is_fetch_in_flight_ = false;
  fetcher_.reset();
  fetch_timeout_timer_.Stop();
  fetch_delay_timer_.Stop();
}

}  // namespace data_reduction_proxy
