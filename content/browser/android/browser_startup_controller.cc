// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/browser_startup_controller.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "content/browser/android/content_startup_flags.h"
#include "content/browser/browser_main_loop.h"
#include "ppapi/buildflags/buildflags.h"

#include "jni/BrowserStartupController_jni.h"

using base::android::JavaParamRef;

namespace content {

void BrowserStartupComplete(int result) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BrowserStartupController_browserStartupComplete(env, result);
}

bool ShouldStartGpuProcessOnBrowserStartup() {
  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_BrowserStartupController_shouldStartGpuProcessOnBrowserStartup(
      env);
}

static void JNI_BrowserStartupController_SetCommandLineFlags(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    jboolean single_process,
    const JavaParamRef<jstring>& plugin_descriptor) {
  std::string plugin_str =
      (plugin_descriptor == NULL
           ? std::string()
           : base::android::ConvertJavaStringToUTF8(env, plugin_descriptor));
  SetContentCommandLineFlags(static_cast<bool>(single_process), plugin_str);
}

static jboolean JNI_BrowserStartupController_IsOfficialBuild(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz) {
#if defined(OFFICIAL_BUILD)
  return true;
#else
  return false;
#endif
}

static jboolean JNI_BrowserStartupController_IsPluginEnabled(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz) {
#if BUILDFLAG(ENABLE_PLUGINS)
  return true;
#else
  return false;
#endif
}

static void JNI_BrowserStartupController_FlushStartupTasks(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz) {
  BrowserMainLoop::GetInstance()->SynchronouslyFlushStartupTasks();
}

}  // namespace content
