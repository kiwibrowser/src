// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODEL_RETICLE_MODEL_H_
#define CHROME_BROWSER_VR_MODEL_RETICLE_MODEL_H_

#include "ui/gfx/geometry/point3_f.h"

namespace vr {

enum CursorType {
  kCursorDefault,
  kCursorReposition,
};

// The ReticleModel contains information related to the target of the
// controller's laser. It is computed by the UiInputManager and is used by the
// input manager in the production of gestures as well as by the Reticle element
// in the scene.
struct ReticleModel {
  gfx::Point3F target_point;
  gfx::PointF target_local_point;
  int target_element_id = 0;
  CursorType cursor_type = kCursorDefault;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODEL_RETICLE_MODEL_H_
