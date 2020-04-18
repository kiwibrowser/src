// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/synthetic_smooth_scroll_gesture.h"

namespace content {

SyntheticSmoothScrollGesture::SyntheticSmoothScrollGesture(
    const SyntheticSmoothScrollGestureParams& params)
    : params_(params) {
}

SyntheticSmoothScrollGesture::~SyntheticSmoothScrollGesture() {
}

SyntheticGesture::Result SyntheticSmoothScrollGesture::ForwardInputEvents(
    const base::TimeTicks& timestamp,
    SyntheticGestureTarget* target) {
  if (!move_gesture_) {
    if (!InitializeMoveGesture(params_.gesture_source_type, target))
      return SyntheticGesture::GESTURE_SOURCE_TYPE_NOT_IMPLEMENTED;
  }
  return move_gesture_->ForwardInputEvents(timestamp, target);
}

SyntheticSmoothMoveGestureParams::InputType
SyntheticSmoothScrollGesture::GetInputSourceType(
    SyntheticGestureParams::GestureSourceType gesture_source_type) {
  if (gesture_source_type == SyntheticGestureParams::MOUSE_INPUT)
    return SyntheticSmoothMoveGestureParams::MOUSE_WHEEL_INPUT;
  else
    return SyntheticSmoothMoveGestureParams::TOUCH_INPUT;
}

bool SyntheticSmoothScrollGesture::InitializeMoveGesture(
    SyntheticGestureParams::GestureSourceType gesture_type,
    SyntheticGestureTarget* target) {
  if (gesture_type == SyntheticGestureParams::DEFAULT_INPUT)
    gesture_type = target->GetDefaultSyntheticGestureSourceType();

  if (gesture_type == SyntheticGestureParams::TOUCH_INPUT ||
      gesture_type == SyntheticGestureParams::MOUSE_INPUT) {
    SyntheticSmoothMoveGestureParams move_params;
    move_params.start_point = params_.anchor;
    move_params.distances = params_.distances;
    move_params.speed_in_pixels_s = params_.speed_in_pixels_s;
    move_params.prevent_fling = params_.prevent_fling;
    move_params.input_type = GetInputSourceType(gesture_type);
    move_params.add_slop = true;
    move_gesture_.reset(new SyntheticSmoothMoveGesture(move_params));
    return true;
  }
  return false;
}

}  // namespace content
