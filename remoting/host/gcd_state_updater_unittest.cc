// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/gcd_state_updater.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/strings/stringize_macros.h"
#include "base/test/simple_test_clock.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "remoting/base/constants.h"
#include "remoting/base/fake_oauth_token_getter.h"
#include "remoting/host/gcd_rest_client.h"
#include "remoting/signaling/fake_signal_strategy.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Return;

namespace remoting {

class GcdStateUpdaterTest : public testing::Test {
 public:
  GcdStateUpdaterTest()
      : task_runner_(new base::TestMockTimeTaskRunner()),
        runner_handler_(task_runner_),
        token_getter_(OAuthTokenGetter::SUCCESS,
                      "<fake_user_email>",
                      "<fake_access_token>"),
        rest_client_(new GcdRestClient("http://gcd_base_url",
                                       "<gcd_device_id>",
                                       nullptr,
                                       &token_getter_)),
        signal_strategy_(SignalingAddress("local_jid")) {
    rest_client_->SetClockForTest(&test_clock_);
  }

  void OnSuccess() { on_success_count_++; }

  void OnHostIdError() { on_host_id_error_count_++; }

 protected:
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle runner_handler_;
  base::SimpleTestClock test_clock_;
  net::TestURLFetcherFactory url_fetcher_factory_;
  FakeOAuthTokenGetter token_getter_;
  std::unique_ptr<GcdRestClient> rest_client_;
  FakeSignalStrategy signal_strategy_;
  int on_success_count_ = 0;
  int on_host_id_error_count_ = 0;
};

TEST_F(GcdStateUpdaterTest, Success) {
  std::unique_ptr<GcdStateUpdater> updater(new GcdStateUpdater(
      base::Bind(&GcdStateUpdaterTest::OnSuccess, base::Unretained(this)),
      base::Bind(&GcdStateUpdaterTest::OnHostIdError, base::Unretained(this)),
      &signal_strategy_, std::move(rest_client_)));

  signal_strategy_.Connect();
  task_runner_->RunUntilIdle();
  net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ("{\"patches\":[{\"patch\":{"
            "\"base\":{\"_hostVersion\":\"" STRINGIZE(VERSION) "\","
            "\"_jabberId\":\"local_jid\"}},"
            "\"timeMs\":0.0}],\"requestTimeMs\":0.0}",
            fetcher->upload_data());
  fetcher->set_response_code(200);
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(1, on_success_count_);

  updater.reset();

  EXPECT_EQ(0, on_host_id_error_count_);
}

TEST_F(GcdStateUpdaterTest, QueuedRequests) {
  std::unique_ptr<GcdStateUpdater> updater(new GcdStateUpdater(
      base::Bind(&GcdStateUpdaterTest::OnSuccess, base::Unretained(this)),
      base::Bind(&GcdStateUpdaterTest::OnHostIdError, base::Unretained(this)),
      &signal_strategy_, std::move(rest_client_)));

  // Connect, then re-connect with a different JID while the status
  // update for the first connection is pending.
  signal_strategy_.Connect();
  task_runner_->RunUntilIdle();
  signal_strategy_.Disconnect();
  task_runner_->RunUntilIdle();
  signal_strategy_.SetLocalAddress(SignalingAddress("local_jid2"));
  signal_strategy_.Connect();
  task_runner_->RunUntilIdle();

  // Let the first status update finish.  This should be a no-op in
  // the updater because the local JID has changed since this request
  // was issued.
  net::TestURLFetcher* fetcher0 = url_fetcher_factory_.GetFetcherByID(0);
  fetcher0->set_response_code(200);
  fetcher0->delegate()->OnURLFetchComplete(fetcher0);
  EXPECT_EQ(0, on_success_count_);

  // Wait for the next retry.
  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(1));

  // There should be a new pending request now with the new local JID.
  net::TestURLFetcher* fetcher1 = url_fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher1);
  EXPECT_EQ("{\"patches\":[{\"patch\":{"
            "\"base\":{\"_hostVersion\":\"" STRINGIZE(VERSION) "\","
            "\"_jabberId\":\"local_jid2\"}},"
            "\"timeMs\":0.0}],\"requestTimeMs\":0.0}",
            fetcher1->upload_data());
  fetcher1->set_response_code(200);
  fetcher1->delegate()->OnURLFetchComplete(fetcher1);
  EXPECT_EQ(1, on_success_count_);

  updater.reset();

  EXPECT_EQ(0, on_host_id_error_count_);
}

TEST_F(GcdStateUpdaterTest, Retry) {
  std::unique_ptr<GcdStateUpdater> updater(new GcdStateUpdater(
      base::Bind(&GcdStateUpdaterTest::OnSuccess, base::Unretained(this)),
      base::Bind(&GcdStateUpdaterTest::OnHostIdError, base::Unretained(this)),
      &signal_strategy_, std::move(rest_client_)));

  signal_strategy_.Connect();
  task_runner_->RunUntilIdle();
  net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(0);
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(1));
  EXPECT_EQ(1.0, task_runner_->Now().ToDoubleT());
  fetcher = url_fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(200);
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(1, on_success_count_);

  updater.reset();

  EXPECT_EQ(0, on_host_id_error_count_);
}

TEST_F(GcdStateUpdaterTest, UnknownHost) {
  std::unique_ptr<GcdStateUpdater> updater(new GcdStateUpdater(
      base::Bind(&GcdStateUpdaterTest::OnSuccess, base::Unretained(this)),
      base::Bind(&GcdStateUpdaterTest::OnHostIdError, base::Unretained(this)),
      &signal_strategy_, std::move(rest_client_)));

  signal_strategy_.Connect();
  task_runner_->RunUntilIdle();
  net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  fetcher->set_response_code(404);
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_EQ(0, on_success_count_);
  EXPECT_EQ(1, on_host_id_error_count_);
}

}  // namespace remoting
