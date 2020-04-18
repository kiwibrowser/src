// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/ime_driver/input_method_bridge_chromeos.h"

#include <memory>
#include <utility>

#include "chrome/browser/ui/views/ime_driver/remote_text_input_client.h"

InputMethodBridge::InputMethodBridge(
    std::unique_ptr<RemoteTextInputClient> client)
    : client_(std::move(client)),
      input_method_chromeos_(
          std::make_unique<ui::InputMethodChromeOS>(client_.get())) {
  input_method_chromeos_->SetFocusedTextInputClient(client_.get());
}

InputMethodBridge::~InputMethodBridge() {}

void InputMethodBridge::OnTextInputTypeChanged(
    ui::TextInputType text_input_type) {
  client_->SetTextInputType(text_input_type);
  input_method_chromeos_->OnTextInputTypeChanged(client_.get());
}

void InputMethodBridge::OnCaretBoundsChanged(const gfx::Rect& caret_bounds) {
  client_->SetCaretBounds(caret_bounds);
  input_method_chromeos_->OnCaretBoundsChanged(client_.get());
}

void InputMethodBridge::ProcessKeyEvent(std::unique_ptr<ui::Event> event,
                                        ProcessKeyEventCallback callback) {
  DCHECK(event->IsKeyEvent());
  ui::KeyEvent* key_event = event->AsKeyEvent();
  if (!key_event->is_char()) {
    input_method_chromeos_->DispatchKeyEvent(key_event, std::move(callback));
  } else {
    const bool handled = false;
    std::move(callback).Run(handled);
  }
}

void InputMethodBridge::CancelComposition() {
  input_method_chromeos_->CancelComposition(client_.get());
}
