// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/infobars/mock_confirm_infobar_delegate.h"

#include "base/strings/utf_string_conversions.h"

const char MockConfirmInfoBarDelegate::kMessage[] = "MockConfirmInfoBarMessage";

MockConfirmInfoBarDelegate::MockConfirmInfoBarDelegate(Owner* owner)
    : ConfirmInfoBarDelegate(),
      owner_(owner),
      closes_on_action_(true),
      icon_accessed_(false),
      message_text_accessed_(false),
      link_text_accessed_(false),
      ok_clicked_(false),
      cancel_clicked_(false),
      link_clicked_(false) {
}

MockConfirmInfoBarDelegate::~MockConfirmInfoBarDelegate() {
  if (owner_)
    owner_->OnInfoBarDelegateClosed(this);
}

infobars::InfoBarDelegate::InfoBarIdentifier
MockConfirmInfoBarDelegate::GetIdentifier() const {
  return TEST_INFOBAR;
}

int MockConfirmInfoBarDelegate::GetIconId() const {
  icon_accessed_ = true;
  return kNoIconID;
}

base::string16 MockConfirmInfoBarDelegate::GetMessageText() const {
  message_text_accessed_ = true;
  return base::ASCIIToUTF16(kMessage);
}

base::string16 MockConfirmInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  return base::ASCIIToUTF16((button == BUTTON_OK) ? "OK" : "Cancel");
}

bool MockConfirmInfoBarDelegate::Accept() {
  ok_clicked_ = true;
  return closes_on_action_;
}

bool MockConfirmInfoBarDelegate::Cancel() {
  cancel_clicked_ = true;
  return closes_on_action_;
}

base::string16 MockConfirmInfoBarDelegate::GetLinkText() const {
  link_text_accessed_ = true;
  return base::string16();
}

bool MockConfirmInfoBarDelegate::LinkClicked(
    WindowOpenDisposition disposition) {
  link_clicked_ = true;
  return closes_on_action_;
}
