// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BROWSER_WATCHER_ENDSESSION_WATCHER_WINDOW_WIN_H_
#define COMPONENTS_BROWSER_WATCHER_ENDSESSION_WATCHER_WINDOW_WIN_H_

#include <windows.h>

#include "base/callback.h"
#include "base/macros.h"

namespace browser_watcher {

class EndSessionWatcherWindow {
 public:
  typedef base::Callback<void(UINT, LPARAM)> EndSessionMessageCallback;

  explicit EndSessionWatcherWindow(
      const EndSessionMessageCallback& on_end_session_message);
  virtual ~EndSessionWatcherWindow();

  // Exposed for testing.
  HWND window() const { return window_; }

 private:
  LRESULT OnEndSessionMessage(UINT message, WPARAM wparam, LPARAM lparam);

  LRESULT WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

  static LRESULT CALLBACK WndProcThunk(HWND hwnd,
                                       UINT message,
                                       WPARAM wparam,
                                       LPARAM lparam);

  HMODULE instance_;
  HWND window_;

  EndSessionMessageCallback on_end_session_message_;

  DISALLOW_COPY_AND_ASSIGN(EndSessionWatcherWindow);
};

}  // namespace browser_watcher

#endif  // COMPONENTS_BROWSER_WATCHER_ENDSESSION_WATCHER_WINDOW_WIN_H_
