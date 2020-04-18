// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_TEST_MUS_INPUT_METHOD_MUS_TEST_API_H_
#define UI_AURA_TEST_MUS_INPUT_METHOD_MUS_TEST_API_H_

#include "base/macros.h"
#include "ui/aura/mus/input_method_mus.h"

namespace aura {

class InputMethodMusTestApi {
 public:
  static void SetInputMethod(InputMethodMus* input_method_mus,
                             ui::mojom::InputMethod* input_method) {
    input_method_mus->input_method_ = input_method;
  }

  static ui::EventDispatchDetails CallSendKeyEventToInputMethod(
      InputMethodMus* input_method_mus,
      const ui::KeyEvent& event,
      InputMethodMus::EventResultCallback ack_callback) WARN_UNUSED_RESULT {
    return input_method_mus->SendKeyEventToInputMethod(event,
                                                       std::move(ack_callback));
  }

  static void Disable(InputMethodMus* input_method) {
    DCHECK(input_method->pending_callbacks_.empty());
    input_method->ime_driver_.reset();
  }

  static void CallOnDidChangeFocusedClient(InputMethodMus* input_method,
                                           ui::TextInputClient* focused_before,
                                           ui::TextInputClient* focused) {
    input_method->OnDidChangeFocusedClient(focused_before, focused);
  }

  static void CallOnTextInputTypeChanged(InputMethodMus* input_method,
                                         ui::TextInputClient* client) {
    input_method->OnTextInputTypeChanged(client);
  }

  static void CallOnCaretBoundsChanged(InputMethodMus* input_method,
                                       ui::TextInputClient* client) {
    input_method->OnCaretBoundsChanged(client);
  }

  static void CallCancelComposition(InputMethodMus* input_method,
                                    ui::TextInputClient* client) {
    input_method->CancelComposition(client);
  }

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(InputMethodMusTestApi);
};

}  // namespace aura

#endif  // UI_AURA_TEST_MUS_INPUT_METHOD_MUS_TEST_API_H_
