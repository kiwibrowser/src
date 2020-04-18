// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"

#include "base/command_line.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/test_with_browser_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "ui/base/ui_base_switches.h"
#include "url/gurl.h"

class BrowserNonClientFrameViewTest : public TestWithBrowserView {
 public:
  explicit BrowserNonClientFrameViewTest(Browser::Type type)
      : TestWithBrowserView(type, false), frame_view_(nullptr) {}

  // TestWithBrowserView override:
  void SetUp() override {
#if defined(OS_WIN)
    // Use opaque frame.
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kDisableDwmComposition);
#endif
    TestWithBrowserView::SetUp();
    views::Widget* widget = browser_view()->GetWidget();
    frame_view_ = static_cast<BrowserNonClientFrameView*>(
        widget->non_client_view()->frame_view());
  }

 protected:
  // Owned by the browser view.
  BrowserNonClientFrameView* frame_view_;

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserNonClientFrameViewTest);
};

class BrowserNonClientFrameViewPopupTest
    : public BrowserNonClientFrameViewTest {
 public:
  BrowserNonClientFrameViewPopupTest()
      : BrowserNonClientFrameViewTest(Browser::TYPE_POPUP) {}
};

TEST_F(BrowserNonClientFrameViewPopupTest, HitTestPopupTopChrome) {
  EXPECT_FALSE(frame_view_->HitTestRect(gfx::Rect(-1, 4, 1, 1)));
  EXPECT_FALSE(frame_view_->HitTestRect(gfx::Rect(4, -1, 1, 1)));
  const int top_inset = frame_view_->GetTopInset(false);
  EXPECT_FALSE(frame_view_->HitTestRect(gfx::Rect(4, top_inset, 1, 1)));
  if (top_inset > 0)
    EXPECT_TRUE(frame_view_->HitTestRect(gfx::Rect(4, top_inset - 1, 1, 1)));
}

class BrowserNonClientFrameViewTabbedTest
    : public BrowserNonClientFrameViewTest {
 public:
  BrowserNonClientFrameViewTabbedTest()
      : BrowserNonClientFrameViewTest(Browser::TYPE_TABBED) {}
};

TEST_F(BrowserNonClientFrameViewTabbedTest, HitTestTabstrip) {
  gfx::Rect tabstrip_bounds =
      frame_view_->browser_view()->tabstrip()->GetLocalBounds();
  EXPECT_FALSE(tabstrip_bounds.IsEmpty());

  // Completely outside bounds.
  EXPECT_FALSE(frame_view_->HitTestRect(
      gfx::Rect(tabstrip_bounds.x() - 1, tabstrip_bounds.y() + 1, 1, 1)));
  EXPECT_FALSE(frame_view_->HitTestRect(
      gfx::Rect(tabstrip_bounds.x() + 1, tabstrip_bounds.y() - 1, 1, 1)));

  // Hits tab strip but not client area.
  EXPECT_TRUE(frame_view_->HitTestRect(
      gfx::Rect(tabstrip_bounds.x() + 1, tabstrip_bounds.bottom() - 1, 1, 1)));

  // Hits tab strip and client area.
  EXPECT_TRUE(frame_view_->HitTestRect(gfx::Rect(
      tabstrip_bounds.x() + 1, tabstrip_bounds.bottom() - 1, 100, 100)));
}
