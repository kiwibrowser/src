// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/extensions/extension_install_dialog_controller.h"

#include <utility>

#include "chrome/browser/extensions/extension_install_prompt_show_params.h"
#include "chrome/browser/extensions/extension_install_prompt_test_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet_controller.h"
#import "chrome/browser/ui/cocoa/extensions/extension_install_prompt_test_utils.h"
#import "chrome/browser/ui/cocoa/extensions/extension_install_view_controller.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "extensions/common/extension.h"

using extensions::Extension;

class ExtensionInstallDialogControllerTest : public InProcessBrowserTest {
public:
  ExtensionInstallDialogControllerTest() {}

  void SetUpOnMainThread() override {
    extension_ = chrome::LoadInstallPromptExtension();
  }

 protected:
  scoped_refptr<Extension> extension_;
};

IN_PROC_BROWSER_TEST_F(ExtensionInstallDialogControllerTest, BasicTest) {
  content::WebContents* tab = browser()->tab_strip_model()->GetWebContentsAt(0);
  ExtensionInstallPromptShowParams show_params(tab);

  ExtensionInstallPromptTestHelper test_helper;
  std::unique_ptr<ExtensionInstallPrompt::Prompt> prompt =
      chrome::BuildExtensionInstallPrompt(extension_.get());

  ExtensionInstallDialogController* controller =
      new ExtensionInstallDialogController(
          &show_params, test_helper.GetCallback(), std::move(prompt));

  base::scoped_nsobject<NSWindow> window(
      [[[controller->view_controller() view] window] retain]);
  EXPECT_TRUE([window isVisible]);

  // Press cancel to close the window.
  [[controller->view_controller() cancelButton] performClick:nil];

  // Wait for the window to finish closing.
  EXPECT_FALSE([window isVisible]);

  EXPECT_EQ(ExtensionInstallPrompt::Result::USER_CANCELED,
            test_helper.result());
}

IN_PROC_BROWSER_TEST_F(ExtensionInstallDialogControllerTest,
                       DISABLED_Permissions) {
  content::WebContents* tab = browser()->tab_strip_model()->GetWebContentsAt(0);
  ExtensionInstallPromptShowParams show_params(tab);

  ExtensionInstallPromptTestHelper test_helper;
  std::unique_ptr<ExtensionInstallPrompt::Prompt> prompt =
      chrome::BuildExtensionPostInstallPermissionsPrompt(extension_.get());

  ExtensionInstallDialogController* controller =
      new ExtensionInstallDialogController(
          &show_params, test_helper.GetCallback(), std::move(prompt));

  base::scoped_nsobject<NSWindow> window(
      [[[controller->view_controller() view] window] retain]);
  EXPECT_TRUE([window isVisible]);

  // Press cancel to close the window.
  [[controller->view_controller() cancelButton] performClick:nil];

  // Wait for the window to finish closing.
  EXPECT_FALSE([window isVisible]);

  EXPECT_EQ(ExtensionInstallPrompt::Result::USER_CANCELED,
            test_helper.result());
}
