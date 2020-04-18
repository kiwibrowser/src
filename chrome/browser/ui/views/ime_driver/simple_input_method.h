// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_IME_DRIVER_SIMPLE_INPUT_METHOD_H_
#define CHROME_BROWSER_UI_VIEWS_IME_DRIVER_SIMPLE_INPUT_METHOD_H_

#include "services/ui/public/interfaces/ime/ime.mojom.h"

// This is to be used on platforms where a proper implementation of
// ui::mojom::InputMethod is missing. It doesn't handle any events and calls
// the callback with false, which will result in client code handling events
// locally.
class SimpleInputMethod : public ui::mojom::InputMethod {
 public:
  SimpleInputMethod();
  ~SimpleInputMethod() override;

  // ui::mojom::InputMethod:
  void OnTextInputTypeChanged(ui::TextInputType text_input_type) override;
  void OnCaretBoundsChanged(const gfx::Rect& caret_bounds) override;
  void ProcessKeyEvent(std::unique_ptr<ui::Event> key_event,
                       ProcessKeyEventCallback callback) override;
  void CancelComposition() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SimpleInputMethod);
};

#endif  // CHROME_BROWSER_UI_VIEWS_IME_DRIVER_SIMPLE_INPUT_METHOD_H_
