/*
 *  Copyright 2019 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "api/task_queue/global_task_queue_factory.h"

#include "api/task_queue/default_task_queue_factory.h"
#include "rtc_base/checks.h"

namespace webrtc {
namespace {

TaskQueueFactory* GlobalOrDefault(TaskQueueFactory* global) {
  static TaskQueueFactory* const factory =
      global ? global : CreateDefaultTaskQueueFactory().release();
  return factory;
}

}  // namespace

void SetGlobalTaskQueueFactory(std::unique_ptr<TaskQueueFactory> factory) {
  RTC_CHECK(factory) << "Can't set nullptr TaskQueueFactory";
  // Own, but never delete the global factory.
  TaskQueueFactory* global = factory.release();
  RTC_CHECK(GlobalOrDefault(global) == global)
      << "Task queue factory set after another SetFactory or after a task "
         "queue was created";
}

TaskQueueFactory& GlobalTaskQueueFactory() {
  return *GlobalOrDefault(nullptr);
}

}  // namespace webrtc
