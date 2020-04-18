// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_IME_INPUT_CONTEXT_HANDLER_INTERFACE_H_
#define UI_BASE_IME_IME_INPUT_CONTEXT_HANDLER_INTERFACE_H_

#include <stdint.h>

#include <string>
#include "ui/base/ime/composition_text.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/ime/ui_base_ime_export.h"
#include "ui/events/event.h"

namespace ui {

class UI_BASE_IME_EXPORT IMEInputContextHandlerInterface {
 public:
  // Called when the engine commit a text.
  virtual void CommitText(const std::string& text) = 0;

  // Called when the engine updates composition text.
  virtual void UpdateCompositionText(const CompositionText& text,
                                     uint32_t cursor_pos,
                                     bool visible) = 0;

  // Called when the engine request deleting surrounding string.
  virtual void DeleteSurroundingText(int32_t offset, uint32_t length) = 0;

  // Called when the engine sends a key event.
  virtual void SendKeyEvent(KeyEvent* event) = 0;

  // Gets the input method pointer.
  virtual InputMethod* GetInputMethod() = 0;
};

}  // namespace ui

#endif  // UI_BASE_IME_IME_INPUT_CONTEXT_HANDLER_INTERFACE_H_
