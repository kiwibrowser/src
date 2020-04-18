// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_request_test_base.h"

#include <memory>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/test/mock_entropy_provider.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/offline_page_feature.h"
#include "components/offline_pages/core/prefetch/prefetch_server_urls.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace offline_pages {

const char PrefetchRequestTestBase::kExperimentValueSetInFieldTrial[] =
    "Test Experiment";

PrefetchRequestTestBase::PrefetchRequestTestBase()
    : task_runner_(new base::TestMockTimeTaskRunner),
      task_runner_handle_(task_runner_),
      request_context_(new net::TestURLRequestContextGetter(
          base::ThreadTaskRunnerHandle::Get())) {}

PrefetchRequestTestBase::~PrefetchRequestTestBase() {}

void PrefetchRequestTestBase::SetUp() {
  field_trial_list_ = std::make_unique<base::FieldTrialList>(
      std::make_unique<base::MockEntropyProvider>());
}

void PrefetchRequestTestBase::SetUpExperimentOption() {
  const std::string kTrialName = "trial_name";
  const std::string kGroupName = "group_name";

  scoped_refptr<base::FieldTrial> trial =
      base::FieldTrialList::CreateFieldTrial(kTrialName, kGroupName);

  std::map<std::string, std::string> params;
  params[kPrefetchingOfflinePagesExperimentsOption] =
      kExperimentValueSetInFieldTrial;
  base::AssociateFieldTrialParams(kTrialName, kGroupName, params);

  std::unique_ptr<base::FeatureList> feature_list =
      std::make_unique<base::FeatureList>();
  feature_list->RegisterFieldTrialOverride(
      kPrefetchingOfflinePagesFeature.name,
      base::FeatureList::OVERRIDE_ENABLE_FEATURE, trial.get());
  scoped_feature_list_.InitWithFeatureList(std::move(feature_list));
}

void PrefetchRequestTestBase::RespondWithNetError(int net_error) {
  net::TestURLFetcher* url_fetcher = GetRunningFetcher();
  DCHECK(url_fetcher);
  url_fetcher->set_status(net::URLRequestStatus::FromError(net_error));
  url_fetcher->SetResponseString("");
  url_fetcher->delegate()->OnURLFetchComplete(url_fetcher);
}

void PrefetchRequestTestBase::RespondWithHttpError(int http_error) {
  net::TestURLFetcher* url_fetcher = GetRunningFetcher();
  DCHECK(url_fetcher);
  url_fetcher->set_status(net::URLRequestStatus());
  url_fetcher->set_response_code(http_error);
  url_fetcher->SetResponseString("");
  url_fetcher->delegate()->OnURLFetchComplete(url_fetcher);
}

void PrefetchRequestTestBase::RespondWithData(const std::string& data) {
  net::TestURLFetcher* url_fetcher = GetRunningFetcher();
  DCHECK(url_fetcher);
  url_fetcher->set_status(net::URLRequestStatus());
  url_fetcher->set_response_code(net::HTTP_OK);
  url_fetcher->SetResponseString(data);
  url_fetcher->delegate()->OnURLFetchComplete(url_fetcher);
}

net::TestURLFetcher* PrefetchRequestTestBase::GetRunningFetcher() {
  // All created TestURLFetchers have ID 0 by default.
  return url_fetcher_factory_.GetFetcherByID(0);
}

std::string PrefetchRequestTestBase::GetExperiementHeaderValue(
    net::TestURLFetcher* fetcher) {
  net::HttpRequestHeaders headers;
  fetcher->GetExtraRequestHeaders(&headers);

  std::string experiment_header;
  headers.GetHeader(kPrefetchExperimentHeaderName, &experiment_header);
  return experiment_header;
}

void PrefetchRequestTestBase::RunUntilIdle() {
  task_runner_->RunUntilIdle();
}

void PrefetchRequestTestBase::FastForwardUntilNoTasksRemain() {
  task_runner_->FastForwardUntilNoTasksRemain();
}

}  // namespace offline_pages
