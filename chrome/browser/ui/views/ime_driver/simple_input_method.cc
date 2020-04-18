// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/ime_driver/simple_input_method.h"

#include <utility>

SimpleInputMethod::SimpleInputMethod() {}

SimpleInputMethod::~SimpleInputMethod() {}

void SimpleInputMethod::OnTextInputTypeChanged(
    ui::TextInputType text_input_type) {}

void SimpleInputMethod::OnCaretBoundsChanged(const gfx::Rect& caret_bounds) {}

void SimpleInputMethod::ProcessKeyEvent(std::unique_ptr<ui::Event> key_event,
                                        ProcessKeyEventCallback callback) {
  std::move(callback).Run(false);
}

void SimpleInputMethod::CancelComposition() {}
