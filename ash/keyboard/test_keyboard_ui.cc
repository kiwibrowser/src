// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/keyboard/test_keyboard_ui.h"

#include "ash/shell.h"
#include "ash/wm/window_util.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/ime/mock_input_method.h"

namespace ash {

TestKeyboardUI::TestKeyboardUI() = default;
TestKeyboardUI::~TestKeyboardUI() = default;

bool TestKeyboardUI::HasContentsWindow() const {
  return !!keyboard_;
}

bool TestKeyboardUI::ShouldWindowOverscroll(aura::Window* window) const {
  return true;
}

aura::Window* TestKeyboardUI::GetContentsWindow() {
  if (!keyboard_) {
    keyboard_.reset(new aura::Window(&delegate_));
    keyboard_->Init(ui::LAYER_NOT_DRAWN);
  }
  return keyboard_.get();
}

ui::InputMethod* TestKeyboardUI::GetInputMethod() {
  aura::Window* active_window = wm::GetActiveWindow();
  aura::Window* root_window = active_window ? active_window->GetRootWindow()
                                            : Shell::GetPrimaryRootWindow();
  return root_window->GetHost()->GetInputMethod();
}

void TestKeyboardUI::ReloadKeyboardIfNeeded() {}
void TestKeyboardUI::InitInsets(const gfx::Rect& keyboard_bounds) {}
void TestKeyboardUI::ResetInsets() {}

}  // namespace ash
