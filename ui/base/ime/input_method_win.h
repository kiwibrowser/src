// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_INPUT_METHOD_WIN_H_
#define UI_BASE_IME_INPUT_METHOD_WIN_H_

#include <windows.h>

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/base/ime/input_method_win_base.h"
#include "ui/base/ime/win/imm32_manager.h"

namespace ui {

// A common InputMethod implementation based on IMM32.
class UI_BASE_IME_EXPORT InputMethodWin : public InputMethodWinBase {
 public:
  InputMethodWin(internal::InputMethodDelegate* delegate,
                 HWND toplevel_window_handle);
  ~InputMethodWin() override;

  // Overridden from InputMethodBase:
  void OnFocus() override;

  // Overridden from InputMethod:
  bool OnUntranslatedIMEMessage(const MSG event,
                                NativeEventResult* result) override;
  ui::EventDispatchDetails DispatchKeyEvent(ui::KeyEvent* event) override;
  void OnTextInputTypeChanged(const TextInputClient* client) override;
  void OnCaretBoundsChanged(const TextInputClient* client) override;
  void CancelComposition(const TextInputClient* client) override;
  void OnInputLocaleChanged() override;
  bool IsInputLocaleCJK() const override;
  bool IsCandidatePopupOpen() const override;

 protected:
  // Overridden from InputMethodBase:
  // If a derived class overrides this method, it should call parent's
  // implementation.
  void OnWillChangeFocusedClient(TextInputClient* focused_before,
                                 TextInputClient* focused) override;
  void OnDidChangeFocusedClient(TextInputClient* focused_before,
                                TextInputClient* focused) override;

 private:
  LRESULT OnImeSetContext(HWND window_handle,
                          UINT message,
                          WPARAM wparam,
                          LPARAM lparam,
                          BOOL* handled);
  LRESULT OnImeStartComposition(HWND window_handle,
                                UINT message,
                                WPARAM wparam,
                                LPARAM lparam,
                                BOOL* handled);
  LRESULT OnImeComposition(HWND window_handle,
                           UINT message,
                           WPARAM wparam,
                           LPARAM lparam,
                           BOOL* handled);
  LRESULT OnImeEndComposition(HWND window_handle,
                              UINT message,
                              WPARAM wparam,
                              LPARAM lparam,
                              BOOL* handled);
  LRESULT OnImeNotify(UINT message,
                      WPARAM wparam,
                      LPARAM lparam,
                      BOOL* handled);

  void RefreshInputLanguage();

  // Asks the client to confirm current composition text.
  void ConfirmCompositionText();

  // Enables or disables the IME according to the current text input type.
  void UpdateIMEState();

  // Callback function for IMEEngineHandlerInterface::ProcessKeyEvent.
  void ProcessKeyEventDone(ui::KeyEvent* event,
                           const std::vector<MSG>* char_msgs,
                           bool is_handled);

  ui::EventDispatchDetails ProcessUnhandledKeyEvent(
      ui::KeyEvent* event,
      const std::vector<MSG>* char_msgs);

  // Windows IMM32 wrapper.
  // (See "ui/base/ime/win/ime_input.h" for its details.)
  ui::IMM32Manager imm32_manager_;

  // The new text direction and layout alignment requested by the user by
  // pressing ctrl-shift. It'll be sent to the text input client when the key
  // is released.
  base::i18n::TextDirection pending_requested_direction_;

  // True when an IME should be allowed to process key events.
  bool enabled_;

  // True if we know for sure that a candidate window is open.
  bool is_candidate_popup_open_;

  // Window handle where composition is on-going. NULL when there is no
  // composition.
  HWND composing_window_handle_;

  // Used for making callbacks.
  base::WeakPtrFactory<InputMethodWin> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(InputMethodWin);
};

}  // namespace ui

#endif  // UI_BASE_IME_INPUT_METHOD_WIN_H_
