// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/devtools/devtools_infobar_delegate.h"

#include "chrome/browser/devtools/global_confirm_info_bar.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

// static
void DevToolsInfoBarDelegate::Create(const base::string16& message,
                                     const Callback& callback) {
  std::unique_ptr<ConfirmInfoBarDelegate> delegate(
      new DevToolsInfoBarDelegate(message, callback));
  GlobalConfirmInfoBar::Show(std::move(delegate));
}

DevToolsInfoBarDelegate::DevToolsInfoBarDelegate(const base::string16& message,
                                                 const Callback& callback)
    : ConfirmInfoBarDelegate(), message_(message), callback_(callback) {}

DevToolsInfoBarDelegate::~DevToolsInfoBarDelegate() {
  if (!callback_.is_null())
    callback_.Run(false);
}

infobars::InfoBarDelegate::InfoBarIdentifier
DevToolsInfoBarDelegate::GetIdentifier() const {
  return DEV_TOOLS_INFOBAR_DELEGATE;
}

base::string16 DevToolsInfoBarDelegate::GetMessageText() const {
  return message_;
}

base::string16 DevToolsInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  return l10n_util::GetStringUTF16((button == BUTTON_OK)
                                       ? IDS_DEV_TOOLS_CONFIRM_ALLOW_BUTTON
                                       : IDS_DEV_TOOLS_CONFIRM_DENY_BUTTON);
}

bool DevToolsInfoBarDelegate::Accept() {
  callback_.Run(true);
  callback_.Reset();
  return true;
}

bool DevToolsInfoBarDelegate::Cancel() {
  callback_.Run(false);
  callback_.Reset();
  return true;
}
