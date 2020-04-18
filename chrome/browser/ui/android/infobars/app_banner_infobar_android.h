// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ANDROID_INFOBARS_APP_BANNER_INFOBAR_ANDROID_H_
#define CHROME_BROWSER_UI_ANDROID_INFOBARS_APP_BANNER_INFOBAR_ANDROID_H_

#include <memory>

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "chrome/browser/ui/android/infobars/confirm_infobar.h"

namespace banners {
class AppBannerInfoBarDelegateAndroid;
}  // namespace banners


class AppBannerInfoBarAndroid : public ConfirmInfoBar {
 public:
  AppBannerInfoBarAndroid(
      std::unique_ptr<banners::AppBannerInfoBarDelegateAndroid> delegate);

  ~AppBannerInfoBarAndroid() override;

  // Called when the installation state of the app may have changed.
  // Updates the InfoBar visuals to match the new state and re-enables controls
  // that may have been disabled.
  void OnInstallStateChanged(int new_state);

 private:
  banners::AppBannerInfoBarDelegateAndroid* GetDelegate();

  // InfoBarAndroid overrides.
  base::android::ScopedJavaLocalRef<jobject> CreateRenderInfoBar(
      JNIEnv* env) override;

  base::android::ScopedJavaGlobalRef<jobject> java_infobar_;

  DISALLOW_COPY_AND_ASSIGN(AppBannerInfoBarAndroid);
};

#endif  // CHROME_BROWSER_UI_ANDROID_INFOBARS_APP_BANNER_INFOBAR_ANDROID_H_
