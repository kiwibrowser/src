// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/permissions/grouped_permission_infobar_delegate_android.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/android/android_theme_resources.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/permissions/permission_prompt_android.h"
#include "chrome/browser/permissions/permission_request.h"
#include "chrome/browser/permissions/permission_util.h"
#include "chrome/browser/ui/android/infobars/grouped_permission_infobar.h"
#include "chrome/grit/generated_resources.h"
#include "components/infobars/core/infobar.h"
#include "components/url_formatter/elide_url.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/strings/grit/ui_strings.h"

GroupedPermissionInfoBarDelegate::~GroupedPermissionInfoBarDelegate() {}

// static
infobars::InfoBar* GroupedPermissionInfoBarDelegate::Create(
    const base::WeakPtr<PermissionPromptAndroid>& permission_prompt,
    InfoBarService* infobar_service) {
  return infobar_service->AddInfoBar(
      std::make_unique<GroupedPermissionInfoBar>(base::WrapUnique(
          new GroupedPermissionInfoBarDelegate(permission_prompt))));
}

size_t GroupedPermissionInfoBarDelegate::PermissionCount() const {
  return permission_prompt_->PermissionCount();
}

ContentSettingsType GroupedPermissionInfoBarDelegate::GetContentSettingType(
    size_t position) const {
  return permission_prompt_->GetContentSettingType(position);
}

int GroupedPermissionInfoBarDelegate::GetIconId() const {
  return permission_prompt_->GetIconId();
}

base::string16 GroupedPermissionInfoBarDelegate::GetMessageText() const {
  return permission_prompt_->GetMessageText();
}

bool GroupedPermissionInfoBarDelegate::Accept() {
  if (permission_prompt_)
    permission_prompt_->Accept();
  return true;
}

bool GroupedPermissionInfoBarDelegate::Cancel() {
  if (permission_prompt_)
    permission_prompt_->Deny();
  return true;
}

void GroupedPermissionInfoBarDelegate::InfoBarDismissed() {
  if (permission_prompt_)
    permission_prompt_->Closing();
}

GroupedPermissionInfoBarDelegate::GroupedPermissionInfoBarDelegate(
    const base::WeakPtr<PermissionPromptAndroid>& permission_prompt)
    : permission_prompt_(permission_prompt) {
  DCHECK(permission_prompt);
}

infobars::InfoBarDelegate::InfoBarIdentifier
GroupedPermissionInfoBarDelegate::GetIdentifier() const {
  return GROUPED_PERMISSION_INFOBAR_DELEGATE_ANDROID;
}

int GroupedPermissionInfoBarDelegate::GetButtons() const {
  return BUTTON_OK | BUTTON_CANCEL;
}

base::string16 GroupedPermissionInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  return l10n_util::GetStringUTF16((button == BUTTON_OK) ? IDS_PERMISSION_ALLOW
                                                         : IDS_PERMISSION_DENY);
}

bool GroupedPermissionInfoBarDelegate::EqualsDelegate(
    infobars::InfoBarDelegate* delegate) const {
  // The PermissionRequestManager doesn't create duplicate infobars so a pointer
  // equality check is sufficient.
  return this == delegate;
}
