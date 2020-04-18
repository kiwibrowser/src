// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/test/ui_controls_aura.h"

#include "base/callback.h"
#include "base/logging.h"

namespace ui_controls {
namespace {
UIControlsAura* instance_ = NULL;
bool g_ui_controls_enabled = false;
}  // namespace

void EnableUIControls() {
  g_ui_controls_enabled = true;
}

// An interface to provide Aura implementation of UI control.
bool SendKeyPress(gfx::NativeWindow window,
                  ui::KeyboardCode key,
                  bool control,
                  bool shift,
                  bool alt,
                  bool command) {
  CHECK(g_ui_controls_enabled);
  return instance_->SendKeyPress(
      window, key, control, shift, alt, command);
}

// static
bool SendKeyPressNotifyWhenDone(gfx::NativeWindow window,
                                ui::KeyboardCode key,
                                bool control,
                                bool shift,
                                bool alt,
                                bool command,
                                base::OnceClosure task) {
  CHECK(g_ui_controls_enabled);
  return instance_->SendKeyPressNotifyWhenDone(window, key, control, shift, alt,
                                               command, std::move(task));
}

// static
bool SendMouseMove(long x, long y) {
  CHECK(g_ui_controls_enabled);
  return instance_->SendMouseMove(x, y);
}

// static
bool SendMouseMoveNotifyWhenDone(long x, long y, base::OnceClosure task) {
  CHECK(g_ui_controls_enabled);
  return instance_->SendMouseMoveNotifyWhenDone(x, y, std::move(task));
}

// static
bool SendMouseEvents(MouseButton type, int state) {
  CHECK(g_ui_controls_enabled);
  return instance_->SendMouseEvents(type, state);
}

// static
bool SendMouseEventsNotifyWhenDone(MouseButton type,
                                   int state,
                                   base::OnceClosure task) {
  CHECK(g_ui_controls_enabled);
  return instance_->SendMouseEventsNotifyWhenDone(type, state, std::move(task));
}

// static
bool SendMouseClick(MouseButton type) {
  CHECK(g_ui_controls_enabled);
  return instance_->SendMouseClick(type);
}

#if defined(OS_WIN)
// static
bool SendTouchEvents(int action, int num, int x, int y) {
  CHECK(g_ui_controls_enabled);
  return instance_->SendTouchEvents(action, num, x, y);
}
#endif

UIControlsAura::UIControlsAura() {
}

UIControlsAura::~UIControlsAura() {
}

// static. declared in ui_controls.h
void InstallUIControlsAura(UIControlsAura* instance) {
  delete instance_;
  instance_ = instance;
}

}  // namespace ui_controls
