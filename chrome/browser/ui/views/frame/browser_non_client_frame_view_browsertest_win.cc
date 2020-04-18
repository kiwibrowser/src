// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"

#include <windows.h>

#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "ui/views/widget/widget.h"
#include "ui/views/win/hwnd_util.h"

using BrowserNonClientFrameViewTestWin = InProcessBrowserTest;

namespace {

int NonClientHitTest(HWND hwnd, int x, int y) {
  return ::SendMessage(hwnd, WM_NCHITTEST, 0, MAKELPARAM(x, y));
}

void HitTestPerimeter(views::View* view) {
  ASSERT_TRUE(view);
  ASSERT_TRUE(view->visible());

  HWND hwnd = views::HWNDForView(view);
  gfx::Rect rect = view->GetBoundsInScreen();

  // Coordinates within the bounds: left/middle/right and top/middle/bottom.
  const int xs[] = { rect.x(), rect.x() + rect.width() / 2, rect.right() - 1 };
  const int ys[] = { rect.y(), rect.y() + rect.height() / 2, rect.bottom() - 1};

  for (int y : ys) {
    EXPECT_NE(HTCLIENT, NonClientHitTest(hwnd, xs[0] - 1, y));
    EXPECT_EQ(HTCLIENT, NonClientHitTest(hwnd, xs[0], y));
    EXPECT_EQ(HTCLIENT, NonClientHitTest(hwnd, xs[2], y));
    EXPECT_NE(HTCLIENT, NonClientHitTest(hwnd, xs[2] + 1, y));
  }
  for (int x : xs) {
    EXPECT_NE(HTCLIENT, NonClientHitTest(hwnd, x, ys[0] - 1));
    EXPECT_EQ(HTCLIENT, NonClientHitTest(hwnd, x, ys[0]));
    EXPECT_EQ(HTCLIENT, NonClientHitTest(hwnd, x, ys[2]));
    EXPECT_NE(HTCLIENT, NonClientHitTest(hwnd, x, ys[2] + 1));
  }
}

}  // namespace

IN_PROC_BROWSER_TEST_F(BrowserNonClientFrameViewTestWin, HitTestFrameItems) {
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  views::Widget* widget = browser_view->GetWidget();

  BrowserNonClientFrameView* frame_view =
      static_cast<BrowserNonClientFrameView*>(
          widget->non_client_view()->frame_view());

  EXPECT_NO_FATAL_FAILURE(
      HitTestPerimeter(frame_view->GetViewByID(VIEW_ID_AVATAR_BUTTON)));
}
