// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/jni_string.h"
#include "components/web_restrictions/browser/web_restrictions_client_result.h"
#include "jni/WebRestrictionsClientResult_jni.h"

namespace web_restrictions {

WebRestrictionsClientResult::WebRestrictionsClientResult(
    base::android::ScopedJavaGlobalRef<jobject>& jresult)
    : jresult_(jresult) {}

WebRestrictionsClientResult::~WebRestrictionsClientResult() = default;

WebRestrictionsClientResult::WebRestrictionsClientResult(
    const WebRestrictionsClientResult& other) = default;

int WebRestrictionsClientResult::GetInt(int column) const {
  return Java_WebRestrictionsClientResult_getInt(
      base::android::AttachCurrentThread(), jresult_, column);
}

std::string WebRestrictionsClientResult::GetString(int column) const {
  JNIEnv* env = base::android::AttachCurrentThread();
  return base::android::ConvertJavaStringToUTF8(
      env, Java_WebRestrictionsClientResult_getString(env, jresult_, column));
}

std::string WebRestrictionsClientResult::GetColumnName(int column) const {
  JNIEnv* env = base::android::AttachCurrentThread();
  return base::android::ConvertJavaStringToUTF8(
      env,
      Java_WebRestrictionsClientResult_getColumnName(env, jresult_, column));
}

bool WebRestrictionsClientResult::ShouldProceed() const {
  return Java_WebRestrictionsClientResult_shouldProceed(
      base::android::AttachCurrentThread(), jresult_);
}

int web_restrictions::WebRestrictionsClientResult::GetColumnCount() const {
  return Java_WebRestrictionsClientResult_getColumnCount(
      base::android::AttachCurrentThread(), jresult_);
}

bool web_restrictions::WebRestrictionsClientResult::IsString(int column) const {
  return Java_WebRestrictionsClientResult_isString(
      base::android::AttachCurrentThread(), jresult_, column);
}

}  // namespace web_restrictions
