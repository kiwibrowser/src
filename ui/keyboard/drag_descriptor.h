// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_KEYBOARD_DRAG_DESCRIPTOR_H_
#define UI_KEYBOARD_DRAG_DESCRIPTOR_H_

#include "ui/events/event.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/vector2d.h"

namespace keyboard {

// Tracks the state of a mouse drag to move the keyboard. The DragDescriptor
// does not actually change while the user drags. It essentially just records
// the offset of the original click on the keyboard along with the original
// location of the keyboard and uses incoming mouse move events to determine
// where the keyboard should be placed using those offsets.
struct DragDescriptor {
  gfx::Point original_keyboard_location;
  gfx::Vector2d original_click_offset;

  // Distinguish whether the current drag is from a touch event or mouse event,
  // so drag/move events can be filtered accordingly
  bool is_touch_drag;

  // The pointer ID provided by the touch event to disambiguate multiple
  // touch points. If this is a mouse event, then this value is -1.
  ui::PointerId pointer_id;
};

}  // namespace keyboard

#endif  // UI_KEYBOARD_DRAG_DESCRIPTOR_H_
