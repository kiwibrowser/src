// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws2/event_test_utils.h"

#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/gfx/geometry/point.h"

namespace ui {
namespace ws2 {

std::string EventToEventType(const Event* event) {
  if (!event)
    return "<null>";
  switch (event->type()) {
    case ET_KEY_PRESSED:
      return "KEY_PRESSED";

    case ET_MOUSE_DRAGGED:
      return "MOUSE_DRAGGED";
    case ET_MOUSE_ENTERED:
      return "MOUSE_ENTERED";
    case ET_MOUSE_MOVED:
      return "MOUSE_MOVED";
    case ET_MOUSE_PRESSED:
      return "MOUSE_PRESSED";
    case ET_MOUSE_RELEASED:
      return "MOUSE_RELEASED";

    case ET_POINTER_DOWN:
      return "POINTER_DOWN";
    case ET_POINTER_ENTERED:
      return "POINTER_ENTERED";
    case ET_POINTER_MOVED:
      return "POINTER_MOVED";
    case ET_POINTER_UP:
      return "POINTER_UP";
    default:
      break;
  }
  return "<unexpected-type>";
}

std::string LocatedEventToEventTypeAndLocation(const Event* event) {
  if (!event)
    return "<null>";
  if (!event->IsLocatedEvent())
    return "<not-located-event>";
  return EventToEventType(event) + " " +
         event->AsLocatedEvent()->location().ToString();
}

}  // namespace ws2
}  // namespace ui
