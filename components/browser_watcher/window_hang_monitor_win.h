// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef COMPONENTS_BROWSER_WATCHER_WINDOW_HANG_MONITOR_WIN_H_
#define COMPONENTS_BROWSER_WATCHER_WINDOW_HANG_MONITOR_WIN_H_

#include <windows.h>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/process/process.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "base/timer/timer.h"

namespace browser_watcher {

// Monitors a window for hanging by periodically sending it a WM_NULL message
// and timing the response.
class WindowHangMonitor {
 public:
  // Represents a detected window event.
  enum WindowEvent {
    // The target process exited before the window was detected.
    WINDOW_NOT_FOUND,
    // The window failed to respond to a ping within the configured timeout.
    WINDOW_HUNG,
    // The window disappeared.
    WINDOW_VANISHED,
  };

  // Called when a window event is detected. Called precisely zero or one
  // time(s).
  typedef base::Callback<void(WindowEvent)> WindowEventCallback;

  // Initialize the monitor with |callback|. The callback is guaranteed
  // to be called no more than once, and never after the WindowHangMonitor is
  // destroyed. The monitor will be configured to issue pings according to
  // |ping_interval|. A failure to respond within |timeout| will be interpreted
  // as a hang.
  WindowHangMonitor(base::TimeDelta ping_interval,
                    base::TimeDelta timeout,
                    const WindowEventCallback& callback);
  ~WindowHangMonitor();

  // Initializes the watcher to monitor the Chrome message window owned
  // by |process|. May be invoked prior to the appearance of the window.
  void Initialize(base::Process process);

 private:
  struct OutstandingPing {
    WindowHangMonitor* monitor;
  };

  // Schedules a delayed task to poll for the appearance of the target window.
  void ScheduleFindWindow();

  // Checks for the appearance of the target window. If found, initiates the
  // ping cycle. Otherwise, calls ScheduleFindWindow.
  void PollForWindow();

  static void CALLBACK
  OnPongReceived(HWND window, UINT msg, ULONG_PTR data, LRESULT lresult);

  // Sends a ping to |hwnd|. Issues a |WINDOW_VANISHED| callback if the
  // operation fails. Schedules OnHangTimeout otherwise.
  void SendPing(HWND hwnd);

  // Runs after a |hang_timeout_| delay after sending a ping to |hwnd|. Checks
  // whether a pong was received. Either issues a callback or schedules
  // OnRetryTimeout.
  void OnHangTimeout(HWND hwnd);

  // Runs periodically at |ping_interval_| interval, as long as the window is
  // still valid and not hung. Verifies that the target window still exists. If
  // so, invokes SendPing. Otherwise, issues a |WINDOW_VANISHED| callback.
  void OnRetryTimeout();

  // Invoked when a window event is detected.
  WindowEventCallback callback_;

  // The name of the (message) window to monitor.
  base::string16 window_name_;

  // The target process in which the monitored window is expected to exist.
  base::Process window_process_;

  // The time the last message was sent.
  base::Time last_ping_;

  // The ping interval, must be larger than |hang_timeout_|.
  base::TimeDelta ping_interval_;

  // The time after which |window_| is assumed hung.
  base::TimeDelta hang_timeout_;

  // The timer used to schedule polls.
  base::Timer timer_;

  // Non-null when there is an outstanding ping.
  // This is intentionally leaked when a hang is detected.
  OutstandingPing* outstanding_ping_;

  DISALLOW_COPY_AND_ASSIGN(WindowHangMonitor);
};

}  // namespace browser_watcher

#endif  // COMPONENTS_BROWSER_WATCHER_WINDOW_HANG_MONITOR_WIN_H_
