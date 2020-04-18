// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gin/public/v8_platform.h"

#include "base/trace_event/trace_event.h"
#include "testing/gtest/include/gtest/gtest.h"

class TestTraceStateObserver
    : public v8::TracingController::TraceStateObserver {
 public:
  void OnTraceEnabled() final { ++enabled_; }
  void OnTraceDisabled() final { ++disabled_; }
  int Enabled() { return enabled_; }
  int Disabled() { return disabled_; }

 private:
  int enabled_ = 0;
  int disabled_ = 0;
};

namespace gin {

TEST(V8PlatformTest, TraceStateObserverAPI) {
  TestTraceStateObserver* test_observer = new TestTraceStateObserver();
  ASSERT_EQ(0, test_observer->Enabled());
  ASSERT_EQ(0, test_observer->Disabled());

  V8Platform::Get()->GetTracingController()->AddTraceStateObserver(
      test_observer);
  base::trace_event::TraceLog::GetInstance()->SetEnabled(
      base::trace_event::TraceConfig("*", ""),
      base::trace_event::TraceLog::RECORDING_MODE);
  ASSERT_EQ(1, test_observer->Enabled());
  ASSERT_EQ(0, test_observer->Disabled());
  base::trace_event::TraceLog::GetInstance()->SetDisabled();
  ASSERT_EQ(1, test_observer->Enabled());
  ASSERT_EQ(1, test_observer->Disabled());

  V8Platform::Get()->GetTracingController()->RemoveTraceStateObserver(
      test_observer);
  base::trace_event::TraceLog::GetInstance()->SetEnabled(
      base::trace_event::TraceConfig("*", ""),
      base::trace_event::TraceLog::RECORDING_MODE);
  base::trace_event::TraceLog::GetInstance()->SetDisabled();
  ASSERT_EQ(1, test_observer->Enabled());
  ASSERT_EQ(1, test_observer->Disabled());
}

TEST(V8PlatformTest, TraceStateObserverFired) {
  TestTraceStateObserver* test_observer = new TestTraceStateObserver();
  ASSERT_EQ(0, test_observer->Enabled());
  ASSERT_EQ(0, test_observer->Disabled());

  base::trace_event::TraceLog::GetInstance()->SetEnabled(
      base::trace_event::TraceConfig("*", ""),
      base::trace_event::TraceLog::RECORDING_MODE);
  V8Platform::Get()->GetTracingController()->AddTraceStateObserver(
      test_observer);
  ASSERT_EQ(1, test_observer->Enabled());
  ASSERT_EQ(0, test_observer->Disabled());
}

}  // namespace gin
