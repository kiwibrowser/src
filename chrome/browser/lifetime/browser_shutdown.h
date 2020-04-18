// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_LIFETIME_BROWSER_SHUTDOWN_H_
#define CHROME_BROWSER_LIFETIME_BROWSER_SHUTDOWN_H_

#include "build/build_config.h"

class PrefRegistrySimple;

namespace browser_shutdown {

// Shutdown flags
enum Flags {
  NO_FLAGS = 0,

  // If |RESTART_LAST_SESSION| is set, the browser will attempt to restart in
  // in the last session after the shutdown.
  RESTART_LAST_SESSION = 1 << 0,

  // Makes a panned restart happen in the background. The browser will just come
  // up in the system tray but not open a new window after restarting. This flag
  // has no effect if |RESTART_LAST_SESSION| is not set.
  RESTART_IN_BACKGROUND = 1 << 1
};

enum ShutdownType {
  // an uninitialized value
  NOT_VALID = 0,
  // the last browser window was closed
  WINDOW_CLOSE,
  // user clicked on the Exit menu item
  BROWSER_EXIT,
  // windows is logging off or shutting down
  END_SESSION
};

constexpr int kNumShutdownTypes = END_SESSION + 1;

void RegisterPrefs(PrefRegistrySimple* registry);

// Called when the browser starts shutting down so that we can measure shutdown
// time.
void OnShutdownStarting(ShutdownType type);

// Get the current shutdown type.
ShutdownType GetShutdownType();

#if !defined(OS_ANDROID)
// Performs the shutdown tasks that need to be done before
// BrowserProcess and the various threads go away.
//
// Returns true if the session should be restarted.
bool ShutdownPreThreadsStop();

// Records the shutdown related prefs, and returns true if the browser should be
// restarted on exit.
bool RecordShutdownInfoPrefs();

// Performs the remaining shutdown tasks after all threads but the
// main thread have been stopped.  This includes deleting g_browser_process.
//
// See |browser_shutdown::Flags| for the possible flag values and their effects.
void ShutdownPostThreadsStop(int shutdown_flags);
#endif

// Called at startup to create a histogram from our previous shutdown time.
void ReadLastShutdownInfo();

// There are various situations where the browser process should continue to
// run after the last browser window has closed - the Mac always continues
// running until the user explicitly quits, and on Windows/Linux the application
// should not shutdown when the last browser window closes if there are any
// BackgroundContents running.
// When the user explicitly chooses to shutdown the app (via the "Exit" or
// "Quit" menu items) BrowserList will call SetTryingToQuit() to tell itself to
// initiate a shutdown when the last window closes.
// If the quit is aborted, then the flag should be reset.

// This is a low-level mutator; in general, don't call SetTryingToQuit(true),
// except from appropriate places in BrowserList. To quit, use usual means,
// e.g., using |chrome_browser_application_mac::Terminate()| on the Mac, or
// |BrowserList::CloseAllWindowsAndExit()| on other platforms. To stop quitting,
// use |chrome_browser_application_mac::CancelTerminate()| on the Mac; other
// platforms can call SetTryingToQuit(false) directly.
void SetTryingToQuit(bool quitting);

// General accessor.
bool IsTryingToQuit();

// Starts to collect shutdown traces. On ChromeOS this will start immediately
// on AttemptUserExit() and all other systems will start once all tabs are
// closed.
void StartShutdownTracing();

}  // namespace browser_shutdown

#endif  // CHROME_BROWSER_LIFETIME_BROWSER_SHUTDOWN_H_
