/*
 *  Copyright 2019 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "api/task_queue/default_task_queue_factory.h"

#include "rtc_base/checks.h"

namespace webrtc {

std::unique_ptr<TaskQueueFactory> CreateDefaultTaskQueueFactory() {
  RTC_CHECK(false)
      << "Default task queue is not implemented for current platform, "
         "overwrite the task queue implementation by setting global factory.";
}

}  // namespace webrtc
