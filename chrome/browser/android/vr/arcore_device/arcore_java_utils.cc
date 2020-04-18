// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/vr/arcore_device/arcore_java_utils.h"

#include "chrome/browser/android/vr/arcore_device/arcore_shim.h"
#include "jni/ArCoreJavaUtils_jni.h"

using base::android::AttachCurrentThread;
using base::android::ScopedJavaLocalRef;

namespace vr {

ScopedJavaLocalRef<jobject> ArCoreJavaUtils::GetApplicationContext() {
  JNIEnv* env = AttachCurrentThread();
  return Java_ArCoreJavaUtils_getApplicationContext(env);
}

bool ArCoreJavaUtils::EnsureLoaded() {
  JNIEnv* env = AttachCurrentThread();
  if (!Java_ArCoreJavaUtils_shouldLoadArCoreSdk(env))
    return false;

  return LoadArCoreSdk();
}

}  // namespace vr
