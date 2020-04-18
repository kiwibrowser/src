// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/translate/android/translate_utils.h"

#include <stddef.h>

#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/android/jni_weak_ref.h"
#include "components/metrics/metrics_log.h"
#include "components/translate/core/browser/translate_infobar_delegate.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

ScopedJavaLocalRef<jobjectArray> TranslateUtils::GetJavaLanguages(
    JNIEnv* env,
    translate::TranslateInfoBarDelegate* delegate) {
  std::vector<base::string16> languages;
  languages.reserve(delegate->num_languages());
  for (size_t i = 0; i < delegate->num_languages(); ++i) {
    languages.push_back(delegate->language_name_at(i));
  }
  return base::android::ToJavaArrayOfStrings(env, languages);
}

ScopedJavaLocalRef<jobjectArray> TranslateUtils::GetJavaLanguageCodes(
    JNIEnv* env,
    translate::TranslateInfoBarDelegate* delegate) {
  std::vector<std::string> codes;
  codes.reserve(delegate->num_languages());
  for (size_t i = 0; i < delegate->num_languages(); ++i) {
    codes.push_back(delegate->language_code_at(i));
  }
  return base::android::ToJavaArrayOfStrings(env, codes);
}

ScopedJavaLocalRef<jintArray> TranslateUtils::GetJavaLanguageHashCodes(
    JNIEnv* env,
    translate::TranslateInfoBarDelegate* delegate) {
  std::vector<int> hashCodes;
  hashCodes.reserve(delegate->num_languages());
  for (size_t i = 0; i < delegate->num_languages(); ++i) {
    hashCodes.push_back(
        metrics::MetricsLog::Hash(delegate->language_code_at(i)));
  }
  return base::android::ToJavaIntArray(env, hashCodes);
}
