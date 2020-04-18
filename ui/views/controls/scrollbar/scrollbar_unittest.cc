// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "ui/views/controls/scrollbar/scroll_bar.h"
#include "ui/views/controls/scrollbar/scroll_bar_views.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/widget/widget.h"

namespace {

// The Scrollbar controller. This is the widget that should do the real
// scrolling of contents.
class TestScrollBarController : public views::ScrollBarController {
 public:
  virtual ~TestScrollBarController() {}

  void ScrollToPosition(views::ScrollBar* source, int position) override {
    last_source = source;
    last_position = position;
  }

  int GetScrollIncrement(views::ScrollBar* source,
                         bool is_page,
                         bool is_positive) override {
    last_source = source;
    last_is_page = is_page;
    last_is_positive = is_positive;

    if (is_page)
      return 20;
    return 10;
  }

  // We save the last values in order to assert the correctness of the scroll
  // operation.
  views::ScrollBar* last_source;
  bool last_is_positive;
  bool last_is_page;
  int last_position;
};

}  // namespace

namespace views {

class ScrollBarViewsTest : public ViewsTestBase {
 public:
  ScrollBarViewsTest() : widget_(nullptr), scrollbar_(nullptr) {}

  void SetUp() override {
    ViewsTestBase::SetUp();
    controller_.reset(new TestScrollBarController());

    widget_ = new Widget;
    Widget::InitParams params = CreateParams(Widget::InitParams::TYPE_POPUP);
    params.bounds = gfx::Rect(0, 0, 100, 100);
    widget_->Init(params);
    View* container = new View();
    widget_->SetContentsView(container);

    scrollbar_ = new ScrollBarViews(true);
    scrollbar_->SetBounds(0, 0, 100, 100);
    scrollbar_->Update(100, 1000, 0);
    scrollbar_->set_controller(controller_.get());
    container->AddChildView(scrollbar_);

    track_size_ = scrollbar_->GetTrackBounds().width();
  }

  void TearDown() override {
    widget_->Close();
    ViewsTestBase::TearDown();
  }

 protected:
  Widget* widget_;

  // This is the Views scrollbar.
  BaseScrollBar* scrollbar_;

  // Keep track of the size of the track. This is how we can tell when we
  // scroll to the middle.
  int track_size_;

  std::unique_ptr<TestScrollBarController> controller_;
};

// TODO(dnicoara) Can't run the test on Windows since the scrollbar |Part|
// isn't handled in NativeTheme.
#if defined(OS_WIN)
#define MAYBE_Scrolling DISABLED_Scrolling
#define MAYBE_ScrollBarFitsToBottom DISABLED_ScrollBarFitsToBottom
#else
#define MAYBE_Scrolling Scrolling
#define MAYBE_ScrollBarFitsToBottom ScrollBarFitsToBottom
#endif

TEST_F(ScrollBarViewsTest, MAYBE_Scrolling) {
  EXPECT_EQ(0, scrollbar_->GetPosition());
  EXPECT_EQ(900, scrollbar_->GetMaxPosition());
  EXPECT_EQ(0, scrollbar_->GetMinPosition());

  // Scroll to middle.
  scrollbar_->ScrollToThumbPosition(track_size_ / 2, true);
  EXPECT_EQ(450, controller_->last_position);
  EXPECT_EQ(scrollbar_, controller_->last_source);

  // Scroll to the end.
  scrollbar_->ScrollToThumbPosition(track_size_, true);
  EXPECT_EQ(900, controller_->last_position);

  // Overscroll. Last position should be the maximum position.
  scrollbar_->ScrollToThumbPosition(track_size_ + 100, true);
  EXPECT_EQ(900, controller_->last_position);

  // Underscroll. Last position should be the minimum position.
  scrollbar_->ScrollToThumbPosition(-10, false);
  EXPECT_EQ(0, controller_->last_position);

  // Test the different fixed scrolling amounts. Generally used by buttons,
  // or click on track.
  scrollbar_->ScrollToThumbPosition(0, false);
  scrollbar_->ScrollByAmount(BaseScrollBar::SCROLL_NEXT_LINE);
  EXPECT_EQ(10, controller_->last_position);

  scrollbar_->ScrollByAmount(BaseScrollBar::SCROLL_PREV_LINE);
  EXPECT_EQ(0, controller_->last_position);

  scrollbar_->ScrollByAmount(BaseScrollBar::SCROLL_NEXT_PAGE);
  EXPECT_EQ(20, controller_->last_position);

  scrollbar_->ScrollByAmount(BaseScrollBar::SCROLL_PREV_PAGE);
  EXPECT_EQ(0, controller_->last_position);
}

TEST_F(ScrollBarViewsTest, MAYBE_ScrollBarFitsToBottom) {
  scrollbar_->Update(100, 1999, 0);
  EXPECT_EQ(0, scrollbar_->GetPosition());
  EXPECT_EQ(1899, scrollbar_->GetMaxPosition());
  EXPECT_EQ(0, scrollbar_->GetMinPosition());

  // Scroll to the midpoint of the document.
  scrollbar_->Update(100, 1999, 950);
  EXPECT_EQ((scrollbar_->GetTrackBounds().width() -
             scrollbar_->GetThumbSizeForTest()) /
                2,
            scrollbar_->GetPosition());

  // Scroll to the end of the document.
  scrollbar_->Update(100, 1999, 1899);
  EXPECT_EQ(
      scrollbar_->GetTrackBounds().width() - scrollbar_->GetThumbSizeForTest(),
      scrollbar_->GetPosition());
}

TEST_F(ScrollBarViewsTest, ScrollToEndAfterShrinkAndExpand) {
  // Scroll to the end of the content.
  scrollbar_->Update(100, 1001, 0);
  EXPECT_TRUE(scrollbar_->ScrollByContentsOffset(-1));
  // Shrink and then re-expand the content.
  scrollbar_->Update(100, 1000, 0);
  scrollbar_->Update(100, 1001, 0);
  // Ensure the scrollbar allows scrolling to the end.
  EXPECT_TRUE(scrollbar_->ScrollByContentsOffset(-1));
}

TEST_F(ScrollBarViewsTest, ThumbFullLengthOfTrack) {
  // Shrink content so that it fits within the viewport.
  scrollbar_->Update(100, 10, 0);
  EXPECT_EQ(scrollbar_->GetTrackBounds().width(),
            scrollbar_->GetThumbSizeForTest());
  // Emulate a click on the full size scroll bar.
  scrollbar_->ScrollToThumbPosition(0, false);
  EXPECT_EQ(0, scrollbar_->GetPosition());
  // Emulate a key down.
  scrollbar_->ScrollByAmount(BaseScrollBar::SCROLL_NEXT_LINE);
  EXPECT_EQ(0, scrollbar_->GetPosition());

  // Expand content so that it fits *exactly* within the viewport.
  scrollbar_->Update(100, 100, 0);
  EXPECT_EQ(scrollbar_->GetTrackBounds().width(),
            scrollbar_->GetThumbSizeForTest());
  // Emulate a click on the full size scroll bar.
  scrollbar_->ScrollToThumbPosition(0, false);
  EXPECT_EQ(0, scrollbar_->GetPosition());
  // Emulate a key down.
  scrollbar_->ScrollByAmount(BaseScrollBar::SCROLL_NEXT_LINE);
  EXPECT_EQ(0, scrollbar_->GetPosition());
}

}  // namespace views
