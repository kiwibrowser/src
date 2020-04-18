// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/set_as_default_browser_ui_win.h"

#include "base/command_line.h"
#include "base/win/windows_version.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "ui/views/widget/widget.h"

namespace {

bool IsBrowserVisible(Browser* browser) {
  return views::Widget::GetWidgetForNativeWindow(
             browser->window()->GetNativeWindow())
      ->IsVisible();
}

}  // namespace

using SetAsDefaultBrowserUIBrowserTestWithoutFirstRun = InProcessBrowserTest;

class SetAsDefaultBrowserUIBrowserTestWithFirstRun
    : public InProcessBrowserTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kForceFirstRun);
  }

 protected:
  void TearDownInProcessBrowserTestFixture() override {
    ASSERT_FALSE(SetAsDefaultBrowserUI::GetDialogWidgetForTesting());
  }
};

IN_PROC_BROWSER_TEST_F(SetAsDefaultBrowserUIBrowserTestWithFirstRun, Test) {
  // Windows 8 only test case.
  if (base::win::GetVersion() != base::win::VERSION_WIN8 &&
      base::win::GetVersion() != base::win::VERSION_WIN8_1) {
    return;
  }
  ASSERT_FALSE(IsBrowserVisible(browser()));
  views::Widget* dialog_widget =
      SetAsDefaultBrowserUI::GetDialogWidgetForTesting();
  ASSERT_TRUE(dialog_widget);
  ASSERT_TRUE(dialog_widget->IsVisible());
  dialog_widget->CloseNow();
  ASSERT_TRUE(IsBrowserVisible(browser()));
}

IN_PROC_BROWSER_TEST_F(SetAsDefaultBrowserUIBrowserTestWithoutFirstRun,
                       TestWithoutFirstRun) {
  ASSERT_TRUE(IsBrowserVisible(browser()));
  EXPECT_EQ(nullptr, SetAsDefaultBrowserUI::GetDialogWidgetForTesting());
}
