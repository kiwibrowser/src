// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PASSWORDS_IOS_CHROME_PASSWORD_MANAGER_INFOBAR_DELEGATE_H_
#define IOS_CHROME_BROWSER_PASSWORDS_IOS_CHROME_PASSWORD_MANAGER_INFOBAR_DELEGATE_H_

#include <memory>

#include "base/macros.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/password_manager/core/browser/password_manager_metrics_util.h"

@protocol ApplicationCommands;

namespace password_manager {
class PasswordFormManagerForUI;
}

// Base class for password manager infobar delegates, e.g.
// IOSChromeSavePasswordInfoBarDelegate and
// IOSChromeUpdatePasswordInfoBarDelegate. Provides link text and action for
// smart lock.
class IOSChromePasswordManagerInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  ~IOSChromePasswordManagerInfoBarDelegate() override;

 protected:
  IOSChromePasswordManagerInfoBarDelegate(
      bool is_smart_lock_branding_enabled,
      std::unique_ptr<password_manager::PasswordFormManagerForUI> form_manager);

  password_manager::PasswordFormManagerForUI* form_to_save() const {
    return form_to_save_.get();
  }

  bool is_smart_lock_branding_enabled() const {
    return is_smart_lock_branding_enabled_;
  }

  void set_infobar_response(
      password_manager::metrics_util::UIDismissalReason response) {
    infobar_response_ = response;
  }

  password_manager::metrics_util::UIDismissalReason infobar_response() const {
    return infobar_response_;
  }

  void set_dispatcher(id<ApplicationCommands> dispatcher) {
    dispatcher_ = dispatcher;
  }

 private:
  // ConfirmInfoBarDelegate implementation.
  base::string16 GetLinkText() const override;
  int GetIconId() const override;
  bool LinkClicked(WindowOpenDisposition disposition) override;

  // The password_manager::PasswordFormManager managing the form we're asking
  // the user about, and should save as per their decision.
  std::unique_ptr<password_manager::PasswordFormManagerForUI> form_to_save_;

  // Used to track the results we get from the info bar.
  password_manager::metrics_util::UIDismissalReason infobar_response_;

  // Whether to show the password manager branded as Smart Lock.
  const bool is_smart_lock_branding_enabled_;

  // Dispatcher for calling Application commands.
  __weak id<ApplicationCommands> dispatcher_ = nil;

  DISALLOW_COPY_AND_ASSIGN(IOSChromePasswordManagerInfoBarDelegate);
};
#endif  // IOS_CHROME_BROWSER_PASSWORDS_IOS_CHROME_PASSWORD_MANAGER_INFOBAR_DELEGATE_H_
