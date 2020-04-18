// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TRANSLATE_ANDROID_TRANSLATE_UTILS_H_
#define CHROME_BROWSER_TRANSLATE_ANDROID_TRANSLATE_UTILS_H_

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"

namespace translate {
class TranslateInfoBarDelegate;
}

class TranslateUtils {
 public:
  // A Java counterpart will be generated for this enum.
  // GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.infobar
  // GENERATED_JAVA_PREFIX_TO_STRIP:OPTION_
  enum TranslateOption {
    OPTION_SOURCE_CODE,
    OPTION_TARGET_CODE,
    OPTION_ALWAYS_TRANSLATE,
    OPTION_NEVER_TRANSLATE,
    OPTION_NEVER_TRANSLATE_SITE
  };

  // A Java counterpart will be generated for this enum.
  // GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.infobar
  // GENERATED_JAVA_PREFIX_TO_STRIP:TYPE_
  enum TranslateSnackbarType {
    TYPE_NONE,
    TYPE_ALWAYS_TRANSLATE,
    TYPE_NEVER_TRANSLATE,
    TYPE_NEVER_TRANSLATE_SITE
  };

  static base::android::ScopedJavaLocalRef<jobjectArray> GetJavaLanguages(
      JNIEnv* env,
      translate::TranslateInfoBarDelegate* delegate);
  static base::android::ScopedJavaLocalRef<jobjectArray> GetJavaLanguageCodes(
      JNIEnv* env,
      translate::TranslateInfoBarDelegate* delegate);
  static base::android::ScopedJavaLocalRef<jintArray> GetJavaLanguageHashCodes(
      JNIEnv* env,
      translate::TranslateInfoBarDelegate* delegate);
};

#endif  // CHROME_BROWSER_TRANSLATE_ANDROID_TRANSLATE_UTILS_H_
