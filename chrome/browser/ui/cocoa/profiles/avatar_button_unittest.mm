// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/profiles/avatar_button.h"

#import "base/mac/scoped_nsobject.h"
#import "ui/base/test/cocoa_helper.h"
#import "ui/events/test/cocoa_test_event_utils.h"

@interface AvatarButton (ExposedForTesting)
- (void)performRightClick;
@end

@interface AvatarButtonTestObserver : NSObject
@property BOOL clicked;

- (void)buttonRightClicked;
@end

@implementation AvatarButtonTestObserver
@synthesize clicked = clicked_;

- (void)buttonRightClicked {
  self.clicked = YES;
}
@end

class AvatarButtonTest : public ui::CocoaTest {
 public:
  AvatarButtonTest() {
    NSRect content_frame = [[test_window() contentView] frame];
    base::scoped_nsobject<AvatarButton> button(
        [[AvatarButton alloc] initWithFrame:content_frame]);
    button_ = button.get();
    [[test_window() contentView] addSubview:button_];
  }

  AvatarButton* button_;
};

TEST_F(AvatarButtonTest, RightClick) {
  base::scoped_nsobject<AvatarButtonTestObserver> observer(
      [[AvatarButtonTestObserver alloc] init]);
  [button_ setTarget:observer.get()];
  [button_ setRightAction:@selector(buttonRightClicked)];

  ASSERT_FALSE(observer.get().clicked);

  [button_ performRightClick];
  ASSERT_TRUE(observer.get().clicked);
}

TEST_F(AvatarButtonTest, RightClickInView) {
  base::scoped_nsobject<AvatarButtonTestObserver> observer(
      [[AvatarButtonTestObserver alloc] init]);
  [button_ setTarget:observer.get()];
  [button_ setRightAction:@selector(buttonRightClicked)];

  ASSERT_FALSE(observer.get().clicked);

  std::pair<NSEvent*, NSEvent*> events =
      cocoa_test_event_utils::RightMouseClickInView(button_, 1);

  [NSApp postEvent:events.second atStart:YES];
  [NSApp sendEvent:events.first];

  ASSERT_TRUE(observer.get().clicked);
}

TEST_F(AvatarButtonTest, RightMouseUpOutOfView) {
  base::scoped_nsobject<AvatarButtonTestObserver> observer(
      [[AvatarButtonTestObserver alloc] init]);
  [button_ setTarget:observer.get()];
  [button_ setRightAction:@selector(buttonRightClicked)];

  ASSERT_FALSE(observer.get().clicked);

  const NSRect bounds = [button_ convertRect:[button_ bounds] toView:nil];
  const NSPoint downLocation = NSMakePoint(NSMidX(bounds), NSMidY(bounds));
  NSEvent* down = cocoa_test_event_utils::MouseEventAtPointInWindow(
      downLocation, NSRightMouseDown, [button_ window], 1);

  const NSPoint upLocation = NSMakePoint(downLocation.x + bounds.size.width,
                                         downLocation.y + bounds.size.height);
  NSEvent* up = cocoa_test_event_utils::MouseEventAtPointInWindow(
      upLocation, NSRightMouseUp, [button_ window], 1);

  [NSApp postEvent:up atStart:YES];
  [NSApp sendEvent:down];

  ASSERT_FALSE(observer.get().clicked);
}
