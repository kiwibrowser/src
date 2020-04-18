// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/password_reuse_warning_dialog_cocoa.h"

#include "chrome/browser/ui/cocoa/browser_dialogs_views_mac.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_window.h"
#import "chrome/browser/ui/cocoa/password_reuse_warning_view_controller.h"

namespace safe_browsing {

void ShowPasswordReuseModalWarningDialog(
    content::WebContents* web_contents,
    ChromePasswordProtectionService* service,
    OnWarningDone done_callback) {
  DCHECK(web_contents);

  if (chrome::ShowAllDialogsWithViewsToolkit()) {
    chrome::ShowPasswordReuseWarningDialog(web_contents, service,
                                           std::move(done_callback));
    return;
  }

  // Dialog owns itself.
  new PasswordReuseWarningDialogCocoa(web_contents, service,
                                      std::move(done_callback));
}

}  // namespace safe_browsing

PasswordReuseWarningDialogCocoa::PasswordReuseWarningDialogCocoa(
    content::WebContents* web_contents,
    safe_browsing::ChromePasswordProtectionService* service,
    safe_browsing::OnWarningDone callback)
    : content::WebContentsObserver(web_contents),
      service_(service),
      url_(web_contents->GetLastCommittedURL()),
      callback_(std::move(callback)) {
  controller_.reset(
      [[PasswordReuseWarningViewController alloc] initWithOwner:this]);

  sheet_.reset([[ConstrainedWindowCustomWindow alloc]
      initWithContentRect:[[controller_ view] bounds]]);
  [[sheet_ contentView] addSubview:[controller_ view]];
  [sheet_ makeFirstResponder:controller_.get()];

  parent_window_ = web_contents->GetTopLevelNativeWindow();
  [parent_window_ beginSheet:sheet_.get()
           completionHandler:^(NSModalResponse result) {
             [sheet_ close];
             [NSApp stopModal];

           }];

  if (service_)
    service_->AddObserver(this);
}

PasswordReuseWarningDialogCocoa::~PasswordReuseWarningDialogCocoa() {
  if (service_)
    service_->RemoveObserver(this);
}

void PasswordReuseWarningDialogCocoa::OnGaiaPasswordChanged() {
  Close();
}

void PasswordReuseWarningDialogCocoa::OnMarkingSiteAsLegitimate(
    const GURL& url) {
  if (url_.GetWithEmptyPath() == url.GetWithEmptyPath())
    Close();
}

void PasswordReuseWarningDialogCocoa::InvokeActionForTesting(
    safe_browsing::ChromePasswordProtectionService::WarningAction action) {
  switch (action) {
    case safe_browsing::ChromePasswordProtectionService::CHANGE_PASSWORD:
      OnChangePassword();
      break;
    case safe_browsing::ChromePasswordProtectionService::IGNORE_WARNING:
      OnIgnore();
      break;
    case safe_browsing::ChromePasswordProtectionService::CLOSE:
      Close();
      break;
    default:
      NOTREACHED();
      break;
  }
}

safe_browsing::ChromePasswordProtectionService::WarningUIType
PasswordReuseWarningDialogCocoa::GetObserverType() {
  return safe_browsing::ChromePasswordProtectionService::MODAL_DIALOG;
}

void PasswordReuseWarningDialogCocoa::OnChangePassword() {
  std::move(callback_).Run(
      safe_browsing::PasswordProtectionService::CHANGE_PASSWORD);
  Close();
}

void PasswordReuseWarningDialogCocoa::OnIgnore() {
  std::move(callback_).Run(
      safe_browsing::PasswordProtectionService::IGNORE_WARNING);
  Close();
}

void PasswordReuseWarningDialogCocoa::Close() {
  if (callback_)
    std::move(callback_).Run(safe_browsing::PasswordProtectionService::CLOSE);

  [parent_window_ endSheet:sheet_.get() returnCode:NSModalResponseStop];
}

void PasswordReuseWarningDialogCocoa::WebContentsDestroyed() {
  Close();
}

base::string16 PasswordReuseWarningDialogCocoa::GetWarningDetailText() {
  return service_->GetWarningDetailText();
}
