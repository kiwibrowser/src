// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_POSE_UTIL_H_
#define CHROME_BROWSER_VR_POSE_UTIL_H_

#include "ui/gfx/geometry/vector3d_f.h"

namespace gfx {
class Transform;
}

namespace vr {

// Provides the direction the head is looking towards as a 3x1 unit vector.
gfx::Vector3dF GetForwardVector(const gfx::Transform& head_pose);

// Returns a vector heading upward from the viewer's head.
gfx::Vector3dF GetUpVector(const gfx::Transform& head_pose);

// Returns true if either the change is gaze direction (via GetForwardVector
// above) exceeds the angular threshold, or if the change in up vector exceeds
// this same threshold.
bool HeadMoveExceedsThreshold(const gfx::Transform& old_pose,
                              const gfx::Transform& new_pose,
                              float angular_threshold_degrees);

}  // namespace vr

#endif  //  CHROME_BROWSER_VR_POSE_UTIL_H_
