// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/profiles/profile_signin_confirmation_dialog_cocoa.h"

#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_sheet.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_window.h"
#include "chrome/browser/ui/sync/profile_signin_confirmation_helper.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

namespace {

// static
void ShowDialog(Browser* browser,
                content::WebContents* web_contents,
                Profile* profile,
                const std::string& username,
                std::unique_ptr<ui::ProfileSigninConfirmationDelegate> delegate,
                bool offer_profile_creation) {
  // The dialog owns itself.
  new ProfileSigninConfirmationDialogCocoa(browser, web_contents, profile,
                                           username, std::move(delegate),
                                           offer_profile_creation);
}

}  // namespace

ProfileSigninConfirmationDialogCocoa::ProfileSigninConfirmationDialogCocoa(
    Browser* browser,
    content::WebContents* web_contents,
    Profile* profile,
    const std::string& username,
    std::unique_ptr<ui::ProfileSigninConfirmationDelegate> delegate,
    bool offer_profile_creation) {
  // Setup the dialog view controller.
  const base::Closure& closeDialogCallback =
      base::Bind(&ProfileSigninConfirmationDialogCocoa::Close,
                 base::Unretained(this));
  controller_.reset([[ProfileSigninConfirmationViewController alloc]
           initWithBrowser:browser
                  username:username
                  delegate:std::move(delegate)
       closeDialogCallback:closeDialogCallback
      offerProfileCreation:offer_profile_creation]);

  // Setup the constrained window that will show the view.
  base::scoped_nsobject<NSWindow> window([[ConstrainedWindowCustomWindow alloc]
      initWithContentRect:[[controller_ view] bounds]]);
  [[window contentView] addSubview:[controller_ view]];
  base::scoped_nsobject<CustomConstrainedWindowSheet> sheet(
      [[CustomConstrainedWindowSheet alloc] initWithCustomWindow:window]);
  window_ = CreateAndShowWebModalDialogMac(this, web_contents, sheet);
}

ProfileSigninConfirmationDialogCocoa::~ProfileSigninConfirmationDialogCocoa() {
}

// static
void ProfileSigninConfirmationDialogCocoa::Show(
    Browser* browser,
    content::WebContents* web_contents,
    Profile* profile,
    const std::string& username,
    std::unique_ptr<ui::ProfileSigninConfirmationDelegate> delegate) {
  ui::CheckShouldPromptForNewProfile(
      profile, base::Bind(ShowDialog, browser, web_contents, profile, username,
                          base::Passed(std::move(delegate))));
}

void ProfileSigninConfirmationDialogCocoa::Close() {
  window_->CloseWebContentsModalDialog();
}

void ProfileSigninConfirmationDialogCocoa::OnConstrainedWindowClosed(
    ConstrainedWindowMac* window) {
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}
