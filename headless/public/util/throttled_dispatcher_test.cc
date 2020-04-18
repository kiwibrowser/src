// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/public/util/throttled_dispatcher.h"

#include <memory>
#include <string>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "headless/public/util/navigation_request.h"
#include "headless/public/util/testing/fake_managed_dispatch_url_request_job.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;

namespace headless {

class ThrottledDispatcherTest : public ::testing::Test {
 protected:
  ThrottledDispatcherTest() = default;
  ~ThrottledDispatcherTest() override = default;

  void SetUp() override {
    throttled_dispatcher_.reset(new ThrottledDispatcher(loop_.task_runner()));
  }

  base::MessageLoop loop_;
  std::unique_ptr<ThrottledDispatcher> throttled_dispatcher_;
};

TEST_F(ThrottledDispatcherTest, DispatchDataReadyInReverseOrder) {
  std::vector<std::string> notifications;
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job1(
      new FakeManagedDispatchURLRequestJob(throttled_dispatcher_.get(), 1,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job2(
      new FakeManagedDispatchURLRequestJob(throttled_dispatcher_.get(), 2,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job3(
      new FakeManagedDispatchURLRequestJob(throttled_dispatcher_.get(), 3,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job4(
      new FakeManagedDispatchURLRequestJob(throttled_dispatcher_.get(), 4,
                                           &notifications));
  job4->DispatchHeadersComplete();
  job3->DispatchHeadersComplete();
  job2->DispatchHeadersComplete();
  job1->DispatchHeadersComplete();

  EXPECT_TRUE(notifications.empty());

  base::RunLoop().RunUntilIdle();
  EXPECT_THAT(notifications,
              ElementsAre("id: 4 OK", "id: 3 OK", "id: 2 OK", "id: 1 OK"));
}

TEST_F(ThrottledDispatcherTest, ErrorsAndDataReadyDispatchedInCallOrder) {
  std::vector<std::string> notifications;
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job1(
      new FakeManagedDispatchURLRequestJob(throttled_dispatcher_.get(), 1,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job2(
      new FakeManagedDispatchURLRequestJob(throttled_dispatcher_.get(), 2,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job3(
      new FakeManagedDispatchURLRequestJob(throttled_dispatcher_.get(), 3,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job4(
      new FakeManagedDispatchURLRequestJob(throttled_dispatcher_.get(), 4,
                                           &notifications));
  job4->DispatchHeadersComplete();
  job3->DispatchStartError(static_cast<net::Error>(-123));
  job2->DispatchHeadersComplete();
  job1->DispatchHeadersComplete();

  EXPECT_TRUE(notifications.empty());

  base::RunLoop().RunUntilIdle();
  EXPECT_THAT(notifications, ElementsAre("id: 4 OK", "id: 3 err: -123",
                                         "id: 2 OK", "id: 1 OK"));
}

TEST_F(ThrottledDispatcherTest, Throttling) {
  std::vector<std::string> notifications;
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job1(
      new FakeManagedDispatchURLRequestJob(throttled_dispatcher_.get(), 1,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job2(
      new FakeManagedDispatchURLRequestJob(throttled_dispatcher_.get(), 2,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job3(
      new FakeManagedDispatchURLRequestJob(throttled_dispatcher_.get(), 3,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job4(
      new FakeManagedDispatchURLRequestJob(throttled_dispatcher_.get(), 4,
                                           &notifications));

  throttled_dispatcher_->PauseRequests();

  job4->DispatchHeadersComplete();
  job3->DispatchStartError(static_cast<net::Error>(-123));
  job2->DispatchHeadersComplete();
  job1->DispatchHeadersComplete();

  base::RunLoop().RunUntilIdle();
  EXPECT_THAT(notifications, ElementsAre("id: 3 err: -123"));

  throttled_dispatcher_->ResumeRequests();
  base::RunLoop().RunUntilIdle();

  EXPECT_THAT(notifications, ElementsAre("id: 3 err: -123", "id: 4 OK",
                                         "id: 2 OK", "id: 1 OK"));
}

}  // namespace headless
