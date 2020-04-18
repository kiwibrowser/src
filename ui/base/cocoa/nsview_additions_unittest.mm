// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/nsview_additions.h"

#include "base/mac/scoped_nsobject.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#import "ui/base/test/cocoa_helper.h"

typedef ui::CocoaTest NSViewChromeAdditionsTest;

@interface ParentView : NSView {
 @private
  int removeCount_;
  int addCount_;
}

@property(readonly, nonatomic) int removeCount;
@property(readonly, nonatomic) int addCount;

@end

@implementation ParentView

@synthesize removeCount = removeCount_;
@synthesize addCount = addCount_;

- (void)willRemoveSubview:(NSView*)view {
  ++removeCount_;
}

- (void)didAddSubview:(NSView*)view {
  ++addCount_;
}

@end

TEST_F(NSViewChromeAdditionsTest, BelowAboveView) {
  base::scoped_nsobject<NSView> parent(
      [[NSView alloc] initWithFrame:NSZeroRect]);
  base::scoped_nsobject<NSView> child1(
      [[NSView alloc] initWithFrame:NSZeroRect]);
  base::scoped_nsobject<NSView> child2(
      [[NSView alloc] initWithFrame:NSZeroRect]);

  [parent addSubview:child1];
  [parent addSubview:child2];
  EXPECT_TRUE([child1 cr_isBelowView:child2]);
  EXPECT_FALSE([child1 cr_isAboveView:child2]);
  EXPECT_FALSE([child2 cr_isBelowView:child1]);
  EXPECT_TRUE([child2 cr_isAboveView:child1]);

  [child1 removeFromSuperview];
  [child2 removeFromSuperview];
  [parent addSubview:child2];
  [parent addSubview:child1];
  EXPECT_FALSE([child1 cr_isBelowView:child2]);
  EXPECT_TRUE([child1 cr_isAboveView:child2]);
  EXPECT_TRUE([child2 cr_isBelowView:child1]);
  EXPECT_FALSE([child2 cr_isAboveView:child1]);
}

TEST_F(NSViewChromeAdditionsTest, EnsurePosition) {
  base::scoped_nsobject<NSView> parent(
      [[NSView alloc] initWithFrame:NSZeroRect]);
  base::scoped_nsobject<NSView> child1(
      [[NSView alloc] initWithFrame:NSZeroRect]);
  base::scoped_nsobject<NSView> child2(
      [[NSView alloc] initWithFrame:NSZeroRect]);

  [parent addSubview:child1];
  [parent cr_ensureSubview:child2
              isPositioned:NSWindowAbove
                relativeTo:child1];
  EXPECT_NSEQ([[parent subviews] objectAtIndex:0], child1);
  EXPECT_NSEQ([[parent subviews] objectAtIndex:1], child2);

  [child2 removeFromSuperview];
  [parent cr_ensureSubview:child2
              isPositioned:NSWindowBelow
                relativeTo:child1];
  EXPECT_NSEQ([[parent subviews] objectAtIndex:0], child2);
  EXPECT_NSEQ([[parent subviews] objectAtIndex:1], child1);
}

// Verify that no view is removed or added when no change is needed.
TEST_F(NSViewChromeAdditionsTest, EnsurePositionNoChange) {
  base::scoped_nsobject<ParentView> parent(
      [[ParentView alloc] initWithFrame:NSZeroRect]);
  base::scoped_nsobject<NSView> child1(
      [[NSView alloc] initWithFrame:NSZeroRect]);
  base::scoped_nsobject<NSView> child2(
      [[NSView alloc] initWithFrame:NSZeroRect]);
  [parent addSubview:child1];
  [parent addSubview:child2];

  EXPECT_EQ(0, [parent removeCount]);
  EXPECT_EQ(2, [parent addCount]);
  [parent cr_ensureSubview:child2
              isPositioned:NSWindowAbove
                relativeTo:child1];
  EXPECT_EQ(0, [parent removeCount]);
  EXPECT_EQ(2, [parent addCount]);
}
