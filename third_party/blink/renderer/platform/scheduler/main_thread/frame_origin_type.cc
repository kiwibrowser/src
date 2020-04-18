// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/main_thread/frame_origin_type.h"

#include "base/macros.h"
#include "third_party/blink/renderer/platform/scheduler/public/frame_scheduler.h"

namespace blink {
namespace scheduler {

FrameOriginType GetFrameOriginType(FrameScheduler* scheduler) {
  DCHECK(scheduler);

  if (scheduler->GetFrameType() == FrameScheduler::FrameType::kMainFrame)
    return FrameOriginType::kMainFrame;

  if (scheduler->IsCrossOrigin()) {
    return FrameOriginType::kCrossOriginFrame;
  } else {
    return FrameOriginType::kSameOriginFrame;
  }
}

const char* FrameOriginTypeToString(FrameOriginType origin) {
  switch (origin) {
    case FrameOriginType::kMainFrame:
      return "main-frame";
    case FrameOriginType::kSameOriginFrame:
      return "same-origin";
    case FrameOriginType::kCrossOriginFrame:
      return "cross-origin";
    case FrameOriginType::kCount:
      NOTREACHED();
  }
  NOTREACHED();
  return nullptr;
}

}  // namespace scheduler
}  // namespace blink
