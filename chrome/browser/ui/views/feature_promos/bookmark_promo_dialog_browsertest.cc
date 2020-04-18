// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/browser/ui/views/feature_promos/bookmark_promo_bubble_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/location_bar/star_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"

class BookmarkPromoDialogTest : public DialogBrowserTest {
 public:
  BookmarkPromoDialogTest() = default;

  // DialogBrowserTest:
  void ShowUi(const std::string& name) override {
    BrowserView::GetBrowserViewForBrowser(browser())
        ->toolbar()
        ->location_bar()
        ->star_view()
        ->ShowPromo();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(BookmarkPromoDialogTest);
};

// Test that calls ShowUi("default").
IN_PROC_BROWSER_TEST_F(BookmarkPromoDialogTest, InvokeUi_BookmarkPromoBubble) {
  ShowAndVerifyUi();
}
