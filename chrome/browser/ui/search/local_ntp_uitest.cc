// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/search/instant_test_utils.h"
#include "chrome/browser/ui/search/instant_uitest_base.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/omnibox/browser/omnibox_edit_model.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "components/omnibox/common/omnibox_focus_state.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/vector2d.h"

// A test class that sets up local_ntp_browsertest.html (which is mostly a copy
// of the real local_ntp.html) as the NTP URL.
class LocalNTPUITest : public InProcessBrowserTest, public InstantUITestBase {
 public:
  LocalNTPUITest() {}

 protected:
  void SetUpInProcessBrowserTestFixture() override {
    ASSERT_TRUE(https_test_server().Start());
    GURL base_url = https_test_server().GetURL("/instant_extended.html?");
    GURL ntp_url =
        https_test_server().GetURL("/local_ntp/local_ntp_browsertest.html?");
    InstantTestBase::Init(base_url, ntp_url, false);
  }
};

IN_PROC_BROWSER_TEST_F(LocalNTPUITest, FakeboxRedirectsToOmnibox) {
  ASSERT_NO_FATAL_FAILURE(SetupInstant(browser()));
  FocusOmnibox();

  ui_test_utils::NavigateToURLWithDisposition(
      browser(), ntp_url(), WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_TAB |
          ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  content::WebContents* active_tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_TRUE(search::IsInstantNTP(active_tab));

  // This is required to make the mouse events we send below arrive at the right
  // place. It *should* be the default for all interactive_ui_tests anyway, but
  // on Mac it isn't; see crbug.com/641969.
  ASSERT_TRUE(ui_test_utils::BringBrowserWindowToFront(browser()));

  // Make sure that the omnibox doesn't have focus already.
  ui_test_utils::ClickOnView(browser(), VIEW_ID_TAB_CONTAINER);
  ASSERT_EQ(OMNIBOX_FOCUS_NONE, omnibox()->model()->focus_state());

  bool result = false;
  ASSERT_TRUE(instant_test_utils::GetBoolFromJS(
      active_tab, "!!setupAdvancedTest()", &result));
  ASSERT_TRUE(result);

  // Get the position of the fakebox on the page.
  double fakebox_x = 0;
  ASSERT_TRUE(instant_test_utils::GetDoubleFromJS(
      active_tab, "getFakeboxPositionX()", &fakebox_x));
  double fakebox_y = 0;
  ASSERT_TRUE(instant_test_utils::GetDoubleFromJS(
      active_tab, "getFakeboxPositionY()", &fakebox_y));

  // Move the mouse over the fakebox.
  gfx::Vector2d fakebox_pos(static_cast<int>(std::ceil(fakebox_x)),
                            static_cast<int>(std::ceil(fakebox_y)));
  gfx::Point origin = active_tab->GetContainerBounds().origin();
  ASSERT_TRUE(ui_test_utils::SendMouseMoveSync(origin + fakebox_pos +
                                               gfx::Vector2d(1, 1)));

  // Click on the fakebox, and wait for the omnibox to receive invisible focus.
  content::WindowedNotificationObserver focus_observer(
      chrome::NOTIFICATION_OMNIBOX_FOCUS_CHANGED,
      content::NotificationService::AllSources());
  ASSERT_TRUE(
      ui_test_utils::SendMouseEventsSync(ui_controls::LEFT, ui_controls::DOWN));
  ASSERT_TRUE(
      ui_test_utils::SendMouseEventsSync(ui_controls::LEFT, ui_controls::UP));
  focus_observer.Wait();
  EXPECT_EQ(OMNIBOX_FOCUS_INVISIBLE, omnibox()->model()->focus_state());

  // The fakebox should now also have focus.
  ASSERT_TRUE(instant_test_utils::GetBoolFromJS(
      active_tab, "!!fakeboxIsFocused()", &result));
  EXPECT_TRUE(result);

  // Type "a" and wait for the omnibox to receive visible focus.
  content::WindowedNotificationObserver focus_observer2(
      chrome::NOTIFICATION_OMNIBOX_FOCUS_CHANGED,
      content::NotificationService::AllSources());
  ASSERT_TRUE(ui_test_utils::SendKeyPressSync(
      browser(), ui::KeyboardCode::VKEY_A,
      /*control=*/false, /*shift=*/false, /*alt=*/false, /*command=*/false));
  focus_observer2.Wait();
  EXPECT_EQ(OMNIBOX_FOCUS_VISIBLE, omnibox()->model()->focus_state());

  // The typed text should have arrived in the omnibox.
  EXPECT_EQ("a", GetOmniboxText());

  // On the JS side, the fakebox should have been hidden.
  ASSERT_TRUE(instant_test_utils::GetBoolFromJS(
      active_tab, "!!fakeboxIsVisible()", &result));
  EXPECT_FALSE(result);
}
