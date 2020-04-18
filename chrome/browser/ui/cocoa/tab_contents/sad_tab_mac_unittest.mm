// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/tab_contents/sad_tab_view_cocoa.h"

#import "base/mac/foundation_util.h"
#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "ui/base/cocoa/controls/hyperlink_text_view.h"

namespace {

template <typename T>
T* FindSadTabSubview(NSView* sadTabView) {
  NSView* containerView = sadTabView.subviews.firstObject;
  for (NSView* view in [containerView subviews]) {
    if (auto subview = base::mac::ObjCCast<T>(view))
      return subview;
  }
  return nil;
}

class SadTabViewTest : public CocoaTest {
 public:
  SadTabViewTest() {
    base::scoped_nsobject<SadTabView> view([SadTabView new]);
    view_ = view;
    [[test_window() contentView] addSubview:view_];
  }

  SadTabView* view_;  // Weak. Owned by the view hierarchy.
};

TEST_VIEW(SadTabViewTest, view_);

TEST(SadTabViewBehaviorTest, ClickOnLinks) {
  using Action = SadTab::Action;

  class MockSadTab : public SadTab {
   public:
    MockSadTab() : SadTab(nullptr, SAD_TAB_KIND_CRASHED) {}
    MOCK_METHOD0(RecordFirstPaint, void());
    MOCK_METHOD1(PerformAction, void(Action));
  };

  MockSadTab sadTab;

  base::scoped_nsobject<SadTabView> view(
      [[SadTabView alloc] initWithFrame:NSZeroRect sadTab:&sadTab]);

  EXPECT_CALL(sadTab, RecordFirstPaint());
  EXPECT_CALL(sadTab, PerformAction(testing::TypedEq<Action>(Action::BUTTON)));
  EXPECT_CALL(sadTab,
              PerformAction(testing::TypedEq<Action>(Action::HELP_LINK)));

  [view displayIfNeeded];
  [FindSadTabSubview<NSButton>(view) performClick:nil];
  [FindSadTabSubview<HyperlinkTextView>(view) clickedOnLink:[NSNull null]
                                                    atIndex:0];
}

}  // namespace
