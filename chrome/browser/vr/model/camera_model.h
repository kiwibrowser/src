// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODEL_CAMERA_MODEL_H_
#define CHROME_BROWSER_VR_MODEL_CAMERA_MODEL_H_

#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/transform.h"

namespace vr {

// An enum for the left and right eye.
enum EyeType {
  kLeftEye = 0,
  kRightEye,
};

struct CameraModel {
  EyeType eye_type;
  gfx::Rect viewport;
  gfx::Transform view_matrix;
  gfx::Transform proj_matrix;
  gfx::Transform view_proj_matrix;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODEL_CAMERA_MODEL_H_
