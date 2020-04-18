// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/cast_back_gesture_dispatcher.h"

#include "chromecast/base/chromecast_switches.h"

namespace chromecast {
namespace shell {

namespace {
constexpr int kDefaultBackGestureHorizontalThreshold = 80;
}  // namespace

CastBackGestureDispatcher::CastBackGestureDispatcher(
    CastContentWindow::Delegate* delegate)
    : horizontal_threshold_(
          GetSwitchValueInt(switches::kBackGestureHorizontalThreshold,
                            kDefaultBackGestureHorizontalThreshold)),
      delegate_(delegate),
      dispatched_back_(false) {
  DCHECK(delegate_);
}
bool CastBackGestureDispatcher::CanHandleSwipe(
    CastSideSwipeOrigin swipe_origin) {
  return swipe_origin == CastSideSwipeOrigin::LEFT &&
         delegate_->CanHandleGesture(GestureType::GO_BACK);
}

void CastBackGestureDispatcher::HandleSideSwipeBegin(
    CastSideSwipeOrigin swipe_origin,
    const gfx::Point& touch_location) {
  if (swipe_origin == CastSideSwipeOrigin::LEFT) {
    dispatched_back_ = false;
  }
}

void CastBackGestureDispatcher::HandleSideSwipeContinue(
    CastSideSwipeOrigin swipe_origin,
    const gfx::Point& touch_location) {
  if (!dispatched_back_ && swipe_origin == CastSideSwipeOrigin::LEFT &&
      touch_location.x() >= horizontal_threshold_) {
    dispatched_back_ = true;
    delegate_->ConsumeGesture(GestureType::GO_BACK);
  }
}

}  // namespace shell
}  // namespace chromecast
