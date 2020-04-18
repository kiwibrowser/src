// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PERMISSIONS_PERMISSION_UPDATE_INFOBAR_DELEGATE_ANDROID_H_
#define CHROME_BROWSER_PERMISSIONS_PERMISSION_UPDATE_INFOBAR_DELEGATE_ANDROID_H_

#include <string>
#include <vector>

#include "base/android/scoped_java_ref.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/infobars/core/confirm_infobar_delegate.h"

namespace content {
class WebContents;
}

// An infobar delegate to be used for requesting missing Android runtime
// permissions for previously allowed ContentSettingsTypes.
class PermissionUpdateInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  using PermissionUpdatedCallback = base::Callback<void(bool)>;

  // Creates an infobar to resolve conflicts in Android runtime permissions.
  // The necessary runtime permissions are generated based on the list of
  // ContentSettingsTypes passed in. Returns the infobar if it was successfully
  // added.
  //
  // This function can only be called with one of
  // CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC,
  // CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA or
  // CONTENT_SETTINGS_TYPE_GEOLOCATION, or with both
  // CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC and
  // CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA.
  //
  // The |callback| will not be triggered if this is deleted.
  static infobars::InfoBar* Create(
      content::WebContents* web_contents,
      const std::vector<ContentSettingsType>& content_settings_types,
      const PermissionUpdatedCallback& callback);

  // Creates an infobar to resolve conflicts in Android runtime permissions.
  // Returns the infobar if it was successfully added.
  //
  // The |callback| will not be triggered if this is deleted.
  static infobars::InfoBar* Create(
      content::WebContents* web_contents,
      const std::vector<std::string>& android_permissions,
      int permission_msg_id,
      const PermissionUpdatedCallback& callback);

  // Return whether the runtime permissions currently granted to Chrome by
  // Android are compatible with ContentSettingTypes previously granted to a
  // site by the user.
  static bool ShouldShowPermissionInfobar(
      content::WebContents* web_contents,
      const std::vector<ContentSettingsType>& content_settings_types);

  void OnPermissionResult(JNIEnv* env,
                          const base::android::JavaParamRef<jobject>& obj,
                          jboolean all_permissions_granted);

 private:
  PermissionUpdateInfoBarDelegate(
      content::WebContents* web_contents,
      const std::vector<std::string>& android_permissions,
      int permission_msg_id,
      const PermissionUpdatedCallback& callback);
  ~PermissionUpdateInfoBarDelegate() override;

  // InfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;

  // PermissionInfoBarDelegate:
  int GetIconId() const override;
  base::string16 GetMessageText() const override;

  // ConfirmInfoBarDelegate:
  int GetButtons() const override;
  base::string16 GetButtonLabel(InfoBarButton button) const override;
  bool Accept() override;
  bool Cancel() override;

  // InfoBarDelegate:
  void InfoBarDismissed() override;

  base::android::ScopedJavaGlobalRef<jobject> java_delegate_;
  std::vector<std::string> android_permissions_;
  int permission_msg_id_;
  PermissionUpdatedCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(PermissionUpdateInfoBarDelegate);
};

#endif  // CHROME_BROWSER_PERMISSIONS_PERMISSION_UPDATE_INFOBAR_DELEGATE_ANDROID_H_
