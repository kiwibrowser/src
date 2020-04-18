// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "content/common/input/synthetic_gesture_params.h"

namespace content {

SyntheticGestureParams::SyntheticGestureParams()
    : gesture_source_type(DEFAULT_INPUT) {}

SyntheticGestureParams::SyntheticGestureParams(
    const SyntheticGestureParams& other)
    : gesture_source_type(other.gesture_source_type) {}

SyntheticGestureParams::~SyntheticGestureParams() {}

bool SyntheticGestureParams::IsGestureSourceTypeSupported(
    GestureSourceType gesture_source_type) {
  if (gesture_source_type == DEFAULT_INPUT)
    return true;

  // These values should change very rarely. We thus hard-code them here rather
  // than having to query the brower's SyntheticGestureTarget.
#if defined(USE_AURA)
  return gesture_source_type == TOUCH_INPUT ||
         gesture_source_type == MOUSE_INPUT;
#elif defined(OS_ANDROID)
  // Android supports mouse wheel events, but mouse drag is not yet
  // supported. See crbug.com/468806.
  return gesture_source_type == TOUCH_INPUT ||
         gesture_source_type == MOUSE_INPUT;
#else
  return gesture_source_type == MOUSE_INPUT;
#endif
}

}  // namespace content
