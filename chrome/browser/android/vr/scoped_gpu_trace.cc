// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/vr/scoped_gpu_trace.h"

#include "base/threading/platform_thread.h"
#include "base/trace_event/trace_event.h"
#include "ui/gl/gl_fence_android_native_fence_sync.h"

namespace vr {

uint32_t ScopedGpuTrace::s_trace_id_ = 0;

ScopedGpuTrace::ScopedGpuTrace(const char* categoray, const char* name)
    : start_time_(base::TimeTicks::Now()),
      fence_(gl::GLFenceAndroidNativeFenceSync::CreateForGpuFence()),
      categoray_(categoray),
      name_(name),
      trace_id_(s_trace_id_++) {}

ScopedGpuTrace::~ScopedGpuTrace() {
  // Discard this trace if fence hasn't completed yet. This may happen during
  // shutdown path.
  if (!fence_->HasCompleted())
    return;

  base::TimeTicks end_time = fence_->GetStatusChangeTime();
  if (end_time.is_null())
    return;

  TRACE_EVENT_BEGIN_WITH_ID_TID_AND_TIMESTAMP0(
      categoray_, name_, trace_id_, base::PlatformThread::CurrentId(),
      start_time_);
  TRACE_EVENT_END_WITH_ID_TID_AND_TIMESTAMP0(categoray_, name_, trace_id_,
                                             base::PlatformThread::CurrentId(),
                                             end_time);
}

}  // namespace vr
