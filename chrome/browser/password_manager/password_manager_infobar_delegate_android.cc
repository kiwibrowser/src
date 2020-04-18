// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/password_manager/password_manager_infobar_delegate_android.h"

#include "chrome/browser/android/android_theme_resources.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "components/infobars/core/infobar.h"
#include "components/password_manager/core/browser/password_manager_constants.h"
#include "content/public/browser/web_contents.h"

PasswordManagerInfoBarDelegate::~PasswordManagerInfoBarDelegate() {}

PasswordManagerInfoBarDelegate::PasswordManagerInfoBarDelegate()
    : ConfirmInfoBarDelegate(), message_link_range_(gfx::Range()) {}

infobars::InfoBarDelegate::InfoBarAutomationType
PasswordManagerInfoBarDelegate::GetInfoBarAutomationType() const {
  return PASSWORD_INFOBAR;
}

int PasswordManagerInfoBarDelegate::GetIconId() const {
  return IDR_ANDROID_INFOBAR_SAVE_PASSWORD;
}

bool PasswordManagerInfoBarDelegate::ShouldExpire(
    const NavigationDetails& details) const {
  return !details.is_redirect && ConfirmInfoBarDelegate::ShouldExpire(details);
}

base::string16 PasswordManagerInfoBarDelegate::GetMessageText() const {
  return message_;
}

GURL PasswordManagerInfoBarDelegate::GetLinkURL() const {
  return GURL(password_manager::kPasswordManagerHelpCenterSmartLock);
}

bool PasswordManagerInfoBarDelegate::LinkClicked(
    WindowOpenDisposition disposition) {
  ConfirmInfoBarDelegate::LinkClicked(disposition);
  return true;
}

void PasswordManagerInfoBarDelegate::SetMessage(const base::string16& message) {
  message_ = message;
}

void PasswordManagerInfoBarDelegate::SetMessageLinkRange(
    const gfx::Range& message_link_range) {
  message_link_range_ = message_link_range;
}
