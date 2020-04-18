// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/keyboard/keyboard_ui.h"

#include <memory>

#include "ash/accessibility/accessibility_controller.h"
#include "ash/accessibility/accessibility_observer.h"
#include "ash/keyboard/keyboard_ui_observer.h"
#include "ash/shell.h"
#include "ui/keyboard/keyboard_controller.h"

namespace ash {

class KeyboardUIImpl : public KeyboardUI, public AccessibilityObserver {
 public:
  KeyboardUIImpl() : enabled_(false) {
    Shell::Get()->accessibility_controller()->AddObserver(this);
  }

  ~KeyboardUIImpl() override {
    if (Shell::HasInstance() && Shell::Get()->accessibility_controller())
      Shell::Get()->accessibility_controller()->RemoveObserver(this);
  }

  void ShowInDisplay(const display::Display& display) override {
    keyboard::KeyboardController* controller =
        keyboard::KeyboardController::GetInstance();
    // Controller may not exist if keyboard has been disabled. crbug.com/749989
    if (!controller)
      return;
    controller->ShowKeyboardInDisplay(display);
  }
  void Hide() override {
    // Do nothing as this is called from ash::Shell, which also calls through
    // to the appropriate keyboard functions.
  }
  bool IsEnabled() override {
    return Shell::Get()->accessibility_controller()->IsVirtualKeyboardEnabled();
  }

  // AccessibilityObserver:
  void OnAccessibilityStatusChanged() override {
    bool enabled = IsEnabled();
    if (enabled_ == enabled)
      return;

    enabled_ = enabled;
    for (auto& observer : *observers())
      observer.OnKeyboardEnabledStateChanged(enabled);
  }

 private:
  bool enabled_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardUIImpl);
};

KeyboardUI::~KeyboardUI() = default;

// static
std::unique_ptr<KeyboardUI> KeyboardUI::Create() {
  return std::make_unique<KeyboardUIImpl>();
}

void KeyboardUI::AddObserver(KeyboardUIObserver* observer) {
  observers_.AddObserver(observer);
}

void KeyboardUI::RemoveObserver(KeyboardUIObserver* observer) {
  observers_.RemoveObserver(observer);
}

KeyboardUI::KeyboardUI() = default;

}  // namespace ash
