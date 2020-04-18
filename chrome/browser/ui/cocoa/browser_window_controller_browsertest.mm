// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/browser_window_controller.h"

#include <stddef.h>
#import "base/mac/mac_util.h"

#include "base/mac/scoped_nsautorelease_pool.h"
#include "base/mac/sdk_forward_declarations.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/devtools/devtools_window_testing.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/bookmarks/bookmark_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/cocoa/browser_window_cocoa.h"
#import "chrome/browser/ui/cocoa/browser_window_controller_private.h"
#import "chrome/browser/ui/cocoa/fast_resize_view.h"
#import "chrome/browser/ui/cocoa/fullscreen/fullscreen_toolbar_controller.h"
#import "chrome/browser/ui/cocoa/history_overlay_controller.h"
#import "chrome/browser/ui/cocoa/infobars/infobar_container_controller.h"
#import "chrome/browser/ui/cocoa/location_bar/location_bar_view_mac.h"
#import "chrome/browser/ui/cocoa/profiles/avatar_base_controller.h"
#import "chrome/browser/ui/cocoa/tab_contents/overlayable_contents_controller.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_view.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller_test.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/browser/ui/find_bar/find_bar.h"
#include "chrome/browser/ui/find_bar/find_bar_controller.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#import "testing/gtest_mac.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "ui/base/cocoa/nsview_additions.h"
#include "ui/gfx/animation/slide_animation.h"

namespace {

// Creates a mock of an NSWindow that has the given |frame|.
id MockWindowWithFrame(NSRect frame) {
  id window = [OCMockObject mockForClass:[NSWindow class]];
  NSValue* window_frame =
      [NSValue valueWithBytes:&frame objCType:@encode(NSRect)];
  [[[window stub] andReturnValue:window_frame] frame];
  return window;
}

void CreateProfileCallback(const base::Closure& quit_closure,
                           Profile* profile,
                           Profile::CreateStatus status) {
  EXPECT_TRUE(profile);
  EXPECT_NE(Profile::CREATE_STATUS_LOCAL_FAIL, status);
  EXPECT_NE(Profile::CREATE_STATUS_REMOTE_FAIL, status);
  // This will be called multiple times. Wait until the profile is initialized
  // fully to quit the loop.
  if (status == Profile::CREATE_STATUS_INITIALIZED)
    quit_closure.Run();
}

enum BrowserViewID {
  BROWSER_VIEW_ID_TOOLBAR,
  BROWSER_VIEW_ID_BOOKMARK_BAR,
  BROWSER_VIEW_ID_INFO_BAR,
  BROWSER_VIEW_ID_FIND_BAR,
  BROWSER_VIEW_ID_DOWNLOAD_SHELF,
  BROWSER_VIEW_ID_TAB_CONTENT_AREA,
  BROWSER_VIEW_ID_FULLSCREEN_FLOATING_BAR,
  BROWSER_VIEW_ID_COUNT,
};

// Checks that no views draw on top of the supposedly exposed view.
class ViewExposedChecker {
 public:
  ViewExposedChecker() : below_exposed_view_(YES) {}
  ~ViewExposedChecker() {}

  void SetExceptions(NSArray* exceptions) {
    exceptions_.reset([exceptions retain]);
  }

  // Checks that no views draw in front of |view|, with the exception of
  // |exceptions|.
  void CheckViewExposed(NSView* view) {
    below_exposed_view_ = YES;
    exposed_view_.reset([view retain]);
    CheckViewsDoNotObscure([[[view window] contentView] superview]);
  }

 private:
  // Checks that |view| does not draw on top of |exposed_view_|.
  void CheckViewDoesNotObscure(NSView* view) {
    NSRect viewWindowFrame = [view convertRect:[view bounds] toView:nil];
    NSRect viewBeingVerifiedWindowFrame =
        [exposed_view_ convertRect:[exposed_view_ bounds] toView:nil];

    // The views do not intersect.
    if (!NSIntersectsRect(viewBeingVerifiedWindowFrame, viewWindowFrame))
      return;

    // No view can be above the view being checked.
    EXPECT_TRUE(below_exposed_view_);

    // If |view| is a parent of |exposed_view_|, then there's nothing else
    // to check.
    NSView* parent = exposed_view_;
    while (parent != nil) {
      parent = [parent superview];
      if (parent == view)
        return;
    }

    if ([exposed_view_ layer])
      return;

    // If the view being verified doesn't have a layer, then no views that
    // intersect it can have a layer.
    if ([exceptions_ containsObject:view]) {
      EXPECT_FALSE([view isOpaque]);
      return;
    }

    EXPECT_TRUE(![view layer]) << [[view description] UTF8String] << " " <<
        [NSStringFromRect(viewWindowFrame) UTF8String];
  }

  // Recursively checks that |view| and its subviews do not draw on top of
  // |exposed_view_|. The recursion passes through all views in order of
  // back-most in Z-order to front-most in Z-order.
  void CheckViewsDoNotObscure(NSView* view) {
    // If this is the view being checked, don't recurse into its subviews. All
    // future views encountered in the recursion are in front of the view being
    // checked.
    if (view == exposed_view_) {
      below_exposed_view_ = NO;
      return;
    }

    CheckViewDoesNotObscure(view);

    // Perform the recursion.
    for (NSView* subview in [view subviews])
      CheckViewsDoNotObscure(subview);
  }

  // The method CheckViewExposed() recurses through the views in the view
  // hierarchy and checks that none of the views obscure |exposed_view_|.
  base::scoped_nsobject<NSView> exposed_view_;

  // While this flag is true, the views being recursed through are below
  // |exposed_view_| in Z-order. After the recursion passes |exposed_view_|,
  // this flag is set to false.
  BOOL below_exposed_view_;

  // Exceptions are allowed to overlap |exposed_view_|. Exceptions must still
  // be Z-order behind |exposed_view_|.
  base::scoped_nsobject<NSArray> exceptions_;

  DISALLOW_COPY_AND_ASSIGN(ViewExposedChecker);
};

}  // namespace

// Mock FullscreenToolbarController used to test if the toolbar reveal animation
// is called correctly.
@interface MockFullscreenToolbarController : FullscreenToolbarController

// True if revealToolbarForTabStripChanges was called.
@property(nonatomic, assign) BOOL isRevealingToolbar;

// Sets isRevealingToolbarForTabstrip back to false.
- (void)resetToolbarFlag;

// Overridden to set isRevealingToolbarForTabstrip to true when it's called.
- (void)revealToolbarForWebContents:(content::WebContents*)contents
                       inForeground:(BOOL)inForeground;

@end

@implementation MockFullscreenToolbarController

@synthesize isRevealingToolbar = isRevealingToolbar_;

- (void)resetToolbarFlag {
  isRevealingToolbar_ = NO;
}

- (void)revealToolbarForWebContents:(content::WebContents*)contents
                       inForeground:(BOOL)inForeground {
  isRevealingToolbar_ = YES;
}

@end

class BrowserWindowControllerTest : public InProcessBrowserTest {
 public:
  BrowserWindowControllerTest() : InProcessBrowserTest() {
  }

  void SetUpOnMainThread() override {
    [[controller() bookmarkBarController] setStateAnimationsEnabled:NO];
    [[controller() bookmarkBarController] setInnerContentAnimationsEnabled:NO];
  }

  BrowserWindowController* controller() const {
    return [BrowserWindowController browserWindowControllerForWindow:
        browser()->window()->GetNativeWindow()];
  }

  NSView* GetViewWithID(BrowserViewID view_id) const {
    switch (view_id) {
      case BROWSER_VIEW_ID_FULLSCREEN_FLOATING_BAR:
        return [controller() floatingBarBackingView];
      case BROWSER_VIEW_ID_TOOLBAR:
        return [[controller() toolbarController] view];
      case BROWSER_VIEW_ID_BOOKMARK_BAR:
        return [[controller() bookmarkBarController] view];
      case BROWSER_VIEW_ID_INFO_BAR:
        return [[controller() infoBarContainerController] view];
      case BROWSER_VIEW_ID_FIND_BAR:
        return [[controller() findBarCocoaController] view];
      case BROWSER_VIEW_ID_DOWNLOAD_SHELF:
        return [[controller() downloadShelf] view];
      case BROWSER_VIEW_ID_TAB_CONTENT_AREA:
        return [controller() tabContentArea];
      default:
        NOTREACHED();
        return nil;
    }
  }

  void VerifyZOrder(const std::vector<BrowserViewID>& view_list) const {
    std::vector<NSView*> visible_views;
    for (size_t i = 0; i < view_list.size(); ++i) {
      NSView* view = GetViewWithID(view_list[i]);
      if ([view superview])
        visible_views.push_back(view);
    }

    for (size_t i = 0; i < visible_views.size() - 1; ++i) {
      NSView* bottom_view = visible_views[i];
      NSView* top_view = visible_views[i + 1];

      EXPECT_NSEQ([bottom_view superview], [top_view superview]);
      EXPECT_TRUE([bottom_view cr_isBelowView:top_view]);
    }

    // Views not in |view_list| must either be nil or not parented.
    for (size_t i = 0; i < BROWSER_VIEW_ID_COUNT; ++i) {
      if (!base::ContainsValue(view_list, i)) {
        NSView* view = GetViewWithID(static_cast<BrowserViewID>(i));
        EXPECT_TRUE(!view || ![view superview]);
      }
    }
  }

  static void CheckBookmarkBarAnimation(
      BookmarkBarController* bookmark_bar_controller,
      const base::Closure& quit_task) {
    if (![bookmark_bar_controller isAnimationRunning])
      quit_task.Run();
  }

  void WaitForBookmarkBarAnimationToFinish() {
    scoped_refptr<content::MessageLoopRunner> runner =
        new content::MessageLoopRunner;

    base::Timer timer(false, true);
    timer.Start(
        FROM_HERE,
        base::TimeDelta::FromMilliseconds(15),
        base::Bind(&CheckBookmarkBarAnimation,
                   [controller() bookmarkBarController],
                   runner->QuitClosure()));
    runner->Run();
  }

  // Nothing should draw on top of the window controls.
  void VerifyWindowControlsZOrder() {
    NSWindow* window = [controller() window];
    ViewExposedChecker checker;

    // The exceptions are the contentView, chromeContentView and tabStripView,
    // which are layer backed but transparent.
    NSArray* exceptions = @[
      [window contentView],
      controller().chromeContentView,
      controller().tabStripView
    ];
    checker.SetExceptions(exceptions);

    checker.CheckViewExposed([window standardWindowButton:NSWindowCloseButton]);
    checker.CheckViewExposed(
        [window standardWindowButton:NSWindowMiniaturizeButton]);
    checker.CheckViewExposed([window standardWindowButton:NSWindowZoomButton]);

    // There is no fullscreen button on OSX 10.6 or OSX 10.10+.
    NSView* view = [window standardWindowButton:NSWindowFullScreenButton];
    if (view)
      checker.CheckViewExposed(view);
  }

  // NOTIFICATION_FULLSCREEN_CHANGED is sent asynchronously.
  // This method toggles fullscreen and waits for the notification.
  void ToggleBrowserFullscreenAndWaitForNotification() {
    std::unique_ptr<FullscreenNotificationObserver> waiter(
        new FullscreenNotificationObserver());
    browser()
        ->exclusive_access_manager()
        ->fullscreen_controller()
        ->ToggleBrowserFullscreenMode();
    waiter->Wait();
  }

  void EnterTabFullscreenAndWaitForNotification() {
    std::unique_ptr<FullscreenNotificationObserver> waiter(
        new FullscreenNotificationObserver());
    content::WebContents* contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    browser()
        ->exclusive_access_manager()
        ->fullscreen_controller()
        ->EnterFullscreenModeForTab(contents, GURL());
    waiter->Wait();
  }

  void ToggleExtensionFullscreenAndWaitForNotification() {
    std::unique_ptr<FullscreenNotificationObserver> waiter(
        new FullscreenNotificationObserver());
    browser()
        ->exclusive_access_manager()
        ->fullscreen_controller()
        ->ToggleBrowserFullscreenModeWithExtension(
            GURL("https://www.google.com"));
    waiter->Wait();
  }

  // Verifies that the flags |blockLayoutSubviews_| and |blockFullscreenResize|
  // are false.
  void VerifyFullscreenResizeFlagsAfterTransition() {
    ASSERT_FALSE([controller() isLayoutSubviewsBlocked]);
    ASSERT_FALSE([controller() isActiveTabContentsControllerResizeBlocked]);
  }

  // Inserts a new tab into the tabstrip at the background.
  void AddTabAtBackground(int index, GURL url) {
    NavigateParams params(browser(), url, ui::PAGE_TRANSITION_LINK);
    params.tabstrip_index = index;
    params.disposition = WindowOpenDisposition::NEW_BACKGROUND_TAB;
    Navigate(&params);
    content::WaitForLoadStopWithoutSuccessCheck(params.target_contents);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserWindowControllerTest);
};

// Tests that adding the first profile moves the Lion fullscreen button over
// correctly.
// DISABLED_ because it regularly times out: http://crbug.com/159002.
IN_PROC_BROWSER_TEST_F(BrowserWindowControllerTest,
                       DISABLED_ProfileAvatarFullscreenButton) {
  // Initialize the locals.
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);

  NSWindow* window = browser()->window()->GetNativeWindow();
  ASSERT_TRUE(window);

  // With only one profile, the fullscreen button should be visible, but the
  // avatar button should not.
  EXPECT_EQ(1u, profile_manager->GetNumberOfProfiles());

  NSButton* fullscreen_button =
      [window standardWindowButton:NSWindowFullScreenButton];
  EXPECT_TRUE(fullscreen_button);
  EXPECT_FALSE([fullscreen_button isHidden]);

  AvatarBaseController* avatar_controller =
      [controller() avatarButtonController];
  NSView* avatar = [avatar_controller view];
  EXPECT_TRUE(avatar);
  EXPECT_TRUE([avatar isHidden]);

  // Create a profile asynchronously and run the loop until its creation
  // is complete.
  base::RunLoop run_loop;

  ProfileManager::CreateCallback create_callback =
      base::Bind(&CreateProfileCallback, run_loop.QuitClosure());
  profile_manager->CreateProfileAsync(
      profile_manager->user_data_dir().Append("test"),
      create_callback,
      base::ASCIIToUTF16("avatar_test"),
      std::string(),
      std::string());

  run_loop.Run();

  // There should now be two profiles, and the avatar button and fullscreen
  // button are both visible.
  EXPECT_EQ(2u, profile_manager->GetNumberOfProfiles());
  EXPECT_FALSE([avatar isHidden]);
  EXPECT_FALSE([fullscreen_button isHidden]);
  EXPECT_EQ([avatar window], [fullscreen_button window]);

  // Make sure the visual order of the buttons is correct and that they don't
  // overlap.
  NSRect avatar_frame = [avatar frame];
  NSRect fullscreen_frame = [fullscreen_button frame];

  EXPECT_LT(NSMinX(fullscreen_frame), NSMinX(avatar_frame));
  EXPECT_LT(NSMaxX(fullscreen_frame), NSMinX(avatar_frame));
}

// Verify that in non-Instant normal mode that the find bar and download shelf
// are above the content area. Everything else should be below it.
IN_PROC_BROWSER_TEST_F(BrowserWindowControllerTest, ZOrderNormal) {
  browser()->GetFindBarController();  // add find bar

  std::vector<BrowserViewID> view_list;
  view_list.push_back(BROWSER_VIEW_ID_DOWNLOAD_SHELF);
  view_list.push_back(BROWSER_VIEW_ID_BOOKMARK_BAR);
  view_list.push_back(BROWSER_VIEW_ID_TOOLBAR);
  view_list.push_back(BROWSER_VIEW_ID_INFO_BAR);
  view_list.push_back(BROWSER_VIEW_ID_TAB_CONTENT_AREA);
  view_list.push_back(BROWSER_VIEW_ID_FIND_BAR);
  VerifyZOrder(view_list);

  [controller() showOverlay];
  [controller() removeOverlay];
  VerifyZOrder(view_list);

  [controller() enterImmersiveFullscreen];
  [controller() exitImmersiveFullscreen];
  VerifyZOrder(view_list);
}

// Verify that in non-Instant presentation mode that the info bar is below the
// content are and everything else is above it.
// DISABLED due to flaky failures on trybots. http://crbug.com/178778
IN_PROC_BROWSER_TEST_F(BrowserWindowControllerTest,
                       DISABLED_ZOrderPresentationMode) {
  chrome::ToggleFullscreenMode(browser());
  browser()->GetFindBarController();  // add find bar

  std::vector<BrowserViewID> view_list;
  view_list.push_back(BROWSER_VIEW_ID_INFO_BAR);
  view_list.push_back(BROWSER_VIEW_ID_TAB_CONTENT_AREA);
  view_list.push_back(BROWSER_VIEW_ID_FULLSCREEN_FLOATING_BAR);
  view_list.push_back(BROWSER_VIEW_ID_BOOKMARK_BAR);
  view_list.push_back(BROWSER_VIEW_ID_TOOLBAR);
  view_list.push_back(BROWSER_VIEW_ID_FIND_BAR);
  view_list.push_back(BROWSER_VIEW_ID_DOWNLOAD_SHELF);
  VerifyZOrder(view_list);
}

// Verify that if the fullscreen floating bar view is below the tab content area
// then calling |updateSubviewZOrder:| will correctly move back above.
IN_PROC_BROWSER_TEST_F(BrowserWindowControllerTest,
                       DISABLED_FloatingBarBelowContentView) {
  // TODO(kbr): re-enable: http://crbug.com/222296
  return;

  chrome::ToggleFullscreenMode(browser());

  NSView* fullscreen_floating_bar =
      GetViewWithID(BROWSER_VIEW_ID_FULLSCREEN_FLOATING_BAR);
  [fullscreen_floating_bar removeFromSuperview];
  [[[controller() window] contentView] addSubview:fullscreen_floating_bar
                                       positioned:NSWindowBelow
                                       relativeTo:nil];
  [controller() updateSubviewZOrder];

  std::vector<BrowserViewID> view_list;
  view_list.push_back(BROWSER_VIEW_ID_INFO_BAR);
  view_list.push_back(BROWSER_VIEW_ID_TAB_CONTENT_AREA);
  view_list.push_back(BROWSER_VIEW_ID_FULLSCREEN_FLOATING_BAR);
  view_list.push_back(BROWSER_VIEW_ID_BOOKMARK_BAR);
  view_list.push_back(BROWSER_VIEW_ID_TOOLBAR);
  view_list.push_back(BROWSER_VIEW_ID_DOWNLOAD_SHELF);
  VerifyZOrder(view_list);
}

IN_PROC_BROWSER_TEST_F(BrowserWindowControllerTest, SheetPosition) {
  ASSERT_TRUE([controller() isKindOfClass:[BrowserWindowController class]]);
  EXPECT_TRUE([controller() isTabbedWindow]);
  EXPECT_TRUE([controller() hasTabStrip]);
  EXPECT_FALSE([controller() hasTitleBar]);
  EXPECT_TRUE([controller() hasToolbar]);
  EXPECT_FALSE([controller() isBookmarkBarVisible]);

  id sheet = MockWindowWithFrame(NSMakeRect(0, 0, 300, 200));
  NSWindow* window = browser()->window()->GetNativeWindow();
  NSRect contentFrame = [[window contentView] frame];
  NSRect defaultLocation =
      NSMakeRect(0, NSMaxY(contentFrame), NSWidth(contentFrame), 0);

  NSRect sheetLocation = [controller() window:window
                            willPositionSheet:nil
                                    usingRect:defaultLocation];
  NSRect toolbarFrame = [[[controller() toolbarController] view] frame];
  EXPECT_EQ(NSMinY(toolbarFrame), NSMinY(sheetLocation));

  // Open sheet with normal browser window, persistent bookmark bar.
  chrome::ToggleBookmarkBarWhenVisible(browser()->profile());
  EXPECT_TRUE([controller() isBookmarkBarVisible]);
  sheetLocation = [controller() window:window
                     willPositionSheet:sheet
                             usingRect:defaultLocation];
  NSRect bookmarkBarFrame = [[[controller() bookmarkBarController] view] frame];
  EXPECT_EQ(NSMinY(bookmarkBarFrame), NSMinY(sheetLocation));

  // If the sheet is too large, it should be positioned at the top of the
  // window.
  sheet = MockWindowWithFrame(NSMakeRect(0, 0, 300, 2000));
  sheetLocation = [controller() window:window
                     willPositionSheet:sheet
                             usingRect:defaultLocation];
  EXPECT_EQ(NSHeight([window frame]), NSMinY(sheetLocation));

  // Reset the sheet's size.
  sheet = MockWindowWithFrame(NSMakeRect(0, 0, 300, 200));

  // Make sure the profile does not have the bookmark visible so that
  // we'll create the shortcut window without the bookmark bar.
  chrome::ToggleBookmarkBarWhenVisible(browser()->profile());
  // Open application mode window.
  OpenAppShortcutWindow(browser()->profile(), GURL("about:blank"));
  Browser* popup_browser = BrowserList::GetInstance()->GetLastActive();
  NSWindow* popupWindow = popup_browser->window()->GetNativeWindow();
  BrowserWindowController* popupController =
      [BrowserWindowController browserWindowControllerForWindow:popupWindow];
  ASSERT_TRUE([popupController isKindOfClass:[BrowserWindowController class]]);
  EXPECT_FALSE([popupController isTabbedWindow]);
  EXPECT_FALSE([popupController hasTabStrip]);
  EXPECT_TRUE([popupController hasTitleBar]);
  EXPECT_FALSE([popupController isBookmarkBarVisible]);
  EXPECT_FALSE([popupController hasToolbar]);

  // Open sheet in an application window.
  NSWindowController* nsWindowController = [popupController nsWindowController];
  EXPECT_TRUE(nsWindowController);
  [nsWindowController showWindow:nil];
  sheetLocation = [popupController window:popupWindow
                        willPositionSheet:sheet
                                usingRect:defaultLocation];
  EXPECT_EQ(NSHeight([[popupWindow contentView] frame]), NSMinY(sheetLocation));

  // Close the application window.
  popup_browser->tab_strip_model()->CloseSelectedTabs();
  [nsWindowController close];
}

// Tests that status bubble's base frame does move when devTools are docked.
IN_PROC_BROWSER_TEST_F(BrowserWindowControllerTest,
                       StatusBubblePositioning) {
  NSPoint origin = [controller() statusBubbleBaseFrame].origin;

  DevToolsWindow* devtools_window =
      DevToolsWindowTesting::OpenDevToolsWindowSync(browser(), true);
  DevToolsWindowTesting::Get(devtools_window)->SetInspectedPageBounds(
      gfx::Rect(10, 10, 100, 100));

  NSPoint originWithDevTools = [controller() statusBubbleBaseFrame].origin;
  EXPECT_NSNE(origin, originWithDevTools);

  DevToolsWindowTesting::CloseDevToolsWindowSync(devtools_window);
}

IN_PROC_BROWSER_TEST_F(BrowserWindowControllerTest, TrafficLightZOrder) {
  // Verify z order immediately after creation.
  VerifyWindowControlsZOrder();

  // Verify z order in and out of overlay.
  [controller() showOverlay];
  VerifyWindowControlsZOrder();
  [controller() removeOverlay];
  VerifyWindowControlsZOrder();

  // Toggle immersive fullscreen, then verify z order. In immersive fullscreen,
  // there are no window controls.
  [controller() enterImmersiveFullscreen];
  [controller() exitImmersiveFullscreen];
  VerifyWindowControlsZOrder();
}

// Ensure that the blocking resize flags set during fullscreen transition to
// are reset correctly after the transition.
IN_PROC_BROWSER_TEST_F(BrowserWindowControllerTest, FullscreenResizeFlags) {
  // Enter fullscreen and verify the flags.
  ToggleBrowserFullscreenAndWaitForNotification();
  VerifyFullscreenResizeFlagsAfterTransition();

  // Exit fullscreen and verify the flags.
  ToggleBrowserFullscreenAndWaitForNotification();
  VerifyFullscreenResizeFlagsAfterTransition();
}

// Tests that the omnibox and tabs are hidden/visible in fullscreen mode.
// Ensure that when the user toggles this setting, the omnibox, tabs and
// preferences are updated correctly.
IN_PROC_BROWSER_TEST_F(BrowserWindowControllerTest,
                       FullscreenToolbarIsVisibleAccordingToPrefs) {
  // Tests that the preference is set to true by default.
  PrefService* prefs = browser()->profile()->GetPrefs();
  EXPECT_TRUE(prefs->GetBoolean(prefs::kShowFullscreenToolbar));

  [[controller() fullscreenToolbarController] setMenubarTracker:nil];

  // Toggle fullscreen and check if the toolbar is shown.
  ToggleBrowserFullscreenAndWaitForNotification();
  EXPECT_EQ(
      FullscreenToolbarStyle::TOOLBAR_PRESENT,
      [[controller() fullscreenToolbarController] computeLayout].toolbarStyle);

  // Toggle the visibility of the fullscreen toolbar. Verify that the toolbar
  // is hidden and the preference is correctly updated.
  chrome::ExecuteCommand(browser(), IDC_TOGGLE_FULLSCREEN_TOOLBAR);
  EXPECT_FALSE(prefs->GetBoolean(prefs::kShowFullscreenToolbar));
  EXPECT_EQ(
      FullscreenToolbarStyle::TOOLBAR_HIDDEN,
      [[controller() fullscreenToolbarController] computeLayout].toolbarStyle);

  // Toggle out and back into fullscreen and verify that the toolbar is still
  // hidden.
  ToggleBrowserFullscreenAndWaitForNotification();
  ToggleBrowserFullscreenAndWaitForNotification();
  EXPECT_EQ(
      FullscreenToolbarStyle::TOOLBAR_HIDDEN,
      [[controller() fullscreenToolbarController] computeLayout].toolbarStyle);

  chrome::ExecuteCommand(browser(), IDC_TOGGLE_FULLSCREEN_TOOLBAR);
  EXPECT_TRUE(prefs->GetBoolean(prefs::kShowFullscreenToolbar));
}

// Tests that the toolbar (tabstrip and omnibox) reveal animation is correctly
// triggered by the changes in the tabstrip. The animation should not trigger
// if the current tab is a NTP, since the location bar would be focused.
IN_PROC_BROWSER_TEST_F(BrowserWindowControllerTest,
                       FullscreenToolbarExposedForTabstripChanges) {
  base::scoped_nsobject<MockFullscreenToolbarController>
      fullscreenToolbarController([[MockFullscreenToolbarController alloc]
          initWithBrowserController:controller()]);
  [controller()
      setFullscreenToolbarController:fullscreenToolbarController.get()];

  ToggleBrowserFullscreenAndWaitForNotification();

  // Insert a non-NTP new tab in the foreground.
  AddTabAtIndex(0, GURL("http://google.com"), ui::PAGE_TRANSITION_LINK);
  ASSERT_FALSE([[controller() toolbarController] isLocationBarFocused]);
  EXPECT_TRUE([fullscreenToolbarController isRevealingToolbar]);
  [fullscreenToolbarController resetToolbarFlag];

  // Insert a new tab in the background.
  AddTabAtBackground(0, GURL("about:blank"));
  EXPECT_TRUE([fullscreenToolbarController isRevealingToolbar]);
  [fullscreenToolbarController resetToolbarFlag];

  // Insert a NTP new tab in the foreground.
  AddTabAtIndex(0, GURL("about:blank"), ui::PAGE_TRANSITION_LINK);
  ASSERT_TRUE([[controller() toolbarController] isLocationBarFocused]);
  EXPECT_TRUE([fullscreenToolbarController isRevealingToolbar]);
  [fullscreenToolbarController resetToolbarFlag];

  AddTabAtBackground(1, GURL("http://google.com"));
  ASSERT_TRUE([[controller() toolbarController] isLocationBarFocused]);
  EXPECT_TRUE([fullscreenToolbarController isRevealingToolbar]);
  [fullscreenToolbarController resetToolbarFlag];

  // Switch to a non-NTP tab.
  TabStripModel* model = browser()->tab_strip_model();
  model->ActivateTabAt(1, true);
  ASSERT_FALSE([[controller() toolbarController] isLocationBarFocused]);
  EXPECT_TRUE([fullscreenToolbarController isRevealingToolbar]);
  [fullscreenToolbarController resetToolbarFlag];
}

// Tests if entering browser fullscreen would log into the histogram.
// See https://crbug.com/771993 re disabling.
IN_PROC_BROWSER_TEST_F(BrowserWindowControllerTest,
                       DISABLED_FullscreenHistogram) {
  base::HistogramTester histogram_tester;
  PrefService* prefs = browser()->profile()->GetPrefs();
  prefs->SetBoolean(prefs::kShowFullscreenToolbar, true);

  // Enter browser fullscreen.
  ToggleBrowserFullscreenAndWaitForNotification();
  histogram_tester.ExpectBucketCount("OSX.Fullscreen.Enter.Source",
                                     (int)FullscreenSource::BROWSER, 1);
  histogram_tester.ExpectTotalCount("OSX.Fullscreen.Enter.WindowLocation", 1);
  histogram_tester.ExpectTotalCount("OSX.Fullscreen.Enter.Style", 1);
  histogram_tester.ExpectBucketCount(
      "OSX.Fullscreen.ToolbarStyle",
      (int)FullscreenToolbarStyle::TOOLBAR_PRESENT, 1);

  // Exit browser fullscreen.
  ToggleBrowserFullscreenAndWaitForNotification();

  prefs->SetBoolean(prefs::kShowFullscreenToolbar, false);
  ToggleBrowserFullscreenAndWaitForNotification();
  histogram_tester.ExpectBucketCount("OSX.Fullscreen.Enter.Source",
                                     (int)FullscreenSource::BROWSER, 2);
  histogram_tester.ExpectTotalCount("OSX.Fullscreen.Enter.WindowLocation", 2);
  histogram_tester.ExpectTotalCount("OSX.Fullscreen.Enter.Style", 2);
  histogram_tester.ExpectBucketCount(
      "OSX.Fullscreen.ToolbarStyle",
      (int)FullscreenToolbarStyle::TOOLBAR_HIDDEN, 1);

  prefs->SetBoolean(prefs::kShowFullscreenToolbar, true);
  [[controller() fullscreenToolbarController]
      layoutToolbarStyleIsExitingTabFullscreen:NO];
  histogram_tester.ExpectBucketCount(
      "OSX.Fullscreen.ToolbarStyle",
      (int)FullscreenToolbarStyle::TOOLBAR_PRESENT, 2);

  // Enter tab fullscreen. Since we're still in browser fullscreen, this
  // should not log anything to OSX.Fullscreen.Enter.
  EnterTabFullscreenAndWaitForNotification();
  histogram_tester.ExpectTotalCount("OSX.Fullscreen.Enter.Source", 2);
  histogram_tester.ExpectTotalCount("OSX.Fullscreen.Enter.WindowLocation", 2);
  histogram_tester.ExpectTotalCount("OSX.Fullscreen.Enter.Style", 2);

  histogram_tester.ExpectBucketCount("OSX.Fullscreen.ToolbarStyle",
                                     (int)FullscreenToolbarStyle::TOOLBAR_NONE,
                                     1);

  // Exit browser fullscreen. This will also cause tab fullscreen to exit.
  ToggleBrowserFullscreenAndWaitForNotification();

  EnterTabFullscreenAndWaitForNotification();
  histogram_tester.ExpectBucketCount("OSX.Fullscreen.Enter.Source",
                                     (int)FullscreenSource::TAB, 1);
  histogram_tester.ExpectTotalCount("OSX.Fullscreen.Enter.WindowLocation", 3);
  histogram_tester.ExpectTotalCount("OSX.Fullscreen.Enter.Style", 3);
  histogram_tester.ExpectBucketCount("OSX.Fullscreen.ToolbarStyle",
                                     (int)FullscreenToolbarStyle::TOOLBAR_NONE,
                                     2);

  ToggleBrowserFullscreenAndWaitForNotification();

  // Enter fullscreen for extension.
  ToggleExtensionFullscreenAndWaitForNotification();
  EXPECT_TRUE(browser()
                  ->exclusive_access_manager()
                  ->fullscreen_controller()
                  ->IsExtensionFullscreenOrPending());
  histogram_tester.ExpectBucketCount("OSX.Fullscreen.Enter.Source",
                                     (int)FullscreenSource::EXTENSION, 1);
  histogram_tester.ExpectTotalCount("OSX.Fullscreen.Enter.WindowLocation", 4);
  histogram_tester.ExpectTotalCount("OSX.Fullscreen.Enter.Style", 4);
  histogram_tester.ExpectBucketCount("OSX.Fullscreen.ToolbarStyle",
                                     (int)FullscreenToolbarStyle::TOOLBAR_NONE,
                                     3);
}

// Ensure that some steps performed during process shutdown can be performed
// during a tab drag. Regression test for https://crbug.com/788271.
IN_PROC_BROWSER_TEST_F(BrowserWindowControllerTest, ShutdownDuringTabDrag) {
  // Rather than trying to trick TabDragController into calling showOverlay,
  // call it directly. Retain it (as if by the OS). Then remove the overlay (to
  // trigger the bug).
  [controller() showOverlay];
  base::scoped_nsobject<NSWindow> simulate_os_retain(
      [[controller() overlayWindow] retain]);
  [controller() removeOverlay];
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());

  // To kick things along, close the window rather than waiting for
  // BrowserCloseManager to do it asynchronously.
  browser()->tab_strip_model()->CloseAllTabs();
  [[controller() window] close];

  content::RunAllPendingInMessageLoop();
  AutoreleasePool()->Recycle();
  EXPECT_EQ(0u, chrome::GetTotalBrowserCount());

  // All browsers are now closed, so this should immediately invoke
  // ShutdownIfNoBrowsers() and HandleAppExitingForPlatform(), which invokes
  // views::Widget::CloseAllSecondaryWidgets(). Must be done via PostTask() to
  // avoid a DCHECK in BrowserProcessImpl::Unpin(), which checks for a run loop.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&chrome::CloseAllBrowsersAndQuit));
  content::RunAllPendingInMessageLoop();
}
