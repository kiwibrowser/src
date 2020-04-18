// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_TEST_TEST_TASK_TIME_OBSERVER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_TEST_TEST_TASK_TIME_OBSERVER_H_

#include "base/time/time.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_time_observer.h"

namespace base {
namespace sequence_manager {

class TestTaskTimeObserver : public TaskTimeObserver {
 public:
  void WillProcessTask(double start_time) override {}
  void DidProcessTask(double start_time, double end_time) override {}
};

}  // namespace sequence_manager
}  // namespace base

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_TEST_TEST_TASK_TIME_OBSERVER_H_
