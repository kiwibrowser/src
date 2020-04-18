// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_PLATFORM_WINDOW_PLATFORM_IME_CONTROLLER_H_
#define UI_PLATFORM_WINDOW_PLATFORM_IME_CONTROLLER_H_

#include "ui/platform_window/text_input_state.h"

namespace ui {

// Platform input method editor controller.
class PlatformImeController {
 public:
  virtual ~PlatformImeController() {}

  // Update the text input state.
  virtual void UpdateTextInputState(const TextInputState& state) = 0;

  // Set visibility of input method editor UI (software keyboard, etc).
  virtual void SetImeVisibility(bool visible) = 0;
};

}  // namespace ui

#endif  // UI_PLATFORM_WINDOW_PLATFORM_IME_CONTROLLER_H_
