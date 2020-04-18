// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_INPUT_METHOD_WIN_TSF_H_
#define UI_BASE_IME_INPUT_METHOD_WIN_TSF_H_

#include <windows.h>

#include <string>

#include "ui/base/ime/input_method_win_base.h"

namespace ui {

class TSFEventRouter;

// An InputMethod implementation based on Windows TSF API.
class UI_BASE_IME_EXPORT InputMethodWinTSF : public InputMethodWinBase {
 public:
  InputMethodWinTSF(internal::InputMethodDelegate* delegate,
                    HWND toplevel_window_handle);
  ~InputMethodWinTSF() override;

  // Overridden from InputMethod:
  ui::EventDispatchDetails DispatchKeyEvent(ui::KeyEvent* event) override;
  void OnFocus() override;
  void OnBlur() override;
  bool OnUntranslatedIMEMessage(const MSG event,
                                NativeEventResult* result) override;
  void OnTextInputTypeChanged(const TextInputClient* client) override;
  void OnCaretBoundsChanged(const TextInputClient* client) override;
  void CancelComposition(const TextInputClient* client) override;
  void DetachTextInputClient(TextInputClient* client) override;
  bool IsCandidatePopupOpen() const override;

  // Overridden from InputMethodBase:
  void OnWillChangeFocusedClient(TextInputClient* focused_before,
                                 TextInputClient* focused) override;
  void OnDidChangeFocusedClient(TextInputClient* focused_before,
                                TextInputClient* focused) override;

 private:
  class TSFEventObserver;

  // Asks the client to confirm current composition text.
  void ConfirmCompositionText();

  // TSF event router and observer.
  std::unique_ptr<TSFEventObserver> tsf_event_observer_;
  std::unique_ptr<TSFEventRouter> tsf_event_router_;

  DISALLOW_COPY_AND_ASSIGN(InputMethodWinTSF);
};

}  // namespace ui

#endif  // UI_BASE_IME_INPUT_METHOD_WIN_TSF_H_
