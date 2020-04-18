// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_MANAGER_INFOBAR_DELEGATE_ANDROID_H_
#define CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_MANAGER_INFOBAR_DELEGATE_ANDROID_H_

#include "base/macros.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "ui/gfx/range/range.h"

// Base class for some of the password manager infobar delegates, e.g.
// SavePasswordInfoBarDelegate.
class PasswordManagerInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  ~PasswordManagerInfoBarDelegate() override;

  const gfx::Range& message_link_range() const { return message_link_range_; }

  // ConfirmInfoBarDelegate:
  InfoBarAutomationType GetInfoBarAutomationType() const override;
  int GetIconId() const override;
  bool ShouldExpire(const NavigationDetails& details) const override;
  base::string16 GetMessageText() const override;
  GURL GetLinkURL() const override;
  bool LinkClicked(WindowOpenDisposition disposition) override;

 protected:
  PasswordManagerInfoBarDelegate();

  void SetMessage(const base::string16& message);
  void SetMessageLinkRange(const gfx::Range& message_link_range);

 private:
  // Message for the infobar: branded as a part of Google Smart Lock for signed
  // users.
  base::string16 message_;

  // If set, describes the location of the link to the help center article for
  // Smart Lock.
  gfx::Range message_link_range_;

  DISALLOW_COPY_AND_ASSIGN(PasswordManagerInfoBarDelegate);
};

#endif  // CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_MANAGER_INFOBAR_DELEGATE_ANDROID_H_
