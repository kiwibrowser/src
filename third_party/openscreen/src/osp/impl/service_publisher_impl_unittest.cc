// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/service_publisher_impl.h"

#include <memory>

#include "third_party/googletest/src/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {
namespace {

using ::testing::Expectation;
using State = ServicePublisher::State;

class MockObserver final : public ServicePublisher::Observer {
 public:
  ~MockObserver() = default;

  MOCK_METHOD0(OnStarted, void());
  MOCK_METHOD0(OnStopped, void());
  MOCK_METHOD0(OnSuspended, void());

  MOCK_METHOD1(OnError, void(ServicePublisherError));

  MOCK_METHOD1(OnMetrics, void(ServicePublisher::Metrics));
};

class MockMdnsDelegate final : public ServicePublisherImpl::Delegate {
 public:
  MockMdnsDelegate() = default;
  ~MockMdnsDelegate() override = default;

  using ServicePublisherImpl::Delegate::SetState;

  MOCK_METHOD0(StartPublisher, void());
  MOCK_METHOD0(StartAndSuspendPublisher, void());
  MOCK_METHOD0(StopPublisher, void());
  MOCK_METHOD0(SuspendPublisher, void());
  MOCK_METHOD0(ResumePublisher, void());
  MOCK_METHOD0(RunTasksPublisher, void());
};

class ServicePublisherImplTest : public ::testing::Test {
 protected:
  void SetUp() override {
    service_publisher_ =
        std::make_unique<ServicePublisherImpl>(nullptr, &mock_delegate_);
  }

  MockMdnsDelegate mock_delegate_;
  std::unique_ptr<ServicePublisherImpl> service_publisher_;
};

}  // namespace

TEST_F(ServicePublisherImplTest, NormalStartStop) {
  ASSERT_EQ(State::kStopped, service_publisher_->state());

  EXPECT_CALL(mock_delegate_, StartPublisher());
  EXPECT_TRUE(service_publisher_->Start());
  EXPECT_FALSE(service_publisher_->Start());
  EXPECT_EQ(State::kStarting, service_publisher_->state());

  mock_delegate_.SetState(State::kRunning);
  EXPECT_EQ(State::kRunning, service_publisher_->state());

  EXPECT_CALL(mock_delegate_, StopPublisher());
  EXPECT_TRUE(service_publisher_->Stop());
  EXPECT_FALSE(service_publisher_->Stop());
  EXPECT_EQ(State::kStopping, service_publisher_->state());

  mock_delegate_.SetState(State::kStopped);
  EXPECT_EQ(State::kStopped, service_publisher_->state());
}

TEST_F(ServicePublisherImplTest, StopBeforeRunning) {
  EXPECT_CALL(mock_delegate_, StartPublisher());
  EXPECT_TRUE(service_publisher_->Start());
  EXPECT_EQ(State::kStarting, service_publisher_->state());

  EXPECT_CALL(mock_delegate_, StopPublisher());
  EXPECT_TRUE(service_publisher_->Stop());
  EXPECT_FALSE(service_publisher_->Stop());
  EXPECT_EQ(State::kStopping, service_publisher_->state());

  mock_delegate_.SetState(State::kStopped);
  EXPECT_EQ(State::kStopped, service_publisher_->state());
}

TEST_F(ServicePublisherImplTest, StartSuspended) {
  EXPECT_CALL(mock_delegate_, StartAndSuspendPublisher());
  EXPECT_CALL(mock_delegate_, StartPublisher()).Times(0);
  EXPECT_TRUE(service_publisher_->StartAndSuspend());
  EXPECT_FALSE(service_publisher_->Start());
  EXPECT_EQ(State::kStarting, service_publisher_->state());

  mock_delegate_.SetState(State::kSuspended);
  EXPECT_EQ(State::kSuspended, service_publisher_->state());
}

TEST_F(ServicePublisherImplTest, SuspendAndResume) {
  EXPECT_TRUE(service_publisher_->Start());
  mock_delegate_.SetState(State::kRunning);

  EXPECT_CALL(mock_delegate_, ResumePublisher()).Times(0);
  EXPECT_CALL(mock_delegate_, SuspendPublisher()).Times(2);
  EXPECT_FALSE(service_publisher_->Resume());
  EXPECT_TRUE(service_publisher_->Suspend());
  EXPECT_TRUE(service_publisher_->Suspend());

  mock_delegate_.SetState(State::kSuspended);
  EXPECT_EQ(State::kSuspended, service_publisher_->state());

  EXPECT_CALL(mock_delegate_, StartPublisher()).Times(0);
  EXPECT_CALL(mock_delegate_, SuspendPublisher()).Times(0);
  EXPECT_CALL(mock_delegate_, ResumePublisher()).Times(2);
  EXPECT_FALSE(service_publisher_->Start());
  EXPECT_FALSE(service_publisher_->Suspend());
  EXPECT_TRUE(service_publisher_->Resume());
  EXPECT_TRUE(service_publisher_->Resume());

  mock_delegate_.SetState(State::kRunning);
  EXPECT_EQ(State::kRunning, service_publisher_->state());

  EXPECT_CALL(mock_delegate_, ResumePublisher()).Times(0);
  EXPECT_FALSE(service_publisher_->Resume());
}

TEST_F(ServicePublisherImplTest, ObserverTransitions) {
  MockObserver observer;
  MockMdnsDelegate mock_delegate;
  service_publisher_ =
      std::make_unique<ServicePublisherImpl>(&observer, &mock_delegate);

  service_publisher_->Start();
  Expectation start_from_stopped = EXPECT_CALL(observer, OnStarted());
  mock_delegate.SetState(State::kRunning);

  service_publisher_->Suspend();
  Expectation suspend_from_running =
      EXPECT_CALL(observer, OnSuspended()).After(start_from_stopped);
  mock_delegate.SetState(State::kSuspended);

  service_publisher_->Resume();
  Expectation resume_from_suspended =
      EXPECT_CALL(observer, OnStarted()).After(suspend_from_running);
  mock_delegate.SetState(State::kRunning);

  service_publisher_->Stop();
  EXPECT_CALL(observer, OnStopped()).After(resume_from_suspended);
  mock_delegate.SetState(State::kStopped);
}

}  // namespace openscreen
