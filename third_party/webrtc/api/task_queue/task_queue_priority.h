/*
 *  Copyright 2019 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef API_TASK_QUEUE_TASK_QUEUE_PRIORITY_H_
#define API_TASK_QUEUE_TASK_QUEUE_PRIORITY_H_

namespace webrtc {

// TODO(bugs.webrtc.org/10191): Move as member class of TaskQueueFactory when
// rtc::TaskQueue would be able to depende on it.
enum class TaskQueuePriority { NORMAL = 0, HIGH, LOW };

}  // namespace webrtc

#endif  // API_TASK_QUEUE_TASK_QUEUE_PRIORITY_H_
