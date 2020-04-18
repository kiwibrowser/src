// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/apps/chrome_native_app_window_views_aura_ash.h"

#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/apps/app_window_interactive_uitest.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chromeos/login/scoped_test_public_session_login_state.h"
#include "extensions/test/extension_test_message_listener.h"

class ChromeNativeAppWindowViewsAuraAshInteractiveTest
    : public AppWindowInteractiveTest {
 public:
  ChromeNativeAppWindowViewsAuraAshInteractiveTest() = default;

  ChromeNativeAppWindowViewsAuraAsh* GetWindowAsh() {
    return static_cast<ChromeNativeAppWindowViewsAuraAsh*>(
        GetFirstAppWindow()->GetBaseWindow());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeNativeAppWindowViewsAuraAshInteractiveTest);
};

IN_PROC_BROWSER_TEST_F(ChromeNativeAppWindowViewsAuraAshInteractiveTest,
                       NoImmersiveOrBubbleOutsidePublicSessionWindow) {
  ExtensionTestMessageListener launched_listener("Launched", true);
  LoadAndLaunchPlatformApp("leave_fullscreen", &launched_listener);

  // We start by making sure the window is actually focused.
  ASSERT_TRUE(ui_test_utils::ShowAndFocusNativeWindow(
      GetFirstAppWindow()->GetNativeWindow()));

  // When receiving the reply, the application will try to go fullscreen using
  // the Window API but there is no synchronous way to know if that actually
  // succeeded. Also, failure will not be notified. A failure case will only be
  // known with a timeout.
  {
    FullscreenChangeWaiter fs_changed(GetFirstAppWindow()->GetBaseWindow());

    launched_listener.Reply("window");

    fs_changed.Wait();
  }

  EXPECT_FALSE(GetWindowAsh()->IsImmersiveModeEnabled());
  EXPECT_FALSE(GetWindowAsh()->exclusive_access_bubble_);
}

IN_PROC_BROWSER_TEST_F(ChromeNativeAppWindowViewsAuraAshInteractiveTest,
                       NoImmersiveOrBubbleOutsidePublicSessionDom) {
  ExtensionTestMessageListener launched_listener("Launched", true);
  LoadAndLaunchPlatformApp("leave_fullscreen", &launched_listener);

  // We start by making sure the window is actually focused.
  ASSERT_TRUE(ui_test_utils::ShowAndFocusNativeWindow(
      GetFirstAppWindow()->GetNativeWindow()));

  launched_listener.Reply("dom");

  // Because the DOM way to go fullscreen requires user gesture, we simulate a
  // key event to get the window entering in fullscreen mode. The reply will
  // make the window listen for the key event. The reply will be sent to the
  // renderer process before the keypress and should be received in that order.
  // When receiving the key event, the application will try to go fullscreen
  // using the Window API but there is no synchronous way to know if that
  // actually succeeded. Also, failure will not be notified. A failure case will
  // only be known with a timeout.
  {
    FullscreenChangeWaiter fs_changed(GetFirstAppWindow()->GetBaseWindow());

    WaitUntilKeyFocus();
    ASSERT_TRUE(SimulateKeyPress(ui::VKEY_A));

    fs_changed.Wait();
  }

  EXPECT_FALSE(GetWindowAsh()->IsImmersiveModeEnabled());
  EXPECT_FALSE(GetWindowAsh()->exclusive_access_bubble_);
}

IN_PROC_BROWSER_TEST_F(ChromeNativeAppWindowViewsAuraAshInteractiveTest,
                       ImmersiveAndBubbleInsidePublicSessionWindow) {
  chromeos::ScopedTestPublicSessionLoginState state;
  ExtensionTestMessageListener launched_listener("Launched", true);
  LoadAndLaunchPlatformApp("leave_fullscreen", &launched_listener);

  // We start by making sure the window is actually focused.
  ASSERT_TRUE(ui_test_utils::ShowAndFocusNativeWindow(
      GetFirstAppWindow()->GetNativeWindow()));

  // When receiving the reply, the application will try to go fullscreen using
  // the Window API but there is no synchronous way to know if that actually
  // succeeded. Also, failure will not be notified. A failure case will only be
  // known with a timeout.
  {
    FullscreenChangeWaiter fs_changed(GetFirstAppWindow()->GetBaseWindow());

    launched_listener.Reply("window");

    fs_changed.Wait();
  }

  EXPECT_TRUE(GetWindowAsh()->IsImmersiveModeEnabled());
  EXPECT_TRUE(GetWindowAsh()->exclusive_access_bubble_);
}

IN_PROC_BROWSER_TEST_F(ChromeNativeAppWindowViewsAuraAshInteractiveTest,
                       ImmersiveAndBubbleInsidePublicSessionDom) {
  chromeos::ScopedTestPublicSessionLoginState state;
  ExtensionTestMessageListener launched_listener("Launched", true);
  LoadAndLaunchPlatformApp("leave_fullscreen", &launched_listener);

  // We start by making sure the window is actually focused.
  ASSERT_TRUE(ui_test_utils::ShowAndFocusNativeWindow(
      GetFirstAppWindow()->GetNativeWindow()));

  launched_listener.Reply("dom");

  // Because the DOM way to go fullscreen requires user gesture, we simulate a
  // key event to get the window entering in fullscreen mode. The reply will
  // make the window listen for the key event. The reply will be sent to the
  // renderer process before the keypress and should be received in that order.
  // When receiving the key event, the application will try to go fullscreen
  // using the Window API but there is no synchronous way to know if that
  // actually succeeded. Also, failure will not be notified. A failure case will
  // only be known with a timeout.
  {
    FullscreenChangeWaiter fs_changed(GetFirstAppWindow()->GetBaseWindow());

    WaitUntilKeyFocus();
    ASSERT_TRUE(SimulateKeyPress(ui::VKEY_A));

    fs_changed.Wait();
  }

  EXPECT_TRUE(GetWindowAsh()->IsImmersiveModeEnabled());
  EXPECT_TRUE(GetWindowAsh()->exclusive_access_bubble_);
}
