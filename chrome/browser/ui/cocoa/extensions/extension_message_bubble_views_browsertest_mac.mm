// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "build/buildflag.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/extensions/browser_action_button.h"
#import "chrome/browser/ui/cocoa/extensions/browser_actions_controller.h"
#import "chrome/browser/ui/cocoa/extensions/toolbar_actions_bar_bubble_views_presenter.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_controller.h"
#include "chrome/browser/ui/extensions/extension_message_bubble_browsertest.h"
#include "chrome/browser/ui/views/toolbar/toolbar_actions_bar_bubble_views.h"
#include "ui/base/ui_features.h"
#import "ui/gfx/mac/coordinate_conversion.h"

namespace {

// Returns the ToolbarController for the given browser.
ToolbarController* ToolbarControllerForBrowser(Browser* browser) {
  return [[BrowserWindowController
      browserWindowControllerForWindow:browser->window()->GetNativeWindow()]
      toolbarController];
}

}  // namespace

@interface BrowserActionsController (ViewsTestingAPI)
- (ToolbarActionsBarBubbleViewsPresenter*)presenter;
@end

@implementation BrowserActionsController (ViewsTestingAPI)
- (ToolbarActionsBarBubbleViewsPresenter*)presenter {
  return viewsBubblePresenter_.get();
}
@end

namespace test {

class ToolbarActionsBarBubbleViewsPresenterTestApi {
 public:
  static ToolbarActionsBarBubbleViews* GetBubble(
      ToolbarActionsBarBubbleViewsPresenter* presenter) {
    return presenter->active_bubble_;
  }
};

}  // namespace test

// static
ToolbarActionsBarBubbleViews*
ExtensionMessageBubbleBrowserTest::GetViewsBubbleForCocoaBrowser(
    Browser* browser) {
  ToolbarController* toolbarController = ToolbarControllerForBrowser(browser);
  BrowserActionsController* actionsController =
      [toolbarController browserActionsController];
  return test::ToolbarActionsBarBubbleViewsPresenterTestApi::GetBubble(
      [actionsController presenter]);
}

#if !BUILDFLAG(MAC_VIEWS_BROWSER)
// static
ToolbarActionsBarBubbleViews*
ExtensionMessageBubbleBrowserTest::GetViewsBubbleForBrowser(Browser* browser) {
  return GetViewsBubbleForCocoaBrowser(browser);
}
#endif

// static
gfx::Rect
ExtensionMessageBubbleBrowserTest::GetAnchorReferenceBoundsForCocoaBrowser(
    Browser* browser,
    AnchorPosition anchor) {
  ToolbarController* toolbarController = ToolbarControllerForBrowser(browser);
  BrowserActionsController* actionsController =
      [toolbarController browserActionsController];
  NSView* anchor_view = nil;
  switch (anchor) {
    case ExtensionMessageBubbleBrowserTest::ANCHOR_BROWSER_ACTION:
      anchor_view = [actionsController buttonWithIndex:0];
      break;
    case ExtensionMessageBubbleBrowserTest::ANCHOR_APP_MENU:
      anchor_view = [toolbarController appMenuButton];
      break;
  }
  EXPECT_TRUE(anchor_view);
  NSWindow* parent_window = [anchor_view window];

  ToolbarActionsBarBubbleViews* bubble = GetViewsBubbleForBrowser(browser);
  EXPECT_TRUE(bubble);
  EXPECT_EQ([parent_window contentView], bubble->parent_window());

  NSRect anchor_bounds_in_window =
      [anchor_view convertRect:[anchor_view bounds] toView:nil];
  NSRect reference_bounds_in_screen =
      [parent_window convertRectToScreen:anchor_bounds_in_window];

  return gfx::ScreenRectFromNSRect(reference_bounds_in_screen);
}

#if !BUILDFLAG(MAC_VIEWS_BROWSER)
// static
gfx::Rect ExtensionMessageBubbleBrowserTest::GetAnchorReferenceBoundsForBrowser(
    Browser* browser,
    AnchorPosition anchor) {
  return GetAnchorReferenceBoundsForCocoaBrowser(browser, anchor);
}
#endif
