// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/util/task_duration_metric_reporter.h"

namespace blink {
namespace scheduler {
namespace internal {

int TakeFullMilliseconds(base::TimeDelta& duration) {
  int milliseconds = static_cast<int>(duration.InMilliseconds());
  duration = duration % base::TimeDelta::FromMilliseconds(1);
  return milliseconds;
}

}  // namespace internal
}  // namespace scheduler
}  // namespace blink
