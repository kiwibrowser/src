// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/zoom_bubble_view.h"

#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller_test.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/views/scoped_macviews_browser_mode.h"
#include "ui/base/ui_features.h"
#include "ui/views/test/test_widget_observer.h"

#if !defined(OS_MACOSX) || BUILDFLAG(MAC_VIEWS_BROWSER)
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/immersive_mode_controller.h"
#endif

#if defined(OS_CHROMEOS)
#include "ash/public/cpp/immersive/immersive_fullscreen_controller_test_api.h"
#include "chrome/browser/ui/views/frame/immersive_mode_controller_ash.h"
#include "ui/aura/test/env_test_helper.h"
#endif

using ZoomBubbleBrowserTest = InProcessBrowserTest;

namespace {

void ShowInActiveTab(Browser* browser) {
  content::WebContents* web_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  ZoomBubbleView::ShowBubble(web_contents, gfx::Point(),
                             ZoomBubbleView::USER_GESTURE);
  EXPECT_TRUE(ZoomBubbleView::GetZoomBubble());
}

}  // namespace

class ZoomBubbleViewsBrowserTest : public ZoomBubbleBrowserTest {
 private:
  test::ScopedMacViewsBrowserMode views_mode_{true};
};

// TODO(linux_aura) http://crbug.com/163931
#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(USE_AURA)
#define MAYBE_NonImmersiveFullscreen DISABLED_NonImmersiveFullscreen
#else
#define MAYBE_NonImmersiveFullscreen NonImmersiveFullscreen
#endif
#if !defined(OS_MACOSX) || BUILDFLAG(MAC_VIEWS_BROWSER)
// Test whether the zoom bubble is anchored and whether it is visible when in
// non-immersive fullscreen.
IN_PROC_BROWSER_TEST_F(ZoomBubbleViewsBrowserTest,
                       MAYBE_NonImmersiveFullscreen) {
  BrowserView* browser_view = static_cast<BrowserView*>(browser()->window());
  content::WebContents* web_contents = browser_view->GetActiveWebContents();

  // The zoom bubble should be anchored when not in fullscreen.
  ZoomBubbleView::ShowBubble(web_contents, gfx::Point(),
                             ZoomBubbleView::AUTOMATIC);
  ASSERT_TRUE(ZoomBubbleView::GetZoomBubble());
  const ZoomBubbleView* zoom_bubble = ZoomBubbleView::GetZoomBubble();
  EXPECT_TRUE(zoom_bubble->GetAnchorView());

  // Entering fullscreen should close the bubble. (We enter into tab fullscreen
  // here because tab fullscreen is non-immersive even on Chrome OS.)
  {
    // NOTIFICATION_FULLSCREEN_CHANGED is sent asynchronously. Wait for the
    // notification before testing the zoom bubble visibility.
    std::unique_ptr<FullscreenNotificationObserver> waiter(
        new FullscreenNotificationObserver());
    browser()
        ->exclusive_access_manager()
        ->fullscreen_controller()
        ->EnterFullscreenModeForTab(web_contents, GURL());
    waiter->Wait();
  }
  ASSERT_FALSE(browser_view->immersive_mode_controller()->IsEnabled());
  EXPECT_FALSE(ZoomBubbleView::GetZoomBubble());

  // The bubble should not be anchored when it is shown in non-immersive
  // fullscreen.
  ZoomBubbleView::ShowBubble(web_contents, gfx::Point(),
                             ZoomBubbleView::AUTOMATIC);
  ASSERT_TRUE(ZoomBubbleView::GetZoomBubble());
  zoom_bubble = ZoomBubbleView::GetZoomBubble();
  EXPECT_FALSE(zoom_bubble->GetAnchorView());

  // Exit fullscreen before ending the test for the sake of sanity.
  {
    std::unique_ptr<FullscreenNotificationObserver> waiter(
        new FullscreenNotificationObserver());
    chrome::ToggleFullscreenMode(browser());
    waiter->Wait();
  }
}
#endif  // !defined(OS_MACOSX) || BUILDFLAG(MAC_VIEWS_BROWSER)

#if defined(OS_CHROMEOS)
// Test whether the zoom bubble is anchored and whether it is visible when in
// immersive fullscreen.
IN_PROC_BROWSER_TEST_F(ZoomBubbleBrowserTest, ImmersiveFullscreen) {
  aura::test::EnvTestHelper().SetAlwaysUseLastMouseLocation(true);
  BrowserView* browser_view = static_cast<BrowserView*>(browser()->window());
  content::WebContents* web_contents = browser_view->GetActiveWebContents();

  ImmersiveModeController* immersive_controller =
      browser_view->immersive_mode_controller();
  ASSERT_EQ(ImmersiveModeController::Type::ASH, immersive_controller->type());
  ash::ImmersiveFullscreenControllerTestApi(
      static_cast<ImmersiveModeControllerAsh*>(immersive_controller)
          ->controller())
      .SetupForTest();

  // Enter immersive fullscreen.
  {
    std::unique_ptr<FullscreenNotificationObserver> waiter(
        new FullscreenNotificationObserver());
    chrome::ToggleFullscreenMode(browser());
    waiter->Wait();
  }
  ASSERT_TRUE(immersive_controller->IsEnabled());
  ASSERT_FALSE(immersive_controller->IsRevealed());

  // The zoom bubble should not be anchored when it is shown in immersive
  // fullscreen and the top-of-window views are not revealed.
  ZoomBubbleView::ShowBubble(web_contents, gfx::Point(),
                             ZoomBubbleView::AUTOMATIC);
  ASSERT_TRUE(ZoomBubbleView::GetZoomBubble());
  const ZoomBubbleView* zoom_bubble = ZoomBubbleView::GetZoomBubble();
  EXPECT_FALSE(zoom_bubble->GetAnchorView());

  // An immersive reveal should hide the zoom bubble.
  std::unique_ptr<ImmersiveRevealedLock> immersive_reveal_lock(
      immersive_controller->GetRevealedLock(
          ImmersiveModeController::ANIMATE_REVEAL_NO));
  ASSERT_TRUE(immersive_controller->IsRevealed());
  EXPECT_EQ(NULL, ZoomBubbleView::zoom_bubble_);

  // The zoom bubble should be anchored when it is shown in immersive fullscreen
  // and the top-of-window views are revealed.
  ZoomBubbleView::ShowBubble(web_contents, gfx::Point(),
                             ZoomBubbleView::AUTOMATIC);
  zoom_bubble = ZoomBubbleView::GetZoomBubble();
  ASSERT_TRUE(zoom_bubble);
  EXPECT_TRUE(zoom_bubble->GetAnchorView());

  // The top-of-window views should not hide till the zoom bubble hides. (It
  // would be weird if the view to which the zoom bubble is anchored hid while
  // the zoom bubble was still visible.)
  immersive_reveal_lock.reset();
  EXPECT_TRUE(immersive_controller->IsRevealed());
  ZoomBubbleView::CloseCurrentBubble();
  // The zoom bubble is deleted on a task.
  content::RunAllPendingInMessageLoop();
  EXPECT_FALSE(immersive_controller->IsRevealed());

  // Exit fullscreen before ending the test for the sake of sanity.
  {
    std::unique_ptr<FullscreenNotificationObserver> waiter(
        new FullscreenNotificationObserver());
    chrome::ToggleFullscreenMode(browser());
    waiter->Wait();
  }
}
#endif  // OS_CHROMEOS

// Tests that trying to open zoom bubble with stale WebContents is safe.
IN_PROC_BROWSER_TEST_F(ZoomBubbleBrowserTest, NoWebContentsIsSafe) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  ZoomBubbleView::ShowBubble(web_contents, gfx::Point(),
                             ZoomBubbleView::AUTOMATIC);
  // Close the current tab and try opening the zoom bubble with stale
  // |web_contents|.
  chrome::CloseTab(browser());
  ZoomBubbleView::ShowBubble(web_contents, gfx::Point(),
                             ZoomBubbleView::AUTOMATIC);
}

// Ensure a tab switch closes the bubble.
IN_PROC_BROWSER_TEST_F(ZoomBubbleBrowserTest, TabSwitchCloses) {
  AddTabAtIndex(0, GURL(url::kAboutBlankURL), ui::PAGE_TRANSITION_LINK);
  ShowInActiveTab(browser());
  chrome::SelectNextTab(browser());
  EXPECT_FALSE(ZoomBubbleView::GetZoomBubble());
}

// Ensure the bubble is dismissed on tab closure and doesn't reference a
// destroyed WebContents.
IN_PROC_BROWSER_TEST_F(ZoomBubbleBrowserTest, DestroyedWebContents) {
  AddTabAtIndex(0, GURL(url::kAboutBlankURL), ui::PAGE_TRANSITION_LINK);
  ShowInActiveTab(browser());

  ZoomBubbleView* bubble = ZoomBubbleView::GetZoomBubble();
  EXPECT_TRUE(bubble);

  views::test::TestWidgetObserver observer(bubble->GetWidget());
  EXPECT_FALSE(bubble->GetWidget()->IsClosed());

  chrome::CloseTab(browser());
  EXPECT_FALSE(ZoomBubbleView::GetZoomBubble());

  // Widget::Close() completes asynchronously, so it's still safe to access
  // |bubble| here, even though GetZoomBubble() returned null.
  EXPECT_FALSE(observer.widget_closed());
  EXPECT_TRUE(bubble->GetWidget()->IsClosed());

  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(observer.widget_closed());
}

class ZoomBubbleDialogTest : public DialogBrowserTest {
 public:
  ZoomBubbleDialogTest() {}

  // DialogBrowserTest:
  void ShowUi(const std::string& name) override { ShowInActiveTab(browser()); }

 private:
  DISALLOW_COPY_AND_ASSIGN(ZoomBubbleDialogTest);
};

// Test that calls ShowUi("default").
IN_PROC_BROWSER_TEST_F(ZoomBubbleDialogTest, InvokeUi_default) {
  ShowAndVerifyUi();
}
