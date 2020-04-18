// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/ui_input_manager.h"

#include <algorithm>

#include "base/containers/adapters.h"
#include "base/macros.h"
#include "chrome/browser/vr/elements/ui_element.h"
#include "chrome/browser/vr/model/controller_model.h"
#include "chrome/browser/vr/model/reticle_model.h"
#include "chrome/browser/vr/model/text_input_info.h"
#include "chrome/browser/vr/ui_renderer.h"
#include "chrome/browser/vr/ui_scene.h"
// TODO(tiborg): Remove include once we use a generic type to pass scroll/fling
// gestures.
#include "third_party/blink/public/platform/web_gesture_event.h"

namespace vr {

namespace {

constexpr gfx::PointF kInvalidTargetPoint =
    gfx::PointF(std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max());

constexpr float kControllerQuiescenceAngularThresholdDegrees = 3.5f;
constexpr float kControllerQuiescenceTemporalThresholdSeconds = 1.2f;
constexpr float kControllerFocusThresholdSeconds = 1.0f;

bool IsCentroidInViewport(const gfx::Transform& view_proj_matrix,
                          const gfx::Transform& world_matrix) {
  if (world_matrix.IsIdentity()) {
    // Uninitialized matrices are considered out of the viewport.
    return false;
  }
  gfx::Transform m = view_proj_matrix * world_matrix;
  gfx::Point3F o;
  m.TransformPoint(&o);
  return o.x() > -1.0f && o.x() < 1.0f && o.y() > -1.0f && o.y() < 1.0f;
}

bool IsScrollEvent(const GestureList& list) {
  if (list.empty()) {
    return false;
  }
  // We assume that we only need to consider the first gesture in the list.
  blink::WebInputEvent::Type type = list.front()->GetType();
  if (type == blink::WebInputEvent::kGestureScrollBegin ||
      type == blink::WebInputEvent::kGestureScrollEnd ||
      type == blink::WebInputEvent::kGestureScrollUpdate ||
      type == blink::WebInputEvent::kGestureFlingCancel) {
    return true;
  }
  return false;
}

void HitTestElements(UiScene* scene,
                     ReticleModel* reticle_model,
                     HitTestRequest* request) {
  std::vector<const UiElement*> elements = scene->GetElementsToHitTest();
  std::vector<const UiElement*> sorted =
      UiRenderer::GetElementsInDrawOrder(elements);

  for (const auto* element : base::Reversed(sorted)) {
    DCHECK(element->IsHitTestable());

    HitTestResult result;
    element->HitTest(*request, &result);
    if (result.type != HitTestResult::Type::kHits) {
      continue;
    }

    reticle_model->target_element_id = element->id();
    reticle_model->target_local_point = result.local_hit_point;
    reticle_model->target_point = result.hit_point;
    reticle_model->cursor_type = element->cursor_type();
    break;
  }
}

}  // namespace

UiInputManager::UiInputManager(UiScene* scene) : scene_(scene) {}

UiInputManager::~UiInputManager() {}

void UiInputManager::HandleInput(base::TimeTicks current_time,
                                 const RenderInfo& render_info,
                                 const ControllerModel& controller_model,
                                 ReticleModel* reticle_model,
                                 GestureList* gesture_list) {
  UpdateQuiescenceState(current_time, controller_model);
  UpdateControllerFocusState(current_time, render_info, controller_model);
  reticle_model->target_element_id = 0;
  reticle_model->target_local_point = kInvalidTargetPoint;
  GetVisualTargetElement(controller_model, reticle_model);
  // TODO(vollick): support multiple dispatch. We may want to, for example,
  // dispatch raw events to several elements we hit (imagine nested horizontal
  // and vertical scrollers). Currently, we only dispatch to one "winner".
  UiElement* target_element =
      scene_->GetUiElementById(reticle_model->target_element_id);
  if (target_element) {
    if (IsScrollEvent(*gesture_list) && !input_capture_element_id_) {
      DCHECK(!in_scroll_ && !in_click_);
      UiElement* ancestor = target_element;
      while (!ancestor->scrollable() && ancestor->parent()) {
        ancestor = ancestor->parent();
      }
      if (ancestor->scrollable()) {
        target_element = ancestor;
      }
    }
  }

  auto element_local_point = reticle_model->target_local_point;
  if (input_capture_element_id_) {
    auto* captured = scene_->GetUiElementById(input_capture_element_id_);
    if (captured && captured->IsVisible()) {
      HitTestRequest request;
      request.ray_target = reticle_model->target_point;
      request.max_distance_to_plane = 2 * scene_->background_distance();
      HitTestResult result;
      captured->HitTest(request, &result);
      element_local_point = result.local_hit_point;
      if (result.type == HitTestResult::Type::kNone) {
        element_local_point = kInvalidTargetPoint;
      }
    }
  }

  SendFlingCancel(gesture_list, element_local_point);
  // For simplicity, don't allow scrolling while clicking until we need to.
  if (!in_click_) {
    SendScrollEnd(gesture_list, element_local_point,
                  controller_model.touchpad_button_state);
    if (!SendScrollBegin(target_element, gesture_list, element_local_point)) {
      SendScrollUpdate(gesture_list, element_local_point);
    }
  }

  // If we're still scrolling, don't hover (and we can't be clicking, because
  // click ends scroll).
  if (in_scroll_) {
    return;
  }

  SendHoverEvents(target_element, reticle_model->target_local_point);
  SendButtonDown(target_element, reticle_model->target_local_point,
                 controller_model.touchpad_button_state);
  SendButtonUp(element_local_point, controller_model.touchpad_button_state);

  previous_button_state_ = controller_model.touchpad_button_state;
}

void UiInputManager::SendFlingCancel(GestureList* gesture_list,
                                     const gfx::PointF& target_point) {
  if (!fling_target_id_) {
    return;
  }
  if (gesture_list->empty() || (gesture_list->front()->GetType() !=
                                blink::WebInputEvent::kGestureFlingCancel)) {
    return;
  }

  // Scrolling currently only supported on content window.
  UiElement* element = scene_->GetUiElementById(fling_target_id_);
  if (element) {
    DCHECK(element->scrollable());
    element->OnFlingCancel(std::move(gesture_list->front()), target_point);
  }
  gesture_list->erase(gesture_list->begin());
  fling_target_id_ = 0;
}

void UiInputManager::SendScrollEnd(GestureList* gesture_list,
                                   const gfx::PointF& target_point,
                                   ButtonState button_state) {
  if (!in_scroll_) {
    return;
  }
  DCHECK_GT(input_capture_element_id_, 0);
  UiElement* element = scene_->GetUiElementById(input_capture_element_id_);

  if (previous_button_state_ != button_state &&
      button_state == ButtonState::DOWN) {
    DCHECK_GT(gesture_list->size(), 0LU);
    DCHECK_EQ(gesture_list->front()->GetType(),
              blink::WebInputEvent::kGestureScrollEnd);
  }
  if (element) {
    DCHECK(element->scrollable());
  }
  if (gesture_list->empty() || gesture_list->front()->GetType() !=
                                   blink::WebInputEvent::kGestureScrollEnd) {
    return;
  }
  DCHECK_LE(gesture_list->size(), 1LU);
  fling_target_id_ = input_capture_element_id_;
  if (element) {
    element->OnScrollEnd(std::move(gesture_list->front()), target_point);
  }
  gesture_list->erase(gesture_list->begin());
  input_capture_element_id_ = 0;
  in_scroll_ = false;
}

bool UiInputManager::SendScrollBegin(UiElement* target,
                                     GestureList* gesture_list,
                                     const gfx::PointF& target_point) {
  if (in_scroll_ || !target) {
    return false;
  }
  // Scrolling currently only supported on content window.
  if (!target->scrollable()) {
    return false;
  }
  if (gesture_list->empty() || (gesture_list->front()->GetType() !=
                                blink::WebInputEvent::kGestureScrollBegin)) {
    return false;
  }
  input_capture_element_id_ = target->id();
  in_scroll_ = true;
  target->OnScrollBegin(std::move(gesture_list->front()), target_point);
  gesture_list->erase(gesture_list->begin());
  return true;
}

void UiInputManager::SendScrollUpdate(GestureList* gesture_list,
                                      const gfx::PointF& target_point) {
  if (!in_scroll_) {
    return;
  }
  DCHECK(input_capture_element_id_);
  if (gesture_list->empty() || (gesture_list->front()->GetType() !=
                                blink::WebInputEvent::kGestureScrollUpdate)) {
    return;
  }
  // Scrolling currently only supported on content window.
  UiElement* element = scene_->GetUiElementById(input_capture_element_id_);
  if (element) {
    DCHECK(element->scrollable());
    element->OnScrollUpdate(std::move(gesture_list->front()), target_point);
  }
  gesture_list->erase(gesture_list->begin());
}

void UiInputManager::SendHoverEvents(UiElement* target,
                                     const gfx::PointF& target_point) {
  if (target && target->id() == hover_target_id_) {
    SendMove(target, target_point);
    return;
  }

  UiElement* prev_hovered = scene_->GetUiElementById(hover_target_id_);
  if (prev_hovered) {
    prev_hovered->OnHoverLeave();
  }
  hover_target_id_ = 0;
  if (target) {
    target->OnHoverEnter(target_point);
    hover_target_id_ = target->id();
  }
}

void UiInputManager::SendMove(UiElement* element,
                              const gfx::PointF& target_point) {
  DCHECK(element);
  if (!element) {
    return;
  }
  // TODO(mthiesse, vollick): Content is currently way too sensitive to
  // mouse moves for how noisy the controller is. It's almost impossible
  // to click a link without unintentionally starting a drag event. For
  // this reason we disable mouse moves, only delivering a down and up
  // event.
  if (element->name() == kContentQuad && in_click_) {
    return;
  }
  element->OnMove(target_point);
}

void UiInputManager::SendButtonDown(UiElement* target,
                                    const gfx::PointF& target_point,
                                    ButtonState button_state) {
  if (in_click_) {
    return;
  }
  if (previous_button_state_ == button_state ||
      button_state != ButtonState::DOWN) {
    return;
  }
  in_click_ = true;
  if (target) {
    target->OnButtonDown(target_point);
    input_capture_element_id_ = target->id();
  } else {
    input_capture_element_id_ = 0;
  }
}

bool UiInputManager::SendButtonUp(const gfx::PointF& target_point,
                                  ButtonState button_state) {
  if (!in_click_) {
    return false;
  }
  if (previous_button_state_ == button_state ||
      button_state != ButtonState::UP) {
    return false;
  }
  in_click_ = false;
  if (!input_capture_element_id_) {
    return false;
  }
  UiElement* element = scene_->GetUiElementById(input_capture_element_id_);
  if (element) {
    element->OnButtonUp(target_point);
    // Clicking outside of the focused element causes it to lose focus.
    if (element->id() != focused_element_id_ && element->focusable()) {
      UnfocusFocusedElement();
    }
  }

  input_capture_element_id_ = 0;
  return true;
}

void UiInputManager::GetVisualTargetElement(
    const ControllerModel& controller_model,
    ReticleModel* reticle_model) const {
  // If we place the reticle based on elements intersecting the controller beam,
  // we can end up with the reticle hiding behind elements, or jumping laterally
  // in the field of view. This is physically correct, but hard to use. For
  // usability, do the following instead:
  //
  // - Project the controller laser onto a distance-limiting sphere.
  // - Create a vector between the eyes and the point on the sphere.
  // - If any UI elements intersect this vector, and are within the bounding
  //   sphere, choose the element that is last in scene draw order (which is
  //   typically the closest to the eye).

  // Compute the distance from the eyes to the distance limiting sphere. Note
  // that the sphere is centered at the controller, rather than the eye, for
  // simplicity.
  float distance = scene_->background_distance();
  reticle_model->target_point =
      controller_model.laser_origin +
      gfx::ScaleVector3d(controller_model.laser_direction, distance);

  // Determine which UI element (if any) intersects the line between the ray
  // origin and the controller target position. The ray origin will typically be
  // the world origin (roughly the eye) to make targeting with a real controller
  // more intuitive. For testing, however, we occasionally hit test along the
  // laser precisely since this geometric accuracy is important and we are not
  // dealing with a physical controller.
  gfx::Point3F ray_origin;
  if (hit_test_strategy_ == HitTestStrategy::PROJECT_TO_LASER_ORIGIN_FOR_TEST) {
    ray_origin = controller_model.laser_origin;
  }

  float distance_limit = (reticle_model->target_point - ray_origin).Length();

  HitTestRequest request;
  request.ray_origin = ray_origin;
  request.ray_target = reticle_model->target_point;
  request.max_distance_to_plane = distance_limit;
  HitTestElements(scene_, reticle_model, &request);
}

void UiInputManager::UpdateQuiescenceState(
    base::TimeTicks current_time,
    const ControllerModel& controller_model) {
  // Update quiescence state.
  gfx::Point3F old_position;
  gfx::Point3F old_forward_position(0, 0, -1);
  last_significant_controller_transform_.TransformPoint(&old_position);
  last_significant_controller_transform_.TransformPoint(&old_forward_position);
  gfx::Vector3dF old_forward = old_forward_position - old_position;
  old_forward.GetNormalized(&old_forward);
  gfx::Point3F new_position;
  gfx::Point3F new_forward_position(0, 0, -1);
  controller_model.transform.TransformPoint(&new_position);
  controller_model.transform.TransformPoint(&new_forward_position);
  gfx::Vector3dF new_forward = new_forward_position - new_position;
  new_forward.GetNormalized(&new_forward);

  float angle = AngleBetweenVectorsInDegrees(old_forward, new_forward);
  if (angle > kControllerQuiescenceAngularThresholdDegrees || in_click_ ||
      in_scroll_) {
    controller_quiescent_ = false;
    last_significant_controller_transform_ = controller_model.transform;
    last_significant_controller_update_time_ = current_time;
  } else if ((current_time - last_significant_controller_update_time_)
                 .InSecondsF() >
             kControllerQuiescenceTemporalThresholdSeconds) {
    controller_quiescent_ = true;
  }
}

void UiInputManager::UpdateControllerFocusState(
    base::TimeTicks current_time,
    const RenderInfo& render_info,
    const ControllerModel& controller_model) {
  if (!IsCentroidInViewport(render_info.left_eye_model.view_proj_matrix,
                            controller_model.transform) &&
      !IsCentroidInViewport(render_info.right_eye_model.view_proj_matrix,
                            controller_model.transform)) {
    last_controller_outside_viewport_time_ = current_time;
    controller_resting_in_viewport_ = false;
    return;
  }

  controller_resting_in_viewport_ =
      (current_time - last_controller_outside_viewport_time_).InSecondsF() >
      kControllerFocusThresholdSeconds;
}

void UiInputManager::UnfocusFocusedElement() {
  if (!focused_element_id_)
    return;

  UiElement* focused = scene_->GetUiElementById(focused_element_id_);
  if (focused && focused->focusable()) {
    focused->OnFocusChanged(false);
  }
  focused_element_id_ = 0;
}

void UiInputManager::RequestFocus(int element_id) {
  if (element_id == focused_element_id_)
    return;

  UnfocusFocusedElement();

  UiElement* focused = scene_->GetUiElementById(element_id);
  if (!focused || !focused->focusable())
    return;

  focused_element_id_ = element_id;
  focused->OnFocusChanged(true);
}

void UiInputManager::RequestUnfocus(int element_id) {
  if (element_id != focused_element_id_)
    return;

  UnfocusFocusedElement();
}

void UiInputManager::OnInputEdited(const EditedText& info) {
  UiElement* focused = scene_->GetUiElementById(focused_element_id_);
  if (!focused)
    return;
  DCHECK(focused->focusable());
  focused->OnInputEdited(info);
}

void UiInputManager::OnInputCommitted(const EditedText& info) {
  UiElement* focused = scene_->GetUiElementById(focused_element_id_);
  if (!focused || !focused->focusable())
    return;
  DCHECK(focused->focusable());
  focused->OnInputCommitted(info);
}

void UiInputManager::OnKeyboardHidden() {
  UnfocusFocusedElement();
}

}  // namespace vr
