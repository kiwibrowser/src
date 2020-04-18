// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/jni_android.h"
#include "base/android/jni_utils.h"
#include "base/android/library_loader/library_loader_hooks.h"
#include "base/bind.h"
#include "content/public/app/content_jni_onload.h"
#include "content/public/app/content_main.h"
#include "content/public/browser/android/compositor.h"
#include "content/shell/android/content_shell_jni_registration.h"
#include "content/shell/app/shell_main_delegate.h"

// This is called by the VM when the shared library is first loaded.
JNI_EXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  base::android::InitVM(vm);
  JNIEnv* env = base::android::AttachCurrentThread();
  if (!RegisterMainDexNatives(env))
    return -1;

  // Do not register JNI methods in secondary dex for non-browser process.
  bool is_browser_process =
      !base::android::IsSelectiveJniRegistrationEnabled(env);
  if (is_browser_process && !RegisterNonMainDexNatives(env))
    return -1;
  if (!content::android::OnJNIOnLoadInit())
    return -1;

  content::Compositor::Initialize();
  content::SetContentMainDelegate(new content::ShellMainDelegate());
  return JNI_VERSION_1_4;
}
