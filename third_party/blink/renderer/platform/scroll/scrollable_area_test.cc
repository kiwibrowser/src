// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scroll/scrollable_area.h"

#include "base/message_loop/message_loop.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/platform/graphics/color.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/scroll/scrollbar_test_suite.h"
#include "third_party/blink/renderer/platform/scroll/scrollbar_theme.h"
#include "third_party/blink/renderer/platform/scroll/scrollbar_theme_mock.h"
#include "third_party/blink/renderer/platform/scroll/scrollbar_theme_overlay_mock.h"
#include "third_party/blink/renderer/platform/testing/fake_graphics_layer.h"
#include "third_party/blink/renderer/platform/testing/fake_graphics_layer_client.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support_with_mock_scheduler.h"

namespace blink {

namespace {

using testing::_;
using testing::Return;

class ScrollbarThemeWithMockInvalidation : public ScrollbarThemeMock {
 public:
  MOCK_CONST_METHOD0(ShouldRepaintAllPartsOnInvalidation, bool());
  MOCK_CONST_METHOD3(InvalidateOnThumbPositionChange,
                     ScrollbarPart(const Scrollbar&, float, float));
};

}  // namespace

class ScrollableAreaTest : public testing::Test {
 private:
  base::MessageLoop message_loop_;
};

TEST_F(ScrollableAreaTest, ScrollAnimatorCurrentPositionShouldBeSync) {
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform;

  MockScrollableArea* scrollable_area =
      MockScrollableArea::Create(ScrollOffset(0, 100));
  scrollable_area->SetScrollOffset(ScrollOffset(0, 10000), kCompositorScroll);
  EXPECT_EQ(100.0,
            scrollable_area->GetScrollAnimator().CurrentOffset().Height());
}

TEST_F(ScrollableAreaTest, ScrollbarTrackAndThumbRepaint) {
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform;

  ScrollbarThemeWithMockInvalidation theme;
  MockScrollableArea* scrollable_area =
      MockScrollableArea::Create(ScrollOffset(0, 100));
  Scrollbar* scrollbar = Scrollbar::CreateForTesting(
      scrollable_area, kHorizontalScrollbar, kRegularScrollbar, &theme);

  EXPECT_CALL(theme, ShouldRepaintAllPartsOnInvalidation())
      .WillRepeatedly(Return(true));
  EXPECT_TRUE(scrollbar->TrackNeedsRepaint());
  EXPECT_TRUE(scrollbar->ThumbNeedsRepaint());
  scrollbar->SetNeedsPaintInvalidation(kNoPart);
  EXPECT_TRUE(scrollbar->TrackNeedsRepaint());
  EXPECT_TRUE(scrollbar->ThumbNeedsRepaint());

  scrollbar->ClearTrackNeedsRepaint();
  scrollbar->ClearThumbNeedsRepaint();
  EXPECT_FALSE(scrollbar->TrackNeedsRepaint());
  EXPECT_FALSE(scrollbar->ThumbNeedsRepaint());
  scrollbar->SetNeedsPaintInvalidation(kThumbPart);
  EXPECT_TRUE(scrollbar->TrackNeedsRepaint());
  EXPECT_TRUE(scrollbar->ThumbNeedsRepaint());

  // When not all parts are repainted on invalidation,
  // setNeedsPaintInvalidation sets repaint bits only on the requested parts.
  EXPECT_CALL(theme, ShouldRepaintAllPartsOnInvalidation())
      .WillRepeatedly(Return(false));
  scrollbar->ClearTrackNeedsRepaint();
  scrollbar->ClearThumbNeedsRepaint();
  EXPECT_FALSE(scrollbar->TrackNeedsRepaint());
  EXPECT_FALSE(scrollbar->ThumbNeedsRepaint());
  scrollbar->SetNeedsPaintInvalidation(kThumbPart);
  EXPECT_FALSE(scrollbar->TrackNeedsRepaint());
  EXPECT_TRUE(scrollbar->ThumbNeedsRepaint());

  // Forced GC in order to finalize objects depending on the mock object.
  ThreadState::Current()->CollectAllGarbage();
}

TEST_F(ScrollableAreaTest, ScrollbarGraphicsLayerInvalidation) {
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform;

  ScrollbarTheme::SetMockScrollbarsEnabled(true);
  MockScrollableArea* scrollable_area =
      MockScrollableArea::Create(ScrollOffset(0, 100));
  FakeGraphicsLayerClient graphics_layer_client;
  graphics_layer_client.SetIsTrackingRasterInvalidations(true);
  FakeGraphicsLayer graphics_layer(graphics_layer_client);
  graphics_layer.SetDrawsContent(true);
  graphics_layer.SetSize(IntSize(111, 222));

  EXPECT_CALL(*scrollable_area, LayerForHorizontalScrollbar())
      .WillRepeatedly(Return(&graphics_layer));

  Scrollbar* scrollbar = Scrollbar::Create(
      scrollable_area, kHorizontalScrollbar, kRegularScrollbar, nullptr);
  graphics_layer.ResetTrackedRasterInvalidations();
  scrollbar->SetNeedsPaintInvalidation(kNoPart);
  EXPECT_TRUE(graphics_layer.HasTrackedRasterInvalidations());

  // Forced GC in order to finalize objects depending on the mock object.
  ThreadState::Current()->CollectAllGarbage();
}

TEST_F(ScrollableAreaTest, InvalidatesNonCompositedScrollbarsWhenThumbMoves) {
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform;

  ScrollbarThemeWithMockInvalidation theme;
  MockScrollableArea* scrollable_area =
      MockScrollableArea::Create(ScrollOffset(100, 100));
  Scrollbar* horizontal_scrollbar = Scrollbar::CreateForTesting(
      scrollable_area, kHorizontalScrollbar, kRegularScrollbar, &theme);
  Scrollbar* vertical_scrollbar = Scrollbar::CreateForTesting(
      scrollable_area, kVerticalScrollbar, kRegularScrollbar, &theme);
  EXPECT_CALL(*scrollable_area, HorizontalScrollbar())
      .WillRepeatedly(Return(horizontal_scrollbar));
  EXPECT_CALL(*scrollable_area, VerticalScrollbar())
      .WillRepeatedly(Return(vertical_scrollbar));

  // Regardless of whether the theme invalidates any parts, non-composited
  // scrollbars have to be repainted if the thumb moves.
  EXPECT_CALL(*scrollable_area, LayerForHorizontalScrollbar())
      .WillRepeatedly(Return(nullptr));
  EXPECT_CALL(*scrollable_area, LayerForVerticalScrollbar())
      .WillRepeatedly(Return(nullptr));
  ASSERT_FALSE(scrollable_area->HasLayerForVerticalScrollbar());
  ASSERT_FALSE(scrollable_area->HasLayerForHorizontalScrollbar());
  EXPECT_CALL(theme, ShouldRepaintAllPartsOnInvalidation())
      .WillRepeatedly(Return(false));
  EXPECT_CALL(theme, InvalidateOnThumbPositionChange(_, _, _))
      .WillRepeatedly(Return(kNoPart));

  // A scroll in each direction should only invalidate one scrollbar.
  scrollable_area->SetScrollOffset(ScrollOffset(0, 50), kProgrammaticScroll);
  EXPECT_FALSE(scrollable_area->HorizontalScrollbarNeedsPaintInvalidation());
  EXPECT_TRUE(scrollable_area->VerticalScrollbarNeedsPaintInvalidation());
  scrollable_area->ClearNeedsPaintInvalidationForScrollControls();
  scrollable_area->SetScrollOffset(ScrollOffset(50, 50), kProgrammaticScroll);
  EXPECT_TRUE(scrollable_area->HorizontalScrollbarNeedsPaintInvalidation());
  EXPECT_FALSE(scrollable_area->VerticalScrollbarNeedsPaintInvalidation());
  scrollable_area->ClearNeedsPaintInvalidationForScrollControls();

  // Forced GC in order to finalize objects depending on the mock object.
  ThreadState::Current()->CollectAllGarbage();
}

TEST_F(ScrollableAreaTest, InvalidatesCompositedScrollbarsIfPartsNeedRepaint) {
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform;

  ScrollbarThemeWithMockInvalidation theme;
  MockScrollableArea* scrollable_area =
      MockScrollableArea::Create(ScrollOffset(100, 100));
  Scrollbar* horizontal_scrollbar = Scrollbar::CreateForTesting(
      scrollable_area, kHorizontalScrollbar, kRegularScrollbar, &theme);
  horizontal_scrollbar->ClearTrackNeedsRepaint();
  horizontal_scrollbar->ClearThumbNeedsRepaint();
  Scrollbar* vertical_scrollbar = Scrollbar::CreateForTesting(
      scrollable_area, kVerticalScrollbar, kRegularScrollbar, &theme);
  vertical_scrollbar->ClearTrackNeedsRepaint();
  vertical_scrollbar->ClearThumbNeedsRepaint();
  EXPECT_CALL(*scrollable_area, HorizontalScrollbar())
      .WillRepeatedly(Return(horizontal_scrollbar));
  EXPECT_CALL(*scrollable_area, VerticalScrollbar())
      .WillRepeatedly(Return(vertical_scrollbar));

  // Composited scrollbars only need repainting when parts become invalid
  // (e.g. if the track changes appearance when the thumb reaches the end).
  FakeGraphicsLayerClient graphics_layer_client;
  graphics_layer_client.SetIsTrackingRasterInvalidations(true);
  FakeGraphicsLayer layer_for_horizontal_scrollbar(graphics_layer_client);
  layer_for_horizontal_scrollbar.SetDrawsContent(true);
  layer_for_horizontal_scrollbar.SetSize(IntSize(10, 10));
  FakeGraphicsLayer layer_for_vertical_scrollbar(graphics_layer_client);
  layer_for_vertical_scrollbar.SetDrawsContent(true);
  layer_for_vertical_scrollbar.SetSize(IntSize(10, 10));
  EXPECT_CALL(*scrollable_area, LayerForHorizontalScrollbar())
      .WillRepeatedly(Return(&layer_for_horizontal_scrollbar));
  EXPECT_CALL(*scrollable_area, LayerForVerticalScrollbar())
      .WillRepeatedly(Return(&layer_for_vertical_scrollbar));
  ASSERT_TRUE(scrollable_area->HasLayerForHorizontalScrollbar());
  ASSERT_TRUE(scrollable_area->HasLayerForVerticalScrollbar());
  EXPECT_CALL(theme, ShouldRepaintAllPartsOnInvalidation())
      .WillRepeatedly(Return(false));

  // First, we'll scroll horizontally, and the theme will require repainting
  // the back button (i.e. the track).
  EXPECT_CALL(theme, InvalidateOnThumbPositionChange(_, _, _))
      .WillOnce(Return(kBackButtonStartPart));
  scrollable_area->SetScrollOffset(ScrollOffset(50, 0), kProgrammaticScroll);
  EXPECT_TRUE(layer_for_horizontal_scrollbar.HasTrackedRasterInvalidations());
  EXPECT_FALSE(layer_for_vertical_scrollbar.HasTrackedRasterInvalidations());
  EXPECT_TRUE(horizontal_scrollbar->TrackNeedsRepaint());
  EXPECT_FALSE(horizontal_scrollbar->ThumbNeedsRepaint());
  layer_for_horizontal_scrollbar.ResetTrackedRasterInvalidations();
  horizontal_scrollbar->ClearTrackNeedsRepaint();

  // Next, we'll scroll vertically, but invalidate the thumb.
  EXPECT_CALL(theme, InvalidateOnThumbPositionChange(_, _, _))
      .WillOnce(Return(kThumbPart));
  scrollable_area->SetScrollOffset(ScrollOffset(50, 50), kProgrammaticScroll);
  EXPECT_FALSE(layer_for_horizontal_scrollbar.HasTrackedRasterInvalidations());
  EXPECT_TRUE(layer_for_vertical_scrollbar.HasTrackedRasterInvalidations());
  EXPECT_FALSE(vertical_scrollbar->TrackNeedsRepaint());
  EXPECT_TRUE(vertical_scrollbar->ThumbNeedsRepaint());
  layer_for_vertical_scrollbar.ResetTrackedRasterInvalidations();
  vertical_scrollbar->ClearThumbNeedsRepaint();

  // Next we'll scroll in both, but the thumb position moving requires no
  // invalidations. Nonetheless the GraphicsLayer should be invalidated,
  // because we still need to update the underlying layer (though no
  // rasterization will be required).
  EXPECT_CALL(theme, InvalidateOnThumbPositionChange(_, _, _))
      .Times(2)
      .WillRepeatedly(Return(kNoPart));
  scrollable_area->SetScrollOffset(ScrollOffset(70, 70), kProgrammaticScroll);
  EXPECT_TRUE(layer_for_horizontal_scrollbar.HasTrackedRasterInvalidations());
  EXPECT_TRUE(layer_for_vertical_scrollbar.HasTrackedRasterInvalidations());
  EXPECT_FALSE(horizontal_scrollbar->TrackNeedsRepaint());
  EXPECT_FALSE(horizontal_scrollbar->ThumbNeedsRepaint());
  EXPECT_FALSE(vertical_scrollbar->TrackNeedsRepaint());
  EXPECT_FALSE(vertical_scrollbar->ThumbNeedsRepaint());

  // Forced GC in order to finalize objects depending on the mock object.
  ThreadState::Current()->CollectAllGarbage();
}

TEST_F(ScrollableAreaTest, RecalculatesScrollbarOverlayIfBackgroundChanges) {
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform;

  MockScrollableArea* scrollable_area =
      MockScrollableArea::Create(ScrollOffset(0, 100));

  EXPECT_EQ(kScrollbarOverlayColorThemeDark,
            scrollable_area->GetScrollbarOverlayColorTheme());
  scrollable_area->RecalculateScrollbarOverlayColorTheme(Color(34, 85, 51));
  EXPECT_EQ(kScrollbarOverlayColorThemeLight,
            scrollable_area->GetScrollbarOverlayColorTheme());
  scrollable_area->RecalculateScrollbarOverlayColorTheme(Color(236, 143, 185));
  EXPECT_EQ(kScrollbarOverlayColorThemeDark,
            scrollable_area->GetScrollbarOverlayColorTheme());
}

TEST_F(ScrollableAreaTest, ScrollableAreaDidScroll) {
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform;

  MockScrollableArea* scrollable_area =
      MockScrollableArea::Create(ScrollOffset(100, 100));
  scrollable_area->SetScrollOrigin(IntPoint(20, 30));
  scrollable_area->DidScroll(gfx::ScrollOffset(40, 51));

  // After calling didScroll, the new offset should account for scroll origin.
  EXPECT_EQ(20, scrollable_area->ScrollOffsetInt().Width());
  EXPECT_EQ(21, scrollable_area->ScrollOffsetInt().Height());
}

// Scrollbars in popups shouldn't fade out since they aren't composited and thus
// they don't appear on hover so users without a wheel can't scroll if they fade
// out.
TEST_F(ScrollableAreaTest, PopupOverlayScrollbarShouldNotFadeOut) {
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform;

  ScopedOverlayScrollbarsForTest overlay_scrollbars(true);
  ScrollbarTheme::SetMockScrollbarsEnabled(true);

  MockScrollableArea* scrollable_area =
      MockScrollableArea::Create(ScrollOffset(0, 100));
  scrollable_area->SetIsPopup();

  ScrollbarThemeOverlayMock& theme =
      (ScrollbarThemeOverlayMock&)scrollable_area->GetPageScrollbarTheme();
  theme.SetOverlayScrollbarFadeOutDelay(1);
  Scrollbar* scrollbar = Scrollbar::CreateForTesting(
      scrollable_area, kHorizontalScrollbar, kRegularScrollbar, &theme);

  DCHECK(scrollbar->IsOverlayScrollbar());
  DCHECK(scrollbar->Enabled());

  scrollable_area->ShowOverlayScrollbars();

  // No fade out animation should be posted.
  EXPECT_FALSE(scrollable_area->fade_overlay_scrollbars_timer_);

  // Forced GC in order to finalize objects depending on the mock object.
  ThreadState::Current()->CollectAllGarbage();
}

}  // namespace blink
