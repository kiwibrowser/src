// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_INSTALL_DIALOG_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_INSTALL_DIALOG_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "chrome/browser/extensions/extension_install_prompt.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_mac.h"
#import "chrome/browser/ui/cocoa/extensions/extension_install_view_controller.h"

class ExtensionInstallPromptShowParams;
@class ExtensionInstallViewController;

// Displays an extension install prompt as a tab modal dialog.
class ExtensionInstallDialogController :
    public ExtensionInstallViewDelegate,
    public ConstrainedWindowMacDelegate {
 public:
  ExtensionInstallDialogController(
      ExtensionInstallPromptShowParams* show_params,
      const ExtensionInstallPrompt::DoneCallback& done_callback,
      std::unique_ptr<ExtensionInstallPrompt::Prompt> prompt);
  ~ExtensionInstallDialogController() override;

  // ExtensionInstallViewDelegate implementation.
  void OnOkButtonClicked() override;
  void OnCancelButtonClicked() override;
  void OnStoreLinkClicked() override;

  // ConstrainedWindowMacDelegate implementation.
  void OnConstrainedWindowClosed(ConstrainedWindowMac* window) override;

  ConstrainedWindowMac* constrained_window() const {
    return constrained_window_.get();
  }
  ExtensionInstallViewController* view_controller() const {
    return view_controller_;
  }

 private:
  void OnPromptButtonClicked(ExtensionInstallPrompt::Result result);

  ExtensionInstallPrompt::DoneCallback done_callback_;
  base::scoped_nsobject<ExtensionInstallViewController> view_controller_;
  std::unique_ptr<ConstrainedWindowMac> constrained_window_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionInstallDialogController);
};

#endif  // CHROME_BROWSER_UI_COCOA_EXTENSIONS_EXTENSION_INSTALL_DIALOG_CONTROLLER_H_
