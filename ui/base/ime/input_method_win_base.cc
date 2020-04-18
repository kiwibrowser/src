// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/input_method_win_base.h"

#include <stddef.h>
#include <stdint.h>
#include <cwctype>

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/win/windows_version.h"
#include "ui/base/ime/ime_bridge.h"
#include "ui/base/ime/ime_engine_handler_interface.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/base/ime/win/on_screen_keyboard_display_manager_input_pane.h"
#include "ui/base/ime/win/on_screen_keyboard_display_manager_tab_tip.h"
#include "ui/base/ime/win/tsf_input_scope.h"
#include "ui/base/ui_base_features.h"
#include "ui/display/win/screen_win.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/events/event_utils.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/gfx/win/hwnd_util.h"

namespace ui {
namespace {

// Extra number of chars before and after selection (or composition) range which
// is returned to IME for improving conversion accuracy.
constexpr size_t kExtraNumberOfChars = 20;

std::unique_ptr<InputMethodKeyboardController> CreateKeyboardController(
    HWND toplevel_window_handle) {
  if (base::FeatureList::IsEnabled(features::kInputPaneOnScreenKeyboard) &&
      base::win::GetVersion() >= base::win::VERSION_WIN10_RS1) {
    return std::make_unique<OnScreenKeyboardDisplayManagerInputPane>(
        toplevel_window_handle);
  } else if (base::win::GetVersion() >= base::win::VERSION_WIN8) {
    return std::make_unique<OnScreenKeyboardDisplayManagerTabTip>(
        toplevel_window_handle);
  }
  return nullptr;
}

}  // namespace

InputMethodWinBase::InputMethodWinBase(internal::InputMethodDelegate* delegate,
                                       HWND toplevel_window_handle)
    : InputMethodBase(delegate,
                      CreateKeyboardController(toplevel_window_handle)),
      toplevel_window_handle_(toplevel_window_handle) {}

InputMethodWinBase::~InputMethodWinBase() {}

void InputMethodWinBase::OnDidChangeFocusedClient(
    TextInputClient* focused_before,
    TextInputClient* focused) {
  if (focused_before != focused)
    accept_carriage_return_ = false;
}

bool InputMethodWinBase::IsWindowFocused(const TextInputClient* client) const {
  if (!client)
    return false;
  // When Aura is enabled, |attached_window_handle| should always be a top-level
  // window. So we can safely assume that |attached_window_handle| is ready for
  // receiving keyboard input as long as it is an active window. This works well
  // even when the |attached_window_handle| becomes active but has not received
  // WM_FOCUS yet.
  return toplevel_window_handle_ &&
         GetActiveWindow() == toplevel_window_handle_;
}

LRESULT InputMethodWinBase::OnChar(HWND window_handle,
                                   UINT message,
                                   WPARAM wparam,
                                   LPARAM lparam,
                                   const MSG& event,
                                   BOOL* handled) {
  *handled = TRUE;

  // We need to send character events to the focused text input client event if
  // its text input type is ui::TEXT_INPUT_TYPE_NONE.
  if (GetTextInputClient()) {
    const base::char16 kCarriageReturn = L'\r';
    const base::char16 ch = static_cast<base::char16>(wparam);
    // A mask to determine the previous key state from |lparam|. The value is 1
    // if the key is down before the message is sent, or it is 0 if the key is
    // up.
    const uint32_t kPrevKeyDownBit = 0x40000000;
    if (ch == kCarriageReturn && !(lparam & kPrevKeyDownBit))
      accept_carriage_return_ = true;
    // Conditionally ignore '\r' events to work around https://crbug.com/319100.
    // TODO(yukawa, IME): Figure out long-term solution.
    if (ch != kCarriageReturn || accept_carriage_return_) {
      ui::KeyEvent char_event = ui::KeyEventFromMSG(event);
      GetTextInputClient()->InsertChar(char_event);
    }
  }

  // Explicitly show the system menu at a good location on [Alt]+[Space].
  // Note: Setting |handled| to FALSE for DefWindowProc triggering of the system
  //       menu causes undesirable titlebar artifacts in the classic theme.
  if (message == WM_SYSCHAR && wparam == VK_SPACE)
    gfx::ShowSystemMenu(window_handle);

  return 0;
}

LRESULT InputMethodWinBase::OnImeRequest(UINT message,
                                         WPARAM wparam,
                                         LPARAM lparam,
                                         BOOL* handled) {
  *handled = FALSE;

  // Should not receive WM_IME_REQUEST message, if IME is disabled.
  const ui::TextInputType type = GetTextInputType();
  if (type == ui::TEXT_INPUT_TYPE_NONE ||
      type == ui::TEXT_INPUT_TYPE_PASSWORD) {
    return 0;
  }

  switch (wparam) {
    case IMR_RECONVERTSTRING:
      *handled = TRUE;
      return OnReconvertString(reinterpret_cast<RECONVERTSTRING*>(lparam));
    case IMR_DOCUMENTFEED:
      *handled = TRUE;
      return OnDocumentFeed(reinterpret_cast<RECONVERTSTRING*>(lparam));
    case IMR_QUERYCHARPOSITION:
      *handled = TRUE;
      return OnQueryCharPosition(reinterpret_cast<IMECHARPOSITION*>(lparam));
    default:
      return 0;
  }
}

LRESULT InputMethodWinBase::OnDocumentFeed(RECONVERTSTRING* reconv) {
  ui::TextInputClient* client = GetTextInputClient();
  if (!client)
    return 0;

  gfx::Range text_range;
  if (!client->GetTextRange(&text_range) || text_range.is_empty())
    return 0;

  bool result = false;
  gfx::Range target_range;
  if (client->HasCompositionText())
    result = client->GetCompositionTextRange(&target_range);

  if (!result || target_range.is_empty()) {
    if (!client->GetSelectionRange(&target_range) || !target_range.IsValid()) {
      return 0;
    }
  }

  if (!text_range.Contains(target_range))
    return 0;

  if (target_range.GetMin() - text_range.start() > kExtraNumberOfChars)
    text_range.set_start(target_range.GetMin() - kExtraNumberOfChars);

  if (text_range.end() - target_range.GetMax() > kExtraNumberOfChars)
    text_range.set_end(target_range.GetMax() + kExtraNumberOfChars);

  size_t len = text_range.length();
  size_t need_size = sizeof(RECONVERTSTRING) + len * sizeof(WCHAR);

  if (!reconv)
    return need_size;

  if (reconv->dwSize < need_size)
    return 0;

  base::string16 text;
  if (!GetTextInputClient()->GetTextFromRange(text_range, &text))
    return 0;
  DCHECK_EQ(text_range.length(), text.length());

  reconv->dwVersion = 0;
  reconv->dwStrLen = len;
  reconv->dwStrOffset = sizeof(RECONVERTSTRING);
  reconv->dwCompStrLen =
      client->HasCompositionText() ? target_range.length() : 0;
  reconv->dwCompStrOffset =
      (target_range.GetMin() - text_range.start()) * sizeof(WCHAR);
  reconv->dwTargetStrLen = target_range.length();
  reconv->dwTargetStrOffset = reconv->dwCompStrOffset;

  memcpy((char*)reconv + sizeof(RECONVERTSTRING), text.c_str(),
         len * sizeof(WCHAR));

  // According to Microsoft API document, IMR_RECONVERTSTRING and
  // IMR_DOCUMENTFEED should return reconv, but some applications return
  // need_size.
  return reinterpret_cast<LRESULT>(reconv);
}

LRESULT InputMethodWinBase::OnReconvertString(RECONVERTSTRING* reconv) {
  ui::TextInputClient* client = GetTextInputClient();
  if (!client)
    return 0;

  // If there is a composition string already, we don't allow reconversion.
  if (client->HasCompositionText())
    return 0;

  gfx::Range text_range;
  if (!client->GetTextRange(&text_range) || text_range.is_empty())
    return 0;

  gfx::Range selection_range;
  if (!client->GetSelectionRange(&selection_range) ||
      selection_range.is_empty()) {
    return 0;
  }

  DCHECK(text_range.Contains(selection_range));

  size_t len = selection_range.length();
  size_t need_size = sizeof(RECONVERTSTRING) + len * sizeof(WCHAR);

  if (!reconv)
    return need_size;

  if (reconv->dwSize < need_size)
    return 0;

  // TODO(penghuang): Return some extra context to help improve IME's
  // reconversion accuracy.
  base::string16 text;
  if (!GetTextInputClient()->GetTextFromRange(selection_range, &text))
    return 0;
  DCHECK_EQ(selection_range.length(), text.length());

  reconv->dwVersion = 0;
  reconv->dwStrLen = len;
  reconv->dwStrOffset = sizeof(RECONVERTSTRING);
  reconv->dwCompStrLen = len;
  reconv->dwCompStrOffset = 0;
  reconv->dwTargetStrLen = len;
  reconv->dwTargetStrOffset = 0;

  memcpy(reinterpret_cast<char*>(reconv) + sizeof(RECONVERTSTRING),
         text.c_str(), len * sizeof(WCHAR));

  // According to Microsoft API document, IMR_RECONVERTSTRING and
  // IMR_DOCUMENTFEED should return reconv, but some applications return
  // need_size.
  return reinterpret_cast<LRESULT>(reconv);
}

LRESULT InputMethodWinBase::OnQueryCharPosition(IMECHARPOSITION* char_positon) {
  if (!char_positon)
    return 0;

  if (char_positon->dwSize < sizeof(IMECHARPOSITION))
    return 0;

  ui::TextInputClient* client = GetTextInputClient();
  if (!client)
    return 0;

  // Tentatively assume that the returned value from |client| is DIP (Density
  // Independent Pixel). See the comment in text_input_client.h and
  // http://crbug.com/360334.
  gfx::Rect dip_rect;
  if (client->HasCompositionText()) {
    if (!client->GetCompositionCharacterBounds(char_positon->dwCharPos,
                                               &dip_rect)) {
      return 0;
    }
  } else {
    // If there is no composition and the first character is queried, returns
    // the caret bounds. This behavior is the same to that of RichEdit control.
    if (char_positon->dwCharPos != 0)
      return 0;
    dip_rect = client->GetCaretBounds();
  }
  const gfx::Rect rect = display::win::ScreenWin::DIPToScreenRect(
      toplevel_window_handle_, dip_rect);

  char_positon->pt.x = rect.x();
  char_positon->pt.y = rect.y();
  char_positon->cLineHeight = rect.height();
  return 1;  // returns non-zero value when succeeded.
}

}  // namespace ui
