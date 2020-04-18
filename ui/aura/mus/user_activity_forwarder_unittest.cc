// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/user_activity_forwarder.h"

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/time/time.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/ui/common/task_runner_test_base.h"
#include "services/ui/public/interfaces/user_activity_monitor.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/user_activity/user_activity_detector.h"

namespace {

// Fake implementation of ui::mojom::UserActivityMonitor for testing that just
// supports tracking and notifying observers.
class FakeUserActivityMonitor : public ui::mojom::UserActivityMonitor {
 public:
  FakeUserActivityMonitor() : binding_(this) {}
  ~FakeUserActivityMonitor() override {}

  ui::mojom::UserActivityMonitorPtr GetPtr() {
    ui::mojom::UserActivityMonitorPtr ptr;
    binding_.Bind(mojo::MakeRequest(&ptr));
    return ptr;
  }

  // Notifies all observers about user activity.
  // ui::TaskRunnerTestBase::RunUntilIdle() must be called after this method in
  // order for observers to receive notifications.
  void NotifyUserActivityObservers() {
    for (auto& observer : activity_observers_)
      observer->OnUserActivity();
  }

  // ui::mojom::UserActivityMonitor:
  void AddUserActivityObserver(
      uint32_t delay_between_notify_secs,
      ui::mojom::UserActivityObserverPtr observer) override {
    activity_observers_.push_back(std::move(observer));
  }
  void AddUserIdleObserver(uint32_t idleness_in_minutes,
                           ui::mojom::UserIdleObserverPtr observer) override {
    NOTREACHED() << "Unexpected AddUserIdleObserver call";
  }

 private:
  mojo::Binding<ui::mojom::UserActivityMonitor> binding_;
  std::vector<ui::mojom::UserActivityObserverPtr> activity_observers_;

  DISALLOW_COPY_AND_ASSIGN(FakeUserActivityMonitor);
};

}  // namespace

namespace aura {

using UserActivityForwarderTest = ui::TaskRunnerTestBase;

TEST_F(UserActivityForwarderTest, ForwardActivityToDetector) {
  FakeUserActivityMonitor monitor;
  ui::UserActivityDetector detector;
  UserActivityForwarder forwarder(monitor.GetPtr(), &detector);

  // Run pending tasks so |monitor| receives |forwarder|'s registration.
  RunUntilIdle();

  base::TimeTicks now =
      base::TimeTicks() + base::TimeDelta::FromMicroseconds(1000);
  detector.set_now_for_test(now);
  monitor.NotifyUserActivityObservers();
  RunUntilIdle();
  EXPECT_EQ(now, detector.last_activity_time());

  now += base::TimeDelta::FromSeconds(10);
  detector.set_now_for_test(now);
  monitor.NotifyUserActivityObservers();
  RunUntilIdle();
  EXPECT_EQ(now, detector.last_activity_time());
}

}  // namespace aura
