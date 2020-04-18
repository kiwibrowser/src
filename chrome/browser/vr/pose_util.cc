// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/pose_util.h"

#include <cmath>

#include "ui/gfx/transform.h"

namespace vr {

// Provides the direction the head is looking towards as a 3x1 unit vector.
gfx::Vector3dF GetForwardVector(const gfx::Transform& head_pose) {
  // Same as multiplying the inverse of the rotation component of the matrix by
  // (0, 0, -1, 0).
  return gfx::Vector3dF(-head_pose.matrix().get(2, 0),
                        -head_pose.matrix().get(2, 1),
                        -head_pose.matrix().get(2, 2));
}

gfx::Vector3dF GetUpVector(const gfx::Transform& head_pose) {
  return gfx::Vector3dF(head_pose.matrix().get(1, 0),
                        head_pose.matrix().get(1, 1),
                        head_pose.matrix().get(1, 2));
}

bool HeadMoveExceedsThreshold(const gfx::Transform& old_pose,
                              const gfx::Transform& new_pose,
                              float angular_threshold_degrees) {
  gfx::Vector3dF old_forward_vector = GetForwardVector(old_pose);
  gfx::Vector3dF new_forward_vector = GetForwardVector(new_pose);
  float angle =
      gfx::AngleBetweenVectorsInDegrees(new_forward_vector, old_forward_vector);

  if (std::abs(angle) > angular_threshold_degrees)
    return true;

  gfx::Vector3dF old_up_vector = GetUpVector(old_pose);
  gfx::Vector3dF new_up_vector = GetUpVector(new_pose);
  angle = gfx::AngleBetweenVectorsInDegrees(new_up_vector, old_up_vector);

  return std::abs(angle) > angular_threshold_degrees;
}

}  // namespace vr
