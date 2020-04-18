// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/graphics/cast_system_gesture_event_handler.h"

#include "chromecast/base/chromecast_switches.h"
#include "ui/aura/window.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/events/event.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/wm/core/coordinate_conversion.h"

namespace chromecast {

namespace {

// The number of pixels from the very left or right of the screen to consider as
// a valid origin for the left or right swipe gesture.
constexpr int kDefaultSideGestureStartWidth = 35;

// The number of pixels from the very top or bottom of the screen to consider as
// a valid origin for the top or bottom swipe gesture.
constexpr int kDefaultSideGestureStartHeight = 35;

}  // namespace

CastSystemGestureEventHandler::CastSystemGestureEventHandler(
    aura::Window* root_window)
    : EventHandler(),
      gesture_start_width_(GetSwitchValueInt(switches::kSystemGestureStartWidth,
                                             kDefaultSideGestureStartWidth)),
      gesture_start_height_(
          GetSwitchValueInt(switches::kSystemGestureStartHeight,
                            kDefaultSideGestureStartHeight)),
      root_window_(root_window),
      current_swipe_(CastSideSwipeOrigin::NONE) {
  DCHECK(root_window);
  root_window->AddPreTargetHandler(this);
}

CastSystemGestureEventHandler::~CastSystemGestureEventHandler() {
  DCHECK(swipe_gesture_handlers_.empty());
  root_window_->RemovePreTargetHandler(this);
}

CastSideSwipeOrigin CastSystemGestureEventHandler::GetDragPosition(
    const gfx::Point& point,
    const gfx::Rect& screen_bounds) const {
  if (point.y() < (screen_bounds.y() + gesture_start_height_)) {
    return CastSideSwipeOrigin::TOP;
  }
  if (point.x() < (screen_bounds.x() + gesture_start_width_)) {
    return CastSideSwipeOrigin::LEFT;
  }
  if (point.x() >
      (screen_bounds.x() + screen_bounds.width() - gesture_start_width_)) {
    return CastSideSwipeOrigin::RIGHT;
  }
  if (point.y() >
      (screen_bounds.y() + screen_bounds.height() - gesture_start_height_)) {
    return CastSideSwipeOrigin::BOTTOM;
  }
  return CastSideSwipeOrigin::NONE;
}

void CastSystemGestureEventHandler::OnTouchEvent(ui::TouchEvent* event) {
  if (swipe_gesture_handlers_.empty()) {
    return;
  }

  gfx::Point touch_location(event->location());
  aura::Window* target = static_cast<aura::Window*>(event->target());
  // Convert the event's point to the point on the physical screen.
  // For cast on root window this is likely going to be identical, but put it
  // through the ScreenPositionClient just to be sure.
  ::wm::ConvertPointToScreen(target, &touch_location);
  gfx::Rect screen_bounds = display::Screen::GetScreen()
                                ->GetDisplayNearestPoint(touch_location)
                                .bounds();
  CastSideSwipeOrigin side_swipe_origin =
      GetDragPosition(touch_location, screen_bounds);

  // A located event has occurred inside the margin. It might be the start of
  // our gesture, or a touch that we need to squash.
  if (current_swipe_ == CastSideSwipeOrigin::NONE &&
      side_swipe_origin != CastSideSwipeOrigin::NONE) {
    // Check to see if we have any potential consumers of events on this side.
    // If not, we can continue on without consuming it.
    bool have_swipe_consumer = false;
    for (auto* side_swipe_handler : swipe_gesture_handlers_) {
      if (side_swipe_handler->CanHandleSwipe(side_swipe_origin)) {
        have_swipe_consumer = true;
        break;
      }
    }
    if (!have_swipe_consumer) {
      return;
    }

    // Detect the beginning of a system gesture swipe.
    if (event->type() == ui::ET_TOUCH_PRESSED) {
      event->StopPropagation();
      current_swipe_ = side_swipe_origin;
      for (auto* side_swipe_handler : swipe_gesture_handlers_) {
        // Let the subscriber know about the gesture begin.
        side_swipe_handler->HandleSideSwipeBegin(side_swipe_origin,
                                                 touch_location);
      }
    }

    return;
  }

  if (current_swipe_ == CastSideSwipeOrigin::NONE) {
    return;
  }

  // A swipe is in progress, or has completed, so stop propagation of underlying
  // gesture/touch events.
  event->StopPropagation();

  // The system gesture has ended.
  if (event->type() == ui::ET_TOUCH_RELEASED) {
    for (auto* side_swipe_handler : swipe_gesture_handlers_) {
      side_swipe_handler->HandleSideSwipeEnd(current_swipe_, touch_location);
    }
    current_swipe_ = CastSideSwipeOrigin::NONE;

    return;
  }

  // The system gesture is ongoing...
  for (auto* side_swipe_handler : swipe_gesture_handlers_) {
    // Let the subscriber know about the gesture begin.
    side_swipe_handler->HandleSideSwipeContinue(current_swipe_, touch_location);
  }
}

void CastSystemGestureEventHandler::AddSideSwipeGestureHandler(
    CastSideSwipeGestureHandlerInterface* handler) {
  swipe_gesture_handlers_.insert(handler);
}

void CastSystemGestureEventHandler::RemoveSideSwipeGestureHandler(
    CastSideSwipeGestureHandlerInterface* handler) {
  swipe_gesture_handlers_.erase(handler);
}

}  // namespace chromecast
