// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/user_activity_monitor.h"

#include "base/run_loop.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/test/test_mock_time_task_runner.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/ui/common/task_runner_test_base.h"
#include "services/ui/ws/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

using ui::mojom::UserIdleObserver;

namespace ui {
namespace ws {
namespace test {

class TestUserActivityObserver : public mojom::UserActivityObserver {
 public:
  TestUserActivityObserver() : binding_(this) {}
  ~TestUserActivityObserver() override {}

  mojom::UserActivityObserverPtr GetPtr() {
    mojom::UserActivityObserverPtr ptr;
    binding_.Bind(mojo::MakeRequest(&ptr));
    return ptr;
  }

  bool GetAndResetReceivedUserActivity() {
    bool val = received_user_activity_;
    received_user_activity_ = false;
    return val;
  }

 private:
  // mojom::UserActivityObserver:
  void OnUserActivity() override { received_user_activity_ = true; }

  mojo::Binding<mojom::UserActivityObserver> binding_;
  bool received_user_activity_ = false;

  DISALLOW_COPY_AND_ASSIGN(TestUserActivityObserver);
};

class TestUserIdleObserver : public mojom::UserIdleObserver {
 public:
  TestUserIdleObserver() : binding_(this) {}
  ~TestUserIdleObserver() override {}

  mojom::UserIdleObserverPtr GetPtr() {
    mojom::UserIdleObserverPtr ptr;
    binding_.Bind(mojo::MakeRequest(&ptr));
    return ptr;
  }

  bool GetAndResetIdleState(UserIdleObserver::IdleState* state) {
    if (!received_idle_state_)
      return false;
    received_idle_state_ = false;
    *state = idle_state_;
    return true;
  }

 private:
  // mojom::UserIdleObserver:
  void OnUserIdleStateChanged(UserIdleObserver::IdleState new_state) override {
    received_idle_state_ = true;
    idle_state_ = new_state;
  }

  mojo::Binding<mojom::UserIdleObserver> binding_;
  bool received_idle_state_ = false;
  UserIdleObserver::IdleState idle_state_ = UserIdleObserver::IdleState::ACTIVE;

  DISALLOW_COPY_AND_ASSIGN(TestUserIdleObserver);
};

class UserActivityMonitorTest : public TaskRunnerTestBase {
 public:
  UserActivityMonitorTest() {}
  ~UserActivityMonitorTest() override {}

  UserActivityMonitor* monitor() { return monitor_.get(); }

 private:
  // testing::Test:
  void SetUp() override {
    TaskRunnerTestBase::SetUp();
    monitor_ = std::make_unique<UserActivityMonitor>(
        task_runner()->DeprecatedGetMockTickClock());
  }

  std::unique_ptr<UserActivityMonitor> monitor_;

  DISALLOW_COPY_AND_ASSIGN(UserActivityMonitorTest);
};

TEST_F(UserActivityMonitorTest, UserActivityObserver) {
  TestUserActivityObserver first_observer, second_observer;
  monitor()->AddUserActivityObserver(3, first_observer.GetPtr());
  monitor()->AddUserActivityObserver(4, second_observer.GetPtr());

  // The first activity should notify both observers.
  monitor()->OnUserActivity();
  RunUntilIdle();
  EXPECT_TRUE(first_observer.GetAndResetReceivedUserActivity());
  EXPECT_TRUE(second_observer.GetAndResetReceivedUserActivity());

  // The next activity after just one second should not notify either observer.
  RunTasksForNext(base::TimeDelta::FromSeconds(1));
  monitor()->OnUserActivity();
  RunUntilIdle();
  EXPECT_FALSE(first_observer.GetAndResetReceivedUserActivity());
  EXPECT_FALSE(second_observer.GetAndResetReceivedUserActivity());

  RunTasksForNext(base::TimeDelta::FromMilliseconds(2001));
  monitor()->OnUserActivity();
  RunUntilIdle();
  EXPECT_TRUE(first_observer.GetAndResetReceivedUserActivity());
  EXPECT_FALSE(second_observer.GetAndResetReceivedUserActivity());

  RunTasksForNext(base::TimeDelta::FromSeconds(1));
  monitor()->OnUserActivity();
  RunUntilIdle();
  EXPECT_FALSE(first_observer.GetAndResetReceivedUserActivity());
  EXPECT_TRUE(second_observer.GetAndResetReceivedUserActivity());
}

// Tests that idleness observers receive the correct notification upon
// connection.
TEST_F(UserActivityMonitorTest, UserIdleObserverConnectNotification) {
  UserIdleObserver::IdleState idle_state;

  // If an observer is added without any user activity, then it still receives
  // an ACTIVE notification immediately.
  TestUserIdleObserver first_observer;
  monitor()->AddUserIdleObserver(1, first_observer.GetPtr());
  RunUntilIdle();
  EXPECT_TRUE(first_observer.GetAndResetIdleState(&idle_state));
  EXPECT_EQ(UserIdleObserver::IdleState::ACTIVE, idle_state);

  // If an observer is added without any user activity and the system has been
  // idle, then the observer receives an IDLE notification immediately.
  RunTasksForNext(base::TimeDelta::FromMinutes(5));
  TestUserIdleObserver second_observer;
  monitor()->AddUserIdleObserver(4, second_observer.GetPtr());
  RunUntilIdle();
  EXPECT_TRUE(second_observer.GetAndResetIdleState(&idle_state));
  EXPECT_EQ(UserIdleObserver::IdleState::IDLE, idle_state);
  EXPECT_TRUE(first_observer.GetAndResetIdleState(&idle_state));
  EXPECT_EQ(UserIdleObserver::IdleState::IDLE, idle_state);

  // If an observer is added after some user activity, then it receives an
  // immediate ACTIVE notification.
  monitor()->OnUserActivity();
  TestUserIdleObserver third_observer;
  monitor()->AddUserIdleObserver(1, third_observer.GetPtr());
  RunUntilIdle();
  EXPECT_TRUE(third_observer.GetAndResetIdleState(&idle_state));
  EXPECT_EQ(UserIdleObserver::IdleState::ACTIVE, idle_state);
  EXPECT_TRUE(second_observer.GetAndResetIdleState(&idle_state));
  EXPECT_EQ(UserIdleObserver::IdleState::ACTIVE, idle_state);
  EXPECT_TRUE(first_observer.GetAndResetIdleState(&idle_state));
  EXPECT_EQ(UserIdleObserver::IdleState::ACTIVE, idle_state);

  RunTasksForNext(base::TimeDelta::FromMinutes(10));
  TestUserIdleObserver fourth_observer;
  monitor()->AddUserIdleObserver(1, fourth_observer.GetPtr());
  RunUntilIdle();
  EXPECT_TRUE(fourth_observer.GetAndResetIdleState(&idle_state));
  EXPECT_EQ(UserIdleObserver::IdleState::IDLE, idle_state);
  EXPECT_TRUE(third_observer.GetAndResetIdleState(&idle_state));
  EXPECT_EQ(UserIdleObserver::IdleState::IDLE, idle_state);
  EXPECT_TRUE(second_observer.GetAndResetIdleState(&idle_state));
  EXPECT_EQ(UserIdleObserver::IdleState::IDLE, idle_state);
  EXPECT_TRUE(first_observer.GetAndResetIdleState(&idle_state));
  EXPECT_EQ(UserIdleObserver::IdleState::IDLE, idle_state);

  // All observers are idle. These should not receive any IDLE notifications as
  // more time passes by.
  RunTasksForNext(base::TimeDelta::FromMinutes(100));
  RunUntilIdle();
  EXPECT_FALSE(first_observer.GetAndResetIdleState(&idle_state));
  EXPECT_FALSE(second_observer.GetAndResetIdleState(&idle_state));
  EXPECT_FALSE(third_observer.GetAndResetIdleState(&idle_state));
  EXPECT_FALSE(fourth_observer.GetAndResetIdleState(&idle_state));

  // Some activity would notify ACTIVE to all observers.
  monitor()->OnUserActivity();
  RunUntilIdle();
  EXPECT_TRUE(fourth_observer.GetAndResetIdleState(&idle_state));
  EXPECT_EQ(UserIdleObserver::IdleState::ACTIVE, idle_state);
  EXPECT_TRUE(third_observer.GetAndResetIdleState(&idle_state));
  EXPECT_EQ(UserIdleObserver::IdleState::ACTIVE, idle_state);
  EXPECT_TRUE(second_observer.GetAndResetIdleState(&idle_state));
  EXPECT_EQ(UserIdleObserver::IdleState::ACTIVE, idle_state);
  EXPECT_TRUE(first_observer.GetAndResetIdleState(&idle_state));
  EXPECT_EQ(UserIdleObserver::IdleState::ACTIVE, idle_state);

  // Yet more activity should not send any notifications.
  monitor()->OnUserActivity();
  RunUntilIdle();
  EXPECT_FALSE(first_observer.GetAndResetIdleState(&idle_state));
  EXPECT_FALSE(second_observer.GetAndResetIdleState(&idle_state));
  EXPECT_FALSE(third_observer.GetAndResetIdleState(&idle_state));
  EXPECT_FALSE(fourth_observer.GetAndResetIdleState(&idle_state));
}

}  // namespace test
}  // namespace ws
}  // namespace ui
