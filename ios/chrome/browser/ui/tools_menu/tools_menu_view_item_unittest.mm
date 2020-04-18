// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tools_menu/tools_menu_view_item.h"

#import <UIKit/UIKit.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

class ToolsMenuViewItemTest : public PlatformTest {
 protected:
  void SetUp() override {
    toolsMenuViewItem_ = [[ToolsMenuViewItem alloc] init];
  }
  ToolsMenuViewItem* toolsMenuViewItem_;
};

TEST_F(ToolsMenuViewItemTest, CustomizeCellAccessibilityTrait) {
  ToolsMenuViewCell* cell1 = [[ToolsMenuViewCell alloc] init];
  ToolsMenuViewCell* cell2 = [[ToolsMenuViewCell alloc] init];

  [toolsMenuViewItem_ setActive:YES];
  [cell1 setAccessibilityTraits:UIAccessibilityTraitNotEnabled];
  [cell1 configureForMenuItem:toolsMenuViewItem_];
  EXPECT_FALSE(cell1.accessibilityTraits & UIAccessibilityTraitNotEnabled);

  [toolsMenuViewItem_ setActive:NO];
  [cell2 configureForMenuItem:toolsMenuViewItem_];
  EXPECT_TRUE(cell2.accessibilityTraits & UIAccessibilityTraitNotEnabled);

  // Happens when cell2 is reused.
  [toolsMenuViewItem_ setActive:YES];
  [cell2 configureForMenuItem:toolsMenuViewItem_];
  EXPECT_FALSE(cell2.accessibilityTraits & UIAccessibilityTraitNotEnabled);
}

}  // namespace
