// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/browser_window_controller.h"

#include <memory>

#include "base/mac/mac_util.h"
#import "base/mac/scoped_nsobject.h"
#import "base/mac/scoped_objc_class_swizzler.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/cocoa/browser_window_layout.h"
#import "chrome/browser/ui/cocoa/fast_resize_view.h"
#include "chrome/browser/ui/cocoa/find_bar/find_bar_bridge.h"
#include "chrome/browser/ui/cocoa/tabs/tab_strip_view.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "chrome/browser/ui/cocoa/test/run_loop_testing.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#import "testing/gtest_mac.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "ui/base/test/scoped_fake_nswindow_fullscreen.h"

using ::testing::Return;

@interface BrowserWindowController (JustForTesting)
// Already defined in BWC.
- (void)saveWindowPositionIfNeeded;
- (void)layoutSubviews;
@end

@interface BrowserWindowController (ExposedForTesting)
// Implementations are below.
- (NSView*)infoBarContainerView;
- (NSView*)toolbarView;
- (NSView*)bookmarkView;
- (BOOL)bookmarkBarVisible;
- (void)dontFocusLocationBar:(BOOL)selectAll;
@end

@implementation BrowserWindowController (ExposedForTesting)
- (NSView*)infoBarContainerView {
  return [infoBarContainerController_ view];
}

- (NSView*)toolbarView {
  return [toolbarController_ view];
}

- (NSView*)bookmarkView {
  return [bookmarkBarController_ view];
}

- (NSView*)findBarView {
  return [findBarCocoaController_ view];
}

- (BOOL)bookmarkBarVisible {
  return [bookmarkBarController_ isVisible];
}

- (void)dontFocusLocationBar:(BOOL)selectAll {
}
@end

class BrowserWindowControllerTest : public CocoaProfileTest {
 public:
  void SetUp() override {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(browser());

    controller_ = [[BrowserWindowController alloc] initWithBrowser:browser()
                                                     takeOwnership:NO];
  }

  void TearDown() override {
    [[controller_ nsWindowController] close];
    CocoaProfileTest::TearDown();
  }

 public:
  BrowserWindowController* controller_;
};

TEST_F(BrowserWindowControllerTest, TestSaveWindowPosition) {
  PrefService* prefs = profile()->GetPrefs();
  ASSERT_TRUE(prefs != NULL);

  // Check to make sure there is no existing pref for window placement.
  const base::DictionaryValue* browser_window_placement =
      prefs->GetDictionary(prefs::kBrowserWindowPlacement);
  ASSERT_TRUE(browser_window_placement);
  EXPECT_TRUE(browser_window_placement->empty());

  // Ask the window to save its position, then check that a preference
  // exists.
  BrowserList::SetLastActive(browser());
  [controller_ saveWindowPositionIfNeeded];
  browser_window_placement =
      prefs->GetDictionary(prefs::kBrowserWindowPlacement);
  ASSERT_TRUE(browser_window_placement);
  EXPECT_FALSE(browser_window_placement->empty());
}

TEST_F(BrowserWindowControllerTest, TestFullScreenWindow) {
  // Confirm that |-createFullscreenWindow| doesn't return nil.
  // See BrowserWindowFullScreenControllerTest for more fullscreen tests.
  EXPECT_TRUE([controller_ createFullscreenWindow]);
}

TEST_F(BrowserWindowControllerTest, TestNormal) {
  // Force the bookmark bar to be shown.
  profile()->GetPrefs()->SetBoolean(bookmarks::prefs::kShowBookmarkBar, true);
  [controller_ browserWindow]->BookmarkBarStateChanged(
      BookmarkBar::DONT_ANIMATE_STATE_CHANGE);

  // Make sure a normal BrowserWindowController is, uh, normal.
  EXPECT_TRUE([controller_ isTabbedWindow]);
  EXPECT_TRUE([controller_ hasTabStrip]);
  EXPECT_FALSE([controller_ hasTitleBar]);
  EXPECT_TRUE([controller_ isBookmarkBarVisible]);

  // And make sure a controller for a pop-up window is not normal.
  // popup_browser will be owned by its window.
  Browser* popup_browser(
      new Browser(Browser::CreateParams(Browser::TYPE_POPUP, profile(), true)));
  NSWindow* cocoaWindow = popup_browser->window()->GetNativeWindow();
  BrowserWindowController* controller =
      [BrowserWindowController browserWindowControllerForWindow:cocoaWindow];
  ASSERT_TRUE([controller isKindOfClass:[BrowserWindowController class]]);
  EXPECT_FALSE([controller isTabbedWindow]);
  EXPECT_FALSE([controller hasTabStrip]);
  EXPECT_TRUE([controller hasTitleBar]);
  EXPECT_FALSE([controller isBookmarkBarVisible]);
  [[controller nsWindowController] close];
}

TEST_F(BrowserWindowControllerTest, TestSetBounds) {
  // Create a normal browser with bounds smaller than the minimum.
  Browser::CreateParams params(Browser::TYPE_TABBED, profile(), true);
  params.initial_bounds = gfx::Rect(0, 0, 50, 50);
  Browser* browser = new Browser(params);
  NSWindow* cocoaWindow = browser->window()->GetNativeWindow();
  BrowserWindowController* controller =
      [BrowserWindowController browserWindowControllerForWindow:cocoaWindow];

  ASSERT_TRUE([controller isTabbedWindow]);
  BrowserWindow* browser_window = [controller browserWindow];
  EXPECT_EQ(browser_window, browser->window());
  EXPECT_EQ(browser_window->GetBounds().size(), kMinCocoaTabbedWindowSize);

  // Try to set the bounds smaller than the minimum.
  browser_window->SetBounds(gfx::Rect(0, 0, 50, 50));
  EXPECT_EQ(browser_window->GetBounds().size(), kMinCocoaTabbedWindowSize);

  [[controller nsWindowController] close];
}

// https://crbug.com/667698 - When Auto Layout is in use, adding the download
// shelf without ever showing it shouldn't prevent the window from being
// resized to its minimum width.
TEST_F(BrowserWindowControllerTest, TestSetBoundsWithDownloadShelf) {
  BrowserWindow* browser_window = [controller_ browserWindow];
  browser_window->SetBounds(gfx::Rect(0, 0, 1000, 50));

  // Auto Layout only acts on the window if it's visible.
  browser_window->ShowInactive();

  // The browser window should lazily create the download shelf when requested.
  EXPECT_NE(nullptr, browser_window->GetDownloadShelf());

  // The controller should now have a download shelf, which should have a view.
  EXPECT_NE(nil, [[controller_ downloadShelf] view]);

  // But, just requesting the download shelf shouldn't make it visible.
  EXPECT_FALSE([controller_ isDownloadShelfVisible]);

  browser_window->SetBounds(gfx::Rect(0, 0, 50, 50));

  // When linking against an SDK >= 10.11, AppKit may lay out the window
  // asynchronously (CFExecutableLinkedOnOrAfter check in -[NSThemeFrame
  // handleSetFrameCommonRedisplay]). Do layout now instead.
  [[controller_ window] layoutIfNeeded];

  // The window should have returned to its minimum size.
  EXPECT_EQ(browser_window->GetBounds().size(), kMinCocoaTabbedWindowSize);

  browser_window->Close();
}

TEST_F(BrowserWindowControllerTest, TestSetBoundsPopup) {
  // Create a popup with bounds smaller than the minimum.
  Browser::CreateParams params(Browser::TYPE_POPUP, profile(), true);
  params.initial_bounds = gfx::Rect(0, 0, 50, 50);
  Browser* browser = new Browser(params);
  NSWindow* cocoaWindow = browser->window()->GetNativeWindow();
  BrowserWindowController* controller =
      [BrowserWindowController browserWindowControllerForWindow:cocoaWindow];

  ASSERT_FALSE([controller isTabbedWindow]);
  BrowserWindow* browser_window = [controller browserWindow];
  EXPECT_EQ(browser_window, browser->window());
  gfx::Rect bounds = browser_window->GetBounds();
  EXPECT_EQ(100, bounds.width());
  EXPECT_EQ(122, bounds.height());

  // Try to set the bounds smaller than the minimum.
  browser_window->SetBounds(gfx::Rect(0, 0, 50, 50));
  bounds = browser_window->GetBounds();
  EXPECT_EQ(100, bounds.width());
  EXPECT_EQ(122, bounds.height());

  [[controller nsWindowController] close];
}

TEST_F(BrowserWindowControllerTest, TestTheme) {
  [controller_ userChangedTheme];
}

TEST_F(BrowserWindowControllerTest, BookmarkBarControllerIndirection) {
  EXPECT_FALSE([controller_ isBookmarkBarVisible]);

  // Explicitly show the bar. Can't use chrome::ToggleBookmarkBarWhenVisible()
  // because of the notification issues.
  profile()->GetPrefs()->SetBoolean(bookmarks::prefs::kShowBookmarkBar, true);

  [controller_ browserWindow]->BookmarkBarStateChanged(
      BookmarkBar::DONT_ANIMATE_STATE_CHANGE);
  EXPECT_TRUE([controller_ isBookmarkBarVisible]);
}

TEST_F(BrowserWindowControllerTest, BookmarkBarToggleRespectMinWindowHeight) {
  Browser::CreateParams params(Browser::TYPE_TABBED, profile(), true);
  params.initial_bounds = gfx::Rect(0, 0, 50, 280);
  Browser* browser = new Browser(params);
  NSWindow* cocoaWindow = browser->window()->GetNativeWindow();
  BrowserWindowController* controller =
      [BrowserWindowController browserWindowControllerForWindow:cocoaWindow];
  BrowserWindow* browser_window = [controller browserWindow];
  gfx::Rect bounds = browser_window->GetBounds();
  EXPECT_EQ(280, bounds.height());

  // Try to set the bounds smaller than the minimum.
  // Explicitly show the bar. Can't use chrome::ToggleBookmarkBarWhenVisible()
  // because of the notification issues.
  [controller adjustWindowHeightBy:-20];
  bounds = browser_window->GetBounds();
  EXPECT_EQ(272, bounds.height());

  [[controller nsWindowController] close];
}

#if 0
// TODO(jrg): This crashes trying to create the BookmarkBarController, adding
// an observer to the BookmarkModel.
TEST_F(BrowserWindowControllerTest, TestIncognitoWidthSpace) {
  std::unique_ptr<TestingProfile> incognito_profile(new TestingProfile());
  incognito_profile->set_off_the_record(true);
  std::unique_ptr<Browser> browser(
      new Browser(Browser::CreateParams(incognito_profile.get(), true)));
  controller_.reset([[BrowserWindowController alloc]
                              initWithBrowser:browser.get()
                                takeOwnership:NO]);

  NSRect tabFrame = [[controller_ tabStripView] frame];
  [controller_ installIncognitoBadge];
  NSRect newTabFrame = [[controller_ tabStripView] frame];
  EXPECT_GT(tabFrame.size.width, newTabFrame.size.width);

  controller_.release();
}
#endif

namespace {

// Returns the frame of the view in window coordinates.
NSRect FrameInWindowForView(NSView* view) {
  return [[view superview] convertRect:[view frame] toView:nil];
}

// Whether the view's frame is within the bounds of the superview.
BOOL ViewContainmentValid(NSView* view) {
  if (NSIsEmptyRect([view frame]))
    return YES;

  return NSContainsRect([[view superview] bounds], [view frame]);
}

// Checks the view hierarchy rooted at |view| to ensure that each view is
// properly contained.
BOOL ViewHierarchyContainmentValid(NSView* view) {
  // TODO(erikchen): Fix these views to have correct containment.
  // http://crbug.com/397665.
  if ([view isKindOfClass:NSClassFromString(@"DownloadShelfView")])
    return YES;
  if ([view isKindOfClass:NSClassFromString(@"BookmarkBarToolbarView")])
    return YES;
  if ([view isKindOfClass:NSClassFromString(@"BrowserActionsContainerView")])
    return YES;

  if (!ViewContainmentValid(view)) {
    LOG(ERROR) << "View violates containment: " <<
        [[view description] UTF8String];
    return NO;
  }

  for (NSView* subview in [view subviews]) {
    BOOL result = ViewHierarchyContainmentValid(subview);
    if (!result)
      return NO;
  }

  return YES;
}

// Verifies that the toolbar, infobar, tab content area, and download shelf
// completely fill the area under the tabstrip.
void CheckViewPositions(BrowserWindowController* controller) {
  EXPECT_TRUE(ViewHierarchyContainmentValid([[controller window] contentView]));

  NSRect contentView = FrameInWindowForView([[controller window] contentView]);
  NSRect tabstrip = FrameInWindowForView([controller tabStripView]);
  NSRect toolbar = FrameInWindowForView([controller toolbarView]);
  NSRect infobar = FrameInWindowForView([controller infoBarContainerView]);
  NSRect tabContent = FrameInWindowForView([controller tabContentArea]);
  NSRect download = NSZeroRect;
  if ([[[controller downloadShelf] view] superview])
    download = [[[controller downloadShelf] view] frame];

  EXPECT_EQ(NSMinY(contentView), NSMinY(download));
  EXPECT_EQ(NSMaxY(download), NSMinY(tabContent));
  EXPECT_EQ(NSMaxY(tabContent), NSMinY(infobar));

  // Bookmark bar frame is random memory when hidden.
  if ([controller bookmarkBarVisible]) {
    NSRect bookmark = [[controller bookmarkView] frame];
    EXPECT_EQ(NSMaxY(infobar), NSMinY(bookmark));
    EXPECT_EQ(NSMaxY(bookmark), NSMinY(toolbar));
    EXPECT_FALSE([[controller bookmarkView] isHidden]);
  } else {
    EXPECT_EQ(NSMaxY(infobar), NSMinY(toolbar));
    EXPECT_TRUE([[controller bookmarkView] isHidden]);
  }

  // Toolbar should start immediately under the tabstrip, but the tabstrip is
  // not necessarily fixed with respect to the content view.
  EXPECT_EQ(NSMinY(tabstrip), NSMaxY(toolbar));
}

}  // end namespace

TEST_F(BrowserWindowControllerTest, TestAdjustWindowHeight) {
  NSWindow* window = [controller_ window];
  NSRect workarea = [[window screen] visibleFrame];

  // Place the window well above the bottom of the screen and try to adjust its
  // height. It should change appropriately (and only downwards). Then get it to
  // shrink by the same amount; it should return to its original state.
  NSRect initialFrame = NSMakeRect(workarea.origin.x, workarea.origin.y + 100,
                                   200, 280);
  [window setFrame:initialFrame display:YES];
  [controller_ resetWindowGrowthState];
  [controller_ adjustWindowHeightBy:40];
  NSRect finalFrame = [window frame];
  EXPECT_NSNE(finalFrame, initialFrame);
  EXPECT_FLOAT_EQ(NSMaxY(finalFrame), NSMaxY(initialFrame));
  EXPECT_FLOAT_EQ(NSHeight(finalFrame), NSHeight(initialFrame) + 40);
  [controller_ adjustWindowHeightBy:-40];
  finalFrame = [window frame];
  EXPECT_FLOAT_EQ(NSMaxY(finalFrame), NSMaxY(initialFrame));
  EXPECT_FLOAT_EQ(NSHeight(finalFrame), NSHeight(initialFrame));

  // Place the window at the bottom of the screen and try again.  Its height
  // should still change, but it should not grow down below the work area; it
  // should instead move upwards. Then shrink it and make sure it goes back to
  // the way it was.
  initialFrame = NSMakeRect(workarea.origin.x, workarea.origin.y, 200, 280);
  [window setFrame:initialFrame display:YES];
  [controller_ resetWindowGrowthState];
  [controller_ adjustWindowHeightBy:40];
  finalFrame = [window frame];
  EXPECT_NSNE(finalFrame, initialFrame);
  EXPECT_FLOAT_EQ(NSMinY(finalFrame), NSMinY(initialFrame));
  EXPECT_FLOAT_EQ(NSHeight(finalFrame), NSHeight(initialFrame) + 40);
  [controller_ adjustWindowHeightBy:-40];
  finalFrame = [window frame];
  EXPECT_FLOAT_EQ(NSMinY(finalFrame), NSMinY(initialFrame));
  EXPECT_FLOAT_EQ(NSHeight(finalFrame), NSHeight(initialFrame));

  // Put the window slightly offscreen and try again.  The height should not
  // change this time.
  initialFrame = NSMakeRect(workarea.origin.x - 10, 0, 200, 280);
  [window setFrame:initialFrame display:YES];
  [controller_ resetWindowGrowthState];
  [controller_ adjustWindowHeightBy:40];
  EXPECT_NSEQ([window frame], initialFrame);
  [controller_ adjustWindowHeightBy:-40];
  EXPECT_NSEQ([window frame], initialFrame);

  // Make the window the same size as the workarea.  Resizing both larger and
  // smaller should have no effect.
  [window setFrame:workarea display:YES];
  [controller_ resetWindowGrowthState];
  [controller_ adjustWindowHeightBy:40];
  EXPECT_NSEQ([window frame], workarea);
  [controller_ adjustWindowHeightBy:-40];
  EXPECT_NSEQ([window frame], workarea);

  // Make the window smaller than the workarea and place it near the bottom of
  // the workarea.  The window should grow down until it hits the bottom and
  // then continue to grow up. Then shrink it, and it should return to where it
  // was.
  initialFrame = NSMakeRect(workarea.origin.x, workarea.origin.y + 5,
                            200, 280);
  [window setFrame:initialFrame display:YES];
  [controller_ resetWindowGrowthState];
  [controller_ adjustWindowHeightBy:40];
  finalFrame = [window frame];
  EXPECT_FLOAT_EQ(NSMinY(workarea), NSMinY(finalFrame));
  EXPECT_FLOAT_EQ(NSHeight(finalFrame), NSHeight(initialFrame) + 40);
  [controller_ adjustWindowHeightBy:-40];
  finalFrame = [window frame];
  EXPECT_FLOAT_EQ(NSMinY(initialFrame), NSMinY(finalFrame));
  EXPECT_FLOAT_EQ(NSHeight(initialFrame), NSHeight(finalFrame));

  // Inset the window slightly from the workarea.  It should not grow to be
  // larger than the workarea. Shrink it; it should return to where it started.
  initialFrame = NSInsetRect(workarea, 0, 5);
  [window setFrame:initialFrame display:YES];
  [controller_ resetWindowGrowthState];
  [controller_ adjustWindowHeightBy:40];
  finalFrame = [window frame];
  EXPECT_FLOAT_EQ(NSMinY(workarea), NSMinY(finalFrame));
  EXPECT_FLOAT_EQ(NSHeight(workarea), NSHeight(finalFrame));
  [controller_ adjustWindowHeightBy:-40];
  finalFrame = [window frame];
  EXPECT_FLOAT_EQ(NSMinY(initialFrame), NSMinY(finalFrame));
  EXPECT_FLOAT_EQ(NSHeight(initialFrame), NSHeight(finalFrame));

  // Place the window at the bottom of the screen and grow; it should grow
  // upwards. Move the window off the bottom, then shrink. It should then shrink
  // from the bottom.
  initialFrame = NSMakeRect(workarea.origin.x, workarea.origin.y, 200, 280);
  [window setFrame:initialFrame display:YES];
  [controller_ resetWindowGrowthState];
  [controller_ adjustWindowHeightBy:40];
  finalFrame = [window frame];
  EXPECT_NSNE(finalFrame, initialFrame);
  EXPECT_FLOAT_EQ(NSMinY(finalFrame), NSMinY(initialFrame));
  EXPECT_FLOAT_EQ(NSHeight(finalFrame), NSHeight(initialFrame) + 40);
  NSPoint oldOrigin = initialFrame.origin;
  NSPoint newOrigin = NSMakePoint(oldOrigin.x, oldOrigin.y + 10);
  [window setFrameOrigin:newOrigin];
  initialFrame = [window frame];
  EXPECT_FLOAT_EQ(NSMinY(initialFrame), oldOrigin.y + 10);
  [controller_ adjustWindowHeightBy:-40];
  finalFrame = [window frame];
  EXPECT_FLOAT_EQ(NSMinY(finalFrame), NSMinY(initialFrame) + 40);
  EXPECT_FLOAT_EQ(NSHeight(finalFrame), NSHeight(initialFrame) - 40);

  // Do the "inset" test above, but using multiple calls to
  // |-adjustWindowHeightBy|; the result should be the same.
  initialFrame = NSInsetRect(workarea, 0, 5);
  [window setFrame:initialFrame display:YES];
  [controller_ resetWindowGrowthState];
  for (int i = 0; i < 8; i++)
    [controller_ adjustWindowHeightBy:5];
  finalFrame = [window frame];
  EXPECT_FLOAT_EQ(NSMinY(workarea), NSMinY(finalFrame));
  EXPECT_FLOAT_EQ(NSHeight(workarea), NSHeight(finalFrame));
  for (int i = 0; i < 8; i++)
    [controller_ adjustWindowHeightBy:-5];
  finalFrame = [window frame];
  EXPECT_FLOAT_EQ(NSMinY(initialFrame), NSMinY(finalFrame));
  EXPECT_FLOAT_EQ(NSHeight(initialFrame), NSHeight(finalFrame));
}

// Test to make sure resizing and relaying-out subviews works correctly.
TEST_F(BrowserWindowControllerTest, TestResizeViews) {
  TabStripView* tabstrip = [controller_ tabStripView];
  NSView* contentView = [[tabstrip window] contentView];
  NSView* toolbar = [controller_ toolbarView];
  NSView* infobar = [controller_ infoBarContainerView];

  // We need to muck with the views a bit to put us in a consistent state before
  // we start resizing.  In particular, we need to move the tab strip to be
  // immediately above the content area, since we layout views to be directly
  // under the tab strip.
  NSRect tabstripFrame = [tabstrip frame];
  tabstripFrame.origin.y = NSMaxY([contentView frame]);
  [tabstrip setFrame:tabstripFrame];

  // Make the download shelf and set its initial height to 0.
  [controller_ createAndAddDownloadShelf];
  NSView* download = [[controller_ downloadShelf] view];
  NSRect downloadFrame = [download frame];
  downloadFrame.size.height = 0;
  [download setFrame:downloadFrame];

  // Force a layout and check each view's frame.
  [controller_ layoutSubviews];
  CheckViewPositions(controller_);

  // Expand the infobar to 60px and recheck
  [controller_ resizeView:infobar newHeight:60];
  CheckViewPositions(controller_);

  // Expand the toolbar to 64px and recheck
  [controller_ resizeView:toolbar newHeight:64];
  CheckViewPositions(controller_);

  // Add a 30px download shelf and recheck
  [controller_ resizeView:download newHeight:30];
  CheckViewPositions(controller_);

  // Shrink the infobar to 0px and toolbar to 39px and recheck
  [controller_ resizeView:infobar newHeight:0];
  [controller_ resizeView:toolbar newHeight:39];
  CheckViewPositions(controller_);
}

TEST_F(BrowserWindowControllerTest, TestResizeViewsWithBookmarkBar) {
  // Force a display of the bookmark bar.
  profile()->GetPrefs()->SetBoolean(bookmarks::prefs::kShowBookmarkBar, true);
  [controller_ browserWindow]->BookmarkBarStateChanged(
      BookmarkBar::DONT_ANIMATE_STATE_CHANGE);

  TabStripView* tabstrip = [controller_ tabStripView];
  NSView* contentView = [[tabstrip window] contentView];
  NSView* toolbar = [controller_ toolbarView];
  NSView* bookmark = [controller_ bookmarkView];
  NSView* infobar = [controller_ infoBarContainerView];

  // We need to muck with the views a bit to put us in a consistent state before
  // we start resizing.  In particular, we need to move the tab strip to be
  // immediately above the content area, since we layout views to be directly
  // under the tab strip.
  NSRect tabstripFrame = [tabstrip frame];
  tabstripFrame.origin.y = NSMaxY([contentView frame]);
  [tabstrip setFrame:tabstripFrame];

  // The download shelf is created lazily.  Force-create it and set its initial
  // height to 0.
  [controller_ createAndAddDownloadShelf];
  NSView* download = [[controller_ downloadShelf] view];
  NSRect downloadFrame = [download frame];
  downloadFrame.size.height = 0;
  [download setFrame:downloadFrame];

  // Force a layout and check each view's frame.
  [controller_ layoutSubviews];
  CheckViewPositions(controller_);

  // Add the bookmark bar and recheck.
  [controller_ resizeView:bookmark newHeight:40];
  CheckViewPositions(controller_);

  // Expand the infobar to 60px and recheck
  [controller_ resizeView:infobar newHeight:60];
  CheckViewPositions(controller_);

  // Expand the toolbar to 64px and recheck
  [controller_ resizeView:toolbar newHeight:64];
  CheckViewPositions(controller_);

  // Add a 30px download shelf and recheck
  [controller_ resizeView:download newHeight:30];
  CheckViewPositions(controller_);

  // Remove the bookmark bar and recheck
  profile()->GetPrefs()->SetBoolean(bookmarks::prefs::kShowBookmarkBar, false);
  [controller_ browserWindow]->BookmarkBarStateChanged(
      BookmarkBar::DONT_ANIMATE_STATE_CHANGE);
  [controller_ resizeView:bookmark newHeight:0];
  CheckViewPositions(controller_);

  // Shrink the infobar to 0px and toolbar to 39px and recheck
  [controller_ resizeView:infobar newHeight:0];
  [controller_ resizeView:toolbar newHeight:39];
  CheckViewPositions(controller_);
}

// Make sure, by default, the bookmark bar and the toolbar are the same width.
TEST_F(BrowserWindowControllerTest, BookmarkBarIsSameWidth) {
  // Set the pref to the bookmark bar is visible when the toolbar is
  // first created.
  profile()->GetPrefs()->SetBoolean(bookmarks::prefs::kShowBookmarkBar, true);

  // Make sure the bookmark bar is the same width as the toolbar
  NSView* bookmarkBarView = [controller_ bookmarkView];
  NSView* toolbarView = [controller_ toolbarView];
  EXPECT_EQ([toolbarView frame].size.width,
            [bookmarkBarView frame].size.width);
}

TEST_F(BrowserWindowControllerTest, TestTopRightForBubble) {
  // The bookmark bubble must be attached to a lit and visible star.
  [controller_ setStarredState:YES];
  NSPoint p = [controller_ bookmarkBubblePoint];  // Window coordinates.
  NSRect all = [[controller_ window] frame];      // Screen coordinates.

  // As a sanity check make sure the point is vaguely in the top right
  // of the window.
  EXPECT_GT(p.y, all.size.height / 2);
  EXPECT_GT(p.x, all.size.width / 2);
}

// By the "zoom frame", we mean what Apple calls the "standard frame".
TEST_F(BrowserWindowControllerTest, TestZoomFrame) {
  NSWindow* window = [controller_ window];
  ASSERT_TRUE(window);
  NSRect screenFrame = [[window screen] visibleFrame];
  ASSERT_FALSE(NSIsEmptyRect(screenFrame));

  // Minimum zoomed width is the larger of 60% of available horizontal space or
  // 60% of available vertical space, subject to available horizontal space.
  CGFloat minZoomWidth =
      std::min(std::max((CGFloat)0.6 * screenFrame.size.width,
                        (CGFloat)0.6 * screenFrame.size.height),
               screenFrame.size.width);

  // |testFrame| is the size of the window we start out with, and |zoomFrame| is
  // the one returned by |-windowWillUseStandardFrame:defaultFrame:|.
  NSRect testFrame;
  NSRect zoomFrame;

  // 1. Test a case where it zooms the window both horizontally and vertically,
  // and only moves it vertically. "+ 32", etc. are just arbitrary constants
  // used to check that the window is moved properly and not just to the origin;
  // they should be small enough to not shove windows off the screen.
  testFrame.size.width = 0.5 * minZoomWidth;
  testFrame.size.height = 0.5 * screenFrame.size.height;
  testFrame.origin.x = screenFrame.origin.x + 32;  // See above.
  testFrame.origin.y = screenFrame.origin.y + 23;
  [window setFrame:testFrame display:NO];
  zoomFrame = [controller_ windowWillUseStandardFrame:window
                                         defaultFrame:screenFrame];
  EXPECT_LE(minZoomWidth, zoomFrame.size.width);
  EXPECT_EQ(screenFrame.size.height, zoomFrame.size.height);
  EXPECT_EQ(testFrame.origin.x, zoomFrame.origin.x);
  EXPECT_EQ(screenFrame.origin.y, zoomFrame.origin.y);

  // 2. Test a case where it zooms the window only horizontally, and only moves
  // it horizontally.
  testFrame.size.width = 0.5 * minZoomWidth;
  testFrame.size.height = screenFrame.size.height;
  testFrame.origin.x = screenFrame.origin.x + screenFrame.size.width -
                       testFrame.size.width;
  testFrame.origin.y = screenFrame.origin.y;
  [window setFrame:testFrame display:NO];
  zoomFrame = [controller_ windowWillUseStandardFrame:window
                                         defaultFrame:screenFrame];
  EXPECT_LE(minZoomWidth, zoomFrame.size.width);
  EXPECT_EQ(screenFrame.size.height, zoomFrame.size.height);
  EXPECT_EQ(screenFrame.origin.x + screenFrame.size.width -
            zoomFrame.size.width, zoomFrame.origin.x);
  EXPECT_EQ(screenFrame.origin.y, zoomFrame.origin.y);

  // 3. Test a case where it zooms the window only vertically, and only moves it
  // vertically.
  testFrame.size.width = std::min((CGFloat)1.1 * minZoomWidth,
                                  screenFrame.size.width);
  testFrame.size.height = 0.3 * screenFrame.size.height;
  testFrame.origin.x = screenFrame.origin.x + 32;  // See above (in 1.).
  testFrame.origin.y = screenFrame.origin.y + 123;
  [window setFrame:testFrame display:NO];
  zoomFrame = [controller_ windowWillUseStandardFrame:window
                                         defaultFrame:screenFrame];
  // Use the actual width of the window frame, since it's subject to rounding.
  EXPECT_EQ([window frame].size.width, zoomFrame.size.width);
  EXPECT_EQ(screenFrame.size.height, zoomFrame.size.height);
  EXPECT_EQ(testFrame.origin.x, zoomFrame.origin.x);
  EXPECT_EQ(screenFrame.origin.y, zoomFrame.origin.y);

  // 4. Test a case where zooming should do nothing (i.e., we're already at a
  // zoomed frame).
  testFrame.size.width = std::min((CGFloat)1.1 * minZoomWidth,
                                  screenFrame.size.width);
  testFrame.size.height = screenFrame.size.height;
  testFrame.origin.x = screenFrame.origin.x + 32;  // See above (in 1.).
  testFrame.origin.y = screenFrame.origin.y;
  [window setFrame:testFrame display:NO];
  zoomFrame = [controller_ windowWillUseStandardFrame:window
                                         defaultFrame:screenFrame];
  // Use the actual width of the window frame, since it's subject to rounding.
  EXPECT_EQ([window frame].size.width, zoomFrame.size.width);
  EXPECT_EQ(screenFrame.size.height, zoomFrame.size.height);
  EXPECT_EQ(testFrame.origin.x, zoomFrame.origin.x);
  EXPECT_EQ(screenFrame.origin.y, zoomFrame.origin.y);
}

TEST_F(BrowserWindowControllerTest, TestFindBarOnTop) {
  FindBarBridge bridge(NULL);
  [controller_ addFindBar:bridge.find_bar_cocoa_controller()];

  // Test that the Z-order of the find bar is on top of everything.
  NSArray* subviews = [controller_.chromeContentView subviews];
  NSUInteger findBar_index =
      [subviews indexOfObject:[controller_ findBarView]];
  EXPECT_NE(static_cast<NSUInteger>(NSNotFound), findBar_index);
  NSUInteger toolbar_index =
      [subviews indexOfObject:[controller_ toolbarView]];
  EXPECT_NE(static_cast<NSUInteger>(NSNotFound), toolbar_index);
  NSUInteger bookmark_index =
      [subviews indexOfObject:[controller_ bookmarkView]];
  EXPECT_NE(static_cast<NSUInteger>(NSNotFound), bookmark_index);

  EXPECT_GT(findBar_index, toolbar_index);
  EXPECT_GT(findBar_index, bookmark_index);
}

// Verify that hit testing works correctly when the bookmark bar overlaps
// web contents.
TEST_F(BrowserWindowControllerTest, BookmarkBarHitTest) {
  profile()->GetPrefs()->SetBoolean(bookmarks::prefs::kShowBookmarkBar, true);
  [controller_ browserWindow]->BookmarkBarStateChanged(
      BookmarkBar::DONT_ANIMATE_STATE_CHANGE);

  NSView* bookmarkView = [controller_ bookmarkView];
  NSView* contentView = [[controller_ window] contentView];
  NSPoint point = [bookmarkView convertPoint:NSMakePoint(1, 1)
                                      toView:[contentView superview]];

  EXPECT_TRUE([[contentView hitTest:point] isDescendantOf:bookmarkView]);
}

// Check that when the window becomes/resigns main, the tab strip's background
// view is redrawn.
TEST_F(BrowserWindowControllerTest, TabStripBackgroundViewRedrawTest) {
  NSView* view = controller_.tabStripBackgroundView;
  id partial_mock = [OCMockObject partialMockForObject:view];

  [[partial_mock expect] setNeedsDisplay:YES];
  [[NSNotificationCenter defaultCenter]
      postNotificationName:NSWindowDidBecomeMainNotification
                    object:controller_.window];
  [partial_mock verify];

  [[partial_mock expect] setNeedsDisplay:YES];
  [[NSNotificationCenter defaultCenter]
      postNotificationName:NSWindowDidResignMainNotification
                    object:controller_.window];
  [partial_mock verify];
}

// Test that the window uses Auto Layout. Since frame-based layout and Auto
// Layout behave differently in subtle ways, we shouldn't start/stop using it
// accidentally. If we don't want Auto Layout, this test should be changed to
// expect that chromeContentView has no constraints.
TEST_F(BrowserWindowControllerTest, UsesAutoLayout) {
  // If Auto Layout is on, there will be synthesized constraints based on the
  // view's frame and autoresizing mask.
  EXPECT_EQ(0u, [[[controller_ chromeContentView] constraints] count]);
}

@interface BrowserWindowControllerFakeFullscreen : BrowserWindowController {
 @private
  // We release the window ourselves, so we don't have to rely on the unittest
  // doing it for us.
  base::scoped_nsobject<NSWindow> testFullscreenWindow_;
}
@end

class BrowserWindowFullScreenControllerTest : public CocoaProfileTest {
 public:
  void SetUp() override {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(browser());

    controller_ =
        [[BrowserWindowControllerFakeFullscreen alloc] initWithBrowser:browser()
                                                         takeOwnership:NO];
  }

  void TearDown() override {
    [[controller_ nsWindowController] close];
    CocoaProfileTest::TearDown();
  }

 public:
  BrowserWindowController* controller_;
};

// Check if the window is front most or if one of its child windows (such
// as a status bubble) is front most.
static bool IsFrontWindow(NSWindow* window) {
  NSWindow* frontmostWindow = [[NSApp orderedWindows] objectAtIndex:0];
  return [frontmostWindow isEqual:window] ||
         [[frontmostWindow parentWindow] isEqual:window];
}

void WaitForFullScreenTransition() {
  content::WindowedNotificationObserver observer(
      chrome::NOTIFICATION_FULLSCREEN_CHANGED,
      content::NotificationService::AllSources());
  observer.Wait();
}

// http://crbug.com/53586
TEST_F(BrowserWindowFullScreenControllerTest, TestFullscreen) {
  ui::test::ScopedFakeNSWindowFullscreen fake_fullscreen;
  [[controller_ nsWindowController] showWindow:nil];
  EXPECT_FALSE([controller_ isInAnyFullscreenMode]);

  // The fix for http://crbug.com/447740 , where the omnibox would lose focus
  // when switching between normal and fullscreen modes, makes some changes to
  // -[BrowserWindowController setContentViewSubviews:]. Those changes appear
  // to have extended the lifetime of the browser window during this test -
  // specifically, the browser window is no longer visible, but it has not been
  // fully freed (possibly being kept around by a reference from the
  // autocompleteTextView). As a result the window still appears in
  // -[NSApplication windows] and causes the test to fail. To get around this
  // problem, I disable -[BrowserWindowController focusLocationBar:] and later
  // force the window to clear its first responder.
  base::mac::ScopedObjCClassSwizzler tmpSwizzler(
      [BrowserWindowController class], @selector(focusLocationBar:),
      @selector(dontFocusLocationBar:));

  [controller_ enterBrowserFullscreen];
  WaitForFullScreenTransition();
  EXPECT_TRUE([controller_ isInAnyFullscreenMode]);

  [controller_ exitAnyFullscreen];
  WaitForFullScreenTransition();
  EXPECT_FALSE([controller_ isInAnyFullscreenMode]);

  [[controller_ window] makeFirstResponder:nil];
}

// If this test fails, it is usually a sign that the bots have some sort of
// problem (such as a modal dialog up).  This tests is a very useful canary, so
// please do not mark it as flaky without first verifying that there are no bot
// problems.
// http://crbug.com/53586
TEST_F(BrowserWindowFullScreenControllerTest, TestActivate) {
  ui::test::ScopedFakeNSWindowFullscreen fake_fullscreen;
  [[controller_ nsWindowController] showWindow:nil];

  EXPECT_FALSE([controller_ isInAnyFullscreenMode]);

  [controller_ activate];
  chrome::testing::NSRunLoopRunAllPending();
  EXPECT_TRUE(IsFrontWindow([controller_ window]));

  // See the comment in TestFullscreen for an explanation of this
  // swizzling and the makeFirstResponder:nil call below.
  base::mac::ScopedObjCClassSwizzler tmpSwizzler(
      [BrowserWindowController class], @selector(focusLocationBar:),
      @selector(dontFocusLocationBar:));

  [controller_ enterBrowserFullscreen];
  WaitForFullScreenTransition();
  [controller_ activate];
  chrome::testing::NSRunLoopRunAllPending();

  // We have to cleanup after ourselves by unfullscreening.
  [controller_ exitAnyFullscreen];
  WaitForFullScreenTransition();

  [[controller_ window] makeFirstResponder:nil];
}

@implementation BrowserWindowControllerFakeFullscreen
// Override |-createFullscreenWindow| to return a dummy window. This isn't
// needed to pass the test, but because the dummy window is only 100x100, it
// prevents the real fullscreen window from flashing up and taking over the
// whole screen. We have to return an actual window because |-layoutSubviews|
// looks at the window's frame.
- (NSWindow*)createFullscreenWindow {
  if (testFullscreenWindow_.get())
    return testFullscreenWindow_.get();

  testFullscreenWindow_.reset(
      [[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,400,400)
                                  styleMask:NSBorderlessWindowMask
                                    backing:NSBackingStoreBuffered
                                      defer:NO]);
  [[testFullscreenWindow_ contentView] setWantsLayer:YES];
  return testFullscreenWindow_.get();
}
@end

/* TODO(???): test other methods of BrowserWindowController */
