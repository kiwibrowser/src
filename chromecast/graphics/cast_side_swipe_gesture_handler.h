// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_GRAPHICS_CAST_SIDE_SWIPE_GESTURE_HANDLER_H_
#define CHROMECAST_GRAPHICS_CAST_SIDE_SWIPE_GESTURE_HANDLER_H_

#include "base/macros.h"

namespace gfx {
class Point;
}  // namespace gfx

namespace chromecast {

enum class CastSideSwipeOrigin { TOP, BOTTOM, LEFT, RIGHT, NONE };

// Interface for handlers triggered on reception of side-swipe gestures on the
// root window.
class CastSideSwipeGestureHandlerInterface {
 public:
  CastSideSwipeGestureHandlerInterface() = default;
  virtual ~CastSideSwipeGestureHandlerInterface() = default;

  // Return true if this handler can handle swipes from the given origin.
  virtual bool CanHandleSwipe(CastSideSwipeOrigin swipe_origin);

  // Triggered on the beginning of a swipe from edge.
  virtual void HandleSideSwipeBegin(CastSideSwipeOrigin swipe_origin,
                                    const gfx::Point& touch_location) {}

  // Triggered on the continuance (finger drag) of a swipe from edge.
  virtual void HandleSideSwipeContinue(CastSideSwipeOrigin swipe_origin,
                                       const gfx::Point& touch_location) {}

  // Triggered on the completion (finger up) of a swipe from edge.
  virtual void HandleSideSwipeEnd(CastSideSwipeOrigin swipe_origin,
                                  const gfx::Point& touch_location) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(CastSideSwipeGestureHandlerInterface);
};

}  // namespace chromecast

#endif  //  CHROMECAST_GRAPHICS_CAST_SIDE_SWIPE_GESTURE_HANDLER_H_
