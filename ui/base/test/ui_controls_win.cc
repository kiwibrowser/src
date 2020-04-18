// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/test/ui_controls.h"

#include "base/callback.h"
#include "ui/base/test/ui_controls_internal_win.h"
#include "ui/gfx/geometry/point.h"

namespace ui_controls {
bool g_ui_controls_enabled = false;

void EnableUIControls() {
  g_ui_controls_enabled = true;
}

bool SendKeyPress(gfx::NativeWindow window,
                  ui::KeyboardCode key,
                  bool control,
                  bool shift,
                  bool alt,
                  bool command) {
  CHECK(g_ui_controls_enabled);
  DCHECK(!command);  // No command key on Windows
  return internal::SendKeyPressImpl(window, key, control, shift, alt,
                                    base::OnceClosure());
}

bool SendKeyPressNotifyWhenDone(gfx::NativeWindow window,
                                ui::KeyboardCode key,
                                bool control,
                                bool shift,
                                bool alt,
                                bool command,
                                base::OnceClosure task) {
  CHECK(g_ui_controls_enabled);
  DCHECK(!command);  // No command key on Windows
  return internal::SendKeyPressImpl(window, key, control, shift, alt,
                                    std::move(task));
}

bool SendMouseMove(long x, long y) {
  CHECK(g_ui_controls_enabled);
  return internal::SendMouseMoveImpl(x, y, base::OnceClosure());
}

bool SendMouseMoveNotifyWhenDone(long x, long y, base::OnceClosure task) {
  CHECK(g_ui_controls_enabled);
  return internal::SendMouseMoveImpl(x, y, std::move(task));
}

bool SendMouseEvents(MouseButton type, int state) {
  CHECK(g_ui_controls_enabled);
  return internal::SendMouseEventsImpl(type, state, base::OnceClosure());
}

bool SendMouseEventsNotifyWhenDone(MouseButton type,
                                   int state,
                                   base::OnceClosure task) {
  CHECK(g_ui_controls_enabled);
  return internal::SendMouseEventsImpl(type, state, std::move(task));
}

bool SendMouseClick(MouseButton type) {
  CHECK(g_ui_controls_enabled);
  return internal::SendMouseEventsImpl(type, UP | DOWN, base::OnceClosure());
}

bool SendTouchEvents(int action, int num, int x, int y) {
  CHECK(g_ui_controls_enabled);
  return internal::SendTouchEventsImpl(action, num, x, y);
}

}  // namespace ui_controls
