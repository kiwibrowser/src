// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_ANDROID_INTERSTITIAL_PAGE_DELEGATE_ANDROID_H_
#define CONTENT_PUBLIC_TEST_ANDROID_INTERSTITIAL_PAGE_DELEGATE_ANDROID_H_

#include <jni.h>
#include <string>

#include "base/android/jni_weak_ref.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/interstitial_page_delegate.h"

namespace content {

class InterstitialPage;

// Native counterpart that allows interstitial pages to be constructed and
// managed from Java.
class InterstitialPageDelegateAndroid : public InterstitialPageDelegate {
 public:
  InterstitialPageDelegateAndroid(JNIEnv* env,
                                  jobject obj,
                                  const std::string& html_content);
  ~InterstitialPageDelegateAndroid() override;

  void set_interstitial_page(InterstitialPage* page) { page_ = page; }

  // Methods called from Java.
  void Proceed(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  void DontProceed(JNIEnv* env,
                   const base::android::JavaParamRef<jobject>& obj);
  void ShowInterstitialPage(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& jurl,
      const base::android::JavaParamRef<jobject>& jweb_contents);

  // Implementation of InterstitialPageDelegate
  std::string GetHTMLContents() override;
  void OnProceed() override;
  void OnDontProceed() override;
  void CommandReceived(const std::string& command) override;

 private:
  JavaObjectWeakGlobalRef weak_java_obj_;

  std::string html_content_;
  InterstitialPage* page_;  // Owns this.

  DISALLOW_COPY_AND_ASSIGN(InterstitialPageDelegateAndroid);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_ANDROID_INTERSTITIAL_PAGE_DELEGATE_ANDROID_H_
