// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <windows.h>

#include <memory>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/current_module.h"
#include "base/win/scoped_gdi_object.h"
#include "base/win/scoped_hdc.h"
#include "base/win/scoped_select_object.h"
#include "remoting/host/client_session_control.h"
#include "remoting/host/host_window.h"
#include "remoting/host/win/core_resource.h"

namespace remoting {

namespace {

const int DISCONNECT_HOTKEY_ID = 1000;

// Maximum length of "Your desktop is shared with ..." message in UTF-16
// characters.
const size_t kMaxSharingWithTextLength = 100;

const wchar_t kShellTrayWindowName[] = L"Shell_TrayWnd";
const int kWindowBorderRadius = 14;

// Margin between dialog controls (in dialog units).
const int kWindowTextMargin = 8;

class DisconnectWindowWin : public HostWindow {
 public:
  DisconnectWindowWin();
  ~DisconnectWindowWin() override;

  // HostWindow overrides.
  void Start(
      const base::WeakPtr<ClientSessionControl>& client_session_control)
      override;

 protected:
  static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT message, WPARAM wparam,
                                     LPARAM lparam);

  BOOL OnDialogMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

  // Creates the dialog window and registers the disconnect hot key.
  bool BeginDialog();

  // Closes the dialog, unregisters the hot key and invokes the disconnect
  // callback, if set.
  void EndDialog();

  // Returns |control| rectangle in the dialog coordinates.
  bool GetControlRect(HWND control, RECT* rect);

  // Trys to position the dialog window above the taskbar.
  void SetDialogPosition();

  // Applies localization string and resizes the dialog.
  bool SetStrings();

  // Used to disconnect the client session.
  base::WeakPtr<ClientSessionControl> client_session_control_;

  // Specifies the remote user name.
  std::string username_;

  HWND hwnd_;
  bool has_hotkey_;
  base::win::ScopedGDIObject<HPEN> border_pen_;

  DISALLOW_COPY_AND_ASSIGN(DisconnectWindowWin);
};

// Returns the text for the given dialog control window.
bool GetControlText(HWND control, base::string16* text) {
  // GetWindowText truncates the text if it is longer than can fit into
  // the buffer.
  WCHAR buffer[256];
  int result = GetWindowText(control, buffer, arraysize(buffer));
  if (!result)
    return false;

  text->assign(buffer);
  return true;
}

// Returns width |text| rendered in |control| window.
bool GetControlTextWidth(HWND control,
                         const base::string16& text,
                         LONG* width) {
  RECT rect = {0, 0, 0, 0};
  base::win::ScopedGetDC dc(control);
  base::win::ScopedSelectObject font(
      dc, (HFONT)SendMessage(control, WM_GETFONT, 0, 0));
  if (!DrawText(dc, text.c_str(), -1, &rect, DT_CALCRECT | DT_SINGLELINE))
    return false;

  *width = rect.right;
  return true;
}

DisconnectWindowWin::DisconnectWindowWin()
    : hwnd_(nullptr),
      has_hotkey_(false),
      border_pen_(CreatePen(PS_SOLID, 5,
                            RGB(0.13 * 255, 0.69 * 255, 0.11 * 255))) {
}

DisconnectWindowWin::~DisconnectWindowWin() {
  EndDialog();
}

void DisconnectWindowWin::Start(
    const base::WeakPtr<ClientSessionControl>& client_session_control) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!client_session_control_);
  DCHECK(client_session_control);

  client_session_control_ = client_session_control;

  std::string client_jid = client_session_control_->client_jid();
  username_ = client_jid.substr(0, client_jid.find('/'));
  if (!BeginDialog())
    EndDialog();
}

INT_PTR CALLBACK DisconnectWindowWin::DialogProc(HWND hwnd,
                                                 UINT message,
                                                 WPARAM wparam,
                                                 LPARAM lparam) {
  LONG_PTR self = 0;
  if (message == WM_INITDIALOG) {
    self = lparam;

    // Store |this| to the window's user data.
    SetLastError(ERROR_SUCCESS);
    LONG_PTR result = SetWindowLongPtr(hwnd, DWLP_USER, self);
    if (result == 0 && GetLastError() != ERROR_SUCCESS)
      reinterpret_cast<DisconnectWindowWin*>(self)->EndDialog();
  } else {
    self = GetWindowLongPtr(hwnd, DWLP_USER);
  }

  if (self) {
    return reinterpret_cast<DisconnectWindowWin*>(self)->OnDialogMessage(
        hwnd, message, wparam, lparam);
  }
  return FALSE;
}

BOOL DisconnectWindowWin::OnDialogMessage(HWND hwnd,
                                          UINT message,
                                          WPARAM wparam,
                                          LPARAM lparam) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  switch (message) {
    // Ignore close messages.
    case WM_CLOSE:
      return TRUE;

    // Handle the Disconnect button.
    case WM_COMMAND:
      switch (LOWORD(wparam)) {
        case IDC_DISCONNECT:
          EndDialog();
          return TRUE;
      }
      return FALSE;

    // Ensure we don't try to use the HWND anymore.
    case WM_DESTROY:
      hwnd_ = nullptr;

      // Ensure that the disconnect callback is invoked even if somehow our
      // window gets destroyed.
      EndDialog();

      return TRUE;

    // Ensure the dialog stays visible if the work area dimensions change.
    case WM_SETTINGCHANGE:
      if (wparam == SPI_SETWORKAREA)
        SetDialogPosition();
      return TRUE;

    // Ensure the dialog stays visible if the display dimensions change.
    case WM_DISPLAYCHANGE:
      SetDialogPosition();
      return TRUE;

    // Handle the disconnect hot-key.
    case WM_HOTKEY:
      EndDialog();
      return TRUE;

    // Let the window be draggable by its client area by responding
    // that the entire window is the title bar.
    case WM_NCHITTEST:
      SetWindowLongPtr(hwnd, DWLP_MSGRESULT, HTCAPTION);
      return TRUE;

    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd_, &ps);
      RECT rect;
      GetClientRect(hwnd_, &rect);
      {
        base::win::ScopedSelectObject border(hdc, border_pen_.get());
        base::win::ScopedSelectObject brush(hdc, GetStockObject(NULL_BRUSH));
        RoundRect(hdc, rect.left, rect.top, rect.right - 1, rect.bottom - 1,
                  kWindowBorderRadius, kWindowBorderRadius);
      }
      EndPaint(hwnd_, &ps);
      return TRUE;
    }
  }
  return FALSE;
}

bool DisconnectWindowWin::BeginDialog() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!hwnd_);

  hwnd_ =
      CreateDialogParam(CURRENT_MODULE(), MAKEINTRESOURCE(IDD_DISCONNECT),
                        nullptr, DialogProc, reinterpret_cast<LPARAM>(this));
  if (!hwnd_)
    return false;

  // Set up handler for Ctrl-Alt-Esc shortcut.
  if (!has_hotkey_ && RegisterHotKey(hwnd_, DISCONNECT_HOTKEY_ID,
                                     MOD_ALT | MOD_CONTROL, VK_ESCAPE)) {
    has_hotkey_ = true;
  }

  if (!SetStrings())
    return false;

  SetDialogPosition();
  ShowWindow(hwnd_, SW_SHOW);
  return IsWindowVisible(hwnd_) != FALSE;
}

void DisconnectWindowWin::EndDialog() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (has_hotkey_) {
    UnregisterHotKey(hwnd_, DISCONNECT_HOTKEY_ID);
    has_hotkey_ = false;
  }

  if (hwnd_) {
    DestroyWindow(hwnd_);
    hwnd_ = nullptr;
  }

  if (client_session_control_)
    client_session_control_->DisconnectSession(protocol::OK);
}

// Returns |control| rectangle in the dialog coordinates.
bool DisconnectWindowWin::GetControlRect(HWND control, RECT* rect) {
  if (!GetWindowRect(control, rect))
    return false;
  SetLastError(ERROR_SUCCESS);
  int result = MapWindowPoints(HWND_DESKTOP, hwnd_,
                               reinterpret_cast<LPPOINT>(rect), 2);
  if (!result && GetLastError() != ERROR_SUCCESS)
    return false;

  return true;
}

void DisconnectWindowWin::SetDialogPosition() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Try to center the window above the task-bar. If that fails, use the
  // primary monitor. If that fails (very unlikely), use the default position.
  HWND taskbar = FindWindow(kShellTrayWindowName, nullptr);
  HMONITOR monitor = MonitorFromWindow(taskbar, MONITOR_DEFAULTTOPRIMARY);
  MONITORINFO monitor_info = {sizeof(monitor_info)};
  RECT window_rect;
  if (GetMonitorInfo(monitor, &monitor_info) &&
      GetWindowRect(hwnd_, &window_rect)) {
    int window_width = window_rect.right - window_rect.left;
    int window_height = window_rect.bottom - window_rect.top;
    int top = monitor_info.rcWork.bottom - window_height;
    int left = (monitor_info.rcWork.right + monitor_info.rcWork.left -
        window_width) / 2;
    SetWindowPos(hwnd_, nullptr, left, top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
  }
}

bool DisconnectWindowWin::SetStrings() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Localize the disconnect button text and measure length of the old and new
  // labels.
  HWND hwnd_button = GetDlgItem(hwnd_, IDC_DISCONNECT);
  HWND hwnd_message = GetDlgItem(hwnd_, IDC_DISCONNECT_SHARINGWITH);
  if (!hwnd_button || !hwnd_message)
    return false;

  base::string16 button_text;
  base::string16 message_text;
  if (!GetControlText(hwnd_button, &button_text) ||
      !GetControlText(hwnd_message, &message_text)) {
    return false;
  }

  // Format and truncate "Your desktop is shared with ..." message.
  message_text = base::ReplaceStringPlaceholders(message_text,
                                                 base::UTF8ToUTF16(username_),
                                                 nullptr);
  if (message_text.length() > kMaxSharingWithTextLength)
    message_text.erase(kMaxSharingWithTextLength);

  if (!SetWindowText(hwnd_message, message_text.c_str()))
    return false;

  // Calculate the margin between controls in pixels.
  RECT rect = {0};
  rect.right = kWindowTextMargin;
  if (!MapDialogRect(hwnd_, &rect))
    return false;
  int margin = rect.right;

  // Resize |hwnd_message| so that the text is not clipped.
  RECT message_rect;
  if (!GetControlRect(hwnd_message, &message_rect))
    return false;

  LONG control_width;
  if (!GetControlTextWidth(hwnd_message, message_text, &control_width))
    return false;
  message_rect.right = message_rect.left + control_width + margin;

  if (!SetWindowPos(hwnd_message, nullptr,
                    message_rect.left, message_rect.top,
                    message_rect.right - message_rect.left,
                    message_rect.bottom - message_rect.top,
                    SWP_NOZORDER)) {
    return false;
  }

  // Reposition and resize |hwnd_button| as well.
  RECT button_rect;
  if (!GetControlRect(hwnd_button, &button_rect))
    return false;

  if (!GetControlTextWidth(hwnd_button, button_text, &control_width))
    return false;

  button_rect.left = message_rect.right;
  button_rect.right = button_rect.left + control_width + margin * 2;
  if (!SetWindowPos(hwnd_button, nullptr,
                    button_rect.left, button_rect.top,
                    button_rect.right - button_rect.left,
                    button_rect.bottom - button_rect.top,
                    SWP_NOZORDER)) {
    return false;
  }

  // Resize the whole window to fit the resized controls.
  RECT window_rect;
  if (!GetWindowRect(hwnd_, &window_rect))
    return false;
  int width = button_rect.right + margin;
  int height = window_rect.bottom - window_rect.top;
  if (!SetWindowPos(hwnd_, nullptr, 0, 0, width, height,
                    SWP_NOMOVE | SWP_NOZORDER)) {
    return false;
  }

  // Make the corners of the disconnect window rounded.
  HRGN rgn = CreateRoundRectRgn(0, 0, width, height, kWindowBorderRadius,
                                kWindowBorderRadius);
  if (!rgn)
    return false;
  if (!SetWindowRgn(hwnd_, rgn, TRUE))
    return false;

  return true;
}

} // namespace

// static
std::unique_ptr<HostWindow> HostWindow::CreateDisconnectWindow() {
  return std::make_unique<DisconnectWindowWin>();
}

}  // namespace remoting
