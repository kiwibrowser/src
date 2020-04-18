// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_MODEL_CONTROLLER_MODEL_H_
#define CHROME_BROWSER_VR_MODEL_CONTROLLER_MODEL_H_

#include "chrome/browser/vr/platform_controller.h"
#include "chrome/browser/vr/ui_input_manager.h"
#include "ui/gfx/geometry/point3_f.h"
#include "ui/gfx/transform.h"

namespace vr {

// The ControllerModel encapsulates generic controller information read from the
// platform-specific VR subsystem (e.g., GVR). It is used by both the
// UiInputManager (for generating gestures), and by the UI for rendering the
// controller.
struct ControllerModel {
  ControllerModel();
  ControllerModel(const ControllerModel& other);
  ~ControllerModel();

  gfx::Transform transform;
  gfx::Vector3dF laser_direction;
  gfx::Point3F laser_origin;
  UiInputManager::ButtonState touchpad_button_state = UiInputManager::UP;
  UiInputManager::ButtonState app_button_state = UiInputManager::UP;
  UiInputManager::ButtonState home_button_state = UiInputManager::UP;
  bool touching_touchpad = false;
  gfx::PointF touchpad_touch_position;
  float opacity = 1.0f;
  bool quiescent = false;
  bool resting_in_viewport = false;
  bool recentered = false;
  bool app_button_long_pressed = false;
  PlatformController::Handedness handedness = PlatformController::kRightHanded;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_MODEL_CONTROLLER_MODEL_H_
