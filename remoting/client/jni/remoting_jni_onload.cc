// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/base_jni_onload.h"
#include "base/android/jni_android.h"
#include "base/bind.h"
#include "base/macros.h"
#include "remoting/client/remoting_jni_registration.h"

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  base::android::InitVM(vm);
  JNIEnv* env = base::android::AttachCurrentThread();
  if (!RegisterMainDexNatives(env) || !RegisterNonMainDexNatives(env)) {
    return -1;
  }

  if (!base::android::OnJNIOnLoadInit()) {
    return -1;
  }
  return JNI_VERSION_1_4;
}
