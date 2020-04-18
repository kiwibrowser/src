// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/one_click_signin_dialog_controller.h"

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_sheet.h"
#include "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_window.h"
#import "chrome/browser/ui/cocoa/one_click_signin_view_controller.h"

OneClickSigninDialogController::OneClickSigninDialogController(
    content::WebContents* web_contents,
    const BrowserWindow::StartSyncCallback& sync_callback,
    const base::string16& email) {
  base::Closure close_callback = base::Bind(
      &OneClickSigninDialogController::PerformClose, base::Unretained(this));
  view_controller_.reset([[OneClickSigninViewController alloc]
      initWithNibName:@"OneClickSigninDialog"
          webContents:web_contents
         syncCallback:sync_callback
        closeCallback:close_callback
         isSyncDialog:YES
                email:email
         errorMessage:nil]);
  base::scoped_nsobject<NSWindow> window([[ConstrainedWindowCustomWindow alloc]
      initWithContentRect:[[view_controller_ view] bounds]]);
  [[window contentView] addSubview:[view_controller_ view]];

  base::scoped_nsobject<CustomConstrainedWindowSheet> sheet(
      [[CustomConstrainedWindowSheet alloc] initWithCustomWindow:window]);
  constrained_window_ =
      CreateAndShowWebModalDialogMac(this, web_contents, sheet);
}

OneClickSigninDialogController::~OneClickSigninDialogController() {
}

void OneClickSigninDialogController::OnConstrainedWindowClosed(
  ConstrainedWindowMac* window) {
  [view_controller_ viewWillClose];
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}

void OneClickSigninDialogController::PerformClose() {
  constrained_window_->CloseWebContentsModalDialog();
}
