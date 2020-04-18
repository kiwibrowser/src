// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/cloud/external_policy_data_updater.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/test/test_pending_task.h"
#include "base/test/test_simple_task_runner.h"
#include "base/time/time.h"
#include "components/policy/core/common/cloud/external_policy_data_fetcher.h"
#include "crypto/sha2.h"
#include "net/base/net_errors.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using testing::Mock;
using testing::Return;

namespace policy {

namespace {

const char* kExternalPolicyDataKeys[] = {"external_policy_data_1",
                                         "external_policy_data_2",
                                         "external_policy_data_3"};
const char* kExternalPolicyDataURLs[] = {"http://example.com/data_1",
                                         "http://example.com/data_2",
                                         "http://example.com/data_3"};
const char* kExternalPolicyDataPayload = "External policy data";

const int64_t kExternalPolicyDataMaxSize = 5 * 1024 * 1024;  // 5 MB.

class MockFetchSuccessCallbackListener {
 public:
  MOCK_METHOD2(OnFetchSuccess, bool(const std::string&, const std::string&));

  ExternalPolicyDataUpdater::FetchSuccessCallback CreateCallback(
      const std::string& key);
};

ExternalPolicyDataUpdater::FetchSuccessCallback
    MockFetchSuccessCallbackListener::CreateCallback(const std::string& key) {
  return base::Bind(&MockFetchSuccessCallbackListener::OnFetchSuccess,
                    base::Unretained(this),
                    key);
}

}  // namespace

class ExternalPolicyDataUpdaterTest : public testing::Test {
 protected:
  void SetUp() override;

  void CreateUpdater(size_t max_parallel_fetches);
  ExternalPolicyDataUpdater::Request CreateRequest(
      const std::string& url) const;
  void RequestExternalDataFetch(int key_index, int url_index);
  void RequestExternalDataFetch(int index);

  net::TestURLFetcherFactory fetcher_factory_;
  MockFetchSuccessCallbackListener callback_listener_;
  scoped_refptr<base::TestSimpleTaskRunner> backend_task_runner_;
  scoped_refptr<base::TestSimpleTaskRunner> io_task_runner_;
  std::unique_ptr<ExternalPolicyDataFetcherBackend> fetcher_backend_;
  std::unique_ptr<ExternalPolicyDataUpdater> updater_;
};

void ExternalPolicyDataUpdaterTest::SetUp() {
  fetcher_factory_.set_remove_fetcher_on_delete(true);
  backend_task_runner_ = new base::TestSimpleTaskRunner();
  io_task_runner_ = new base::TestSimpleTaskRunner();
}

void ExternalPolicyDataUpdaterTest::CreateUpdater(size_t max_parallel_fetches) {
  fetcher_backend_.reset(new ExternalPolicyDataFetcherBackend(
      io_task_runner_,
      scoped_refptr<net::URLRequestContextGetter>()));
  updater_.reset(new ExternalPolicyDataUpdater(
      backend_task_runner_,
      fetcher_backend_->CreateFrontend(backend_task_runner_),
      max_parallel_fetches));
}

void ExternalPolicyDataUpdaterTest::RequestExternalDataFetch(int key_index,
                                                             int url_index) {
  updater_->FetchExternalData(
      kExternalPolicyDataKeys[key_index],
      CreateRequest(kExternalPolicyDataURLs[url_index]),
      callback_listener_.CreateCallback(kExternalPolicyDataKeys[key_index]));
  io_task_runner_->RunUntilIdle();
  backend_task_runner_->RunPendingTasks();
}

void ExternalPolicyDataUpdaterTest::RequestExternalDataFetch(int index) {
  RequestExternalDataFetch(index, index);
}

ExternalPolicyDataUpdater::Request
    ExternalPolicyDataUpdaterTest::CreateRequest(const std::string& url) const {
  return ExternalPolicyDataUpdater::Request(
      url,
      crypto::SHA256HashString(kExternalPolicyDataPayload),
      kExternalPolicyDataMaxSize);
}

TEST_F(ExternalPolicyDataUpdaterTest, FetchSuccess) {
  // Create an updater that runs one fetch at a time.
  CreateUpdater(1);

  // Make two fetch requests.
  RequestExternalDataFetch(0);
  RequestExternalDataFetch(1);

  // Verify that the second fetch has not been started yet.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(1));

  // Verify that the first fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Complete the first fetch.
  fetcher->set_response_code(200);
  fetcher->SetResponseString(kExternalPolicyDataPayload);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Accept the data when the callback is invoked.
  EXPECT_CALL(callback_listener_,
              OnFetchSuccess(kExternalPolicyDataKeys[0],
                             kExternalPolicyDataPayload))
      .Times(1)
      .WillOnce(Return(true));
  backend_task_runner_->RunPendingTasks();
  Mock::VerifyAndClearExpectations(&callback_listener_);
  io_task_runner_->RunUntilIdle();

  // Verify that the first fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the second fetch has been started.
  fetcher = fetcher_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[1]), fetcher->GetOriginalURL());

  // Verify that no retries have been scheduled.
  EXPECT_FALSE(backend_task_runner_->HasPendingTask());
}

TEST_F(ExternalPolicyDataUpdaterTest, PayloadSizeExceedsLimit) {
  // Create an updater that runs one fetch at a time.
  CreateUpdater(1);

  // Make two fetch requests.
  RequestExternalDataFetch(0);
  RequestExternalDataFetch(1);

  // Verify that the second fetch has not been started yet.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(1));

  // Verify that the first fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Indicate that the payload size will exceed allowed maximum.
  fetcher->delegate()->OnURLFetchDownloadProgress(
      fetcher, kExternalPolicyDataMaxSize + 1, -1,
      kExternalPolicyDataMaxSize + 1);
  backend_task_runner_->RunPendingTasks();
  io_task_runner_->RunUntilIdle();

  // Verify that the first fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the second fetch has been started.
  fetcher = fetcher_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[1]), fetcher->GetOriginalURL());

  // Verify that a retry has been scheduled for the first fetch.
  EXPECT_EQ(1u, backend_task_runner_->NumPendingTasks());
}

TEST_F(ExternalPolicyDataUpdaterTest, FetchFailure) {
  // Create an updater that runs one fetch at a time.
  CreateUpdater(1);

  // Make two fetch requests.
  RequestExternalDataFetch(0);
  RequestExternalDataFetch(1);

  // Verify that the second fetch has not been started yet.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(1));

  // Verify that the first fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Make the first fetch fail due to an interrupted connection.
  fetcher->set_status(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                            net::ERR_NETWORK_CHANGED));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  backend_task_runner_->RunPendingTasks();
  io_task_runner_->RunUntilIdle();

  // Verify that the first fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the second fetch has been started.
  fetcher = fetcher_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[1]), fetcher->GetOriginalURL());

  // Verify that a retry has been scheduled for the first fetch.
  EXPECT_EQ(1u, backend_task_runner_->NumPendingTasks());
}

TEST_F(ExternalPolicyDataUpdaterTest, ServerFailure) {
  // Create an updater that runs one fetch at a time.
  CreateUpdater(1);

  // Make two fetch requests.
  RequestExternalDataFetch(0);
  RequestExternalDataFetch(1);

  // Verify that the second fetch has not been started yet.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(1));

  // Verify that the first fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Make the first fetch fail with a server error.
  fetcher->set_response_code(500);
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  backend_task_runner_->RunPendingTasks();
  io_task_runner_->RunUntilIdle();

  // Verify that the first fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the second fetch has been started.
  fetcher = fetcher_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[1]), fetcher->GetOriginalURL());

  // Verify that a retry has been scheduled for the first fetch.
  EXPECT_EQ(1u, backend_task_runner_->NumPendingTasks());
}

TEST_F(ExternalPolicyDataUpdaterTest, RetryLimit) {
  // Create an updater that runs one fetch at a time.
  CreateUpdater(1);

  // Make a fetch request.
  RequestExternalDataFetch(0);

  int fetcher_id = 0;

  // Verify that client failures cause the fetch to be retried three times.
  for (int i = 0; i < 3; ++i) {
    // Verify that the fetch has been (re)started.
    net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(fetcher_id);
    ASSERT_TRUE(fetcher);
    EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

    // Make the fetch fail with a client error.
    fetcher->set_response_code(400);
    fetcher->delegate()->OnURLFetchComplete(fetcher);
    backend_task_runner_->RunPendingTasks();
    io_task_runner_->RunUntilIdle();

    // Verify that the fetch is no longer running.
    EXPECT_FALSE(fetcher_factory_.GetFetcherByID(fetcher_id));

    // Verify that a retry has been scheduled.
    EXPECT_EQ(1u, backend_task_runner_->NumPendingTasks());

    // Fast-forward time to the scheduled retry.
    backend_task_runner_->RunPendingTasks();
    io_task_runner_->RunUntilIdle();
    EXPECT_FALSE(backend_task_runner_->HasPendingTask());
    ++fetcher_id;
  }

  // Verify that the fetch has been restarted.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(fetcher_id);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Make the fetch fail once more.
  fetcher->set_response_code(400);
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  backend_task_runner_->RunPendingTasks();
  io_task_runner_->RunUntilIdle();
  ++fetcher_id;

  // Verify that the fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(fetcher_id));

  // Verify that no further retries have been scheduled.
  EXPECT_FALSE(backend_task_runner_->HasPendingTask());
}

TEST_F(ExternalPolicyDataUpdaterTest, RetryWithBackoff) {
  // Create an updater that runs one fetch at a time.
  CreateUpdater(1);

  // Make a fetch request.
  RequestExternalDataFetch(0);

  base::TimeDelta expected_delay = base::TimeDelta::FromSeconds(15);
  const base::TimeDelta delay_cap = base::TimeDelta::FromHours(12);

  int fetcher_id = 0;

  // The backoff delay is capped at 12 hours, which is reached after 12 retries:
  // 15 * 2^12 == 61440 > 43200 == 12 * 60 * 60
  for (int i = 0; i < 20; ++i) {
    // Verify that the fetch has been (re)started.
    net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(fetcher_id);
    ASSERT_TRUE(fetcher);
    EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

    // Make the fetch fail with a server error.
    fetcher->set_response_code(500);
    fetcher->delegate()->OnURLFetchComplete(fetcher);
    backend_task_runner_->RunPendingTasks();
    io_task_runner_->RunUntilIdle();

    // Verify that the fetch is no longer running.
    EXPECT_FALSE(fetcher_factory_.GetFetcherByID(fetcher_id));

    // Verify that a retry has been scheduled.
    EXPECT_EQ(1u, backend_task_runner_->NumPendingTasks());

    // Verify that the retry delay has been doubled, with random jitter from 80%
    // to 100%.
    base::TimeDelta delay = backend_task_runner_->NextPendingTaskDelay();
    EXPECT_GT(delay,
              base::TimeDelta::FromMilliseconds(
                  0.799 * expected_delay.InMilliseconds()));
    EXPECT_LE(delay, expected_delay);

    if (i < 12) {
      // The delay cap has not been reached yet.
      EXPECT_LT(expected_delay, delay_cap);
      expected_delay *= 2;

      if (i == 11) {
        // The last doubling reached the cap.
        EXPECT_GT(expected_delay, delay_cap);
        expected_delay = delay_cap;
      }
    }

    // Fast-forward time to the scheduled retry.
    backend_task_runner_->RunPendingTasks();
    io_task_runner_->RunUntilIdle();
    EXPECT_FALSE(backend_task_runner_->HasPendingTask());
    ++fetcher_id;
  }
}

TEST_F(ExternalPolicyDataUpdaterTest, HashInvalid) {
  // Create an updater that runs one fetch at a time.
  CreateUpdater(1);

  // Make two fetch requests.
  RequestExternalDataFetch(0);
  RequestExternalDataFetch(1);

  // Verify that the second fetch has not been started yet.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(1));

  // Verify that the first fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Make the first fetch retrieve data whose hash does not match the expected
  // value.
  fetcher->set_response_code(200);
  fetcher->SetResponseString("Invalid external policy data");
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  backend_task_runner_->RunPendingTasks();
  io_task_runner_->RunUntilIdle();

  // Verify that the first fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the second fetch has been started.
  fetcher = fetcher_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[1]), fetcher->GetOriginalURL());

  // Verify that a retry has been scheduled for the first fetch.
  EXPECT_EQ(1u, backend_task_runner_->NumPendingTasks());
}

TEST_F(ExternalPolicyDataUpdaterTest, DataRejectedByCallback) {
  // Create an updater that runs one fetch at a time.
  CreateUpdater(1);

  // Make a fetch request.
  RequestExternalDataFetch(0);

  // Verify that the fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Complete the fetch.
  fetcher->set_response_code(200);
  fetcher->SetResponseString(kExternalPolicyDataPayload);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Reject the data when the callback is invoked.
  EXPECT_CALL(callback_listener_,
              OnFetchSuccess(kExternalPolicyDataKeys[0],
                             kExternalPolicyDataPayload))
      .Times(1)
      .WillOnce(Return(false));
  backend_task_runner_->RunPendingTasks();
  Mock::VerifyAndClearExpectations(&callback_listener_);
  io_task_runner_->RunUntilIdle();

  // Verify that the fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that a retry has been scheduled.
  EXPECT_EQ(1u, backend_task_runner_->NumPendingTasks());

  // Fast-forward time to the scheduled retry.
  backend_task_runner_->RunPendingTasks();
  io_task_runner_->RunUntilIdle();
  EXPECT_FALSE(backend_task_runner_->HasPendingTask());

  // Verify that the fetch has been restarted.
  fetcher = fetcher_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Complete the fetch.
  fetcher->set_response_code(200);
  fetcher->SetResponseString(kExternalPolicyDataPayload);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Accept the data when the callback is invoked this time.
  EXPECT_CALL(callback_listener_,
              OnFetchSuccess(kExternalPolicyDataKeys[0],
                             kExternalPolicyDataPayload))
      .Times(1)
      .WillOnce(Return(true));
  backend_task_runner_->RunPendingTasks();
  Mock::VerifyAndClearExpectations(&callback_listener_);
  io_task_runner_->RunUntilIdle();

  // Verify that no retries have been scheduled.
  EXPECT_FALSE(backend_task_runner_->HasPendingTask());
}

TEST_F(ExternalPolicyDataUpdaterTest, URLChanged) {
  // Create an updater that runs one fetch at a time.
  CreateUpdater(1);

  // Make a fetch request.
  RequestExternalDataFetch(0);

  // Verify that the fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Make another fetch request with the same key but an updated URL.
  RequestExternalDataFetch(0, 1);

  // Verify that the original fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that a new fetch has been started with the updated URL.
  fetcher = fetcher_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[1]), fetcher->GetOriginalURL());

  // Verify that no retries have been scheduled.
  EXPECT_FALSE(backend_task_runner_->HasPendingTask());
}

TEST_F(ExternalPolicyDataUpdaterTest, JobInvalidated) {
  // Create an updater that runs one fetch at a time.
  CreateUpdater(1);

  // Make two fetch requests.
  RequestExternalDataFetch(0);
  RequestExternalDataFetch(1);

  // Verify that the second fetch has not been started yet.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(1));

  // Verify that the first fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Make another fetch request with the same key as the second request but an
  // updated URL.
  RequestExternalDataFetch(1, 2);

  // Verify that the first fetch is still running.
  fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Verify that the second fetch has still not been started.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(1));

  // Make the first fetch fail with a server error.
  fetcher->set_response_code(500);
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  backend_task_runner_->RunPendingTasks();
  io_task_runner_->RunUntilIdle();

  // Verify that the first fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the second fetch was invalidated and the third fetch has been
  // started instead.
  fetcher = fetcher_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[2]), fetcher->GetOriginalURL());
}

TEST_F(ExternalPolicyDataUpdaterTest, FetchCanceled) {
  // Create an updater that runs one fetch at a time.
  CreateUpdater(1);

  // Make a fetch request.
  RequestExternalDataFetch(0);

  // Verify that the fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Cancel the fetch request.
  updater_->CancelExternalDataFetch(kExternalPolicyDataKeys[0]);
  io_task_runner_->RunUntilIdle();
  backend_task_runner_->RunPendingTasks();

  // Verify that the fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that no retries have been scheduled.
  EXPECT_FALSE(backend_task_runner_->HasPendingTask());
}

TEST_F(ExternalPolicyDataUpdaterTest, ParallelJobs) {
  // Create an updater that runs up to two fetches in parallel.
  CreateUpdater(2);

  // Make three fetch requests.
  RequestExternalDataFetch(0);
  RequestExternalDataFetch(1);
  RequestExternalDataFetch(2);

  // Verify that the third fetch has not been started yet.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(2));

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

  // Accept the data when the callback is invoked.
  EXPECT_CALL(callback_listener_,
              OnFetchSuccess(kExternalPolicyDataKeys[0],
                             kExternalPolicyDataPayload))
      .Times(1)
      .WillOnce(Return(true));
  backend_task_runner_->RunPendingTasks();
  Mock::VerifyAndClearExpectations(&callback_listener_);
  io_task_runner_->RunUntilIdle();

  // Verify that the first fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the third fetch has been started.
  fetcher = fetcher_factory_.GetFetcherByID(2);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[2]), fetcher->GetOriginalURL());

  // Verify that the second fetch is still running.
  fetcher = fetcher_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[1]), fetcher->GetOriginalURL());

  // Complete the second fetch.
  fetcher->set_response_code(200);
  fetcher->SetResponseString(kExternalPolicyDataPayload);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Accept the data when the callback is invoked.
  EXPECT_CALL(callback_listener_,
              OnFetchSuccess(kExternalPolicyDataKeys[1],
                             kExternalPolicyDataPayload))
      .Times(1)
      .WillOnce(Return(true));
  backend_task_runner_->RunPendingTasks();
  Mock::VerifyAndClearExpectations(&callback_listener_);
  io_task_runner_->RunUntilIdle();

  // Verify that the second fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(1));

  // Verify that the third fetch is still running.
  fetcher = fetcher_factory_.GetFetcherByID(2);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[2]), fetcher->GetOriginalURL());

  // Complete the third fetch.
  fetcher->set_response_code(200);
  fetcher->SetResponseString(kExternalPolicyDataPayload);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Accept the data when the callback is invoked.
  EXPECT_CALL(callback_listener_,
              OnFetchSuccess(kExternalPolicyDataKeys[2],
                             kExternalPolicyDataPayload))
      .Times(1)
      .WillOnce(Return(true));
  backend_task_runner_->RunPendingTasks();
  Mock::VerifyAndClearExpectations(&callback_listener_);
  io_task_runner_->RunUntilIdle();

  // Verify that the third fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(2));

  // Verify that no retries have been scheduled.
  EXPECT_FALSE(backend_task_runner_->HasPendingTask());
}

TEST_F(ExternalPolicyDataUpdaterTest, ParallelJobsFinishingOutOfOrder) {
  // Create an updater that runs up to two fetches in parallel.
  CreateUpdater(2);

  // Make three fetch requests.
  RequestExternalDataFetch(0);
  RequestExternalDataFetch(1);
  RequestExternalDataFetch(2);

  // Verify that the third fetch has not been started yet.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(2));

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

  // Accept the data when the callback is invoked.
  EXPECT_CALL(callback_listener_,
              OnFetchSuccess(kExternalPolicyDataKeys[1],
                             kExternalPolicyDataPayload))
      .Times(1)
      .WillOnce(Return(true));
  backend_task_runner_->RunPendingTasks();
  Mock::VerifyAndClearExpectations(&callback_listener_);
  io_task_runner_->RunUntilIdle();

  // Verify that the second fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(1));

  // Verify that the third fetch has been started.
  fetcher = fetcher_factory_.GetFetcherByID(2);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[2]), fetcher->GetOriginalURL());

  // Verify that the first fetch is still running.
  fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Complete the first fetch.
  fetcher->set_response_code(200);
  fetcher->SetResponseString(kExternalPolicyDataPayload);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Accept the data when the callback is invoked.
  EXPECT_CALL(callback_listener_,
              OnFetchSuccess(kExternalPolicyDataKeys[0],
                             kExternalPolicyDataPayload))
      .Times(1)
      .WillOnce(Return(true));
  backend_task_runner_->RunPendingTasks();
  Mock::VerifyAndClearExpectations(&callback_listener_);
  io_task_runner_->RunUntilIdle();

  // Verify that the first fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the third fetch is still running.
  fetcher = fetcher_factory_.GetFetcherByID(2);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[2]), fetcher->GetOriginalURL());

  // Complete the third fetch.
  fetcher->set_response_code(200);
  fetcher->SetResponseString(kExternalPolicyDataPayload);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Accept the data when the callback is invoked.
  EXPECT_CALL(callback_listener_,
              OnFetchSuccess(kExternalPolicyDataKeys[2],
                             kExternalPolicyDataPayload))
      .Times(1)
      .WillOnce(Return(true));
  backend_task_runner_->RunPendingTasks();
  Mock::VerifyAndClearExpectations(&callback_listener_);
  io_task_runner_->RunUntilIdle();

  // Verify that the third fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(2));

  // Verify that no retries have been scheduled.
  EXPECT_FALSE(backend_task_runner_->HasPendingTask());
}

TEST_F(ExternalPolicyDataUpdaterTest, ParallelJobsWithRetry) {
  // Create an updater that runs up to two fetches in parallel.
  CreateUpdater(2);

  // Make three fetch requests.
  RequestExternalDataFetch(0);
  RequestExternalDataFetch(1);
  RequestExternalDataFetch(2);

  // Verify that the third fetch has not been started yet.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(2));

  // Verify that the second fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[1]), fetcher->GetOriginalURL());

  // Verify that the first fetch has been started.
  fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Make the first fetch fail with a client error.
  fetcher->set_response_code(400);
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  backend_task_runner_->RunPendingTasks();
  io_task_runner_->RunUntilIdle();

  // Verify that the first fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the third fetch has been started.
  fetcher = fetcher_factory_.GetFetcherByID(2);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[2]), fetcher->GetOriginalURL());

  // Verify that a retry has been scheduled for the first fetch.
  EXPECT_EQ(1u, backend_task_runner_->NumPendingTasks());

  // Fast-forward time to the scheduled retry.
  backend_task_runner_->RunPendingTasks();
  io_task_runner_->RunUntilIdle();
  EXPECT_FALSE(backend_task_runner_->HasPendingTask());

  // Verify that the first fetch has not been restarted yet.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(3));

  // Complete the third fetch.
  fetcher->set_response_code(200);
  fetcher->SetResponseString(kExternalPolicyDataPayload);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Accept the data when the callback is invoked.
  EXPECT_CALL(callback_listener_,
              OnFetchSuccess(kExternalPolicyDataKeys[2],
                             kExternalPolicyDataPayload))
      .Times(1)
      .WillOnce(Return(true));
  backend_task_runner_->RunPendingTasks();
  Mock::VerifyAndClearExpectations(&callback_listener_);
  io_task_runner_->RunUntilIdle();

  // Verify that the third fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(2));

  // Verify that the second fetch is still running
  fetcher = fetcher_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[1]), fetcher->GetOriginalURL());

  // Verify that the first fetch has been restarted.
  fetcher = fetcher_factory_.GetFetcherByID(3);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Verify that no further retries have been scheduled.
  EXPECT_FALSE(backend_task_runner_->HasPendingTask());
}

TEST_F(ExternalPolicyDataUpdaterTest, ParallelJobsWithCancel) {
  // Create an updater that runs up to two fetches in parallel.
  CreateUpdater(2);

  // Make three fetch requests.
  RequestExternalDataFetch(0);
  RequestExternalDataFetch(1);
  RequestExternalDataFetch(2);

  // Verify that the third fetch has not been started yet.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(2));

  // Verify that the second fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[1]), fetcher->GetOriginalURL());

  // Verify that the first fetch has been started.
  fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Cancel the fetch request.
  updater_->CancelExternalDataFetch(kExternalPolicyDataKeys[0]);
  io_task_runner_->RunUntilIdle();
  backend_task_runner_->RunPendingTasks();

  // Verify that the fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(0));

  // Verify that the third fetch has been started.
  fetcher = fetcher_factory_.GetFetcherByID(2);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[2]), fetcher->GetOriginalURL());

  // Verify that the second fetch is still running.
  fetcher = fetcher_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[1]), fetcher->GetOriginalURL());

  // Complete the second fetch.
  fetcher->set_response_code(200);
  fetcher->SetResponseString(kExternalPolicyDataPayload);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Accept the data when the callback is invoked.
  EXPECT_CALL(callback_listener_,
              OnFetchSuccess(kExternalPolicyDataKeys[1],
                             kExternalPolicyDataPayload))
      .Times(1)
      .WillOnce(Return(true));
  backend_task_runner_->RunPendingTasks();
  Mock::VerifyAndClearExpectations(&callback_listener_);
  io_task_runner_->RunUntilIdle();

  // Verify that the second fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(1));

  // Verify that the third fetch is still running.
  fetcher = fetcher_factory_.GetFetcherByID(2);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[2]), fetcher->GetOriginalURL());

  // Complete the third fetch.
  fetcher->set_response_code(200);
  fetcher->SetResponseString(kExternalPolicyDataPayload);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Accept the data when the callback is invoked.
  EXPECT_CALL(callback_listener_,
              OnFetchSuccess(kExternalPolicyDataKeys[2],
                             kExternalPolicyDataPayload))
      .Times(1)
      .WillOnce(Return(true));
  backend_task_runner_->RunPendingTasks();
  Mock::VerifyAndClearExpectations(&callback_listener_);
  io_task_runner_->RunUntilIdle();

  // Verify that the third fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(2));

  // Verify that no retries have been scheduled.
  EXPECT_FALSE(backend_task_runner_->HasPendingTask());
}

TEST_F(ExternalPolicyDataUpdaterTest, ParallelJobsWithInvalidatedJob) {
  // Create an updater that runs up to two fetches in parallel.
  CreateUpdater(2);

  // Make two fetch requests.
  RequestExternalDataFetch(0);
  RequestExternalDataFetch(1);

  // Verify that the first fetch has been started.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Verify that the second fetch has been started.
  fetcher = fetcher_factory_.GetFetcherByID(1);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[1]), fetcher->GetOriginalURL());

  // Make another fetch request with the same key as the second request but an
  // updated URL.
  RequestExternalDataFetch(1, 2);

  // Verify that the first fetch is still running.
  fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[0]), fetcher->GetOriginalURL());

  // Verify that the second fetch is no longer running.
  EXPECT_FALSE(fetcher_factory_.GetFetcherByID(1));

  // Verify that the third fetch has been started.
  fetcher = fetcher_factory_.GetFetcherByID(2);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kExternalPolicyDataURLs[2]), fetcher->GetOriginalURL());
}

}  // namespace policy
