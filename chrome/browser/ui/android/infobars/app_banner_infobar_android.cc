// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/infobars/app_banner_infobar_android.h"

#include <utility>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "chrome/browser/banners/app_banner_infobar_delegate_android.h"
#include "chrome/browser/banners/app_banner_ui_delegate_android.h"
#include "components/url_formatter/elide_url.h"
#include "jni/AppBannerInfoBarAndroid_jni.h"
#include "ui/gfx/android/java_bitmap.h"

AppBannerInfoBarAndroid::AppBannerInfoBarAndroid(
    std::unique_ptr<banners::AppBannerInfoBarDelegateAndroid> delegate)
    : ConfirmInfoBar(std::move(delegate)) {}

AppBannerInfoBarAndroid::~AppBannerInfoBarAndroid() {}

base::android::ScopedJavaLocalRef<jobject>
AppBannerInfoBarAndroid::CreateRenderInfoBar(JNIEnv* env) {
  const banners::AppBannerUiDelegateAndroid* delegate =
      GetDelegate()->GetUiDelegate();

  base::android::ScopedJavaLocalRef<jstring> app_title =
      base::android::ConvertUTF16ToJavaString(env, delegate->GetAppTitle());

  DCHECK(!delegate->GetPrimaryIcon().drawsNothing());
  base::android::ScopedJavaLocalRef<jobject> java_bitmap =
      gfx::ConvertToJavaBitmap(&delegate->GetPrimaryIcon());

  base::android::ScopedJavaLocalRef<jobject> infobar;
  if (delegate->GetType() ==
      banners::AppBannerUiDelegateAndroid::AppType::NATIVE) {
    infobar.Reset(Java_AppBannerInfoBarAndroid_createNativeAppInfoBar(
        env, app_title, java_bitmap, delegate->GetNativeAppData()));
  } else {
    // Trim down the app URL to the origin. Banners only show on secure origins,
    // so elide the scheme.
    base::android::ScopedJavaLocalRef<jstring> app_url =
        base::android::ConvertUTF16ToJavaString(
            env, url_formatter::FormatUrlForSecurityDisplay(
                     delegate->GetWebAppUrl(),
                     url_formatter::SchemeDisplay::OMIT_CRYPTOGRAPHIC));

    infobar.Reset(Java_AppBannerInfoBarAndroid_createWebAppInfoBar(
        env, app_title, java_bitmap, app_url));
  }

  java_infobar_.Reset(env, infobar.obj());
  return infobar;
}

void AppBannerInfoBarAndroid::OnInstallStateChanged(int new_state) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_AppBannerInfoBarAndroid_onInstallStateChanged(env, java_infobar_,
                                                     new_state);
}

banners::AppBannerInfoBarDelegateAndroid*
AppBannerInfoBarAndroid::GetDelegate() {
  return static_cast<banners::AppBannerInfoBarDelegateAndroid*>(delegate());
}
