// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/graphics/cast_side_swipe_gesture_handler.h"

namespace chromecast {

bool CastSideSwipeGestureHandlerInterface::CanHandleSwipe(
    CastSideSwipeOrigin swipe_origin) {
  return false;
}

}  // namespace chromecast
