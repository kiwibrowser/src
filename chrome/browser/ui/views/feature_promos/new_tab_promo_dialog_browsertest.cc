// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"

class NewTabPromoDialogTest : public DialogBrowserTest {
 public:
  NewTabPromoDialogTest() = default;

  // DialogBrowserTest:
  void ShowUi(const std::string& name) override {
    BrowserView::GetBrowserViewForBrowser(browser())
        ->tabstrip()
        ->new_tab_button()
        ->ShowPromo();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(NewTabPromoDialogTest);
};

// Test that calls ShowUi("default").
IN_PROC_BROWSER_TEST_F(NewTabPromoDialogTest, InvokeUi_NewTabPromo) {
  ShowAndVerifyUi();
}
