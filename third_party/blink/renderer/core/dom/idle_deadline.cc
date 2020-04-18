// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/idle_deadline.h"

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/timing/performance.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread_scheduler.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

IdleDeadline::IdleDeadline(double deadline_seconds, CallbackType callback_type)
    : deadline_seconds_(deadline_seconds), callback_type_(callback_type) {}

double IdleDeadline::timeRemaining() const {
  double time_remaining = deadline_seconds_ - CurrentTimeTicksInSeconds();
  if (time_remaining < 0) {
    return 0;
  } else if (Platform::Current()
                 ->CurrentThread()
                 ->Scheduler()
                 ->ShouldYieldForHighPriorityWork()) {
    return 0;
  }

  return 1000.0 * Performance::ClampTimeResolution(time_remaining);
}

}  // namespace blink
