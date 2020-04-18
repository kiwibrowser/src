// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WIN_WINDOWS_SESSION_CHANGE_OBSERVER_H_
#define UI_VIEWS_WIN_WINDOWS_SESSION_CHANGE_OBSERVER_H_

#include <windows.h>

#include "base/callback.h"
#include "base/macros.h"

namespace views {

// Calls the provided callback on WM_WTSSESSION_CHANGE messages along with
// managing the tricky business of observing a singleton object.
class WindowsSessionChangeObserver {
 public:
  typedef base::Callback<void(WPARAM)> WtsCallback;
  explicit WindowsSessionChangeObserver(const WtsCallback& callback);
  ~WindowsSessionChangeObserver();

 private:
  class WtsRegistrationNotificationManager;

  void OnSessionChange(WPARAM wparam);
  void ClearCallback();

  WtsCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(WindowsSessionChangeObserver);
};

}  // namespace views

#endif  // UI_VIEWS_WIN_WINDOWS_SESSION_CHANGE_OBSERVER_H_
