// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/platform_window/android/platform_ime_controller_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "jni/PlatformImeControllerAndroid_jni.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace ui {

PlatformImeControllerAndroid::PlatformImeControllerAndroid() {
}

PlatformImeControllerAndroid::~PlatformImeControllerAndroid() {
}

void PlatformImeControllerAndroid::Init(JNIEnv* env,
                                        const JavaParamRef<jobject>& jobj) {
  DCHECK(java_platform_ime_controller_android_.is_uninitialized());
  java_platform_ime_controller_android_ = JavaObjectWeakGlobalRef(env, jobj);
}

void PlatformImeControllerAndroid::UpdateTextInputState(
    const TextInputState& state) {
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> scoped_obj =
      java_platform_ime_controller_android_.get(env);
  if (scoped_obj.is_null())
    return;
  Java_PlatformImeControllerAndroid_updateTextInputState(
      env, scoped_obj, state.type, state.flags,
      base::android::ConvertUTF8ToJavaString(env, state.text),
      state.selection_start, state.selection_end, state.composition_start,
      state.composition_end);
}

void PlatformImeControllerAndroid::SetImeVisibility(bool visible) {
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jobject> scoped_obj =
      java_platform_ime_controller_android_.get(env);
  if (scoped_obj.is_null())
    return;
  Java_PlatformImeControllerAndroid_setImeVisibility(env, scoped_obj, visible);
}

}  // namespace ui
