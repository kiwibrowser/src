// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/incognito_window/incognito_window_tracker.h"
#include "chrome/browser/feature_engagement/incognito_window/incognito_window_tracker_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"

namespace feature_engagement {

namespace {

class IncognitoWindowPromoDialogTest : public DialogBrowserTest {
 public:
  IncognitoWindowPromoDialogTest() = default;

  // DialogBrowserTest:
  void ShowUi(const std::string& name) override {
    feature_engagement::IncognitoWindowTrackerFactory::GetInstance()
        ->GetForProfile(browser()->profile())
        ->ShowPromo();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(IncognitoWindowPromoDialogTest);
};

}  // namespace

// Test that calls ShowUi("default").
IN_PROC_BROWSER_TEST_F(IncognitoWindowPromoDialogTest,
                       InvokeUi_IncognitoWindowPromo) {
  ShowAndVerifyUi();
}

}  // namespace feature_engagement
