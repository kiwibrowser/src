// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "chrome/common/chrome_content_client.h"
#include "jni/ContentUtils_jni.h"

static base::android::ScopedJavaLocalRef<jstring>
JNI_ContentUtils_GetBrowserUserAgent(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& clazz) {
  return base::android::ConvertUTF8ToJavaString(env, GetUserAgent());
}
