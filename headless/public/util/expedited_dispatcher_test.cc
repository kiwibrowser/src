// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/public/util/expedited_dispatcher.h"

#include <memory>
#include <string>
#include <vector>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "headless/public/util/navigation_request.h"
#include "headless/public/util/testing/fake_managed_dispatch_url_request_job.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;

namespace headless {

class ExpeditedDispatcherTest : public ::testing::Test {
 protected:
  ExpeditedDispatcherTest() = default;
  ~ExpeditedDispatcherTest() override = default;

  void SetUp() override {
    expedited_dispatcher_.reset(new ExpeditedDispatcher(loop_.task_runner()));
  }

  base::MessageLoop loop_;
  std::unique_ptr<ExpeditedDispatcher> expedited_dispatcher_;
};

TEST_F(ExpeditedDispatcherTest, DispatchDataReadyInReverseOrder) {
  std::vector<std::string> notifications;
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job1(
      new FakeManagedDispatchURLRequestJob(expedited_dispatcher_.get(), 1,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job2(
      new FakeManagedDispatchURLRequestJob(expedited_dispatcher_.get(), 2,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job3(
      new FakeManagedDispatchURLRequestJob(expedited_dispatcher_.get(), 3,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job4(
      new FakeManagedDispatchURLRequestJob(expedited_dispatcher_.get(), 4,
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

TEST_F(ExpeditedDispatcherTest, ErrorsAndDataReadyDispatchedInCallOrder) {
  std::vector<std::string> notifications;
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job1(
      new FakeManagedDispatchURLRequestJob(expedited_dispatcher_.get(), 1,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job2(
      new FakeManagedDispatchURLRequestJob(expedited_dispatcher_.get(), 2,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job3(
      new FakeManagedDispatchURLRequestJob(expedited_dispatcher_.get(), 3,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job4(
      new FakeManagedDispatchURLRequestJob(expedited_dispatcher_.get(), 4,
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

namespace {
class NavigationRequestForTest : public NavigationRequest {
 public:
  explicit NavigationRequestForTest(base::Closure* done_closure)
      : done_closure_(done_closure) {}

  ~NavigationRequestForTest() override = default;

  // NavigationRequest implementation:
  void StartProcessing(base::Closure done_callback) override {
    *done_closure_ = std::move(done_callback);
  }

 private:
  base::Closure* done_closure_;  // NOT OWNED
};
}  // namespace

TEST_F(ExpeditedDispatcherTest, NavigationDoesNotBlockUrlRequests) {
  std::vector<std::string> notifications;
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job1(
      new FakeManagedDispatchURLRequestJob(expedited_dispatcher_.get(), 1,
                                           &notifications));
  base::Closure navigation_done_closure;
  expedited_dispatcher_->NavigationRequested(
      std::make_unique<NavigationRequestForTest>(&navigation_done_closure));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job2(
      new FakeManagedDispatchURLRequestJob(expedited_dispatcher_.get(), 2,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job3(
      new FakeManagedDispatchURLRequestJob(expedited_dispatcher_.get(), 3,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job4(
      new FakeManagedDispatchURLRequestJob(expedited_dispatcher_.get(), 4,
                                           &notifications));
  job1->DispatchHeadersComplete();
  job2->DispatchHeadersComplete();
  job3->DispatchHeadersComplete();
  job4->DispatchHeadersComplete();

  EXPECT_TRUE(notifications.empty());

  base::RunLoop().RunUntilIdle();
  EXPECT_THAT(notifications,
              ElementsAre("id: 1 OK", "id: 2 OK", "id: 3 OK", "id: 4 OK"));
}

}  // namespace headless
