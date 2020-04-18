// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet_controller.h"

#include <stddef.h>

#include "base/mac/sdk_forward_declarations.h"
#include "base/macros.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_sheet.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#import "testing/gtest_mac.h"

namespace {

const int kSystemSheetReturnCode = 77;

}  // namespace

@interface ConstrainedWindowSystemSheetTest
    : NSObject <ConstrainedWindowSheet> {
  int returnCode_;
  NSAlert* alert_;  // weak
}

@property(nonatomic, readonly) int returnCode;
@property(nonatomic, assign) NSAlert* alert;

@end

@implementation ConstrainedWindowSystemSheetTest

@synthesize returnCode = returnCode_;
@synthesize alert = alert_;

- (void)showSheetForWindow:(NSWindow*)window {
  [alert_ beginSheetModalForWindow:window
                     modalDelegate:self
                    didEndSelector:@selector(alertDidEnd:returnCode:ctxInfo:)
                       contextInfo:NULL];
}

- (void)closeSheetWithAnimation:(BOOL)withAnimation {
  [NSApp endSheet:[alert_ window] returnCode:kSystemSheetReturnCode];
}

- (void)hideSheet {
}

- (void)unhideSheet {
}

- (void)pulseSheet {
}

- (void)makeSheetKeyAndOrderFront {
}

- (void)updateSheetPosition {
}

- (void)resizeWithNewSize:(NSSize)size {
  // NOOP
}

- (NSWindow*)sheetWindow {
  return [alert_ window];
}

- (void)alertDidEnd:(NSAlert *)alert
         returnCode:(NSInteger)returnCode
            ctxInfo:(void *)contextInfo {
  returnCode_ = returnCode;
}

@end

class ConstrainedWindowSheetControllerTest : public CocoaTest {
 protected:
  void SetUp() override {
    CocoaTest::SetUp();

    // Center the window so that the sheet doesn't go offscreen.
    [test_window() center];

    // The real view setup is quite a few levels deep; recreate something
    // similar.
    NSRect dummy_rect = NSMakeRect(0, 0, 50, 50);
    tab_view_parent_ = [test_window() contentView];
    for (int i = 0; i < 3; ++i) {
      base::scoped_nsobject<NSView> new_view(
          [[NSView alloc] initWithFrame:dummy_rect]);
      [tab_view_parent_ addSubview:new_view.get()];
      tab_view_parent_ = new_view.get();
    }

    controller_.reset([[ConstrainedWindowSheetController
            controllerForParentWindow:test_window()] retain]);
    EXPECT_TRUE(controller_);

    // Create two dummy tabs and make the first one active.
    tab_views_.reset([[NSMutableArray alloc] init]);
    sheet_windows_.reset([[NSMutableArray alloc] init]);
    sheets_.reset([[NSMutableArray alloc] init]);
    for (int i = 0; i < 2; ++i) {
      base::scoped_nsobject<NSView> view(
          [[NSView alloc] initWithFrame:dummy_rect]);
      base::scoped_nsobject<NSWindow> sheet_window(
          [[NSWindow alloc]
            initWithContentRect:dummy_rect
                      styleMask:NSTitledWindowMask
                        backing:NSBackingStoreBuffered
                          defer:NO]);
      [sheet_window setReleasedWhenClosed:NO];
      base::scoped_nsobject<CustomConstrainedWindowSheet> sheet(
          [[CustomConstrainedWindowSheet alloc]
              initWithCustomWindow:sheet_window]);
      EXPECT_FALSE([ConstrainedWindowSheetController controllerForSheet:sheet]);
      [tab_views_ addObject:view];
      [sheet_windows_ addObject:sheet_window];
      [sheets_ addObject:sheet];
    }
    tab0_ = [tab_views_ objectAtIndex:0];
    tab1_ = [tab_views_ objectAtIndex:1];
    sheet_window0_ = [sheet_windows_ objectAtIndex:0];
    sheet_window1_ = [sheet_windows_ objectAtIndex:1];
    sheet_window_ = sheet_window0_;
    sheet0_ = [sheets_ objectAtIndex:0];
    sheet1_ = [sheets_ objectAtIndex:1];
    sheet_ = sheet0_;

    active_tab_view_ = tab0_;
    [tab_view_parent_ addSubview:active_tab_view_];
  }

  void TearDown() override {
    sheets_.reset();
    sheet_windows_.reset();
    CocoaTest::TearDown();
  }

  void ActivateTabView(NSView* tab_view) {
    for (NSView* view in tab_views_.get())
      [view removeFromSuperview];
    [tab_view_parent_ addSubview:tab_view];
    CustomConstrainedWindowSheet* current_sheet =
        [sheets_ objectAtIndex:[tab_views_ indexOfObject:active_tab_view_]];
    [[ConstrainedWindowSheetController controllerForSheet:current_sheet]
        hideSheet:current_sheet];

    active_tab_view_ = tab_view;
    ConstrainedWindowSheetController* controller =
        [ConstrainedWindowSheetController
            controllerForParentWindow:test_window()];
    EXPECT_TRUE(controller);

    CustomConstrainedWindowSheet* sheet =
        [sheets_ objectAtIndex:[tab_views_ indexOfObject:active_tab_view_]];
    EXPECT_TRUE(sheet);
    [controller showSheet:sheet forParentView:active_tab_view_];
  }

  NSRect GetViewFrameInScreenCoordinates(NSView* view) {
    NSRect rect = [view convertRect:[view bounds] toView:nil];
    rect = [[view window] convertRectToScreen:rect];
    return rect;
  }

  void VerifySheetXPosition(NSRect sheet_frame, NSView* parent_view) {
    NSRect parent_frame = GetViewFrameInScreenCoordinates(parent_view);
    CGFloat expected_x = NSMinX(parent_frame) +
        (NSWidth(parent_frame) - NSWidth(sheet_frame)) / 2.0;
    EXPECT_EQ(expected_x, NSMinX(sheet_frame));
  }

  CGFloat GetSheetYOffset(NSRect sheet_frame, NSView* parent_view) {
    return NSMaxY(sheet_frame) -
           NSMaxY(GetViewFrameInScreenCoordinates(parent_view));
  }

  base::scoped_nsobject<NSMutableArray> sheet_windows_;
  NSWindow* sheet_window0_;
  NSWindow* sheet_window1_;
  NSWindow* sheet_window_;
  base::scoped_nsobject<NSMutableArray> sheets_;
  CustomConstrainedWindowSheet* sheet0_;
  CustomConstrainedWindowSheet* sheet1_;
  CustomConstrainedWindowSheet* sheet_;
  base::scoped_nsobject<ConstrainedWindowSheetController> controller_;
  base::scoped_nsobject<NSMutableArray> tab_views_;
  NSView* tab_view_parent_;
  NSView* active_tab_view_;
  NSView* tab0_;
  NSView* tab1_;
};

// Test showing then hiding the sheet.
TEST_F(ConstrainedWindowSheetControllerTest, ShowHide) {
  EXPECT_FALSE([sheet_window_ isVisible]);
  [controller_ showSheet:sheet_ forParentView:active_tab_view_];
  EXPECT_TRUE([ConstrainedWindowSheetController controllerForSheet:sheet_]);
  EXPECT_TRUE([sheet_window_ isVisible]);

  [controller_ closeSheet:sheet_];
  EXPECT_FALSE([ConstrainedWindowSheetController controllerForSheet:sheet_]);
  EXPECT_FALSE([sheet_window_ isVisible]);
}

// Test that switching tabs correctly hides the inactive tab's sheet.
TEST_F(ConstrainedWindowSheetControllerTest, SwitchTabs) {
  [controller_ showSheet:sheet_ forParentView:active_tab_view_];

  EXPECT_TRUE([sheet_window_ isVisible]);
  EXPECT_EQ(1.0, [sheet_window_ alphaValue]);
  ActivateTabView([tab_views_ objectAtIndex:1]);
  EXPECT_TRUE([sheet_window_ isVisible]);
  EXPECT_EQ(0.0, [sheet_window_ alphaValue]);
  ActivateTabView([tab_views_ objectAtIndex:0]);
  EXPECT_TRUE([sheet_window_ isVisible]);
  EXPECT_EQ(1.0, [sheet_window_ alphaValue]);
}

// Test that hiding a hidden tab for the second time is not affecting the
// visible one. See http://crbug.com/589074.
TEST_F(ConstrainedWindowSheetControllerTest, DoubleHide) {
  ActivateTabView([tab_views_ objectAtIndex:1]);
  ActivateTabView([tab_views_ objectAtIndex:0]);

  ASSERT_TRUE([ConstrainedWindowSheetController controllerForSheet:sheet1_]);
  [[ConstrainedWindowSheetController controllerForSheet:sheet1_]
      hideSheet:sheet1_];
  EXPECT_TRUE([sheet_window_ isVisible]);
  EXPECT_EQ(1.0, [sheet_window_ alphaValue]);
}

// Test that two parent windows with two sheet controllers don't conflict.
TEST_F(ConstrainedWindowSheetControllerTest, TwoParentWindows) {
  base::scoped_nsobject<NSWindow> parent_window2(
      [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 30, 30)
                                  styleMask:NSTitledWindowMask
                                    backing:NSBackingStoreBuffered
                                      defer:NO]);
  [parent_window2 setReleasedWhenClosed:NO];

  ConstrainedWindowSheetController* controller2 =
      [ConstrainedWindowSheetController
          controllerForParentWindow:parent_window2];
  EXPECT_TRUE(controller2);
  EXPECT_NSNE(controller_, controller2);

  [controller2 showSheet:sheet_ forParentView:[parent_window2 contentView]];
  EXPECT_NSEQ(controller2,
              [ConstrainedWindowSheetController controllerForSheet:sheet_]);

  [parent_window2 close];
}

// Test that resizing sheet works.
TEST_F(ConstrainedWindowSheetControllerTest, Resize) {
  [controller_ showSheet:sheet_ forParentView:active_tab_view_];

  NSRect old_frame = [sheet_window_ frame];

  NSRect sheet_frame;
  sheet_frame.size = NSMakeSize(NSWidth(old_frame) + 100,
                                NSHeight(old_frame) + 50);
  sheet_frame.origin = [controller_ originForSheet:sheet_
                                    withWindowSize:sheet_frame.size];

  // Y pos should not have changed.
  EXPECT_EQ(NSMaxY(sheet_frame), NSMaxY(old_frame));

  // X pos should be centered on parent view.
  VerifySheetXPosition(sheet_frame, active_tab_view_);
}

// Test that resizing a hidden sheet works.
TEST_F(ConstrainedWindowSheetControllerTest, ResizeHiddenSheet) {
  [controller_ showSheet:sheet_ forParentView:tab0_];
  EXPECT_EQ(1.0, [sheet_window_ alphaValue]);
  ActivateTabView(tab1_);
  EXPECT_EQ(0.0, [sheet_window_ alphaValue]);

  NSRect old_frame = [sheet_window_ frame];
  NSRect new_inactive_frame = NSInsetRect(old_frame, -30, -40);
  [sheet_window_ setFrame:new_inactive_frame display:YES];

  ActivateTabView(tab0_);
  EXPECT_EQ(1.0, [sheet_window_ alphaValue]);

  NSRect new_active_frame = [sheet_window_ frame];
  EXPECT_EQ(NSWidth(new_inactive_frame), NSWidth(new_active_frame));
  EXPECT_EQ(NSHeight(new_inactive_frame), NSHeight(new_active_frame));
}

// Test resizing parent window keeps the sheet anchored.
TEST_F(ConstrainedWindowSheetControllerTest, ResizeParentWindow) {
  [controller_ showSheet:sheet_ forParentView:active_tab_view_];
  CGFloat sheet_offset =
      GetSheetYOffset([sheet_window_ frame], active_tab_view_);

  // Test 3x3 different parent window sizes.
  CGFloat insets[] = {-10, 0, 10};
  NSRect old_frame = [test_window() frame];

  for (size_t x = 0; x < arraysize(insets); x++) {
    for (size_t y = 0; y < arraysize(insets); y++) {
      NSRect resized_frame = NSInsetRect(old_frame, insets[x], insets[y]);
      [test_window() setFrame:resized_frame display:YES];
      NSRect sheet_frame = [sheet_window_ frame];

      // Y pos should track parent view's position.
      EXPECT_EQ(sheet_offset, GetSheetYOffset(sheet_frame, active_tab_view_));

      // X pos should be centered on parent view.
      VerifySheetXPosition(sheet_frame, active_tab_view_);
    }
  }
}

// Test system sheets.
TEST_F(ConstrainedWindowSheetControllerTest, SystemSheet) {
  base::scoped_nsobject<ConstrainedWindowSystemSheetTest> system_sheet(
      [[ConstrainedWindowSystemSheetTest alloc] init]);
  base::scoped_nsobject<NSAlert> alert([[NSAlert alloc] init]);
  [system_sheet setAlert:alert];

  EXPECT_FALSE([[alert window] isVisible]);
  [controller_ showSheet:system_sheet forParentView:active_tab_view_];
  EXPECT_TRUE([[alert window] isVisible]);

  [controller_ closeSheet:system_sheet];
  EXPECT_FALSE([[alert window] isVisible]);
  EXPECT_EQ(kSystemSheetReturnCode, [system_sheet returnCode]);
}
