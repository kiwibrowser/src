// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_PROCESS_STATE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_PROCESS_STATE_H_

#include <atomic>

namespace blink {
namespace scheduler {
namespace internal {

// Helper lock-free struct to share main state of the process between threads
// for recording methods.
// This class should not be used for synchronization between threads.
struct ProcessState {
  static ProcessState* Get();

  std::atomic_bool is_process_backgrounded;
};

}  // namespace internal
}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_PROCESS_STATE_H_
