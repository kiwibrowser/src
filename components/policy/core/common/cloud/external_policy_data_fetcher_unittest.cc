// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/cloud/external_policy_data_fetcher.h"

#include <stdint.h>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "base/test/test_simple_task_runner.h"
#include "net/base/net_errors.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {

namespace {

const char* kExternalPolicyDataURLs[] = {
    "http://localhost/data_1",
    "http://localhost/data_2"
};

const int64_t kExternalPolicyDataMaxSize = 5 * 1024 * 1024;  // 5 MB.

const char* kExternalPolicyDataPayload = "External policy data";

}  // namespace

class ExternalPolicyDataFetcherTest : public testing::Test {
 protected:
  ExternalPolicyDataFetcherTest();
  ~ExternalPolicyDataFetcherTest() override;

  // testing::Test:
  void SetUp() override;

  void StartJob(int index);
  void CancelJob(int index);

  void OnJobFinished(int job_index,
                     ExternalPolicyDataFetcher::Result result,
                     std::unique_ptr<std::string> data);
  int GetAndResetCallbackCount();

  net::TestURLFetcherFactory fetcher_factory_;
  scoped_refptr<base::TestSimpleTaskRunner> owner_task_runner_;
  scoped_refptr<base::TestSimpleTaskRunner> io_task_runner_;
  std::unique_ptr<ExternalPolicyDataFetcherBackend> fetcher_backend_;
  std::unique_ptr<ExternalPolicyDataFetcher> fetcher_;

  std::map<int, ExternalPolicyDataFetcher::Job*> jobs_;  // Not owned.

  int callback_count_;
  int callback_job_index_;
  ExternalPolicyDataFetcher::Result callback_result_;
  std::unique_ptr<std::string> callback_data_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ExternalPolicyDataFetcherTest);
};

ExternalPolicyDataFetcherTest::ExternalPolicyDataFetcherTest()
    : callback_count_(0) {
}

ExternalPolicyDataFetcherTest::~ExternalPolicyDataFetcherTest() {
}

void ExternalPolicyDataFetcherTest::SetUp() {
  fetcher_factory_.set_remove_fetcher_on_delete(true);
  io_task_runner_ = new base::TestSimpleTaskRunner();
  owner_task_runner_ = new base::TestSimpleTaskRunner();
  fetcher_backend_.reset(new ExternalPolicyDataFetcherBackend(
      io_task_runner_,
      scoped_refptr<net::URLRequestContextGetter>()));
  fetcher_.reset(
      fetcher_backend_->CreateFrontend(owner_task_runner_).release());
}

void ExternalPolicyDataFetcherTest::StartJob(int index) {
  jobs_[index] = fetcher_->StartJob(
      GURL(kExternalPolicyDataURLs[index]),
      kExternalPolicyDataMaxSize,
      base::Bind(&ExternalPolicyDataFetcherTest::OnJobFinished,
                 base::Unretained(this), index));
  io_task_runner_->RunUntilIdle();
}

void ExternalPolicyDataFetcherTest::CancelJob(int index) {
  std::map<int, ExternalPolicyDataFetcher::Job*>::iterator it =
      jobs_.find(index);
  ASSERT_TRUE(it != jobs_.end());
  ExternalPolicyDataFetcher::Job* job = it->second;
  jobs_.erase(it);
  fetcher_->CancelJob(job);
}

void ExternalPolicyDataFetcherTest::OnJobFinished(
    int job_index,
    ExternalPolicyDataFetcher::Result result,
    std::unique_ptr<std::string> data) {
  ++callback_count_;
  callback_job_index_ = job_index;
  callback_result_ = result;
  callback_data_ = std::move(data);
  jobs_.erase(job_index);
}

int ExternalPolicyDataFetcherTest::GetAndResetCallbackCount() {
  const int callback_count = callback_count_;
  callback_count_ = 0;
  return callback_count;
}

TEST_F(ExternalPolicyDataFetcherTest, Success) {
  // Start a fetch job.
  StartJob(0);

  // Verify that the fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Complete the fetch.
  fetcher->set_response_code(200);
  fetcher->SetResponseString(kExternalPolicyDataPayload);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Verify that the fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the callback is invoked with the retrieved data.
  owner_task_runner_->RunUntilIdle();
  EXPECT_EQ(1, GetAndResetCallbackCount());
  EXPECT_EQ(0, callback_job_index_);
  EXPECT_EQ(ExternalPolicyDataFetcher::SUCCESS, callback_result_);
  ASSERT_TRUE(callback_data_);
  EXPECT_EQ(kExternalPolicyDataPayload, *callback_data_);
}

TEST_F(ExternalPolicyDataFetcherTest, MaxSizeExceeded) {
  // Start a fetch job.
  StartJob(0);

  // Verify that the fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Indicate that the data size will exceed maximum allowed.
  fetcher->delegate()->OnURLFetchDownloadProgress(
      fetcher, kExternalPolicyDataMaxSize + 1, -1,
      kExternalPolicyDataMaxSize + 1);

  // Verify that the fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the callback is invoked with the correct error code.
  owner_task_runner_->RunUntilIdle();
  EXPECT_EQ(1, GetAndResetCallbackCount());
  EXPECT_EQ(0, callback_job_index_);
  EXPECT_EQ(ExternalPolicyDataFetcher::MAX_SIZE_EXCEEDED, callback_result_);
  EXPECT_FALSE(callback_data_);
}

TEST_F(ExternalPolicyDataFetcherTest, ConnectionInterrupted) {
  // Start a fetch job.
  StartJob(0);

  // Verify that the fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Make the fetch fail due to an interrupted connection.
  fetcher->set_status(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                            net::ERR_CONNECTION_RESET));
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Verify that the fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the callback is invoked with the correct error code.
  owner_task_runner_->RunUntilIdle();
  EXPECT_EQ(1, GetAndResetCallbackCount());
  EXPECT_EQ(0, callback_job_index_);
  EXPECT_EQ(ExternalPolicyDataFetcher::CONNECTION_INTERRUPTED,
            callback_result_);
  EXPECT_FALSE(callback_data_);
}

TEST_F(ExternalPolicyDataFetcherTest, NetworkError) {
  // Start a fetch job.
  StartJob(0);

  // Verify that the fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Make the fetch fail due to a network error.
  fetcher->set_status(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                            net::ERR_NETWORK_CHANGED));
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Verify that the fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the callback is invoked with the correct error code.
  owner_task_runner_->RunUntilIdle();
  EXPECT_EQ(1, GetAndResetCallbackCount());
  EXPECT_EQ(0, callback_job_index_);
  EXPECT_EQ(ExternalPolicyDataFetcher::NETWORK_ERROR, callback_result_);
  EXPECT_FALSE(callback_data_);
}

TEST_F(ExternalPolicyDataFetcherTest, ServerError) {
  // Start a fetch job.
  StartJob(0);

  // Verify that the fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Make the fetch fail with a server error.
  fetcher->set_response_code(500);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Verify that the fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the callback is invoked with the correct error code.
  owner_task_runner_->RunUntilIdle();
  EXPECT_EQ(1, GetAndResetCallbackCount());
  EXPECT_EQ(0, callback_job_index_);
  EXPECT_EQ(ExternalPolicyDataFetcher::SERVER_ERROR, callback_result_);
  EXPECT_FALSE(callback_data_);
}

TEST_F(ExternalPolicyDataFetcherTest, ClientError) {
  // Start a fetch job.
  StartJob(0);

  // Verify that the fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Make the fetch fail with a client error.
  fetcher->set_response_code(400);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Verify that the fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the callback is invoked with the correct error code.
  owner_task_runner_->RunUntilIdle();
  EXPECT_EQ(1, GetAndResetCallbackCount());
  EXPECT_EQ(0, callback_job_index_);
  EXPECT_EQ(ExternalPolicyDataFetcher::CLIENT_ERROR, callback_result_);
  EXPECT_FALSE(callback_data_);
}

TEST_F(ExternalPolicyDataFetcherTest, HTTPError) {
  // Start a fetch job.
  StartJob(0);

  // Verify that the fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Make the fetch fail with an HTTP error.
  fetcher->set_response_code(300);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Verify that the fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the callback is invoked with the correct error code.
  owner_task_runner_->RunUntilIdle();
  EXPECT_EQ(1, GetAndResetCallbackCount());
  EXPECT_EQ(0, callback_job_index_);
  EXPECT_EQ(ExternalPolicyDataFetcher::HTTP_ERROR, callback_result_);
  EXPECT_FALSE(callback_data_);
}

TEST_F(ExternalPolicyDataFetcherTest, Canceled) {
  // Start a fetch job.
  StartJob(0);

  // Verify that the fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Cancel the fetch job.
  CancelJob(0);
  io_task_runner_->RunUntilIdle();

  // Verify that the fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the callback is not invoked.
  owner_task_runner_->RunUntilIdle();
  EXPECT_EQ(0, GetAndResetCallbackCount());
}

TEST_F(ExternalPolicyDataFetcherTest, SuccessfulCanceled) {
  // Start a fetch job.
  StartJob(0);

  // Verify that the fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Complete the fetch.
  fetcher->set_response_code(200);
  fetcher->SetResponseString(kExternalPolicyDataPayload);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Verify that the fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Cancel the fetch job before the successful fetch result has arrived from
  // the backend.
  CancelJob(0);

  // Verify that the callback is not invoked.
  owner_task_runner_->RunUntilIdle();
  EXPECT_EQ(0, GetAndResetCallbackCount());
}

TEST_F(ExternalPolicyDataFetcherTest, ParallelJobs) {
  // Start two fetch jobs.
  StartJob(0);
  StartJob(1);

  // Verify that the second fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[1]), fetcher->GetOriginalURL());

  // Verify that the first fetch has been started.
  fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Complete the first fetch.
  fetcher->set_response_code(200);
  fetcher->SetResponseString(kExternalPolicyDataPayload);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Verify that the first fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the callback is invoked with the retrieved data.
  owner_task_runner_->RunUntilIdle();
  EXPECT_EQ(1, GetAndResetCallbackCount());
  EXPECT_EQ(0, callback_job_index_);
  EXPECT_EQ(ExternalPolicyDataFetcher::SUCCESS, callback_result_);
  ASSERT_TRUE(callback_data_);
  EXPECT_EQ(kExternalPolicyDataPayload, *callback_data_);

  // Verify that the second fetch is still running.
  fetcher = fetcher_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[1]), fetcher->GetOriginalURL());

  // Complete the second fetch.
  fetcher->set_response_code(200);
  fetcher->SetResponseString(kExternalPolicyDataPayload);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Verify that the second fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(1));

  // Verify that the callback is invoked with the retrieved data.
  owner_task_runner_->RunUntilIdle();
  EXPECT_EQ(1, GetAndResetCallbackCount());
  EXPECT_EQ(1, callback_job_index_);
  EXPECT_EQ(ExternalPolicyDataFetcher::SUCCESS, callback_result_);
  ASSERT_TRUE(callback_data_);
  EXPECT_EQ(kExternalPolicyDataPayload, *callback_data_);
}

TEST_F(ExternalPolicyDataFetcherTest, ParallelJobsFinishingOutOfOrder) {
  // Start two fetch jobs.
  StartJob(0);
  StartJob(1);

  // Verify that the first fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Verify that the second fetch has been started.
  fetcher = fetcher_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[1]), fetcher->GetOriginalURL());

  // Complete the second fetch.
  fetcher->set_response_code(200);
  fetcher->SetResponseString(kExternalPolicyDataPayload);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Verify that the second fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(1));

  // Verify that the callback is invoked with the retrieved data.
  owner_task_runner_->RunUntilIdle();
  EXPECT_EQ(1, GetAndResetCallbackCount());
  EXPECT_EQ(1, callback_job_index_);
  EXPECT_EQ(ExternalPolicyDataFetcher::SUCCESS, callback_result_);
  ASSERT_TRUE(callback_data_);
  EXPECT_EQ(kExternalPolicyDataPayload, *callback_data_);

  // Verify that the first fetch is still running.
  fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Complete the first fetch.
  fetcher->set_response_code(200);
  fetcher->SetResponseString(kExternalPolicyDataPayload);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Verify that the first fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the callback is invoked with the retrieved data.
  owner_task_runner_->RunUntilIdle();
  EXPECT_EQ(1, GetAndResetCallbackCount());
  EXPECT_EQ(0, callback_job_index_);
  EXPECT_EQ(ExternalPolicyDataFetcher::SUCCESS, callback_result_);
  ASSERT_TRUE(callback_data_);
  EXPECT_EQ(kExternalPolicyDataPayload, *callback_data_);
}

TEST_F(ExternalPolicyDataFetcherTest, ParallelJobsWithCancel) {
  // Start two fetch jobs.
  StartJob(0);
  StartJob(1);

  // Verify that the second fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[1]), fetcher->GetOriginalURL());

  // Verify that the first fetch has been started.
  fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Cancel the first fetch job.
  CancelJob(0);
  io_task_runner_->RunUntilIdle();

  // Verify that the first fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the callback is not invoked.
  owner_task_runner_->RunUntilIdle();
  EXPECT_EQ(0, GetAndResetCallbackCount());

  // Verify that the second fetch is still running.
  fetcher = fetcher_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[1]), fetcher->GetOriginalURL());

  // Complete the second fetch.
  fetcher->set_response_code(200);
  fetcher->SetResponseString(kExternalPolicyDataPayload);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Verify that the second fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(1));

  // Verify that the callback is invoked with the retrieved data.
  owner_task_runner_->RunUntilIdle();
  EXPECT_EQ(1, GetAndResetCallbackCount());
  EXPECT_EQ(1, callback_job_index_);
  EXPECT_EQ(ExternalPolicyDataFetcher::SUCCESS, callback_result_);
  ASSERT_TRUE(callback_data_);
  EXPECT_EQ(kExternalPolicyDataPayload, *callback_data_);
}

}  // namespace policy
