// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_PASSWORD_REUSE_WARNING_DIALOG_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_PASSWORD_REUSE_WARNING_DIALOG_COCOA_H_

#import <Cocoa/Cocoa.h>

#import "base/mac/scoped_nsobject.h"
#include "chrome/browser/safe_browsing/chrome_password_protection_service.h"
#include "content/public/browser/web_contents_observer.h"

@class ConstrainedWindowCustomWindow;
@class PasswordReuseWarningViewController;

// A modal dialog that warns users about a password reuse.
class PasswordReuseWarningDialogCocoa
    : public safe_browsing::ChromePasswordProtectionService::Observer,
      public content::WebContentsObserver {
 public:
  PasswordReuseWarningDialogCocoa(
      content::WebContents* web_contents,
      safe_browsing::ChromePasswordProtectionService* service,
      safe_browsing::OnWarningDone callback);

  ~PasswordReuseWarningDialogCocoa() override;

  // ChromePasswordProtectionService::Observer:
  void OnGaiaPasswordChanged() override;
  void OnMarkingSiteAsLegitimate(const GURL& url) override;
  void InvokeActionForTesting(
      safe_browsing::ChromePasswordProtectionService::WarningAction action)
      override;
  safe_browsing::ChromePasswordProtectionService::WarningUIType
  GetObserverType() override;

  // content::WebContentsObserver:
  void WebContentsDestroyed() override;

  // Called by |controller_| when a dialog button is selected.
  void OnChangePassword();
  void OnIgnore();

  // Closes the dialog.
  void Close();

  // Called by |controller_| to get the detailed warning text.
  base::string16 GetWarningDetailText();

 private:
  // This class observes the |service_| to check if the password reuse
  // status has changed. Weak.
  safe_browsing::ChromePasswordProtectionService* service_;

  // The url of the site that triggered this dialog.
  const GURL url_;

  // Dialog button callback.
  safe_browsing::OnWarningDone callback_;

  // The sheet that contains the dialog view.
  base::scoped_nsobject<ConstrainedWindowCustomWindow> sheet_;

  // The window that runs the modal dialog. Weak.
  NSWindow* parent_window_;

  // Controller for the dialog view.
  base::scoped_nsobject<PasswordReuseWarningViewController> controller_;

  DISALLOW_COPY_AND_ASSIGN(PasswordReuseWarningDialogCocoa);
};

#endif  // CHROME_BROWSER_UI_COCOA_PASSWORD_REUSE_WARNING_DIALOG_COCOA_H_
