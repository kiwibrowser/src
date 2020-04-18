// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/probe/platform_probes.h"

namespace blink {
namespace probe {

TimeTicks ProbeBase::CaptureStartTime() const {
  if (start_time_.is_null())
    start_time_ = CurrentTimeTicks();
  return start_time_;
}

TimeTicks ProbeBase::CaptureEndTime() const {
  if (end_time_.is_null())
    end_time_ = CurrentTimeTicks();
  return end_time_;
}

TimeDelta ProbeBase::Duration() const {
  DCHECK(!start_time_.is_null());
  return CaptureEndTime() - start_time_;
}

}  // namespace probe
}  // namespace blink
