// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_UI_INPUT_MANAGER_H_
#define CHROME_BROWSER_VR_UI_INPUT_MANAGER_H_

#include <memory>
#include <vector>

#include "base/time/time.h"
#include "ui/gfx/geometry/point3_f.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/vector3d_f.h"
#include "ui/gfx/transform.h"

namespace blink {
class WebGestureEvent;
}

namespace vr {

class UiScene;
class UiElement;
struct ControllerModel;
struct RenderInfo;
struct ReticleModel;
struct EditedText;

using GestureList = std::vector<std::unique_ptr<blink::WebGestureEvent>>;

// Based on controller input finds the hit UI element and determines the
// interaction with UI elements and the web contents.
class UiInputManager {
 public:
  enum ButtonState {
    UP,       // The button is released.
    DOWN,     // The button is pressed.
  };

  // When testing, it can be useful to hit test directly along the laser.
  // Updating the strategy permits this behavior, but it should not be used in
  // production. In production, we hit test along a ray that extends from the
  // world origin to a point far along the laser.
  enum HitTestStrategy {
    PROJECT_TO_WORLD_ORIGIN,
    PROJECT_TO_LASER_ORIGIN_FOR_TEST,
  };

  explicit UiInputManager(UiScene* scene);
  ~UiInputManager();
  // TODO(tiborg): Use generic gesture type instead of blink::WebGestureEvent.
  void HandleInput(base::TimeTicks current_time,
                   const RenderInfo& render_info,
                   const ControllerModel& controller_model,
                   ReticleModel* reticle_model,
                   GestureList* gesture_list);

  // Text input related.
  void RequestFocus(int element_id);
  void RequestUnfocus(int element_id);
  void OnInputEdited(const EditedText& info);
  void OnInputCommitted(const EditedText& info);
  void OnKeyboardHidden();

  bool controller_quiescent() const { return controller_quiescent_; }
  bool controller_resting_in_viewport() const {
    return controller_resting_in_viewport_;
  }

  void set_hit_test_strategy(HitTestStrategy strategy) {
    hit_test_strategy_ = strategy;
  }

 private:
  void SendFlingCancel(GestureList* gesture_list,
                       const gfx::PointF& target_point);
  void SendScrollEnd(GestureList* gesture_list,
                     const gfx::PointF& target_point,
                     ButtonState button_state);
  bool SendScrollBegin(UiElement* target,
                       GestureList* gesture_list,
                       const gfx::PointF& target_point);
  void SendScrollUpdate(GestureList* gesture_list,
                        const gfx::PointF& target_point);

  void SendHoverEvents(UiElement* target, const gfx::PointF& target_point);
  void SendMove(UiElement* element, const gfx::PointF& target_point);
  void SendButtonDown(UiElement* target,
                      const gfx::PointF& target_point,
                      ButtonState button_state);
  bool SendButtonUp(const gfx::PointF& target_point, ButtonState button_state);
  void GetVisualTargetElement(const ControllerModel& controller_model,
                              ReticleModel* reticle_model) const;
  void UpdateQuiescenceState(base::TimeTicks current_time,
                             const ControllerModel& controller_model);
  void UpdateControllerFocusState(base::TimeTicks current_time,
                                  const RenderInfo& render_info,
                                  const ControllerModel& controller_model);

  void UnfocusFocusedElement();

  UiScene* scene_;
  int hover_target_id_ = 0;
  // TODO(mthiesse): We shouldn't have a fling target. Elements should fling
  // independently and we should only cancel flings on the relevant element
  // when we do cancel flings.
  int fling_target_id_ = 0;
  int input_capture_element_id_ = 0;
  int focused_element_id_ = 0;
  bool in_click_ = false;
  bool in_scroll_ = false;

  HitTestStrategy hit_test_strategy_ = HitTestStrategy::PROJECT_TO_WORLD_ORIGIN;

  ButtonState previous_button_state_ = ButtonState::UP;

  base::TimeTicks last_significant_controller_update_time_;
  gfx::Transform last_significant_controller_transform_;
  bool controller_quiescent_ = false;

  base::TimeTicks last_controller_outside_viewport_time_;
  bool controller_resting_in_viewport_ = false;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_UI_INPUT_MANAGER_H_
