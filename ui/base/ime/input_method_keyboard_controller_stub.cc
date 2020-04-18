// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/input_method_keyboard_controller_stub.h"

namespace ui {

// InputMethodKeyboardControllerStub member definitions.
InputMethodKeyboardControllerStub::InputMethodKeyboardControllerStub() {}

InputMethodKeyboardControllerStub::~InputMethodKeyboardControllerStub() {}

bool InputMethodKeyboardControllerStub::DisplayVirtualKeyboard() {
  return false;
}

void InputMethodKeyboardControllerStub::DismissVirtualKeyboard() {}

void InputMethodKeyboardControllerStub::AddObserver(
    InputMethodKeyboardControllerObserver* observer) {}

void InputMethodKeyboardControllerStub::RemoveObserver(
    InputMethodKeyboardControllerObserver* observer) {}

bool InputMethodKeyboardControllerStub::IsKeyboardVisible() {
  return false;
}

}  // namespace ui
