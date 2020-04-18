// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/synthetic_pinch_gesture.h"

#include "content/browser/renderer_host/input/synthetic_touchpad_pinch_gesture.h"
#include "content/browser/renderer_host/input/synthetic_touchscreen_pinch_gesture.h"

namespace content {

SyntheticPinchGesture::SyntheticPinchGesture(
    const SyntheticPinchGestureParams& params)
    : params_(params) {}
SyntheticPinchGesture::~SyntheticPinchGesture() {}

SyntheticGesture::Result SyntheticPinchGesture::ForwardInputEvents(
    const base::TimeTicks& timestamp,
    SyntheticGestureTarget* target) {
  if (!lazy_gesture_) {
    SyntheticGestureParams::GestureSourceType source_type =
        params_.gesture_source_type;
    if (source_type == SyntheticGestureParams::DEFAULT_INPUT) {
      source_type = target->GetDefaultSyntheticGestureSourceType();
    }

    DCHECK_NE(SyntheticGestureParams::DEFAULT_INPUT, source_type);
    if (source_type == SyntheticGestureParams::TOUCH_INPUT) {
      lazy_gesture_.reset(new SyntheticTouchscreenPinchGesture(params_));
    } else {
      DCHECK_EQ(SyntheticGestureParams::MOUSE_INPUT, source_type);
      lazy_gesture_.reset(new SyntheticTouchpadPinchGesture(params_));
    }
  }

  return lazy_gesture_->ForwardInputEvents(timestamp, target);
}

}  // namespace content
