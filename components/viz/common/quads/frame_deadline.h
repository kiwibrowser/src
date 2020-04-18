// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_QUADS_FRAME_DEADLINE_H_
#define COMPONENTS_VIZ_COMMON_QUADS_FRAME_DEADLINE_H_

#include "components/viz/common/viz_common_export.h"

#include "base/time/time.h"

namespace viz {

class VIZ_COMMON_EXPORT FrameDeadline {
 public:
  FrameDeadline() = default;
  FrameDeadline(base::TimeTicks frame_start_time,
                uint32_t deadline_in_frames,
                base::TimeDelta frame_interval,
                bool use_default_lower_bound_deadline)
      : frame_start_time_(frame_start_time),
        deadline_in_frames_(deadline_in_frames),
        frame_interval_(frame_interval),
        use_default_lower_bound_deadline_(use_default_lower_bound_deadline) {}

  FrameDeadline(const FrameDeadline& other) = default;

  FrameDeadline& operator=(const FrameDeadline& other) = default;

  bool operator==(const FrameDeadline& other) const {
    return other.frame_start_time_ == frame_start_time_ &&
           other.deadline_in_frames_ == deadline_in_frames_ &&
           other.frame_interval_ == frame_interval_ &&
           other.use_default_lower_bound_deadline_ ==
               use_default_lower_bound_deadline_;
  }

  bool operator!=(const FrameDeadline& other) const {
    return !(*this == other);
  }

  base::TimeTicks frame_start_time() const { return frame_start_time_; }

  uint32_t deadline_in_frames() const { return deadline_in_frames_; }

  base::TimeDelta frame_interval() const { return frame_interval_; }

  bool use_default_lower_bound_deadline() const {
    return use_default_lower_bound_deadline_;
  }

 private:
  base::TimeTicks frame_start_time_;
  uint32_t deadline_in_frames_ = 0u;
  base::TimeDelta frame_interval_;
  bool use_default_lower_bound_deadline_ = true;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_QUADS_FRAME_DEADLINE_H_
