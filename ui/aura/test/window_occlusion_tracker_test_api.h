// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_TEST_WINDOW_OCCLUSION_TRACKER_TEST_API_H_
#define UI_AURA_TEST_WINDOW_OCCLUSION_TRACKER_TEST_API_H_

#include "base/macros.h"

namespace aura {
namespace test {

class WindowOcclusionTrackerTestApi {
 public:
  WindowOcclusionTrackerTestApi();
  ~WindowOcclusionTrackerTestApi();

  // Returns the number of times that occlusion was recomputed in this process.
  int GetNumTimesOcclusionRecomputed() const;

  // Returns true if WindowOcclusionTracker had to recompute occlusion too many
  // times before becoming stable since the last call to this.
  bool WasOcclusionRecomputedTooManyTimes();

 private:
  DISALLOW_COPY_AND_ASSIGN(WindowOcclusionTrackerTestApi);
};

}  // namespace test
}  // namespace aura

#endif  // UI_AURA_TEST_WINDOW_OCCLUSION_TRACKER_TEST_API_H_
