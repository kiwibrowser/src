// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/touch/touch_uma.h"

#include "ash/shell.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/optional.h"
#include "base/strings/stringprintf.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/base/class_property.h"
#include "ui/events/event.h"
#include "ui/events/event_utils.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/views/widget/widget.h"

namespace {

struct WindowTouchDetails {
  // Last time-stamp of the last touch-end event.
  base::Optional<base::TimeTicks> last_release_time_;

  // Stores the time of the last touch released on this window (if there was a
  // multi-touch gesture on the window, then this is the release-time of the
  // last touch on the window).
  base::Optional<base::TimeTicks> last_mt_time_;
};

DEFINE_OWNED_UI_CLASS_PROPERTY_KEY(WindowTouchDetails,
                                   kWindowTouchDetails,
                                   NULL);
}

DEFINE_UI_CLASS_PROPERTY_TYPE(WindowTouchDetails*);

namespace ash {

// static
TouchUMA* TouchUMA::GetInstance() {
  return base::Singleton<TouchUMA>::get();
}

void TouchUMA::RecordGestureEvent(aura::Window* target,
                                  const ui::GestureEvent& event) {
  GestureActionType action = FindGestureActionType(target, event);
  RecordGestureAction(action);

  if (event.type() == ui::ET_GESTURE_END &&
      event.details().touch_points() == 2) {
    WindowTouchDetails* details = target->GetProperty(kWindowTouchDetails);
    if (!details) {
      LOG(ERROR) << "Window received gesture events without receiving any touch"
                    " events";
      return;
    }
    details->last_mt_time_ = event.time_stamp();
  }
}

void TouchUMA::RecordGestureAction(GestureActionType action) {
  if (action == GESTURE_UNKNOWN || action >= GESTURE_ACTION_COUNT)
    return;
  UMA_HISTOGRAM_ENUMERATION("Ash.GestureTarget", action, GESTURE_ACTION_COUNT);
}

void TouchUMA::RecordTouchEvent(aura::Window* target,
                                const ui::TouchEvent& event) {
  UMA_HISTOGRAM_CUSTOM_COUNTS(
      "Ash.TouchRadius",
      static_cast<int>(std::max(event.pointer_details().radius_x,
                                event.pointer_details().radius_y)),
      1, 500, 100);

  WindowTouchDetails* details = target->GetProperty(kWindowTouchDetails);
  if (!details) {
    details = new WindowTouchDetails;
    target->SetProperty(kWindowTouchDetails, details);
  }

  // Record the location of the touch points.
  const int kBucketCountForLocation = 100;
  const gfx::Rect bounds = target->GetRootWindow()->bounds();
  const int bucket_size_x =
      std::max(1, bounds.width() / kBucketCountForLocation);
  const int bucket_size_y =
      std::max(1, bounds.height() / kBucketCountForLocation);

  gfx::Point position = event.root_location();

  // Prefer raw event location (when available) over calibrated location.
  if (event.HasNativeEvent()) {
    position =
        gfx::ToFlooredPoint(ui::EventLocationFromNative(event.native_event()));
    position = gfx::ScaleToFlooredPoint(
        position, 1.f / target->layer()->device_scale_factor());
  }

  position.set_x(std::min(bounds.width() - 1, std::max(0, position.x())));
  position.set_y(std::min(bounds.height() - 1, std::max(0, position.y())));

  UMA_HISTOGRAM_CUSTOM_COUNTS(
      "Ash.TouchPositionX", position.x() / bucket_size_x, 1,
      kBucketCountForLocation, kBucketCountForLocation + 1);
  UMA_HISTOGRAM_CUSTOM_COUNTS(
      "Ash.TouchPositionY", position.y() / bucket_size_y, 1,
      kBucketCountForLocation, kBucketCountForLocation + 1);

  if (event.type() == ui::ET_TOUCH_PRESSED) {
    base::RecordAction(base::UserMetricsAction("Touchscreen_Down"));

    if (details->last_release_time_) {
      // Measuring the interval between a touch-release and the next
      // touch-start is probably less useful when doing multi-touch (e.g.
      // gestures, or multi-touch friendly apps). So count this only if the user
      // hasn't done any multi-touch during the last 30 seconds.
      base::TimeDelta diff = event.time_stamp() -
                             details->last_mt_time_.value_or(base::TimeTicks());
      if (diff.InSeconds() > 30) {
        base::TimeDelta gap = event.time_stamp() - *details->last_release_time_;
        UMA_HISTOGRAM_COUNTS_10000("Ash.TouchStartAfterEnd",
                                   gap.InMilliseconds());
      }
    }
  } else if (event.type() == ui::ET_TOUCH_RELEASED) {
    details->last_release_time_ = event.time_stamp();
  }
}

TouchUMA::TouchUMA() = default;

TouchUMA::~TouchUMA() = default;

GestureActionType TouchUMA::FindGestureActionType(
    aura::Window* window,
    const ui::GestureEvent& event) {
  if (!window || window->GetRootWindow() == window) {
    if (event.type() == ui::ET_GESTURE_SCROLL_BEGIN)
      return GESTURE_BEZEL_SCROLL;
    if (event.type() == ui::ET_GESTURE_BEGIN)
      return GESTURE_BEZEL_DOWN;
    return GESTURE_UNKNOWN;
  }

  std::string name = window ? window->GetName() : std::string();

  const char kWallpaperView[] = "WallpaperView";
  if (name == kWallpaperView) {
    if (event.type() == ui::ET_GESTURE_SCROLL_BEGIN)
      return GESTURE_DESKTOP_SCROLL;
    if (event.type() == ui::ET_GESTURE_PINCH_BEGIN)
      return GESTURE_DESKTOP_PINCH;
    return GESTURE_UNKNOWN;
  }

  const char kWebPage[] = "RenderWidgetHostViewAura";
  if (name == kWebPage) {
    if (event.type() == ui::ET_GESTURE_PINCH_BEGIN)
      return GESTURE_WEBPAGE_PINCH;
    if (event.type() == ui::ET_GESTURE_SCROLL_BEGIN)
      return GESTURE_WEBPAGE_SCROLL;
    if (event.type() == ui::ET_GESTURE_TAP)
      return GESTURE_WEBPAGE_TAP;
    return GESTURE_UNKNOWN;
  }

  views::Widget* widget = views::Widget::GetWidgetForNativeView(window);
  if (!widget)
    return GESTURE_UNKNOWN;

  // |widget| may be in the process of destroying if it has ownership
  // views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET and |event| was
  // dispatched as part of gesture state cleanup. In this case the RootView
  // of |widget| may no longer exist, so check before calling into any
  // RootView methods.
  if (!widget->GetRootView())
    return GESTURE_UNKNOWN;

  views::View* view =
      widget->GetRootView()->GetEventHandlerForPoint(event.location());
  if (!view)
    return GESTURE_UNKNOWN;

  name = view->GetClassName();

  const char kTabStrip[] = "TabStrip";
  const char kTab[] = "BrowserTab";
  if (name == kTabStrip || name == kTab) {
    if (event.type() == ui::ET_GESTURE_SCROLL_BEGIN)
      return GESTURE_TABSTRIP_SCROLL;
    if (event.type() == ui::ET_GESTURE_PINCH_BEGIN)
      return GESTURE_TABSTRIP_PINCH;
    if (event.type() == ui::ET_GESTURE_TAP)
      return GESTURE_TABSTRIP_TAP;
    return GESTURE_UNKNOWN;
  }

  const char kOmnibox[] = "BrowserOmniboxViewViews";
  if (name == kOmnibox) {
    if (event.type() == ui::ET_GESTURE_SCROLL_BEGIN)
      return GESTURE_OMNIBOX_SCROLL;
    if (event.type() == ui::ET_GESTURE_PINCH_BEGIN)
      return GESTURE_OMNIBOX_PINCH;
    return GESTURE_UNKNOWN;
  }

  return GESTURE_UNKNOWN;
}

}  // namespace ash
