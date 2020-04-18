// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_error_ui.h"

#include "base/logging.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/global_error/global_error.h"
#include "chrome/grit/generated_resources.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_set.h"
#include "ui/base/l10n/l10n_util.h"

namespace extensions {

namespace {

base::string16 GenerateMessageSection(const ExtensionSet& extensions,
                                      int extension_template_message_id,
                                      int app_template_message_id) {
  CHECK(extension_template_message_id);
  CHECK(app_template_message_id);
  base::string16 message;

  for (ExtensionSet::const_iterator iter = extensions.begin();
       iter != extensions.end();
       ++iter) {
    message += l10n_util::GetStringFUTF16(
        (*iter)->is_app() ? app_template_message_id
                          : extension_template_message_id,
        base::UTF8ToUTF16((*iter)->name())) + base::char16('\n');
  }
  return message;
}

}  // namespace

ExtensionErrorUI::ExtensionErrorUI(Delegate* delegate) : delegate_(delegate) {}

ExtensionErrorUI::~ExtensionErrorUI() {}

std::vector<base::string16> ExtensionErrorUI::GetBubbleViewMessages() {
  if (message_.empty()) {
    message_ = GenerateMessage();
    if (message_[message_.size()-1] == '\n')
      message_.resize(message_.size()-1);
  }
  return std::vector<base::string16>(1, message_);
}

base::string16 ExtensionErrorUI::GetBubbleViewTitle() {
  return l10n_util::GetStringUTF16(IDS_EXTENSION_ALERT_TITLE);
}

base::string16 ExtensionErrorUI::GetBubbleViewAcceptButtonLabel() {
  return l10n_util::GetStringUTF16(IDS_EXTENSION_ALERT_ITEM_OK);
}

base::string16 ExtensionErrorUI::GetBubbleViewCancelButtonLabel() {
  return l10n_util::GetStringUTF16(IDS_EXTENSION_ALERT_ITEM_DETAILS);
}

void ExtensionErrorUI::BubbleViewDidClose() {
  delegate_->OnAlertClosed();
}

void ExtensionErrorUI::BubbleViewAcceptButtonPressed() {
  delegate_->OnAlertAccept();
}

void ExtensionErrorUI::BubbleViewCancelButtonPressed() {
  delegate_->OnAlertDetails();
}

base::string16 ExtensionErrorUI::GenerateMessage() {
  return GenerateMessageSection(delegate_->GetExternalExtensions(),
                                IDS_EXTENSION_ALERT_ITEM_EXTERNAL,
                                IDS_APP_ALERT_ITEM_EXTERNAL) +
         GenerateMessageSection(delegate_->GetBlacklistedExtensions(),
                                IDS_EXTENSION_ALERT_ITEM_BLACKLISTED,
                                IDS_APP_ALERT_ITEM_BLACKLISTED);
}

}  // namespace extensions
