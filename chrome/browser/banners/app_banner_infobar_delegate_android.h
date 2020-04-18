// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BANNERS_APP_BANNER_INFOBAR_DELEGATE_ANDROID_H_
#define CHROME_BROWSER_BANNERS_APP_BANNER_INFOBAR_DELEGATE_ANDROID_H_

#include <memory>

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "components/infobars/core/confirm_infobar_delegate.h"

namespace content {
class WebContents;
}

namespace banners {

class AppBannerUiDelegateAndroid;

// Manages installation of an app being promoted by a page.
class AppBannerInfoBarDelegateAndroid : public ConfirmInfoBarDelegate {
 public:
  // Creates an infobar and delegate for promoting the installation of an app,
  // and displays the infobar for |web_contents|.
  static bool Create(content::WebContents* web_contents,
                     std::unique_ptr<AppBannerUiDelegateAndroid> ui_delegate);

  ~AppBannerInfoBarDelegateAndroid() override;

  const AppBannerUiDelegateAndroid* GetUiDelegate() const;

  // Called when the AppBannerInfoBarAndroid's button needs to be updated.
  void UpdateInstallState(JNIEnv* env,
                          const base::android::JavaParamRef<jobject>& obj);

  // Called when the native app installation Intent has been handled and focus
  // has been returned to Chrome.
  void OnInstallIntentReturned(JNIEnv* env,
                               const base::android::JavaParamRef<jobject>& obj,
                               jboolean jis_installing);

  // Called when the native app InstallerDelegate task has finished.
  void OnInstallFinished(JNIEnv* env,
                         const base::android::JavaParamRef<jobject>& obj,
                         jboolean success);

  // ConfirmInfoBarDelegate:
  bool Accept() override;
  base::string16 GetMessageText() const override;

 private:
  AppBannerInfoBarDelegateAndroid(
      std::unique_ptr<AppBannerUiDelegateAndroid> ui_delegate);

  void CreateJavaDelegate(JNIEnv* env);

  // ConfirmInfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  void InfoBarDismissed() override;
  int GetButtons() const override;
  bool LinkClicked(WindowOpenDisposition disposition) override;

  std::unique_ptr<AppBannerUiDelegateAndroid> ui_delegate_;
  base::android::ScopedJavaGlobalRef<jobject> java_delegate_;

  DISALLOW_COPY_AND_ASSIGN(AppBannerInfoBarDelegateAndroid);
};

}  // namespace banners

#endif  // CHROME_BROWSER_BANNERS_APP_BANNER_INFOBAR_DELEGATE_ANDROID_H_
