// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_SHELF_BEZEL_EVENT_HANDLER_H_
#define ASH_SHELF_SHELF_BEZEL_EVENT_HANDLER_H_

#include "base/macros.h"
#include "ui/events/event_handler.h"

namespace gfx {
class Point;
class Rect;
}

namespace ash {
class Shelf;

// Forwards touch gestures on a bezel sensor to the shelf.
class ShelfBezelEventHandler : public ui::EventHandler {
 public:
  explicit ShelfBezelEventHandler(Shelf* shelf);
  ~ShelfBezelEventHandler() override;

  // Overridden from ui::EventHandler:
  void OnGestureEvent(ui::GestureEvent* event) override;

 private:
  bool IsShelfOnBezel(const gfx::Rect& screen, const gfx::Point& point) const;

  Shelf* shelf_;
  bool in_touch_drag_;

  DISALLOW_COPY_AND_ASSIGN(ShelfBezelEventHandler);
};

}  // namespace ash

#endif  // ASH_SHELF_SHELF_BEZEL_EVENT_HANDLER_H_
