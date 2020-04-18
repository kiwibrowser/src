// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_KEYBOARD_KEYBOARD_EVENT_FILTER_H_
#define UI_KEYBOARD_KEYBOARD_EVENT_FILTER_H_

#include "base/macros.h"
#include "ui/events/event.h"
#include "ui/events/event_handler.h"
#include "ui/keyboard/keyboard_export.h"

namespace keyboard {

// EventFilter for the keyboard window, which intercepts events before they are
// processed by the keyboard window.
class KEYBOARD_EXPORT KeyboardEventFilter : public ui::EventHandler {
 public:
  KeyboardEventFilter() = default;
  ~KeyboardEventFilter() override = default;

  // ui::EventHandler overrides:
  void OnGestureEvent(ui::GestureEvent* event) override;
  void OnTouchEvent(ui::TouchEvent* event) override;
  void OnMouseEvent(ui::MouseEvent* event) override;

 private:
  void ProcessPointerEvent(ui::LocatedEvent* event);

  DISALLOW_COPY_AND_ASSIGN(KeyboardEventFilter);
};

}  // namespace keyboard

#endif  // UI_KEYBOARD_KEYBOARD_EVENT_FILTER_H_
