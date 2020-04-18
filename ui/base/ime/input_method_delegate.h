// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_INPUT_METHOD_DELEGATE_H_
#define UI_BASE_IME_INPUT_METHOD_DELEGATE_H_

#include "ui/base/ime/ui_base_ime_export.h"
#include "ui/events/event_dispatcher.h"

namespace ui {

class KeyEvent;

namespace internal {

// An interface implemented by the object that handles events sent back from an
// ui::InputMethod implementation.
class UI_BASE_IME_EXPORT InputMethodDelegate {
 public:
  virtual ~InputMethodDelegate() {}

  // Dispatch a key event already processed by the input method.
  // Returns true if the event was processed.
  virtual ui::EventDispatchDetails DispatchKeyEventPostIME(
      ui::KeyEvent* key_event) = 0;
};

}  // namespace internal
}  // namespace ui

#endif  // UI_BASE_IME_INPUT_METHOD_DELEGATE_H_
