// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_alert.h"

#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#import "testing/gtest_mac.h"

class ConstrainedWindowAlertTest : public CocoaTest {
};

@interface ConstrainedWindowAlertTestTarget : NSObject {
 @private
  int linkClickedCount_;
}
@property(nonatomic, readonly) int linkClickedCount;

- (void)onLinkClicked:(id)sender;
@end

@implementation ConstrainedWindowAlertTestTarget

@synthesize linkClickedCount = linkClickedCount_;

- (void)onLinkClicked:(id)sender {
  ++linkClickedCount_;
}

@end

// Test showing the alert.
TEST_F(ConstrainedWindowAlertTest, Show) {
  base::scoped_nsobject<ConstrainedWindowAlert> alert(
      [[ConstrainedWindowAlert alloc] init]);
  EXPECT_TRUE([alert window]);
  EXPECT_TRUE([alert closeButton]);

  [alert setMessageText:@"Message text"];
  [alert setInformativeText:@"Informative text"];
  [alert addButtonWithTitle:@"OK" keyEquivalent:@"" target:nil action:NULL];
  [alert addButtonWithTitle:@"Cancel" keyEquivalent:@"" target:nil action:NULL];

  [alert layout];
  [[alert window] makeKeyAndOrderFront:nil];
}

// Test showing the alert with no buttons.
TEST_F(ConstrainedWindowAlertTest, NoButtons) {
  base::scoped_nsobject<ConstrainedWindowAlert> alert(
      [[ConstrainedWindowAlert alloc] init]);
  [alert layout];
  [[alert window] makeKeyAndOrderFront:nil];
}

// Test adding an accessory view to an alert.
TEST_F(ConstrainedWindowAlertTest, AccessoryView) {
  base::scoped_nsobject<ConstrainedWindowAlert> alert(
      [[ConstrainedWindowAlert alloc] init]);
  [alert addButtonWithTitle:@"OK" keyEquivalent:@"" target:nil action:NULL];
  [alert addButtonWithTitle:@"Cancel" keyEquivalent:@"" target:nil action:NULL];

  NSRect view_rect = NSMakeRect(0, 0, 700, 300);
  base::scoped_nsobject<NSView> view([[NSView alloc] initWithFrame:view_rect]);
  EXPECT_FALSE([alert accessoryView]);
  [alert setAccessoryView:view];
  EXPECT_NSEQ([alert accessoryView], view);

  [alert layout];
  NSRect window_rect = [[alert window] frame];
  EXPECT_GT(NSWidth(window_rect), NSWidth(view_rect));
  EXPECT_GT(NSHeight(window_rect), NSHeight(view_rect));

  [[alert window] makeKeyAndOrderFront:nil];
}

// Test adding a link to an alert.
TEST_F(ConstrainedWindowAlertTest, LinkView) {
  base::scoped_nsobject<ConstrainedWindowAlert> alert(
      [[ConstrainedWindowAlert alloc] init]);
  base::scoped_nsobject<ConstrainedWindowAlertTestTarget> target(
      [[ConstrainedWindowAlertTestTarget alloc] init]);

  [alert layout];
  NSRect initial_window_rect = [[alert window] frame];

  EXPECT_EQ(nil, [alert linkView]);
  NSString* linkText = @"Text of the link";
  [alert setLinkText:linkText
              target:target.get()
              action:@selector(onLinkClicked:)];
  EXPECT_EQ([linkText length], [[[alert linkView] title] length]);

  [alert layout];
  NSRect window_rect = [[alert window] frame];

  EXPECT_GT(NSHeight(window_rect), NSHeight(initial_window_rect));

  [[alert window] makeKeyAndOrderFront:nil];

  [[alert linkView] performClick:nil];
  EXPECT_EQ(1, [target linkClickedCount]);
}
