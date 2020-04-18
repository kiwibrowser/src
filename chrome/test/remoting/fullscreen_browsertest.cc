// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "chrome/test/remoting/remote_desktop_browsertest.h"
#include "chrome/test/remoting/waiter.h"

namespace remoting {

class FullscreenBrowserTest : public RemoteDesktopBrowserTest {
 protected:
  bool WaitForFullscreenChange(bool expect_fullscreen);
};

bool FullscreenBrowserTest::WaitForFullscreenChange(bool expect_fullscreen) {
  std::string javascript = expect_fullscreen ?
      "remoting.fullscreen.isActive()" :
      "!remoting.fullscreen.isActive()";
  ConditionalTimeoutWaiter waiter(
      base::TimeDelta::FromSeconds(20),
      base::TimeDelta::FromSeconds(1),
      base::Bind(&RemoteDesktopBrowserTest::IsHostActionComplete,
                 active_web_contents(),
                 javascript));
  bool result = waiter.Wait();
  // Entering or leaving full-screen mode causes local and remote desktop
  // reconfigurations that can take a while to settle down, so wait a few
  // seconds before continuing.
  TimeoutWaiter(base::TimeDelta::FromSeconds(10)).Wait();
  return result;
}

IN_PROC_BROWSER_TEST_F(FullscreenBrowserTest, MANUAL_Me2Me_Fullscreen) {
  SetUpTest();
  ConnectToLocalHost(false);

  // Verify that we're initially not full-screen.
  EXPECT_FALSE(ExecuteScriptAndExtractBool(
      "remoting.fullscreen.isActive()"));

  // Click the full-screen button and verify that it activates full-screen mode.
  ClickOnControl("toggle-full-screen");
  EXPECT_TRUE(WaitForFullscreenChange(true));

  // Click the full-screen button again and verify that it deactivates
  // full-screen mode.
  ClickOnControl("toggle-full-screen");
  EXPECT_TRUE(WaitForFullscreenChange(false));

  // Enter full-screen mode again, then disconnect and verify that full-screen
  // mode is deactivated upon disconnection.
  // TODO(jamiewalch): For the v2 app, activate full-screen mode indirectly by
  // maximizing the window for the second test.
  ClickOnControl("toggle-full-screen");
  EXPECT_TRUE(WaitForFullscreenChange(true));
  DisconnectMe2Me();
  EXPECT_TRUE(WaitForFullscreenChange(false));

  Cleanup();
}

IN_PROC_BROWSER_TEST_F(FullscreenBrowserTest, MANUAL_Me2Me_Bump_Scroll) {
  content::WebContents* content = SetUpTest();
  LoadScript(content, FILE_PATH_LITERAL("bump_scroll_browser_test.js"));

  RunJavaScriptTest(content, "Bump_Scroll", "{pin: '" + me2me_pin() + "'}");

  Cleanup();
}

}  // namespace remoting
