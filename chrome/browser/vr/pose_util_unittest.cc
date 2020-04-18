// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/pose_util.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/transform.h"

namespace vr {

TEST(PoseUtil, HeadMoveDetection) {
  const float angular_threshold_degrees = 2.5f;

  gfx::Transform neutral;
  gfx::Transform within_gaze_threshold;
  within_gaze_threshold.RotateAboutYAxis(1.0f);

  EXPECT_FALSE(HeadMoveExceedsThreshold(neutral, within_gaze_threshold,
                                        angular_threshold_degrees));

  gfx::Transform beyond_gaze_threshold;
  within_gaze_threshold.RotateAboutYAxis(3.0f);

  EXPECT_TRUE(HeadMoveExceedsThreshold(neutral, within_gaze_threshold,
                                       angular_threshold_degrees));

  gfx::Transform within_tilt_threshold;
  within_tilt_threshold.RotateAboutZAxis(1.0f);

  EXPECT_FALSE(HeadMoveExceedsThreshold(neutral, within_tilt_threshold,
                                        angular_threshold_degrees));

  gfx::Transform beyond_tilt_threshold;
  within_tilt_threshold.RotateAboutZAxis(3.0f);

  EXPECT_TRUE(HeadMoveExceedsThreshold(neutral, within_tilt_threshold,
                                       angular_threshold_degrees));
}

}  // namespace vr
