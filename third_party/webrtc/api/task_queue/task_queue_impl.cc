/*
 *  Copyright 2019 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "api/task_queue/task_queue_impl.h"

#include "rtc_base/checks.h"

namespace rtc {

// Fake ref counting: implementers of the TaskQueueBase shouldn't expect it is
// stored in a refererence counter pointer.
void TaskQueue::Impl::AddRef() {
  // AddRef should be called exactly once by rtc::TaskQueue constructor when
  // raw pointer converted into scoped_refptr<Impl>,
  // just before TaskQueue constructor assign task_queue_ member.
  RTC_CHECK(task_queue_ == nullptr);
}

void TaskQueue::Impl::Release() {
  // TaskQueue destructor manually destroyes this object, thus Release should
  // never be called.
  RTC_CHECK(false);
}

}  // namespace rtc
