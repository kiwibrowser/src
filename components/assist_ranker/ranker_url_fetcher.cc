// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/assist_ranker/ranker_url_fetcher.h"

#include "base/memory/ref_counted.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_status.h"

namespace assist_ranker {

namespace {

// Retry parameter for fetching.
const int kMaxRetry = 16;

}  // namespace

RankerURLFetcher::RankerURLFetcher() : state_(IDLE), retry_count_(0) {}

RankerURLFetcher::~RankerURLFetcher() {}

bool RankerURLFetcher::Request(
    const GURL& url,
    const RankerURLFetcher::Callback& callback,
    net::URLRequestContextGetter* request_context_getter) {
  // This function is not supposed to be called if the previous operation is not
  // finished.
  if (state_ == REQUESTING) {
    NOTREACHED();
    return false;
  }

  if (retry_count_ >= kMaxRetry)
    return false;
  retry_count_++;

  state_ = REQUESTING;
  url_ = url;
  callback_ = callback;

  if (request_context_getter == nullptr)
    return false;

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("ranker_url_fetcher", R"(
        semantics {
          sender: "AssistRanker"
          description:
            "Chrome can provide a better UI experience by using machine "
            "learning models to determine if we should show you or not an "
            "assist prompt. For instance, Chrome may use features such as "
            "the detected language of the current page and the past "
            "interaction with the TransalteUI to decide whether or not we "
            "should offer you to translate this page. Google returns "
            "trained machine learning models that will be used to take "
            "such decision."
          trigger:
            "At startup."
          data:
            "Path to a model. No user data is included."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "NA"
          policy_exception_justification:
            "Not implemented, considered not necessary as no user data is sent."
        })");
  // Create and initialize the URL fetcher.
  fetcher_ = net::URLFetcher::Create(url_, net::URLFetcher::GET, this,
                                     traffic_annotation);
  data_use_measurement::DataUseUserData::AttachToFetcher(
      fetcher_.get(),
      data_use_measurement::DataUseUserData::MACHINE_INTELLIGENCE);
  fetcher_->SetLoadFlags(net::LOAD_DO_NOT_SEND_COOKIES |
                         net::LOAD_DO_NOT_SAVE_COOKIES);
  fetcher_->SetRequestContext(request_context_getter);

  // Set retry parameter for HTTP status code 5xx. This doesn't work against
  // 106 (net::ERR_INTERNET_DISCONNECTED) and so on.
  fetcher_->SetMaxRetriesOn5xx(max_retry_on_5xx_);
  fetcher_->Start();

  return true;
}

void RankerURLFetcher::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK(fetcher_.get() == source);

  std::string data;
  if (source->GetStatus().status() == net::URLRequestStatus::SUCCESS &&
      source->GetResponseCode() == net::HTTP_OK) {
    state_ = COMPLETED;
    source->GetResponseAsString(&data);
  } else {
    state_ = FAILED;
  }

  // Transfer URLFetcher's ownership before invoking a callback.
  std::unique_ptr<const net::URLFetcher> delete_ptr(fetcher_.release());
  callback_.Run(state_ == COMPLETED, data);
}

}  // namespace assist_ranker
