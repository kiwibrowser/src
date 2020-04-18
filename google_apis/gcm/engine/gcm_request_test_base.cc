// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cmath>

#include "google_apis/gcm/engine/gcm_request_test_base.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace {

// Backoff policy for testing registration request.
const net::BackoffEntry::Policy kDefaultBackoffPolicy = {
  // Number of initial errors (in sequence) to ignore before applying
  // exponential back-off rules.
  0,

  // Initial delay for exponential back-off in ms.
  15000,  // 15 seconds.

  // Factor by which the waiting time will be multiplied.
  2,

  // Fuzzing percentage. ex: 10% will spread requests randomly
  // between 90%-100% of the calculated time.
  0.5,  // 50%.

  // Maximum amount of time we are willing to delay our request in ms.
  1000 * 60 * 5, // 5 minutes.

  // Time to keep an entry from being discarded even when it
  // has no significant state, -1 to never discard.
  -1,

  // Don't use initial delay unless the last request was an error.
  false,
};

}  // namespace

namespace gcm {

GCMRequestTestBase::GCMRequestTestBase()
    : task_runner_(new base::TestMockTimeTaskRunner),
      task_runner_handle_(task_runner_),
      url_request_context_getter_(new net::TestURLRequestContextGetter(
          task_runner_)),
      retry_count_(0) {
}

GCMRequestTestBase::~GCMRequestTestBase() {
}

const net::BackoffEntry::Policy& GCMRequestTestBase::GetBackoffPolicy() const {
  return kDefaultBackoffPolicy;
}

net::TestURLFetcher* GCMRequestTestBase::GetFetcher() const {
  return url_fetcher_factory_.GetFetcherByID(0);
}

void GCMRequestTestBase::SetResponse(net::HttpStatusCode status_code,
                                     const std::string& response_body) {
  if (retry_count_++)
    FastForwardToTriggerNextRetry();

  net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(status_code);
  fetcher->SetResponseString(response_body);
}

void GCMRequestTestBase::CompleteFetch() {
  net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->delegate()->OnURLFetchComplete(fetcher);
}

void GCMRequestTestBase::VerifyFetcherUploadData(
    std::map<std::string, std::string>* expected_pairs) {
  net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);

  // Verify data was formatted properly.
  std::string upload_data = fetcher->upload_data();
  base::StringTokenizer data_tokenizer(upload_data, "&=");
  while (data_tokenizer.GetNext()) {
    auto iter = expected_pairs->find(data_tokenizer.token());
    ASSERT_TRUE(iter != expected_pairs->end()) << data_tokenizer.token();
    ASSERT_TRUE(data_tokenizer.GetNext()) << data_tokenizer.token();
    ASSERT_EQ(iter->second, data_tokenizer.token());
    // Ensure that none of the keys appears twice.
    expected_pairs->erase(iter);
  }

  ASSERT_EQ(0UL, expected_pairs->size());
}

void GCMRequestTestBase::FastForwardToTriggerNextRetry() {
  // Here we compute the maximum delay time by skipping the jitter fluctuation
  // that only affects in the negative way.
  int next_retry_delay_ms = kDefaultBackoffPolicy.initial_delay_ms;
  next_retry_delay_ms *=
      pow(kDefaultBackoffPolicy.multiply_factor, retry_count_);
  task_runner_->FastForwardBy(
      base::TimeDelta::FromMilliseconds(next_retry_delay_ms));
}

}  // namespace gcm
