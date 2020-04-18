// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_toolbar_view.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/image/image_skia.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgPointee;

// Allows us to control which way the view is rendered.
@interface DrawDetachedBarFakeController :
    NSObject<BookmarkBarState, BookmarkBarToolbarViewController> {
 @private
  int currentTabContentsHeight_;
  Profile* profile_;
  BookmarkBar::State state_;
  BOOL isEmpty_;
}
@property (nonatomic, assign) int currentTabContentsHeight;
@property (nonatomic, assign) Profile* profile;
@property (nonatomic, assign) BookmarkBar::State state;
@property (nonatomic, assign) BOOL isEmpty;

// |BookmarkBarState| protocol:
- (BOOL)isVisible;
- (BOOL)isAnimationRunning;
- (BOOL)isInState:(BookmarkBar::State)state;
- (BOOL)isAnimatingToState:(BookmarkBar::State)state;
- (BOOL)isAnimatingFromState:(BookmarkBar::State)state;
- (BOOL)isAnimatingFromState:(BookmarkBar::State)fromState
                     toState:(BookmarkBar::State)toState;
- (BOOL)isAnimatingBetweenState:(BookmarkBar::State)fromState
                       andState:(BookmarkBar::State)toState;
- (CGFloat)detachedMorphProgress;
@end

@implementation DrawDetachedBarFakeController
@synthesize currentTabContentsHeight = currentTabContentsHeight_;
@synthesize profile = profile_;
@synthesize state = state_;
@synthesize isEmpty = isEmpty_;

- (id)init {
  if ((self = [super init])) {
    [self setState:BookmarkBar::HIDDEN];
  }
  return self;
}

- (BOOL)isVisible { return YES; }
- (BOOL)isAnimationRunning { return NO; }
- (BOOL)isInState:(BookmarkBar::State)state
    { return ([self state] == state) ? YES : NO; }
- (BOOL)isAnimatingToState:(BookmarkBar::State)state { return NO; }
- (BOOL)isAnimatingFromState:(BookmarkBar::State)state { return NO; }
- (BOOL)isAnimatingFromState:(BookmarkBar::State)fromState
                     toState:(BookmarkBar::State)toState { return NO; }
- (BOOL)isAnimatingBetweenState:(BookmarkBar::State)fromState
                       andState:(BookmarkBar::State)toState { return NO; }
- (CGFloat)detachedMorphProgress { return 1; }
@end

class BookmarkBarToolbarViewTest : public CocoaTest {
 public:
  BookmarkBarToolbarViewTest() {
    controller_.reset([[DrawDetachedBarFakeController alloc] init]);
    NSRect frame = NSMakeRect(0, 0, 400, 40);
    base::scoped_nsobject<BookmarkBarToolbarView> view(
        [[BookmarkBarToolbarView alloc] initWithFrame:frame]);
    view_ = view.get();
    [[test_window() contentView] addSubview:view_];
    [view_ setController:controller_.get()];
  }

  base::scoped_nsobject<DrawDetachedBarFakeController> controller_;
  BookmarkBarToolbarView* view_;
};

TEST_VIEW(BookmarkBarToolbarViewTest, view_)

// Test drawing, mostly to ensure nothing leaks or crashes.
TEST_F(BookmarkBarToolbarViewTest, DisplayAsNormalBar) {
  [controller_.get() setState:BookmarkBar::SHOW];
  [view_ display];
}

// Actions used in DisplayAsDetachedBarWithBgImage.
ACTION(SetBackgroundTiling) {
  *arg1 = ThemeProperties::NO_REPEAT;
  return true;
}

ACTION(SetAlignLeft) {
  *arg1 = ThemeProperties::ALIGN_LEFT;
  return true;
}

// TODO(viettrungluu): write more unit tests, especially after my refactoring.
