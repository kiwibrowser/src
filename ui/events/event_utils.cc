// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/event_utils.h"

#include <vector>

#include "base/metrics/histogram_macros.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace ui {

namespace {
int g_custom_event_types = ET_LAST;
}  // namespace

std::unique_ptr<Event> EventFromNative(const PlatformEvent& native_event) {
  std::unique_ptr<Event> event;
  EventType type = EventTypeFromNative(native_event);
  switch(type) {
    case ET_KEY_PRESSED:
    case ET_KEY_RELEASED:
      event.reset(new KeyEvent(native_event));
      break;

    case ET_MOUSE_PRESSED:
    case ET_MOUSE_DRAGGED:
    case ET_MOUSE_RELEASED:
    case ET_MOUSE_MOVED:
    case ET_MOUSE_ENTERED:
    case ET_MOUSE_EXITED:
      event.reset(new MouseEvent(native_event));
      break;

    case ET_MOUSEWHEEL:
      event.reset(new MouseWheelEvent(native_event));
      break;

    case ET_SCROLL_FLING_START:
    case ET_SCROLL_FLING_CANCEL:
    case ET_SCROLL:
      event.reset(new ScrollEvent(native_event));
      break;

    case ET_TOUCH_RELEASED:
    case ET_TOUCH_PRESSED:
    case ET_TOUCH_MOVED:
    case ET_TOUCH_CANCELLED:
      event.reset(new TouchEvent(native_event));
      break;

    default:
      break;
  }
  return event;
}

int RegisterCustomEventType() {
  return ++g_custom_event_types;
}

bool IsValidTimebase(base::TimeTicks now, base::TimeTicks timestamp) {
  int64_t delta = (now - timestamp).InMilliseconds();
  return delta >= 0 && delta <= 60 * 1000;
}

void ValidateEventTimeClock(base::TimeTicks* timestamp) {
#if defined(USE_X11)

  // Restrict this correction to X11 which is known to provide bogus timestamps
  // that require correction (crbug.com/611950).
  base::TimeTicks now = EventTimeForNow();
  if (!IsValidTimebase(now, *timestamp))
    *timestamp = now;

#elif DCHECK_IS_ON()

  if (base::debug::BeingDebugged())
    return;

  base::TimeTicks now = EventTimeForNow();
  DCHECK(IsValidTimebase(now, *timestamp))
      << "Event timestamp (" << *timestamp << ") is not consistent with "
      << "current time (" << now << ").";

#endif
}

bool ShouldDefaultToNaturalScroll() {
  return GetInternalDisplayTouchSupport() ==
         display::Display::TouchSupport::AVAILABLE;
}

display::Display::TouchSupport GetInternalDisplayTouchSupport() {
  display::Screen* screen = display::Screen::GetScreen();
  // No screen in some unit tests.
  if (!screen)
    return display::Display::TouchSupport::UNKNOWN;
  const std::vector<display::Display>& displays = screen->GetAllDisplays();
  for (std::vector<display::Display>::const_iterator it = displays.begin();
       it != displays.end(); ++it) {
    if (it->IsInternal())
      return it->touch_support();
  }
  return display::Display::TouchSupport::UNAVAILABLE;
}

void ComputeEventLatencyOS(const PlatformEvent& native_event) {
  base::TimeTicks current_time = EventTimeForNow();
  base::TimeTicks time_stamp = EventTimeFromNative(native_event);
  base::TimeDelta delta = current_time - time_stamp;

  EventType type = EventTypeFromNative(native_event);
  switch (type) {
#if defined(OS_MACOSX)
    // On Mac, ET_SCROLL and ET_MOUSEWHEEL represent the same class of events.
    case ET_SCROLL:
#endif
    case ET_MOUSEWHEEL:
      UMA_HISTOGRAM_CUSTOM_COUNTS("Event.Latency.OS.MOUSE_WHEEL",
                                  delta.InMicroseconds(), 1, 1000000, 50);
      return;
    case ET_TOUCH_MOVED:
      UMA_HISTOGRAM_CUSTOM_COUNTS("Event.Latency.OS.TOUCH_MOVED",
                                  delta.InMicroseconds(), 1, 1000000, 50);
      return;
    case ET_TOUCH_PRESSED:
      UMA_HISTOGRAM_CUSTOM_COUNTS("Event.Latency.OS.TOUCH_PRESSED",
                                  delta.InMicroseconds(), 1, 1000000, 50);
      return;
    case ET_TOUCH_RELEASED:
      UMA_HISTOGRAM_CUSTOM_COUNTS("Event.Latency.OS.TOUCH_RELEASED",
                                  delta.InMicroseconds(), 1, 1000000, 50);
      return;
    default:
      return;
  }
}

void ConvertEventLocationToTargetWindowLocation(
    const gfx::Point& target_window_origin,
    const gfx::Point& current_window_origin,
    ui::LocatedEvent* located_event) {
  if (current_window_origin == target_window_origin)
    return;

  DCHECK(located_event);
  gfx::Vector2d offset = current_window_origin - target_window_origin;
  gfx::PointF location_in_pixel_in_host =
      located_event->location_f() + gfx::Vector2dF(offset);
  located_event->set_location_f(location_in_pixel_in_host);
  located_event->set_root_location_f(location_in_pixel_in_host);
}

}  // namespace ui
