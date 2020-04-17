// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/service_listener_impl.h"

#include <memory>

#include "third_party/googletest/src/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {
namespace {

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Expectation;

using State = ServiceListener::State;

class MockObserver final : public ServiceListener::Observer {
 public:
  ~MockObserver() = default;

  MOCK_METHOD0(OnStarted, void());
  MOCK_METHOD0(OnStopped, void());
  MOCK_METHOD0(OnSuspended, void());
  MOCK_METHOD0(OnSearching, void());

  MOCK_METHOD1(OnReceiverAdded, void(const ServiceInfo& info));
  MOCK_METHOD1(OnReceiverChanged, void(const ServiceInfo& info));
  MOCK_METHOD1(OnReceiverRemoved, void(const ServiceInfo& info));
  MOCK_METHOD0(OnAllReceiversRemoved, void());

  MOCK_METHOD1(OnError, void(ServiceListenerError));

  MOCK_METHOD1(OnMetrics, void(ServiceListener::Metrics));
};

class MockMdnsDelegate final : public ServiceListenerImpl::Delegate {
 public:
  MockMdnsDelegate() = default;
  ~MockMdnsDelegate() override = default;

  using ServiceListenerImpl::Delegate::SetState;

  MOCK_METHOD0(StartListener, void());
  MOCK_METHOD0(StartAndSuspendListener, void());
  MOCK_METHOD0(StopListener, void());
  MOCK_METHOD0(SuspendListener, void());
  MOCK_METHOD0(ResumeListener, void());
  MOCK_METHOD1(SearchNow, void(State));
  MOCK_METHOD0(RunTasksListener, void());
};

class ServiceListenerImplTest : public ::testing::Test {
 protected:
  void SetUp() override {
    service_listener_ = std::make_unique<ServiceListenerImpl>(&mock_delegate_);
  }

  MockMdnsDelegate mock_delegate_;
  std::unique_ptr<ServiceListenerImpl> service_listener_;
};

}  // namespace

TEST_F(ServiceListenerImplTest, NormalStartStop) {
  ASSERT_EQ(State::kStopped, service_listener_->state());

  EXPECT_CALL(mock_delegate_, StartListener());
  EXPECT_TRUE(service_listener_->Start());
  EXPECT_FALSE(service_listener_->Start());
  EXPECT_EQ(State::kStarting, service_listener_->state());

  mock_delegate_.SetState(State::kRunning);
  EXPECT_EQ(State::kRunning, service_listener_->state());

  EXPECT_CALL(mock_delegate_, StopListener());
  EXPECT_TRUE(service_listener_->Stop());
  EXPECT_FALSE(service_listener_->Stop());
  EXPECT_EQ(State::kStopping, service_listener_->state());

  mock_delegate_.SetState(State::kStopped);
  EXPECT_EQ(State::kStopped, service_listener_->state());
}

TEST_F(ServiceListenerImplTest, StopBeforeRunning) {
  EXPECT_CALL(mock_delegate_, StartListener());
  EXPECT_TRUE(service_listener_->Start());
  EXPECT_EQ(State::kStarting, service_listener_->state());

  EXPECT_CALL(mock_delegate_, StopListener());
  EXPECT_TRUE(service_listener_->Stop());
  EXPECT_FALSE(service_listener_->Stop());
  EXPECT_EQ(State::kStopping, service_listener_->state());

  mock_delegate_.SetState(State::kStopped);
  EXPECT_EQ(State::kStopped, service_listener_->state());
}

TEST_F(ServiceListenerImplTest, StartSuspended) {
  EXPECT_CALL(mock_delegate_, StartAndSuspendListener());
  EXPECT_CALL(mock_delegate_, StartListener()).Times(0);
  EXPECT_TRUE(service_listener_->StartAndSuspend());
  EXPECT_FALSE(service_listener_->Start());
  EXPECT_EQ(State::kStarting, service_listener_->state());

  mock_delegate_.SetState(State::kSuspended);
  EXPECT_EQ(State::kSuspended, service_listener_->state());
}

TEST_F(ServiceListenerImplTest, SuspendWhileStarting) {
  EXPECT_CALL(mock_delegate_, StartListener()).Times(1);
  EXPECT_CALL(mock_delegate_, SuspendListener()).Times(1);
  EXPECT_TRUE(service_listener_->Start());
  EXPECT_TRUE(service_listener_->Suspend());
  EXPECT_EQ(State::kStarting, service_listener_->state());

  mock_delegate_.SetState(State::kSuspended);
  EXPECT_EQ(State::kSuspended, service_listener_->state());
}

TEST_F(ServiceListenerImplTest, SuspendAndResume) {
  EXPECT_TRUE(service_listener_->Start());
  mock_delegate_.SetState(State::kRunning);

  EXPECT_CALL(mock_delegate_, ResumeListener()).Times(0);
  EXPECT_CALL(mock_delegate_, SuspendListener()).Times(2);
  EXPECT_FALSE(service_listener_->Resume());
  EXPECT_TRUE(service_listener_->Suspend());
  EXPECT_TRUE(service_listener_->Suspend());

  mock_delegate_.SetState(State::kSuspended);
  EXPECT_EQ(State::kSuspended, service_listener_->state());

  EXPECT_CALL(mock_delegate_, StartListener()).Times(0);
  EXPECT_CALL(mock_delegate_, SuspendListener()).Times(0);
  EXPECT_CALL(mock_delegate_, ResumeListener()).Times(2);
  EXPECT_FALSE(service_listener_->Start());
  EXPECT_FALSE(service_listener_->Suspend());
  EXPECT_TRUE(service_listener_->Resume());
  EXPECT_TRUE(service_listener_->Resume());

  mock_delegate_.SetState(State::kRunning);
  EXPECT_EQ(State::kRunning, service_listener_->state());

  EXPECT_CALL(mock_delegate_, ResumeListener()).Times(0);
  EXPECT_FALSE(service_listener_->Resume());
}

TEST_F(ServiceListenerImplTest, SearchWhileRunning) {
  EXPECT_CALL(mock_delegate_, SearchNow(_)).Times(0);
  EXPECT_FALSE(service_listener_->SearchNow());
  EXPECT_TRUE(service_listener_->Start());
  mock_delegate_.SetState(State::kRunning);

  EXPECT_CALL(mock_delegate_, SearchNow(State::kRunning)).Times(2);
  EXPECT_TRUE(service_listener_->SearchNow());
  EXPECT_TRUE(service_listener_->SearchNow());

  mock_delegate_.SetState(State::kSearching);
  EXPECT_EQ(State::kSearching, service_listener_->state());

  EXPECT_CALL(mock_delegate_, SearchNow(_)).Times(0);
  EXPECT_FALSE(service_listener_->SearchNow());

  mock_delegate_.SetState(State::kRunning);
  EXPECT_EQ(State::kRunning, service_listener_->state());
}

TEST_F(ServiceListenerImplTest, SearchWhileSuspended) {
  EXPECT_CALL(mock_delegate_, SearchNow(_)).Times(0);
  EXPECT_FALSE(service_listener_->SearchNow());
  EXPECT_TRUE(service_listener_->Start());
  mock_delegate_.SetState(State::kRunning);
  EXPECT_TRUE(service_listener_->Suspend());
  mock_delegate_.SetState(State::kSuspended);

  EXPECT_CALL(mock_delegate_, SearchNow(State::kSuspended)).Times(2);
  EXPECT_TRUE(service_listener_->SearchNow());
  EXPECT_TRUE(service_listener_->SearchNow());

  mock_delegate_.SetState(State::kSearching);
  EXPECT_EQ(State::kSearching, service_listener_->state());

  mock_delegate_.SetState(State::kSuspended);
  EXPECT_EQ(State::kSuspended, service_listener_->state());
}

TEST_F(ServiceListenerImplTest, StopWhileSearching) {
  EXPECT_TRUE(service_listener_->Start());
  mock_delegate_.SetState(State::kRunning);
  EXPECT_TRUE(service_listener_->SearchNow());
  mock_delegate_.SetState(State::kSearching);

  EXPECT_CALL(mock_delegate_, StopListener());
  EXPECT_TRUE(service_listener_->Stop());
  EXPECT_FALSE(service_listener_->Stop());
  EXPECT_EQ(State::kStopping, service_listener_->state());

  mock_delegate_.SetState(State::kStopped);
  EXPECT_EQ(State::kStopped, service_listener_->state());
}

TEST_F(ServiceListenerImplTest, ResumeWhileSearching) {
  EXPECT_TRUE(service_listener_->Start());
  mock_delegate_.SetState(State::kRunning);
  EXPECT_TRUE(service_listener_->Suspend());
  mock_delegate_.SetState(State::kSuspended);
  EXPECT_TRUE(service_listener_->SearchNow());
  mock_delegate_.SetState(State::kSearching);

  EXPECT_CALL(mock_delegate_, ResumeListener()).Times(2);
  EXPECT_TRUE(service_listener_->Resume());
  EXPECT_TRUE(service_listener_->Resume());

  mock_delegate_.SetState(State::kRunning);
  EXPECT_EQ(State::kRunning, service_listener_->state());
}

TEST_F(ServiceListenerImplTest, SuspendWhileSearching) {
  EXPECT_TRUE(service_listener_->Start());
  mock_delegate_.SetState(State::kRunning);
  EXPECT_TRUE(service_listener_->SearchNow());
  mock_delegate_.SetState(State::kSearching);

  EXPECT_CALL(mock_delegate_, SuspendListener()).Times(2);
  EXPECT_TRUE(service_listener_->Suspend());
  EXPECT_TRUE(service_listener_->Suspend());

  mock_delegate_.SetState(State::kSuspended);
  EXPECT_EQ(State::kSuspended, service_listener_->state());
}

TEST_F(ServiceListenerImplTest, ObserveTransitions) {
  MockObserver observer;
  service_listener_->AddObserver(&observer);

  service_listener_->Start();
  Expectation start_from_stopped = EXPECT_CALL(observer, OnStarted());
  mock_delegate_.SetState(State::kRunning);

  service_listener_->SearchNow();
  Expectation search_from_running =
      EXPECT_CALL(observer, OnSearching()).After(start_from_stopped);
  mock_delegate_.SetState(State::kSearching);
  EXPECT_CALL(observer, OnStarted());
  mock_delegate_.SetState(State::kRunning);

  service_listener_->Suspend();
  Expectation suspend_from_running =
      EXPECT_CALL(observer, OnSuspended()).After(search_from_running);
  mock_delegate_.SetState(State::kSuspended);

  service_listener_->SearchNow();
  Expectation search_from_suspended =
      EXPECT_CALL(observer, OnSearching()).After(suspend_from_running);
  mock_delegate_.SetState(State::kSearching);
  EXPECT_CALL(observer, OnSuspended());
  mock_delegate_.SetState(State::kSuspended);

  service_listener_->Resume();
  Expectation resume_from_suspended =
      EXPECT_CALL(observer, OnStarted()).After(suspend_from_running);
  mock_delegate_.SetState(State::kRunning);

  service_listener_->Stop();
  Expectation stop =
      EXPECT_CALL(observer, OnStopped()).After(resume_from_suspended);
  mock_delegate_.SetState(State::kStopped);

  service_listener_->StartAndSuspend();
  EXPECT_CALL(observer, OnSuspended()).After(stop);
  mock_delegate_.SetState(State::kSuspended);
  service_listener_->RemoveObserver(&observer);
}

TEST_F(ServiceListenerImplTest, ObserveFromSearching) {
  MockObserver observer;
  service_listener_->AddObserver(&observer);

  service_listener_->Start();
  mock_delegate_.SetState(State::kRunning);

  service_listener_->SearchNow();
  mock_delegate_.SetState(State::kSearching);

  service_listener_->Suspend();
  EXPECT_CALL(observer, OnSuspended());
  mock_delegate_.SetState(State::kSuspended);

  EXPECT_TRUE(service_listener_->SearchNow());
  mock_delegate_.SetState(State::kSearching);

  service_listener_->Resume();
  EXPECT_CALL(observer, OnStarted());
  mock_delegate_.SetState(State::kRunning);
  service_listener_->RemoveObserver(&observer);
}

TEST_F(ServiceListenerImplTest, ReceiverObserverPassThrough) {
  const ServiceInfo receiver1{
      "id1", "name1", 1, {{192, 168, 1, 10}, 12345}, {}};
  const ServiceInfo receiver2{
      "id2", "name2", 1, {{192, 168, 1, 11}, 12345}, {}};
  const ServiceInfo receiver3{
      "id3", "name3", 1, {{192, 168, 1, 12}, 12345}, {}};
  const ServiceInfo receiver1_alt_name{
      "id1", "name1 alt", 1, {{192, 168, 1, 10}, 12345}, {}};
  MockObserver observer;
  service_listener_->AddObserver(&observer);

  EXPECT_CALL(observer, OnReceiverAdded(receiver1));
  service_listener_->OnReceiverAdded(receiver1);
  EXPECT_THAT(service_listener_->GetReceivers(), ElementsAre(receiver1));

  EXPECT_CALL(observer, OnReceiverChanged(receiver1_alt_name));
  service_listener_->OnReceiverChanged(receiver1_alt_name);
  EXPECT_THAT(service_listener_->GetReceivers(),
              ElementsAre(receiver1_alt_name));

  EXPECT_CALL(observer, OnReceiverChanged(receiver2)).Times(0);
  service_listener_->OnReceiverChanged(receiver2);

  EXPECT_CALL(observer, OnReceiverRemoved(receiver1_alt_name));
  service_listener_->OnReceiverRemoved(receiver1_alt_name);
  EXPECT_TRUE(service_listener_->GetReceivers().empty());

  EXPECT_CALL(observer, OnReceiverRemoved(receiver1_alt_name)).Times(0);
  service_listener_->OnReceiverRemoved(receiver1_alt_name);

  EXPECT_CALL(observer, OnReceiverAdded(receiver2));
  service_listener_->OnReceiverAdded(receiver2);
  EXPECT_THAT(service_listener_->GetReceivers(), ElementsAre(receiver2));

  EXPECT_CALL(observer, OnReceiverAdded(receiver3));
  service_listener_->OnReceiverAdded(receiver3);
  EXPECT_THAT(service_listener_->GetReceivers(),
              ElementsAre(receiver2, receiver3));

  EXPECT_CALL(observer, OnAllReceiversRemoved());
  service_listener_->OnAllReceiversRemoved();
  EXPECT_TRUE(service_listener_->GetReceivers().empty());

  EXPECT_CALL(observer, OnAllReceiversRemoved()).Times(0);
  service_listener_->OnAllReceiversRemoved();
  service_listener_->RemoveObserver(&observer);
}

TEST_F(ServiceListenerImplTest, MultipleObservers) {
  MockObserver observer1;
  MockObserver observer2;
  service_listener_->AddObserver(&observer1);

  service_listener_->Start();
  EXPECT_CALL(observer1, OnStarted());
  EXPECT_CALL(observer2, OnStarted()).Times(0);
  mock_delegate_.SetState(State::kRunning);

  service_listener_->AddObserver(&observer2);

  service_listener_->SearchNow();
  EXPECT_CALL(observer1, OnSearching());
  EXPECT_CALL(observer2, OnSearching());
  mock_delegate_.SetState(State::kSearching);
  EXPECT_CALL(observer1, OnStarted());
  EXPECT_CALL(observer2, OnStarted());
  mock_delegate_.SetState(State::kRunning);

  service_listener_->RemoveObserver(&observer1);

  service_listener_->Suspend();
  EXPECT_CALL(observer1, OnSuspended()).Times(0);
  EXPECT_CALL(observer2, OnSuspended());
  mock_delegate_.SetState(State::kSuspended);

  service_listener_->RemoveObserver(&observer2);

  service_listener_->Resume();
  EXPECT_CALL(observer1, OnStarted()).Times(0);
  EXPECT_CALL(observer2, OnStarted()).Times(0);
  mock_delegate_.SetState(State::kRunning);
}

}  // namespace openscreen
