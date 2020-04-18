// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_EVENT_LOCATION_H_
#define SERVICES_UI_WS_EVENT_LOCATION_H_

#include <stdint.h>

#include "ui/display/types/display_constants.h"
#include "ui/gfx/geometry/point_f.h"

namespace ui {
namespace ws {

// Contains a location relative to a particular display in two different
// coordinate systems. The location the event is received at is |raw_location|.
// |raw_location| is in terms of the pixel display layout. |location| is derived
// from |raw_location| and in terms of the DIP display layout (but in pixels).
// Typically |raw_location| is on a single display, in which case |location| is
// exactly the same value. If there is a grab then the events are still
// generated for the display the grab was initiated on, even if the mouse moves
// to another display. As the pixel layout of displays differs from the DIP
// layout |location| is converted to follow the DIP layout (but in pixels).
//
// For example, two displays might have pixels bounds of:
// 0,0 1000x1000 and 0,1060 1000x1000
// where as the DIP bounds might be:
// 0,0 500x500 and 500,0 1000x1000
// Notice the pixel bounds are staggered along the y-axis and the DIP bounds
// along the x-axis.
// If there is a grab on the first display and the mouse moves into the second
// display then |raw_location| would be 0,1060 which is converted to a
// |location| of 1000,0 (|display_id| remains the same regardless of the display
// the mouse is on.
struct EventLocation {
  EventLocation() : display_id(display::kInvalidDisplayId) {}
  explicit EventLocation(int64_t display_id) : display_id(display_id) {}
  EventLocation(const gfx::PointF& raw_location,
                const gfx::PointF& location,
                int64_t display_id)
      : raw_location(raw_location),
        location(location),
        display_id(display_id) {}

  // Location of event in terms of pixel display layout.
  gfx::PointF raw_location;

  // Location of event in terms of DIP display layout (but in pixels).
  gfx::PointF location;

  // Id of the display the event was generated from.
  int64_t display_id;
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_EVENT_LOCATION_H_
