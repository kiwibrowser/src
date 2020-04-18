// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_CAST_BACK_GESTURE_DISPATCHER_H_
#define CHROMECAST_BROWSER_CAST_BACK_GESTURE_DISPATCHER_H_

#include "base/macros.h"
#include "chromecast/browser/cast_content_window.h"
#include "chromecast/graphics/cast_side_swipe_gesture_handler.h"

namespace chromecast {
namespace shell {

// Takes side swipe gestures destined for implementations of
// CastContentWindow and dispatches them to a CastContentWindow::Delegate if the
// side swipe is a back gesture.
class CastBackGestureDispatcher : public CastSideSwipeGestureHandlerInterface {
 public:
  explicit CastBackGestureDispatcher(CastContentWindow::Delegate* delegate);

  // CastSideSwipeGestureHandlerInterface implementation:
  bool CanHandleSwipe(CastSideSwipeOrigin swipe_origin) override;
  void HandleSideSwipeBegin(CastSideSwipeOrigin swipe_origin,
                            const gfx::Point& touch_location) override;
  void HandleSideSwipeContinue(CastSideSwipeOrigin swipe_origin,
                               const gfx::Point& touch_location) override;

 private:
  // Number of pixels past swipe origin to consider as a back gesture.
  const int horizontal_threshold_;
  CastContentWindow::Delegate* const delegate_;
  bool dispatched_back_;
};

}  // namespace shell
}  // namespace chromecast

#endif  // CHROMECAST_BROWSER_CAST_BACK_GESTURE_DISPATCHER_H_
