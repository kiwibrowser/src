// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browser_watcher/endsession_watcher_window_win.h"

#include "base/logging.h"
#include "base/win/wrapped_window_proc.h"

namespace browser_watcher {

namespace {

const wchar_t kWindowClassName[] = L"Chrome_BrowserWatcherWindow";

}  // namespace

EndSessionWatcherWindow::EndSessionWatcherWindow(
    const EndSessionMessageCallback& on_end_session_message) :
        on_end_session_message_(on_end_session_message) {
  WNDCLASSEX window_class = {0};
  base::win::InitializeWindowClass(
      kWindowClassName,
      &base::win::WrappedWindowProc<EndSessionWatcherWindow::WndProcThunk>, 0,
      0, 0, nullptr, nullptr, nullptr, nullptr, nullptr, &window_class);
  instance_ = window_class.hInstance;
  ATOM clazz = ::RegisterClassEx(&window_class);
  DCHECK(clazz);

  // TODO(siggi): will a message window do here?
  window_ = ::CreateWindow(kWindowClassName, nullptr, 0, 0, 0, 0, 0, nullptr,
                           nullptr, instance_, nullptr);
  ::SetWindowLongPtr(window_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
  DCHECK_EQ(::GetWindowLongPtr(window_, GWLP_USERDATA),
            reinterpret_cast<LONG_PTR>(this));
}

EndSessionWatcherWindow::~EndSessionWatcherWindow() {
  if (window_) {
    ::DestroyWindow(window_);
    ::UnregisterClass(kWindowClassName, instance_);
  }
}

LRESULT EndSessionWatcherWindow::OnEndSessionMessage(UINT message,
                                                     WPARAM wparam,
                                                     LPARAM lparam) {
  on_end_session_message_.Run(message, lparam);
  return 0;
}

LRESULT CALLBACK EndSessionWatcherWindow::WndProcThunk(HWND hwnd,
                                                       UINT message,
                                                       WPARAM wparam,
                                                       LPARAM lparam) {
  EndSessionWatcherWindow* msg_wnd =
      reinterpret_cast<EndSessionWatcherWindow*>(
          ::GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (msg_wnd)
    return msg_wnd->WndProc(hwnd, message, wparam, lparam);

  return ::DefWindowProc(hwnd, message, wparam, lparam);
}

LRESULT EndSessionWatcherWindow::WndProc(HWND hwnd,
                                         UINT message,
                                         WPARAM wparam,
                                         LPARAM lparam) {
  switch (message) {
    case WM_ENDSESSION:
    case WM_QUERYENDSESSION:
      return OnEndSessionMessage(message, wparam, lparam);
    default:
      break;
  }

  return ::DefWindowProc(hwnd, message, wparam, lparam);
}

}  // namespace browser_watcher
