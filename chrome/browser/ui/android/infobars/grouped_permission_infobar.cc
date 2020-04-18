// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/infobars/grouped_permission_infobar.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "chrome/browser/android/resource_mapper.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/permissions/grouped_permission_infobar_delegate_android.h"
#include "chrome/browser/ui/android/infobars/permission_infobar.h"

GroupedPermissionInfoBar::GroupedPermissionInfoBar(
    std::unique_ptr<GroupedPermissionInfoBarDelegate> delegate)
    : ConfirmInfoBar(std::move(delegate)) {}

GroupedPermissionInfoBar::~GroupedPermissionInfoBar() {
}

base::android::ScopedJavaLocalRef<jobject>
GroupedPermissionInfoBar::CreateRenderInfoBar(JNIEnv* env) {
  GroupedPermissionInfoBarDelegate* delegate = GetDelegate();

  base::string16 message_text = delegate->GetMessageText();
  base::string16 link_text = delegate->GetLinkText();
  base::string16 ok_button_text = GetTextFor(ConfirmInfoBarDelegate::BUTTON_OK);
  base::string16 cancel_button_text =
      GetTextFor(ConfirmInfoBarDelegate::BUTTON_CANCEL);

  int permission_icon =
      ResourceMapper::MapFromChromiumId(delegate->GetIconId());

  std::vector<int> content_settings_types;
  for (size_t i = 0; i < delegate->PermissionCount(); i++) {
    content_settings_types.push_back(delegate->GetContentSettingType(i));
  }

  return PermissionInfoBar::CreateRenderInfoBarHelper(
      env, permission_icon, GetTab()->GetJavaObject(), nullptr, message_text,
      link_text, ok_button_text, cancel_button_text, content_settings_types);
}

GroupedPermissionInfoBarDelegate* GroupedPermissionInfoBar::GetDelegate() {
  return static_cast<GroupedPermissionInfoBarDelegate*>(delegate());
}
