// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_SAFE_BROWSING_PASSWORD_REUSE_MODAL_WARNING_DIALOG_H_
#define CHROME_BROWSER_UI_VIEWS_SAFE_BROWSING_PASSWORD_REUSE_MODAL_WARNING_DIALOG_H_

#include "base/callback.h"
#include "chrome/browser/safe_browsing/chrome_password_protection_service.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/views/window/dialog_delegate.h"

namespace content {
class WebContents;
}  // namespace content

namespace safe_browsing {

// Implementation of password reuse modal dialog.
class PasswordReuseModalWarningDialog
    : public views::DialogDelegateView,
      public ChromePasswordProtectionService::Observer,
      public content::WebContentsObserver {
 public:
  PasswordReuseModalWarningDialog(content::WebContents* web_contents,
                                  ChromePasswordProtectionService* service,
                                  OnWarningDone done_callback);

  ~PasswordReuseModalWarningDialog() override;

  // views::DialogDelegateView:
  gfx::Size CalculatePreferredSize() const override;
  ui::ModalType GetModalType() const override;
  base::string16 GetWindowTitle() const override;
  bool ShouldShowCloseButton() const override;
  gfx::ImageSkia GetWindowIcon() override;
  bool ShouldShowWindowIcon() const override;
  bool Cancel() override;
  bool Accept() override;
  bool Close() override;
  int GetDefaultDialogButton() const override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;

  // ChromePasswordProtectionService::Observer:
  void OnGaiaPasswordChanged() override;
  void OnMarkingSiteAsLegitimate(const GURL& url) override;
  void InvokeActionForTesting(
      ChromePasswordProtectionService::WarningAction action) override;
  ChromePasswordProtectionService::WarningUIType GetObserverType() override;

  // content::WebContentsObserver:
  void WebContentsDestroyed() override;

 private:
  OnWarningDone done_callback_;
  ChromePasswordProtectionService* service_;
  const GURL url_;

  DISALLOW_COPY_AND_ASSIGN(PasswordReuseModalWarningDialog);
};

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_UI_VIEWS_SAFE_BROWSING_PASSWORD_REUSE_MODAL_WARNING_DIALOG_H_
