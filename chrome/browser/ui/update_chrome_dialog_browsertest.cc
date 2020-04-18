// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"

class UpdateRecommendedDialogTest : public DialogBrowserTest {
 public:
  UpdateRecommendedDialogTest() {}

  // DialogBrowserTest:
  void SetUp() override {
    UseMdOnly();
    DialogBrowserTest::SetUp();
  }

  void ShowUi(const std::string& name) override {
    InProcessBrowserTest::browser()->window()->ShowUpdateChromeDialog();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(UpdateRecommendedDialogTest);
};

// Test that calls ShowUi("default").
IN_PROC_BROWSER_TEST_F(UpdateRecommendedDialogTest, InvokeUi_default) {
  ShowAndVerifyUi();
}
