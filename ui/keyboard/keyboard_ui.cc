// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/keyboard/keyboard_ui.h"

#include "base/command_line.h"
#include "ui/aura/window.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/base/ui_base_switches.h"
#include "ui/keyboard/keyboard_controller.h"

namespace keyboard {

KeyboardUI::KeyboardUI() : keyboard_controller_(nullptr) {}
KeyboardUI::~KeyboardUI() {}

void KeyboardUI::ShowKeyboardContainer(aura::Window* container) {
  if (HasContentsWindow()) {
    {
      TRACE_EVENT0("vk", "ShowKeyboardContainerWindow");
      GetContentsWindow()->Show();
    }
    {
      TRACE_EVENT0("vk", "ShowKeyboardContainer");
      container->Show();
    }
  }
}

void KeyboardUI::HideKeyboardContainer(aura::Window* container) {
  if (HasContentsWindow()) {
    container->Hide();
    GetContentsWindow()->Hide();
  }
}

void KeyboardUI::EnsureCaretInWorkArea(const gfx::Rect& occluded_bounds) {
  if (!GetInputMethod())
    return;

  TRACE_EVENT0("vk", "EnsureCaretInWorkArea");

  if (keyboard_controller_->IsOverscrollAllowed()) {
    GetInputMethod()->SetOnScreenKeyboardBounds(occluded_bounds);
  } else if (GetInputMethod()->GetTextInputClient()) {
    GetInputMethod()->GetTextInputClient()->EnsureCaretNotInRect(
        occluded_bounds);
  }
}

void KeyboardUI::SetController(KeyboardController* controller) {
  keyboard_controller_ = controller;
}

}  // namespace keyboard
