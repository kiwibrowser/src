// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/extensions/chooser_dialog_cocoa.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/chooser_controller/chooser_controller.h"
#include "chrome/browser/extensions/api/chrome_device_permissions_prompt.h"
#include "chrome/browser/extensions/chrome_extension_chooser_dialog.h"
#include "chrome/browser/extensions/device_permissions_dialog_controller.h"
#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_sheet.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_window.h"
#import "chrome/browser/ui/cocoa/extensions/chooser_dialog_cocoa_controller.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"

ChooserDialogCocoa::ChooserDialogCocoa(
    content::WebContents* web_contents,
    std::unique_ptr<ChooserController> chooser_controller)
    : web_contents_(web_contents) {
  DCHECK(web_contents_);
  DCHECK(chooser_controller);
  chooser_dialog_cocoa_controller_.reset([[ChooserDialogCocoaController alloc]
      initWithChooserDialogCocoa:this
               chooserController:std::move(chooser_controller)]);
}

ChooserDialogCocoa::~ChooserDialogCocoa() {}

void ChooserDialogCocoa::OnConstrainedWindowClosed(
    ConstrainedWindowMac* window) {
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}

void ChooserDialogCocoa::ShowDialog() {
  base::scoped_nsobject<NSWindow> window([[ConstrainedWindowCustomWindow alloc]
      initWithContentRect:[[chooser_dialog_cocoa_controller_ view] bounds]]);
  [[window contentView] addSubview:[chooser_dialog_cocoa_controller_ view]];
  base::scoped_nsobject<CustomConstrainedWindowSheet> sheet(
      [[CustomConstrainedWindowSheet alloc] initWithCustomWindow:window]);
  constrained_window_ =
      CreateAndShowWebModalDialogMac(this, web_contents_, sheet);
}

void ChooserDialogCocoa::Dismissed() {
  if (constrained_window_)
    constrained_window_->CloseWebContentsModalDialog();
}

void ChromeExtensionChooserDialog::ShowDialog(
    std::unique_ptr<ChooserController> chooser_controller) const {
  if (chrome::ShowAllDialogsWithViewsToolkit()) {
    return ChromeExtensionChooserDialog::ShowDialogImpl(
        std::move(chooser_controller));
  }

  web_modal::WebContentsModalDialogManager* manager =
      web_modal::WebContentsModalDialogManager::FromWebContents(web_contents_);
  if (manager) {
    // These objects will delete themselves when the dialog closes.
    ChooserDialogCocoa* chooser_dialog =
        new ChooserDialogCocoa(web_contents_, std::move(chooser_controller));
    chooser_dialog->ShowDialog();
  }
}

void ChromeDevicePermissionsPrompt::ShowDialog() {
  if (chrome::ShowAllDialogsWithViewsToolkit())
    return ChromeDevicePermissionsPrompt::ShowDialogViews();

  std::unique_ptr<ChooserController> chooser_controller(
      new DevicePermissionsDialogController(web_contents()->GetMainFrame(),
                                            prompt()));

  // These objects will delete themselves when the dialog closes.
  ChooserDialogCocoa* chooser_dialog =
      new ChooserDialogCocoa(web_contents(), std::move(chooser_controller));
  chooser_dialog->ShowDialog();
}
