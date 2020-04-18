// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/policy/enterprise_startup_dialog_view.h"

#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"

namespace policy {
namespace {
constexpr char kMessage[] = "message";
constexpr char kButton[] = "button";

void DialogResultCallback(bool result, bool can_show_browser_window) {}
}  // namespace

class EnterpriseStartupDialogViewBrowserTest : public DialogBrowserTest {
 public:
  EnterpriseStartupDialogViewBrowserTest() = default;
  ~EnterpriseStartupDialogViewBrowserTest() override = default;

  // override DialogBrowserTest
  void ShowUi(const std::string& name) override {
    dialog =
        new EnterpriseStartupDialogView(base::BindOnce(&DialogResultCallback));
    if (name == "Information") {
      dialog->DisplayLaunchingInformationWithThrobber(
          base::ASCIIToUTF16(kMessage));
    } else if (name == "Error") {
      dialog->DisplayErrorMessage(base::ASCIIToUTF16(kMessage),
                                  base::ASCIIToUTF16(kButton),
                                  base::ASCIIToUTF16(kButton));
    } else if (name == "Switch") {
      dialog->DisplayLaunchingInformationWithThrobber(
          base::ASCIIToUTF16(kMessage));
      dialog->DisplayErrorMessage(base::ASCIIToUTF16(kMessage),
                                  base::ASCIIToUTF16(kButton),
                                  base::ASCIIToUTF16(kButton));
    }
  }

 private:
  EnterpriseStartupDialogView* dialog;

  DISALLOW_COPY_AND_ASSIGN(EnterpriseStartupDialogViewBrowserTest);
};

IN_PROC_BROWSER_TEST_F(EnterpriseStartupDialogViewBrowserTest,
                       InvokeUi_Information) {
  ShowAndVerifyUi();
}

IN_PROC_BROWSER_TEST_F(EnterpriseStartupDialogViewBrowserTest, InvokeUi_Error) {
  ShowAndVerifyUi();
}

IN_PROC_BROWSER_TEST_F(EnterpriseStartupDialogViewBrowserTest,
                       InvokeUi_Switch) {
  ShowAndVerifyUi();
}

}  // namespace policy
