// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/remoting/remote_desktop_browsertest.h"

namespace remoting {

IN_PROC_BROWSER_TEST_F(RemoteDesktopBrowserTest, MANUAL_Launch) {
  VerifyInternetAccess();

  Install();

  LaunchChromotingApp(false);

  Cleanup();
}

IN_PROC_BROWSER_TEST_F(RemoteDesktopBrowserTest, MANUAL_LaunchDeferredStart) {
  VerifyInternetAccess();

  Install();

  LaunchChromotingApp(true);
  StartChromotingApp();

  Cleanup();
}

}  // namespace remoting
