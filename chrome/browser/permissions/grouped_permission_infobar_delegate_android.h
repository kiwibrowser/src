// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PERMISSIONS_GROUPED_PERMISSION_INFOBAR_DELEGATE_ANDROID_H_
#define CHROME_BROWSER_PERMISSIONS_GROUPED_PERMISSION_INFOBAR_DELEGATE_ANDROID_H_

#include <memory>

#include "base/callback.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/infobars/core/confirm_infobar_delegate.h"

class InfoBarService;
class PermissionPromptAndroid;

// An InfoBar that displays a group of permission requests, each of which can be
// allowed or blocked independently.
// TODO(timloh): This is incorrectly named as we've removed grouped permissions,
// rename it to PermissionInfoBarDelegate once crbug.com/606138 is done.
class GroupedPermissionInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  // Public so we can have std::unique_ptr<GroupedPermissionInfoBarDelegate>.
  ~GroupedPermissionInfoBarDelegate() override;

  static infobars::InfoBar* Create(
      const base::WeakPtr<PermissionPromptAndroid>& permission_prompt,
      InfoBarService* infobar_service);

  size_t PermissionCount() const;

  ContentSettingsType GetContentSettingType(size_t position) const;

  // InfoBarDelegate:
  int GetIconId() const override;

  // ConfirmInfoBarDelegate:
  base::string16 GetMessageText() const override;
  bool Accept() override;
  bool Cancel() override;
  void InfoBarDismissed() override;

 private:
  GroupedPermissionInfoBarDelegate(
      const base::WeakPtr<PermissionPromptAndroid>& permission_prompt);

  // ConfirmInfoBarDelegate:
  InfoBarIdentifier GetIdentifier() const override;
  int GetButtons() const override;
  base::string16 GetButtonLabel(InfoBarButton button) const override;

  // InfoBarDelegate:
  bool EqualsDelegate(infobars::InfoBarDelegate* delegate) const override;

  base::WeakPtr<PermissionPromptAndroid> permission_prompt_;

  DISALLOW_COPY_AND_ASSIGN(GroupedPermissionInfoBarDelegate);
};

#endif  // CHROME_BROWSER_PERMISSIONS_GROUPED_PERMISSION_INFOBAR_DELEGATE_ANDROID_H_
