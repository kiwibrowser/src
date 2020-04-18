// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>
#include <stddef.h>

#import "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#import "chrome/browser/ui/cocoa/tabs/alert_indicator_button_cocoa.h"
#import "chrome/browser/ui/cocoa/tabs/tab_controller.h"
#import "chrome/browser/ui/cocoa/tabs/tab_controller_target.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_drag_controller.h"
#import "chrome/browser/ui/cocoa/tabs/tab_view.h"
#include "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "chrome/browser/ui/cocoa/test/scoped_force_rtl_mac.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "ui/base/resource/resource_bundle.h"
#import "ui/base/test/menu_test_observer.h"
#import "ui/events/test/cocoa_test_event_utils.h"
#include "ui/resources/grit/ui_resources.h"

// Implements the target interface for the tab, which gets sent messages when
// the tab is clicked on by the user and when its close box is clicked.
@interface TabControllerTestTarget : NSObject<TabControllerTarget> {
 @private
  bool selected_;
  bool closed_;
  base::scoped_nsobject<TabStripDragController> dragController_;
}
- (bool)selected;
- (bool)closed;
@end

@implementation TabControllerTestTarget
- (id)init {
  if ((self = [super init])) {
    dragController_.reset(
        [[TabStripDragController alloc] initWithTabStripController:nil]);
  }
  return self;
}
- (bool)selected {
  return selected_;
}
- (bool)closed {
  return closed_;
}
- (void)selectTab:(id)sender {
  selected_ = true;
}
- (void)closeTab:(id)sender {
  closed_ = true;
}
- (void)mouseTimer:(NSTimer*)timer {
  // Fire the mouseUp to break the TabView drag loop.
  NSEvent* current = [NSApp currentEvent];
  NSWindow* window = [timer userInfo];
  NSEvent* up = [NSEvent mouseEventWithType:NSLeftMouseUp
                                   location:[current locationInWindow]
                              modifierFlags:0
                                  timestamp:[current timestamp]
                               windowNumber:[window windowNumber]
                                    context:nil
                                eventNumber:0
                                 clickCount:1
                                   pressure:1.0];
  [window postEvent:up atStart:YES];
}
- (void)commandDispatch:(TabStripModel::ContextMenuCommand)command
          forController:(TabController*)controller {
}
- (BOOL)isCommandEnabled:(TabStripModel::ContextMenuCommand)command
           forController:(TabController*)controller {
  return NO;
}
- (ui::SimpleMenuModel*)contextMenuModelForController:(TabController*)controller
    menuDelegate:(ui::SimpleMenuModel::Delegate*)delegate {
  ui::SimpleMenuModel* model = new ui::SimpleMenuModel(delegate);
  model->AddItem(1, base::ASCIIToUTF16("Hello World"));
  model->AddItem(2, base::ASCIIToUTF16("Allays"));
  model->AddItem(3, base::ASCIIToUTF16("Chromium"));
  return model;
}
- (id<TabDraggingEventTarget>)dragController {
  return dragController_.get();
}
@end

namespace {

CGFloat LeftMargin(NSRect superFrame, NSRect subFrame) {
  return NSMinX(subFrame) - NSMinX(superFrame);
}

CGFloat RightMargin(NSRect superFrame, NSRect subFrame) {
  return NSMaxX(superFrame) - NSMaxX(subFrame);
}

// The dragging code in TabView makes heavy use of autorelease pools so
// inherit from CocoaTest to have one created for us.
class TabControllerTest : public CocoaTest {
 public:
  TabControllerTest() { }

 protected:
  void CheckLayoutAndVisibilityOfSubviewsForAllStates(bool is_rtl) {
    static const TabAlertState kAlertStatesToTest[] = {
        TabAlertState::NONE, TabAlertState::TAB_CAPTURING,
        TabAlertState::AUDIO_PLAYING, TabAlertState::AUDIO_MUTING};

    NSWindow* const window = test_window();

    // Create TabController instance and place its view into the test window.
    base::scoped_nsobject<TabController> controller(
        [[TabController alloc] init]);
    [[window contentView] addSubview:[controller view]];

    // Create favicon.
    ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    base::scoped_nsobject<NSImage> favicon(
        rb.GetNativeImageNamed(IDR_DEFAULT_FAVICON).CopyNSImage());

    // Trigger TabController to auto-create the AlertIndicatorButton.
    [controller setAlertState:TabAlertState::AUDIO_PLAYING];
    [controller setAlertState:TabAlertState::NONE];
    base::scoped_nsobject<AlertIndicatorButton> alertIndicatorButton(
        [[controller alertIndicatorButton] retain]);
    ASSERT_TRUE(alertIndicatorButton.get());

    // Perform layout over all possible combinations, checking for correct
    // results.
    for (int isPinnedTab = 0; isPinnedTab < 2; ++isPinnedTab) {
      for (int isActiveTab = 0; isActiveTab < 2; ++isActiveTab) {
        for (size_t alertStateIndex = 0;
             alertStateIndex < arraysize(kAlertStatesToTest);
             ++alertStateIndex) {
          const TabAlertState alertState = kAlertStatesToTest[alertStateIndex];
          SCOPED_TRACE(::testing::Message()
                       << (isActiveTab ? "Active" : "Inactive") << ' '
                       << (isPinnedTab ? "Pinned " : "")
                       << "Tab with alert indicator state "
                       << static_cast<uint8_t>(alertState));

          // Simulate what tab_strip_controller would do to set up the
          // TabController state.
          [controller setPinned:(isPinnedTab ? YES : NO)];
          [controller setActive:(isActiveTab ? YES : NO)];
          [controller setIconImage:favicon
                   forLoadingState:kTabDone
                          showIcon:YES];
          [controller setAlertState:alertState];
          [controller updateVisibility];

          // Test layout for every width from maximum to minimum.
          NSRect tabFrame = [[controller view] frame];
          int minWidth;
          if (isPinnedTab) {
            tabFrame.size.width = minWidth = [TabController pinnedTabWidth];
          } else {
            tabFrame.size.width = [TabController maxTabWidth];
            minWidth = isActiveTab ? [TabController minActiveTabWidth]
                                   : [TabController minTabWidth];
          }
          while (NSWidth(tabFrame) >= minWidth) {
            SCOPED_TRACE(::testing::Message() << "width="
                                              << tabFrame.size.width);
            [[controller view] setFrame:tabFrame];
            if (is_rtl)
              CheckForExpectedLayoutAndVisibilityOfSubviewsRTL(controller);
            else
              CheckForExpectedLayoutAndVisibilityOfSubviews(controller);
            --tabFrame.size.width;
          }
        }
      }
    }
  }

 private:
  static void CheckForExpectedLayoutAndVisibilityOfSubviews(
      const TabController* controller) {
    CheckVisibilityOfSubviews(controller);

    // Check positioning of elements with respect to each other, and that they
    // are fully within the tab frame.
    const NSRect tabFrame = [[controller view] frame];
    const NSRect titleFrame = [[controller tabView] titleFrame];
    if ([controller shouldShowIcon]) {
      const NSRect iconFrame = [[controller iconView] frame];
      EXPECT_LE(NSMinX(tabFrame), NSMinX(iconFrame));
      if (NSWidth(titleFrame) > 0)
        EXPECT_LE(NSMaxX(iconFrame), NSMinX(titleFrame));
      EXPECT_LE(NSMinY(tabFrame), NSMinY(iconFrame));
      EXPECT_LE(NSMaxY(iconFrame), NSMaxY(tabFrame));
    }
    if ([controller shouldShowIcon] && [controller shouldShowAlertIndicator]) {
      EXPECT_LE(NSMaxX([[controller iconView] frame]),
                NSMinX([[controller alertIndicatorButton] frame]));
    }
    if ([controller shouldShowAlertIndicator]) {
      const NSRect alertIndicatorFrame =
          [[controller alertIndicatorButton] frame];
      if (NSWidth(titleFrame) > 0)
        EXPECT_LE(NSMaxX(titleFrame), NSMinX(alertIndicatorFrame));
      EXPECT_LE(NSMaxX(alertIndicatorFrame), NSMaxX(tabFrame));
      EXPECT_LE(NSMinY(tabFrame), NSMinY(alertIndicatorFrame));
      EXPECT_LE(NSMaxY(alertIndicatorFrame), NSMaxY(tabFrame));
    }
    if ([controller shouldShowAlertIndicator] &&
        [controller shouldShowCloseButton]) {
      EXPECT_LE(NSMaxX([[controller alertIndicatorButton] frame]),
                NSMinX([[controller closeButton] frame]));
    }
    if ([controller shouldShowCloseButton]) {
      const NSRect closeButtonFrame = [[controller closeButton] frame];
      if (NSWidth(titleFrame) > 0)
        EXPECT_LE(NSMaxX(titleFrame), NSMinX(closeButtonFrame));
      EXPECT_LE(NSMaxX(closeButtonFrame), NSMaxX(tabFrame));
      EXPECT_LE(NSMinY(tabFrame), NSMinY(closeButtonFrame));
      EXPECT_LE(NSMaxY(closeButtonFrame), NSMaxY(tabFrame));
    }
  }
  static void CheckForExpectedLayoutAndVisibilityOfSubviewsRTL(
      const TabController* controller) {
    CheckVisibilityOfSubviews(controller);
    // Check positioning of elements with respect to each other, and that they
    // are fully within the tab frame.
    const NSRect tabFrame = [[controller view] frame];
    const NSRect titleFrame = [[controller tabView] titleFrame];
    if ([controller shouldShowCloseButton]) {
      const NSRect closeButtonFrame = [[controller closeButton] frame];
      EXPECT_TRUE(NSContainsRect(tabFrame, closeButtonFrame));
      if (NSWidth(titleFrame) > 0)
        EXPECT_LE(NSMaxX(closeButtonFrame), NSMinX(titleFrame));
    }
    if ([controller shouldShowCloseButton] &&
        [controller shouldShowAlertIndicator]) {
      EXPECT_LE(NSMaxX([[controller closeButton] frame]),
                NSMinX([[controller alertIndicatorButton] frame]));
    }
    if ([controller shouldShowAlertIndicator]) {
      const NSRect alertIndicatorFrame =
          [[controller alertIndicatorButton] frame];
      EXPECT_TRUE(NSContainsRect(tabFrame, alertIndicatorFrame));
      if (NSWidth(titleFrame) > 0)
        EXPECT_LE(NSMaxX(alertIndicatorFrame), NSMinX(titleFrame));
    }
    if ([controller shouldShowAlertIndicator] && [controller shouldShowIcon]) {
      EXPECT_LE(NSMaxX([[controller alertIndicatorButton] frame]),
                NSMinX([[controller iconView] frame]));
    }
    if ([controller shouldShowIcon]) {
      const NSRect iconFrame = [[controller iconView] frame];
      EXPECT_TRUE(NSContainsRect(tabFrame, iconFrame));
      if (NSWidth(titleFrame) > 0)
        EXPECT_LE(NSMaxX(titleFrame), NSMinX(iconFrame));
    }
  }
  // Common for RTL and LTR
  static void CheckVisibilityOfSubviews(const TabController* controller) {
    // Check whether subviews should be visible when they are supposed to be,
    // given Tab size and TabRendererData state.
    const TabAlertState indicatorState =
        [[controller alertIndicatorButton] showingAlertState];
    if ([controller pinned]) {
      EXPECT_EQ(1, [controller iconCapacity]);
      if (indicatorState != TabAlertState::NONE) {
        EXPECT_FALSE([controller shouldShowIcon]);
        EXPECT_TRUE([controller shouldShowAlertIndicator]);
      } else {
        EXPECT_TRUE([controller shouldShowIcon]);
        EXPECT_FALSE([controller shouldShowAlertIndicator]);
      }
      EXPECT_FALSE([controller shouldShowCloseButton]);
    } else if ([controller selected]) {
      EXPECT_TRUE([controller shouldShowCloseButton]);
      switch ([controller iconCapacity]) {
        case 0:
        case 1:
          EXPECT_FALSE([controller shouldShowIcon]);
          EXPECT_FALSE([controller shouldShowAlertIndicator]);
          break;
        case 2:
          if (indicatorState != TabAlertState::NONE) {
            EXPECT_FALSE([controller shouldShowIcon]);
            EXPECT_TRUE([controller shouldShowAlertIndicator]);
          } else {
            EXPECT_TRUE([controller shouldShowIcon]);
            EXPECT_FALSE([controller shouldShowAlertIndicator]);
          }
          break;
        default:
          EXPECT_LE(3, [controller iconCapacity]);
          EXPECT_TRUE([controller shouldShowIcon]);
          if (indicatorState != TabAlertState::NONE)
            EXPECT_TRUE([controller shouldShowAlertIndicator]);
          else
            EXPECT_FALSE([controller shouldShowAlertIndicator]);
          break;
      }
    } else {  // Tab not selected/active and not pinned tab.
      switch ([controller iconCapacity]) {
        case 0:
          EXPECT_FALSE([controller shouldShowCloseButton]);
          EXPECT_FALSE([controller shouldShowIcon]);
          EXPECT_FALSE([controller shouldShowAlertIndicator]);
          break;
        case 1:
          EXPECT_FALSE([controller shouldShowCloseButton]);
          if (indicatorState != TabAlertState::NONE) {
            EXPECT_FALSE([controller shouldShowIcon]);
            EXPECT_TRUE([controller shouldShowAlertIndicator]);
          } else {
            EXPECT_TRUE([controller shouldShowIcon]);
            EXPECT_FALSE([controller shouldShowAlertIndicator]);
          }
          break;
        default:
          EXPECT_LE(2, [controller iconCapacity]);
          EXPECT_TRUE([controller shouldShowIcon]);
          if (indicatorState != TabAlertState::NONE)
            EXPECT_TRUE([controller shouldShowAlertIndicator]);
          else
            EXPECT_FALSE([controller shouldShowAlertIndicator]);
          break;
      }
    }
  }

  base::MessageLoop message_loop_;
};

// Tests creating the controller, sticking it in a window, and removing it.
TEST_F(TabControllerTest, Creation) {
  NSWindow* window = test_window();
  base::scoped_nsobject<TabController> controller([[TabController alloc] init]);
  [[window contentView] addSubview:[controller view]];
  EXPECT_TRUE([controller tabView]);
  EXPECT_EQ([[controller view] window], window);
  [[controller view] display];  // Test drawing to ensure nothing leaks/crashes.
  [[controller view] removeFromSuperview];
}

// Tests sending it a close message and ensuring that the target/action get
// called. Mimics the user clicking on the close button in the tab.
TEST_F(TabControllerTest, Close) {
  NSWindow* window = test_window();
  base::scoped_nsobject<TabController> controller([[TabController alloc] init]);
  [[window contentView] addSubview:[controller view]];

  base::scoped_nsobject<TabControllerTestTarget> target(
      [[TabControllerTestTarget alloc] init]);
  EXPECT_FALSE([target closed]);
  [controller setTarget:target];
  EXPECT_EQ(target.get(), [controller target]);

  [controller closeTab:nil];
  EXPECT_TRUE([target closed]);

  [[controller view] removeFromSuperview];
}

// Tests setting the |selected| property via code.
TEST_F(TabControllerTest, APISelection) {
  NSWindow* window = test_window();
  base::scoped_nsobject<TabController> controller([[TabController alloc] init]);
  [[window contentView] addSubview:[controller view]];

  EXPECT_FALSE([controller selected]);
  [controller setSelected:YES];
  EXPECT_TRUE([controller selected]);

  [[controller view] removeFromSuperview];
}

// Tests setting the |loading| property via code.
TEST_F(TabControllerTest, Loading) {
  NSWindow* window = test_window();
  base::scoped_nsobject<TabController> controller([[TabController alloc] init]);
  [[window contentView] addSubview:[controller view]];

  EXPECT_EQ(kTabDone, [controller loadingState]);
  [controller setIconImage:nil forLoadingState:kTabWaiting showIcon:YES];
  EXPECT_EQ(kTabWaiting, [controller loadingState]);
  [controller setIconImage:nil forLoadingState:kTabLoading showIcon:YES];
  EXPECT_EQ(kTabLoading, [controller loadingState]);
  // Create favicon.
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  base::scoped_nsobject<NSImage> favicon(
      rb.GetNativeImageNamed(IDR_DEFAULT_FAVICON).CopyNSImage());
  [controller setIconImage:favicon forLoadingState:kTabDone showIcon:YES];
  EXPECT_EQ(kTabDone, [controller loadingState]);

  [[controller view] removeFromSuperview];
}

// Tests selecting the tab with the mouse click and ensuring the target/action
// get called.
TEST_F(TabControllerTest, UserSelection) {
  NSWindow* window = test_window();

  // Create a tab at a known location in the window that we can click on
  // to activate selection.
  base::scoped_nsobject<TabController> controller([[TabController alloc] init]);
  [[window contentView] addSubview:[controller view]];
  NSRect frame = [[controller view] frame];
  frame.size.width = [TabController minTabWidth];
  frame.origin = NSZeroPoint;
  [[controller view] setFrame:frame];

  // Set the target and action.
  base::scoped_nsobject<TabControllerTestTarget> target(
      [[TabControllerTestTarget alloc] init]);
  EXPECT_FALSE([target selected]);
  [controller setTarget:target];
  [controller setAction:@selector(selectTab:)];
  EXPECT_EQ(target.get(), [controller target]);
  EXPECT_EQ(@selector(selectTab:), [controller action]);

  // In order to track a click, we have to fake a mouse down and a mouse
  // up, but the down goes into a tight drag loop. To break the loop, we have
  // to fire a timer that sends a mouse up event while the "drag" is ongoing.
  [NSTimer scheduledTimerWithTimeInterval:0.1
                                   target:target.get()
                                 selector:@selector(mouseTimer:)
                                 userInfo:window
                                  repeats:NO];
  NSEvent* current = [NSApp currentEvent];
  NSPoint click_point = NSMakePoint(frame.size.width / 2,
                                    frame.size.height / 2);
  NSEvent* down = [NSEvent mouseEventWithType:NSLeftMouseDown
                                     location:click_point
                                modifierFlags:0
                                    timestamp:[current timestamp]
                                 windowNumber:[window windowNumber]
                                      context:nil
                                  eventNumber:0
                                   clickCount:1
                                     pressure:1.0];
  [[controller view] mouseDown:down];

  // Check our target was told the tab got selected.
  EXPECT_TRUE([target selected]);

  [[controller view] removeFromSuperview];
}

TEST_F(TabControllerTest, IconCapacity) {
  NSWindow* window = test_window();
  base::scoped_nsobject<TabController> controller([[TabController alloc] init]);
  [[window contentView] addSubview:[controller view]];
  int cap = [controller iconCapacity];
  EXPECT_GE(cap, 1);

  NSRect frame = [[controller view] frame];
  frame.size.width += 500;
  [[controller view] setFrame:frame];
  int newcap = [controller iconCapacity];
  EXPECT_GT(newcap, cap);
}

TEST_F(TabControllerTest, ShouldShowIcon) {
  NSWindow* window = test_window();
  base::scoped_nsobject<TabController> controller([[TabController alloc] init]);
  [[window contentView] addSubview:[controller view]];
  int cap = [controller iconCapacity];
  EXPECT_GT(cap, 0);

  // Tab is minimum width, both icon and close box should be hidden.
  NSRect frame = [[controller view] frame];
  frame.size.width = [TabController minTabWidth];
  [[controller view] setFrame:frame];
  EXPECT_FALSE([controller shouldShowIcon]);
  EXPECT_FALSE([controller shouldShowCloseButton]);

  // Setting the icon when tab is at min width should not show icon (bug 18359).
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  base::scoped_nsobject<NSImage> favicon(
      rb.GetNativeImageNamed(IDR_DEFAULT_FAVICON).CopyNSImage());
  [controller setIconImage:favicon forLoadingState:kTabDone showIcon:YES];
  [controller updateVisibility];
  NSView* newIcon = [controller iconView];
  EXPECT_TRUE([newIcon isHidden]);

  // Tab is at active minimum width. Since it's active, the close box
  // should be visible.
  [controller setActive:YES];
  frame = [[controller view] frame];
  frame.size.width = [TabController minActiveTabWidth];
  [[controller view] setFrame:frame];
  EXPECT_FALSE([controller shouldShowIcon]);
  EXPECT_TRUE([newIcon isHidden]);
  EXPECT_TRUE([controller shouldShowCloseButton]);

  // Test expanding the tab to max width and ensure the icon and close box
  // get put back, even when de-activated.
  frame.size.width = [TabController maxTabWidth];
  [[controller view] setFrame:frame];
  EXPECT_TRUE([controller shouldShowIcon]);
  EXPECT_FALSE([newIcon isHidden]);
  EXPECT_TRUE([controller shouldShowCloseButton]);
  [controller setActive:NO];
  EXPECT_TRUE([controller shouldShowIcon]);
  EXPECT_TRUE([controller shouldShowCloseButton]);

  cap = [controller iconCapacity];
  EXPECT_GT(cap, 0);
}

TEST_F(TabControllerTest, Menu) {
  NSWindow* window = test_window();
  base::scoped_nsobject<TabController> controller([[TabController alloc] init]);
  base::scoped_nsobject<TabControllerTestTarget> target(
      [[TabControllerTestTarget alloc] init]);
  [controller setTarget:target];

  [[window contentView] addSubview:[controller view]];
  int cap = [controller iconCapacity];
  EXPECT_GT(cap, 0);

  // Asking the view for its menu should yield a valid menu.
  NSMenu* menu = [[controller view] menu];
  EXPECT_TRUE(menu);
  EXPECT_EQ(3, [menu numberOfItems]);
}

// Regression test for https://crbug.com/778776. An accessibility message can
// cause -[TabController menu] to be called while the existing menu is open.
// Test that this does not cause the running menu to be deleted.
TEST_F(TabControllerTest, RecursiveMenu) {
  base::scoped_nsobject<TabController> controller([[TabController alloc] init]);
  base::scoped_nsobject<TabControllerTestTarget> target(
      [[TabControllerTestTarget alloc] init]);
  [controller setTarget:target];

  NSMenu* menu = [controller menu];

  base::scoped_nsobject<MenuTestObserver> menu_observer(
      [[MenuTestObserver alloc] initWithMenu:menu]);
  [menu_observer setCloseAfterOpening:YES];
  [menu_observer setOpenCallback:^(MenuTestObserver*) {
    NSMenu* open_menu = [controller menu];
    EXPECT_TRUE(open_menu);
    EXPECT_EQ(menu, open_menu);
  }];

  [NSMenu
      popUpContextMenu:menu
             withEvent:cocoa_test_event_utils::LeftMouseDownAtPoint(NSZeroPoint)
               forView:[controller view]];

  EXPECT_NE(menu, [controller menu]);
}

// Tests that the title field is correctly positioned and sized when the
// view is resized.
TEST_F(TabControllerTest, TitleViewLayout) {
  NSWindow* window = test_window();

  base::scoped_nsobject<TabController> controller([[TabController alloc] init]);
  [[window contentView] addSubview:[controller view]];
  NSRect tabFrame = [[controller view] frame];
  tabFrame.size.width = [TabController maxTabWidth];
  [[controller view] setFrame:tabFrame];

  // Set up the favicon in the tabview.
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  base::scoped_nsobject<NSImage> favicon(
      rb.GetNativeImageNamed(IDR_DEFAULT_FAVICON).CopyNSImage());
  [controller setIconImage:favicon forLoadingState:kTabDone showIcon:YES];
  [controller updateVisibility];

  const NSRect originalTabFrame = [[controller view] frame];
  const NSRect originalIconFrame = [[controller iconView] frame];
  const NSRect originalCloseFrame = [[controller closeButton] frame];
  const NSRect originalTitleFrame = [[controller tabView] titleFrame];

  // Sanity check the start state.
  EXPECT_FALSE([[controller iconView] isHidden]);
  EXPECT_FALSE([[controller closeButton] isHidden]);
  EXPECT_GT(NSWidth([[controller view] frame]),
            NSWidth([[controller tabView] titleFrame]));

  // Resize the tab so that that the it shrinks.
  tabFrame.size.width = [TabController minTabWidth];
  [[controller view] setFrame:tabFrame];

  // The icon view and close button should be hidden and the title view should
  // resize to take up their space.
  EXPECT_TRUE([[controller iconView] isHidden]);
  EXPECT_TRUE([[controller closeButton] isHidden]);
  EXPECT_GT(NSWidth([[controller view] frame]),
            NSWidth([[controller tabView] titleFrame]));
  EXPECT_EQ(LeftMargin(originalTabFrame, originalIconFrame),
            LeftMargin([[controller view] frame],
                       [[controller tabView] titleFrame]));
  EXPECT_EQ(RightMargin(originalTabFrame, originalCloseFrame),
            RightMargin([[controller view] frame],
                        [[controller tabView] titleFrame]));

  // Resize the tab so that that the it grows.
  tabFrame.size.width = static_cast<int>([TabController maxTabWidth] * 0.75);
  [[controller view] setFrame:tabFrame];

  // The icon view and close button should be visible again and the title view
  // should be resized to make room for them.
  EXPECT_FALSE([[controller iconView] isHidden]);
  EXPECT_FALSE([[controller closeButton] isHidden]);
  EXPECT_GT(NSWidth([[controller view] frame]),
            NSWidth([[controller tabView] titleFrame]));
  EXPECT_EQ(LeftMargin(originalTabFrame, originalTitleFrame),
            LeftMargin([[controller view] frame],
                       [[controller tabView] titleFrame]));
  EXPECT_EQ(RightMargin(originalTabFrame, originalTitleFrame),
            RightMargin([[controller view] frame],
                        [[controller tabView] titleFrame]));
}

TEST_F(TabControllerTest, LayoutAndVisibilityOfSubviews) {
  CheckLayoutAndVisibilityOfSubviewsForAllStates(false);
}

TEST_F(TabControllerTest, LayoutAndVisibilityOfSubviewsRTL) {
  cocoa_l10n_util::ScopedForceRTLMac scoped_rtl;

  CheckLayoutAndVisibilityOfSubviewsForAllStates(true);
}

TEST_F(TabControllerTest, TabSelection) {
  NSWindow* window = test_window();
  base::scoped_nsobject<TabController> controller([[TabController alloc] init]);
  [[window contentView] addSubview:[controller view]];

  base::scoped_nsobject<TabControllerTestTarget> target(
      [[TabControllerTestTarget alloc] init]);
  [controller setTarget:target];
  [controller setAction:@selector(selectTab:)];

  EXPECT_FALSE([target selected]);
  [controller performClick:controller];
  EXPECT_TRUE([target selected]);
}

}  // namespace
