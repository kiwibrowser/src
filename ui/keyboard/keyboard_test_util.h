// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_KEYBOARD_KEYBOARD_TEST_UTIL_
#define UI_KEYBOARD_KEYBOARD_TEST_UTIL_

#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/window.h"
#include "ui/base/ime/dummy_input_method.h"
#include "ui/keyboard/keyboard_controller.h"
#include "ui/keyboard/keyboard_ui.h"

namespace gfx {
class Rect;
}

namespace keyboard {

// Waits until the keyboard is shown. Return false if there is no keyboard
// window created.
bool WaitUntilShown();

// Waits until the keyboard is hidden. Return false if there is no keyboard
// window created.
bool WaitUntilHidden();

// Waits until the keyboard state is changed to the given state.
void WaitControllerStateChangesTo(const KeyboardControllerState state);

// Gets the calculated keyboard bounds from |root_bounds|. The keyboard height
// is specified by |keyboard_height|.
gfx::Rect KeyboardBoundsFromRootBounds(const gfx::Rect& root_bounds,
                                       int keyboard_height);

class TestKeyboardUI : public KeyboardUI {
 public:
  TestKeyboardUI(ui::InputMethod* input_method);
  ~TestKeyboardUI() override;

  // Overridden from KeyboardUI:
  bool HasContentsWindow() const override;
  bool ShouldWindowOverscroll(aura::Window* window) const override;
  aura::Window* GetContentsWindow() override;
  ui::InputMethod* GetInputMethod() override;
  void ReloadKeyboardIfNeeded() override {}
  void InitInsets(const gfx::Rect& keyboard_bounds) override {}
  void ResetInsets() override {}

 private:
  std::unique_ptr<aura::Window> window_;
  aura::test::TestWindowDelegate delegate_;
  ui::InputMethod* input_method_;

  DISALLOW_COPY_AND_ASSIGN(TestKeyboardUI);
};

}  // namespace keyboard

#endif  // UI_KEYBOARD_KEYBOARD_TEST_UTIL_
