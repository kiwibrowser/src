// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scroll/scrollbar_theme_aura.h"

#include "base/message_loop/message_loop.h"
#include "third_party/blink/renderer/platform/scroll/scrollbar_test_suite.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support_with_mock_scheduler.h"

namespace blink {

using testing::Return;

namespace {

class ScrollbarThemeAuraButtonOverride final : public ScrollbarThemeAura {
 public:
  ScrollbarThemeAuraButtonOverride() : has_scrollbar_buttons_(true) {}

  void SetHasScrollbarButtons(bool value) { has_scrollbar_buttons_ = value; }

  bool HasScrollbarButtons(ScrollbarOrientation unused) const override {
    return has_scrollbar_buttons_;
  }

 private:
  bool has_scrollbar_buttons_;
};

}  // namespace

class ScrollbarThemeAuraTest : public testing::Test {
 private:
  base::MessageLoop message_loop_;
};

TEST_F(ScrollbarThemeAuraTest, ButtonSizeHorizontal) {
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform;

  MockScrollableArea* mock_scrollable_area = MockScrollableArea::Create();
  ScrollbarThemeMock mock_theme;
  Scrollbar* scrollbar =
      Scrollbar::CreateForTesting(mock_scrollable_area, kHorizontalScrollbar,
                                  kRegularScrollbar, &mock_theme);
  ScrollbarThemeAuraButtonOverride theme;

  IntRect scrollbar_size_normal_dimensions(11, 22, 444, 66);
  scrollbar->SetFrameRect(scrollbar_size_normal_dimensions);
  IntSize size1 = theme.ButtonSize(*scrollbar);
  EXPECT_EQ(66, size1.Width());
  EXPECT_EQ(66, size1.Height());

  IntRect scrollbar_size_squashed_dimensions(11, 22, 444, 666);
  scrollbar->SetFrameRect(scrollbar_size_squashed_dimensions);
  IntSize size2 = theme.ButtonSize(*scrollbar);
  EXPECT_EQ(222, size2.Width());
  EXPECT_EQ(666, size2.Height());

  ThreadState::Current()->CollectAllGarbage();
}

TEST_F(ScrollbarThemeAuraTest, ButtonSizeVertical) {
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform;

  MockScrollableArea* mock_scrollable_area = MockScrollableArea::Create();
  ScrollbarThemeMock mock_theme;
  Scrollbar* scrollbar = Scrollbar::CreateForTesting(
      mock_scrollable_area, kVerticalScrollbar, kRegularScrollbar, &mock_theme);
  ScrollbarThemeAuraButtonOverride theme;

  IntRect scrollbar_size_normal_dimensions(11, 22, 44, 666);
  scrollbar->SetFrameRect(scrollbar_size_normal_dimensions);
  IntSize size1 = theme.ButtonSize(*scrollbar);
  EXPECT_EQ(44, size1.Width());
  EXPECT_EQ(44, size1.Height());

  IntRect scrollbar_size_squashed_dimensions(11, 22, 444, 666);
  scrollbar->SetFrameRect(scrollbar_size_squashed_dimensions);
  IntSize size2 = theme.ButtonSize(*scrollbar);
  EXPECT_EQ(444, size2.Width());
  EXPECT_EQ(333, size2.Height());

  ThreadState::Current()->CollectAllGarbage();
}

TEST_F(ScrollbarThemeAuraTest, NoButtonsReturnsSize0) {
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform;

  MockScrollableArea* mock_scrollable_area = MockScrollableArea::Create();
  ScrollbarThemeMock mock_theme;
  Scrollbar* scrollbar = Scrollbar::CreateForTesting(
      mock_scrollable_area, kVerticalScrollbar, kRegularScrollbar, &mock_theme);
  ScrollbarThemeAuraButtonOverride theme;
  theme.SetHasScrollbarButtons(false);

  scrollbar->SetFrameRect(IntRect(1, 2, 3, 4));
  IntSize size = theme.ButtonSize(*scrollbar);
  EXPECT_EQ(0, size.Width());
  EXPECT_EQ(0, size.Height());

  ThreadState::Current()->CollectAllGarbage();
}

}  // namespace blink
