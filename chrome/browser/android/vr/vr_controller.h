// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_VR_VR_CONTROLLER_H_
#define CHROME_BROWSER_ANDROID_VR_VR_CONTROLLER_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/time/time.h"
#include "chrome/browser/android/vr/gvr_util.h"
#include "chrome/browser/vr/platform_controller.h"
#include "device/vr/android/gvr/gvr_gamepad_data_provider.h"
#include "device/vr/public/mojom/vr_service.mojom.h"
#include "third_party/gvr-android-sdk/src/libraries/headers/vr/gvr/capi/include/gvr_types.h"
#include "ui/gfx/geometry/point3_f.h"
#include "ui/gfx/geometry/quaternion.h"
#include "ui/gfx/geometry/vector2d_f.h"
#include "ui/gfx/geometry/vector3d_f.h"
#include "ui/gfx/transform.h"

namespace blink {
class WebGestureEvent;
}

namespace gfx {
class Transform;
}

namespace gvr {
class ControllerState;
}

namespace vr {

// Angle (radians) the beam down from the controller axis, for wrist comfort.
constexpr float kErgoAngleOffset = 0.26f;

using GestureList = std::vector<std::unique_ptr<blink::WebGestureEvent>>;

class VrController : public PlatformController {
 public:
  // Controller API entry point.
  explicit VrController(gvr_context* gvr_context);
  ~VrController() override;

  // Must be called when the Activity gets OnResume().
  void OnResume();

  // Must be called when the Activity gets OnPause().
  void OnPause();

  device::GvrGamepadData GetGamepadData();
  device::mojom::XRInputSourceStatePtr GetInputSourceState();

  // Called once per frame to update controller state.
  void UpdateState(const gfx::Transform& head_pose);

  std::unique_ptr<GestureList> DetectGestures();

  bool IsTouching();

  float TouchPosX();

  float TouchPosY();

  gfx::Quaternion Orientation() const;
  gfx::Point3F Position() const;
  void GetTransform(gfx::Transform* out) const;
  void GetRelativePointerTransform(gfx::Transform* out) const;
  void GetPointerTransform(gfx::Transform* out) const;
  float GetOpacity() const;
  gfx::Point3F GetPointerStart() const;

  bool TouchDownHappened();

  bool TouchUpHappened();

  bool ButtonUpHappened(gvr::ControllerButton button);
  bool ButtonDownHappened(gvr::ControllerButton button);
  bool ButtonState(gvr::ControllerButton button) const;

  bool IsConnected();

  // PlatformController
  bool IsButtonDown(PlatformController::ButtonType type) const override;
  base::TimeTicks GetLastOrientationTimestamp() const override;
  base::TimeTicks GetLastTouchTimestamp() const override;
  base::TimeTicks GetLastButtonTimestamp() const override;
  PlatformController::Handedness GetHandedness() const override;
  bool GetRecentered() const override;

 private:
  enum GestureDetectorState {
    WAITING,     // waiting for user to touch down
    TOUCHING,    // touching the touch pad but not scrolling
    SCROLLING,   // scrolling on the touch pad
    POST_SCROLL  // scroll has finished and we are hallucinating events
  };

  struct TouchPoint {
    gfx::Vector2dF position;
    int64_t timestamp;
  };

  struct TouchInfo {
    TouchPoint touch_point;
    bool touch_up;
    bool touch_down;
    bool is_touching;
  };

  struct ButtonInfo {
    gvr::ControllerButton button;
    bool button_up;
    bool button_down;
    bool button_state;
    int64_t timestamp;
  };

  void UpdateGestureFromTouchInfo(blink::WebGestureEvent* gesture);
  void UpdateGestureWithScrollDelta(blink::WebGestureEvent* gesture);

  bool GetButtonLongPressFromButtonInfo();

  void HandleWaitingState(blink::WebGestureEvent* gesture);
  void HandleDetectingState(blink::WebGestureEvent* gesture);
  void HandleScrollingState(blink::WebGestureEvent* gesture);
  void HandlePostScrollingState(blink::WebGestureEvent* gesture);

  void UpdateTouchInfo();

  // Returns true if the touch position is within the slop of the initial touch
  // point, false otherwise.
  bool InSlop(const gfx::Vector2dF touch_position);

  // Returns true if the gesture is in horizontal direction.
  bool IsHorizontalGesture();

  void Reset();

  void UpdateGestureParameters();

  // If the user is touching the touch pad and the touch point is different from
  // before, update the touch point and return true. Otherwise, return false.
  bool UpdateCurrentTouchpoint();

  void UpdateOverallVelocity();

  void UpdateAlpha();

  // State of gesture detector.
  GestureDetectorState state_;

  std::unique_ptr<gvr::ControllerApi> controller_api_;

  // The last controller state (updated once per frame).
  std::unique_ptr<gvr::ControllerState> controller_state_;

  std::unique_ptr<gvr::GvrApi> gvr_api_;

  float last_qx_;
  bool pinch_started_;
  bool zoom_in_progress_ = false;
  bool touch_position_changed_ = false;
  // TODO(https://crbug.com/824194): Remove this and associated logic once the
  // GVR-side bug is fixed and we don't need to keep track of click states
  // ourselves.
  bool previous_button_states_[GVR_CONTROLLER_BUTTON_COUNT];

  // Handedness from user prefs.
  gvr::ControllerHandedness handedness_;

  // Current touch info after the extrapolation.
  std::unique_ptr<TouchInfo> touch_info_;

  // A pointer storing the touch point from previous frame.
  std::unique_ptr<TouchPoint> prev_touch_point_;

  // A pointer storing the touch point from current frame.
  std::unique_ptr<TouchPoint> cur_touch_point_;

  // Initial touch point.
  std::unique_ptr<TouchPoint> init_touch_point_;

  // Overall velocity
  gfx::Vector2dF overall_velocity_;

  // Last velocity that is used for direction detection
  gfx::Vector2dF last_velocity_;

  // Displacement of the touch point from the previews to the current touch
  gfx::Vector2dF displacement_;

  // Head offset. Keeps the controller at the user's side with 6DoF headsets.
  gfx::Point3F head_offset_;

  int64_t last_touch_timestamp_ = 0;
  int64_t last_timestamp_nanos_ = 0;

  // Number of consecutively extrapolated touch points
  int extrapolated_touch_ = 0;

  float alpha_value_ = 1.0f;

  DISALLOW_COPY_AND_ASSIGN(VrController);
};

}  // namespace vr

#endif  // CHROME_BROWSER_ANDROID_VR_VR_CONTROLLER_H_
