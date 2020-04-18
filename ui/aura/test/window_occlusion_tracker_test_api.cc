// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/test/window_occlusion_tracker_test_api.h"

#include "ui/aura/window_occlusion_tracker.h"

namespace aura {
namespace test {

WindowOcclusionTrackerTestApi::WindowOcclusionTrackerTestApi() = default;
WindowOcclusionTrackerTestApi::~WindowOcclusionTrackerTestApi() = default;

int WindowOcclusionTrackerTestApi::GetNumTimesOcclusionRecomputed() const {
  return WindowOcclusionTracker::GetInstance()->num_times_occlusion_recomputed_;
}

bool WindowOcclusionTrackerTestApi::WasOcclusionRecomputedTooManyTimes() {
  const bool local_was_occlusion_recomputed_too_many_times =
      WindowOcclusionTracker::GetInstance()
          ->was_occlusion_recomputed_too_many_times_;
  WindowOcclusionTracker::GetInstance()
      ->was_occlusion_recomputed_too_many_times_ = false;
  return local_was_occlusion_recomputed_too_many_times;
}

}  // namespace test
}  // namespace aura
