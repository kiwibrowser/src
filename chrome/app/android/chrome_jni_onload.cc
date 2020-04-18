// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/app/android/chrome_jni_onload.h"

#include "base/android/jni_android.h"
#include "base/android/jni_registrar.h"
#include "base/android/jni_utils.h"
#include "chrome/app/android/chrome_android_initializer.h"
#include "content/public/app/content_jni_onload.h"
#include "device/vr/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_VR)
#include "third_party/gvr-android-sdk/display_synchronizer_jni.h"
#include "third_party/gvr-android-sdk/gvr_api_jni.h"
#include "third_party/gvr-android-sdk/native_callbacks_jni.h"
#endif

namespace android {

// These VR native functions are not handled by the automatic registration, so
// they are manually registered here.
#if BUILDFLAG(ENABLE_VR)
static base::android::RegistrationMethod kChromeRegisteredMethods[] = {
    {"DisplaySynchronizer",
     DisplaySynchronizer::RegisterDisplaySynchronizerNatives},
    {"GvrApi", GvrApi::RegisterGvrApiNatives},
    {"NativeCallbacks", NativeCallbacks::RegisterNativeCallbacksNatives},
};
#endif

bool OnJNIOnLoadRegisterJNI(JNIEnv* env) {
#if BUILDFLAG(ENABLE_VR)
  // Register manually when on the browser process.
  if (!base::android::IsSelectiveJniRegistrationEnabled(env)) {
    return RegisterNativeMethods(env, kChromeRegisteredMethods,
                                 arraysize(kChromeRegisteredMethods));
  }
#endif
  return true;
}

bool OnJNIOnLoadInit() {
  if (!content::android::OnJNIOnLoadInit())
    return false;

  return RunChrome();
}

}  // namespace android
