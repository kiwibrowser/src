// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_REQUEST_FETCHER_TEST_BASE_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_REQUEST_FETCHER_TEST_BASE_H_

#include "base/memory/ref_counted.h"
#include "base/metrics/field_trial.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

// Base class for testing prefetch requests with simulated responses.
class PrefetchRequestTestBase : public testing::Test {
 public:
  static const char kExperimentValueSetInFieldTrial[];

  PrefetchRequestTestBase();
  ~PrefetchRequestTestBase() override;

  void SetUp() override;

  void SetUpExperimentOption();

  void RespondWithNetError(int net_error);
  void RespondWithHttpError(int http_error);
  void RespondWithData(const std::string& data);
  net::TestURLFetcher* GetRunningFetcher();
  std::string GetExperiementHeaderValue(net::TestURLFetcher* fetcher);

  net::URLRequestContextGetter* request_context() const {
    return request_context_.get();
  }

  void RunUntilIdle();
  void FastForwardUntilNoTasksRemain();

 protected:
  // Derived classes may need these to construct other members.
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner() {
    return task_runner_;
  }

 private:
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  net::TestURLFetcherFactory url_fetcher_factory_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_;
  std::unique_ptr<base::FieldTrialList> field_trial_list_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_REQUEST_FETCHER_TEST_BASE_H_
