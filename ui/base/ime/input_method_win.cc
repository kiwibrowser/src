// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/input_method_win.h"

#include <stddef.h>
#include <stdint.h>
#include <cwctype>

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "ui/base/ime/ime_bridge.h"
#include "ui/base/ime/ime_engine_handler_interface.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/base/ime/win/tsf_input_scope.h"
#include "ui/display/win/screen_win.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/events/event_utils.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/win/hwnd_util.h"

namespace ui {
namespace {

ui::EventDispatchDetails DispatcherDestroyedDetails() {
  ui::EventDispatchDetails dispatcher_details;
  dispatcher_details.dispatcher_destroyed = true;
  return dispatcher_details;
}

}  // namespace

InputMethodWin::InputMethodWin(internal::InputMethodDelegate* delegate,
                               HWND toplevel_window_handle)
    : InputMethodWinBase(delegate, toplevel_window_handle),
      pending_requested_direction_(base::i18n::UNKNOWN_DIRECTION),
      enabled_(false),
      is_candidate_popup_open_(false),
      composing_window_handle_(NULL),
      weak_ptr_factory_(this) {
  imm32_manager_.SetInputLanguage();
}

InputMethodWin::~InputMethodWin() {}

void InputMethodWin::OnFocus() {
  InputMethodBase::OnFocus();
  RefreshInputLanguage();
}

bool InputMethodWin::OnUntranslatedIMEMessage(
    const MSG event,
    InputMethod::NativeEventResult* result) {
  LRESULT original_result = 0;
  BOOL handled = FALSE;

  switch (event.message) {
    case WM_IME_SETCONTEXT:
      original_result = OnImeSetContext(
          event.hwnd, event.message, event.wParam, event.lParam, &handled);
      break;
    case WM_IME_STARTCOMPOSITION:
      original_result = OnImeStartComposition(
          event.hwnd, event.message, event.wParam, event.lParam, &handled);
      break;
    case WM_IME_COMPOSITION:
      original_result = OnImeComposition(
          event.hwnd, event.message, event.wParam, event.lParam, &handled);
      break;
    case WM_IME_ENDCOMPOSITION:
      original_result = OnImeEndComposition(
          event.hwnd, event.message, event.wParam, event.lParam, &handled);
      break;
    case WM_IME_REQUEST:
      original_result = OnImeRequest(
          event.message, event.wParam, event.lParam, &handled);
      break;
    case WM_CHAR:
    case WM_SYSCHAR:
      original_result = OnChar(event.hwnd, event.message, event.wParam,
                               event.lParam, event, &handled);
      break;
    case WM_IME_NOTIFY:
      original_result = OnImeNotify(
          event.message, event.wParam, event.lParam, &handled);
      break;
    default:
      NOTREACHED() << "Unknown IME message:" << event.message;
      break;
  }
  if (result)
    *result = original_result;
  return !!handled;
}

ui::EventDispatchDetails InputMethodWin::DispatchKeyEvent(ui::KeyEvent* event) {
  MSG native_key_event = MSGFromKeyEvent(event);
  if (native_key_event.message == WM_CHAR) {
    auto ref = weak_ptr_factory_.GetWeakPtr();
    BOOL handled = FALSE;
    OnChar(native_key_event.hwnd, native_key_event.message,
           native_key_event.wParam, native_key_event.lParam, native_key_event,
           &handled);
    if (!ref)
      return DispatcherDestroyedDetails();
    if (handled)
      event->StopPropagation();
    return ui::EventDispatchDetails();
  }

  std::vector<MSG> char_msgs;
  // Combines the WM_KEY* and WM_CHAR messages in the event processing flow
  // which is necessary to let Chrome IME extension to process the key event
  // and perform corresponding IME actions.
  // Chrome IME extension may wants to consume certain key events based on
  // the character information of WM_CHAR messages. Holding WM_KEY* messages
  // until WM_CHAR is processed by the IME extension is not feasible because
  // there is no way to know wether there will or not be a WM_CHAR following
  // the WM_KEY*.
  // Chrome never handles dead chars so it is safe to remove/ignore
  // WM_*DEADCHAR messages.
  MSG msg;
  while (::PeekMessage(&msg, native_key_event.hwnd, WM_CHAR, WM_DEADCHAR,
                       PM_REMOVE)) {
    if (msg.message == WM_CHAR)
      char_msgs.push_back(msg);
  }
  while (::PeekMessage(&msg, native_key_event.hwnd, WM_SYSCHAR,
                       WM_SYSDEADCHAR, PM_REMOVE)) {
    if (msg.message == WM_SYSCHAR)
      char_msgs.push_back(msg);
  }

  // Handles ctrl-shift key to change text direction and layout alignment.
  if (ui::IMM32Manager::IsRTLKeyboardLayoutInstalled() &&
      !IsTextInputTypeNone()) {
    ui::KeyboardCode code = event->key_code();
    if (event->type() == ui::ET_KEY_PRESSED) {
      if (code == ui::VKEY_SHIFT) {
        base::i18n::TextDirection dir;
        if (ui::IMM32Manager::IsCtrlShiftPressed(&dir))
          pending_requested_direction_ = dir;
      } else if (code != ui::VKEY_CONTROL) {
        pending_requested_direction_ = base::i18n::UNKNOWN_DIRECTION;
      }
    } else if (event->type() == ui::ET_KEY_RELEASED &&
               (code == ui::VKEY_SHIFT || code == ui::VKEY_CONTROL) &&
               pending_requested_direction_ != base::i18n::UNKNOWN_DIRECTION) {
      GetTextInputClient()->ChangeTextDirectionAndLayoutAlignment(
          pending_requested_direction_);
      pending_requested_direction_ = base::i18n::UNKNOWN_DIRECTION;
    }
  }

  // If only 1 WM_CHAR per the key event, set it as the character of it.
  if (char_msgs.size() == 1 &&
      !std::iswcntrl(static_cast<wint_t>(char_msgs[0].wParam)))
    event->set_character(static_cast<base::char16>(char_msgs[0].wParam));

  // Dispatches the key events to the Chrome IME extension which is listening to
  // key events on the following two situations:
  // 1) |char_msgs| is empty when the event is non-character key.
  // 2) |char_msgs|.size() == 1 when the event is character key and the WM_CHAR
  // messages have been combined in the event processing flow.
  if (char_msgs.size() <= 1 && GetEngine() &&
      GetEngine()->IsInterestedInKeyEvent()) {
    ui::IMEEngineHandlerInterface::KeyEventDoneCallback callback =
        base::BindOnce(&InputMethodWin::ProcessKeyEventDone,
                       weak_ptr_factory_.GetWeakPtr(),
                       base::Owned(new ui::KeyEvent(*event)),
                       base::Owned(new std::vector<MSG>(char_msgs)));
    GetEngine()->ProcessKeyEvent(*event, std::move(callback));
    return ui::EventDispatchDetails();
  }

  return ProcessUnhandledKeyEvent(event, &char_msgs);
}

void InputMethodWin::ProcessKeyEventDone(ui::KeyEvent* event,
                                         const std::vector<MSG>* char_msgs,
                                         bool is_handled) {
  if (is_handled)
    return;
  ProcessUnhandledKeyEvent(event, char_msgs);
}

ui::EventDispatchDetails InputMethodWin::ProcessUnhandledKeyEvent(
    ui::KeyEvent* event,
    const std::vector<MSG>* char_msgs) {
  DCHECK(event);
  ui::EventDispatchDetails details = DispatchKeyEventPostIME(event);
  if (details.dispatcher_destroyed || details.target_destroyed ||
      event->stopped_propagation()) {
    return details;
  }

  BOOL handled;
  for (const auto& msg : (*char_msgs)) {
    auto ref = weak_ptr_factory_.GetWeakPtr();
    OnChar(msg.hwnd, msg.message, msg.wParam, msg.lParam, msg, &handled);
    if (!ref)
      return DispatcherDestroyedDetails();
  }
  return details;
}

void InputMethodWin::OnTextInputTypeChanged(const TextInputClient* client) {
  if (!IsTextInputClientFocused(client) || !IsWindowFocused(client))
    return;
  imm32_manager_.CancelIME(toplevel_window_handle_);
  UpdateIMEState();
}

void InputMethodWin::OnCaretBoundsChanged(const TextInputClient* client) {
  if (!IsTextInputClientFocused(client) || !IsWindowFocused(client))
    return;
  NotifyTextInputCaretBoundsChanged(client);
  TextInputType text_input_type = GetTextInputType();
  if (client == GetTextInputClient() &&
      text_input_type != TEXT_INPUT_TYPE_NONE &&
      text_input_type != TEXT_INPUT_TYPE_PASSWORD && GetEngine()) {
    // |enabled_| == false could be faked, and the engine should rely on the
    // real type from GetTextInputType().
    GetEngine()->SetCompositionBounds(GetCompositionBounds(client));
  }
  if (!enabled_)
    return;

  // The current text input type should not be NONE if |client| is focused.
  DCHECK(!IsTextInputTypeNone());
  // Tentatively assume that the returned value is DIP (Density Independent
  // Pixel). See the comment in text_input_client.h and http://crbug.com/360334.
  const gfx::Rect dip_screen_bounds(GetTextInputClient()->GetCaretBounds());
  const gfx::Rect screen_bounds =
      display::win::ScreenWin::DIPToScreenRect(toplevel_window_handle_,
                                               dip_screen_bounds);

  HWND attached_window = toplevel_window_handle_;
  // TODO(ime): see comment in TextInputClient::GetCaretBounds(), this
  // conversion shouldn't be necessary.
  RECT r = {};
  GetClientRect(attached_window, &r);
  POINT window_point = { screen_bounds.x(), screen_bounds.y() };
  ScreenToClient(attached_window, &window_point);
  gfx::Rect caret_rect(gfx::Point(window_point.x, window_point.y),
                       screen_bounds.size());
  imm32_manager_.UpdateCaretRect(attached_window, caret_rect);
}

void InputMethodWin::CancelComposition(const TextInputClient* client) {
  if (IsTextInputClientFocused(client)) {
    // |enabled_| == false could be faked, and the engine should rely on the
    // real type get from GetTextInputType().
    TextInputType text_input_type = GetTextInputType();
    if (text_input_type != TEXT_INPUT_TYPE_NONE &&
        text_input_type != TEXT_INPUT_TYPE_PASSWORD && GetEngine()) {
      GetEngine()->Reset();
    }

    if (enabled_)
      imm32_manager_.CancelIME(toplevel_window_handle_);
  }
}

void InputMethodWin::OnInputLocaleChanged() {
  // Note: OnInputLocaleChanged() is for capturing the input language which can
  // be used to determine the appropriate TextInputType for Omnibox.
  // See crbug.com/344834.
  // Currently OnInputLocaleChanged() on Windows relies on WM_INPUTLANGCHANGED,
  // which is known to be incompatible with TSF.
  // TODO(shuchen): Use ITfLanguageProfileNotifySink instead.
  OnInputMethodChanged();
  RefreshInputLanguage();
}

bool InputMethodWin::IsInputLocaleCJK() const {
  return imm32_manager_.IsInputLanguageCJK();
}

bool InputMethodWin::IsCandidatePopupOpen() const {
  return is_candidate_popup_open_;
}

void InputMethodWin::OnWillChangeFocusedClient(TextInputClient* focused_before,
                                               TextInputClient* focused) {
  if (IsWindowFocused(focused_before))
    ConfirmCompositionText();
}

void InputMethodWin::OnDidChangeFocusedClient(
    TextInputClient* focused_before,
    TextInputClient* focused) {
  if (IsWindowFocused(focused)) {
    // Force to update the input type since client's TextInputStateChanged()
    // function might not be called if text input types before the client loses
    // focus and after it acquires focus again are the same.
    OnTextInputTypeChanged(focused);

    // Force to update caret bounds, in case the client thinks that the caret
    // bounds has not changed.
    OnCaretBoundsChanged(focused);
  }
  InputMethodWinBase::OnDidChangeFocusedClient(focused_before, focused);
}

LRESULT InputMethodWin::OnImeSetContext(HWND window_handle,
                                        UINT message,
                                        WPARAM wparam,
                                        LPARAM lparam,
                                        BOOL* handled) {
  if (!!wparam) {
    imm32_manager_.CreateImeWindow(window_handle);
    // Delay initialize the tsf to avoid perf regression.
    // Loading tsf dll causes some time, so doing it in UpdateIMEState() will
    // slow down the browser window creation.
    // See crbug.com/509984.
    tsf_inputscope::InitializeTsfForInputScopes();
    tsf_inputscope::SetInputScopeForTsfUnawareWindow(
        toplevel_window_handle_, GetTextInputType(), GetTextInputMode());
  }

  OnInputMethodChanged();
  return imm32_manager_.SetImeWindowStyle(
      window_handle, message, wparam, lparam, handled);
}

LRESULT InputMethodWin::OnImeStartComposition(HWND window_handle,
                                              UINT message,
                                              WPARAM wparam,
                                              LPARAM lparam,
                                              BOOL* handled) {
  // We have to prevent WTL from calling ::DefWindowProc() because the function
  // calls ::ImmSetCompositionWindow() and ::ImmSetCandidateWindow() to
  // over-write the position of IME windows.
  *handled = TRUE;

  // Reset the composition status and create IME windows.
  composing_window_handle_ = window_handle;
  imm32_manager_.CreateImeWindow(window_handle);
  imm32_manager_.ResetComposition(window_handle);
  return 0;
}

LRESULT InputMethodWin::OnImeComposition(HWND window_handle,
                                         UINT message,
                                         WPARAM wparam,
                                         LPARAM lparam,
                                         BOOL* handled) {
  // We have to prevent WTL from calling ::DefWindowProc() because we do not
  // want for the IMM (Input Method Manager) to send WM_IME_CHAR messages.
  *handled = TRUE;

  // At first, update the position of the IME window.
  imm32_manager_.UpdateImeWindow(window_handle);

  // Retrieve the result string and its attributes of the ongoing composition
  // and send it to a renderer process.
  ui::CompositionText composition;
  if (imm32_manager_.GetResult(window_handle, lparam, &composition.text)) {
    if (!IsTextInputTypeNone())
      GetTextInputClient()->InsertText(composition.text);
    imm32_manager_.ResetComposition(window_handle);
    // Fall though and try reading the composition string.
    // Japanese IMEs send a message containing both GCS_RESULTSTR and
    // GCS_COMPSTR, which means an ongoing composition has been finished
    // by the start of another composition.
  }
  // Retrieve the composition string and its attributes of the ongoing
  // composition and send it to a renderer process.
  if (imm32_manager_.GetComposition(window_handle, lparam, &composition) &&
      !IsTextInputTypeNone())
    GetTextInputClient()->SetCompositionText(composition);

  return 0;
}

LRESULT InputMethodWin::OnImeEndComposition(HWND window_handle,
                                            UINT message,
                                            WPARAM wparam,
                                            LPARAM lparam,
                                            BOOL* handled) {
  // Let WTL call ::DefWindowProc() and release its resources.
  *handled = FALSE;

  composing_window_handle_ = NULL;

  // This is a hack fix for MS Korean IME issue (crbug.com/647150).
  // Messages received when hitting Space key during composition:
  //   1. WM_IME_ENDCOMPOSITION (we usually clear composition for this MSG)
  //   2. WM_IME_COMPOSITION with GCS_RESULTSTR (we usually commit composition)
  // (Which is in the reversed order compared to MS Japanese and Chinese IME.)
  // Hack fix:
  //   * Discard WM_IME_ENDCOMPOSITION message if it's followed by a
  //     WM_IME_COMPOSITION message with GCS_RESULTSTR.
  // This works because we don't require WM_IME_ENDCOMPOSITION after committing
  // composition (it doesn't do anything if there is no on-going composition).
  // Also see Firefox's implementation:
  // https://dxr.mozilla.org/mozilla-beta/source/widget/windows/IMMHandler.cpp#800
  // TODO(crbug.com/654865): Further investigations and clean-ups required.
  MSG compositionMsg;
  if (::PeekMessage(&compositionMsg, window_handle, WM_IME_STARTCOMPOSITION,
                    WM_IME_COMPOSITION, PM_NOREMOVE) &&
      compositionMsg.message == WM_IME_COMPOSITION &&
      (compositionMsg.lParam & GCS_RESULTSTR))
    return 0;

  if (!IsTextInputTypeNone() && GetTextInputClient()->HasCompositionText())
    GetTextInputClient()->ClearCompositionText();

  imm32_manager_.ResetComposition(window_handle);
  imm32_manager_.DestroyImeWindow(window_handle);
  return 0;
}

LRESULT InputMethodWin::OnImeNotify(UINT message,
                                    WPARAM wparam,
                                    LPARAM lparam,
                                    BOOL* handled) {
  *handled = FALSE;

  // Update |is_candidate_popup_open_|, whether a candidate window is open.
  switch (wparam) {
  case IMN_OPENCANDIDATE:
    is_candidate_popup_open_ = true;
    break;
  case IMN_CLOSECANDIDATE:
    is_candidate_popup_open_ = false;
    break;
  }

  return 0;
}

void InputMethodWin::RefreshInputLanguage() {
  TextInputType type_original = GetTextInputType();
  imm32_manager_.SetInputLanguage();
  if (type_original != GetTextInputType()) {
    // Only update the IME state when necessary.
    // It's unnecessary to report IME state, when:
    // 1) Switching betweeen 2 top-level windows, and the switched-away window
    //    receives OnInputLocaleChanged.
    // 2) The text input type is not changed by |SetInputLanguage|.
    // Please refer to crbug.com/679564.
    UpdateIMEState();
  }
}

void InputMethodWin::ConfirmCompositionText() {
  if (composing_window_handle_)
    imm32_manager_.CleanupComposition(composing_window_handle_);

  // Though above line should confirm the client's composition text by sending a
  // result text to us, in case the input method and the client are in
  // inconsistent states, we check the client's composition state again.
  if (!IsTextInputTypeNone() && GetTextInputClient()->HasCompositionText()) {
    GetTextInputClient()->ConfirmCompositionText();

    if (GetEngine())
      GetEngine()->Reset();
  }
}

void InputMethodWin::UpdateIMEState() {
  // Use switch here in case we are going to add more text input types.
  // We disable input method in password field.
  const HWND window_handle = toplevel_window_handle_;
  const TextInputType text_input_type =
      (GetEngine() && GetEngine()->IsInterestedInKeyEvent())
          ? TEXT_INPUT_TYPE_NONE
          : GetTextInputType();
  const TextInputMode text_input_mode = GetTextInputMode();
  switch (text_input_type) {
    case ui::TEXT_INPUT_TYPE_NONE:
    case ui::TEXT_INPUT_TYPE_PASSWORD:
      imm32_manager_.DisableIME(window_handle);
      enabled_ = false;
      break;
    default:
      imm32_manager_.EnableIME(window_handle);
      enabled_ = true;
      break;
  }

  imm32_manager_.SetTextInputMode(window_handle, text_input_mode);
  tsf_inputscope::SetInputScopeForTsfUnawareWindow(
      window_handle, text_input_type, text_input_mode);

  if (!ui::IMEBridge::Get())  // IMEBridge could be null for tests.
    return;

  const TextInputType old_text_input_type =
      ui::IMEBridge::Get()->GetCurrentInputContext().type;
  ui::IMEEngineHandlerInterface::InputContext context(
      GetTextInputType(), GetTextInputMode(), GetTextInputFlags(),
      ui::TextInputClient::FOCUS_REASON_OTHER, GetClientShouldDoLearning());
  ui::IMEBridge::Get()->SetCurrentInputContext(context);

  ui::IMEEngineHandlerInterface* engine = GetEngine();
  if (engine) {
    if (old_text_input_type != ui::TEXT_INPUT_TYPE_NONE)
      engine->FocusOut();
    if (GetTextInputType() != ui::TEXT_INPUT_TYPE_NONE)
      engine->FocusIn(context);
  }
}

}  // namespace ui
