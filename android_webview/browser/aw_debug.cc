// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/common/crash_reporter/aw_microdump_crash_reporter.h"
#include "android_webview/common/crash_reporter/crash_keys.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/debug/dump_without_crashing.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/threading/thread_restrictions.h"
#include "components/crash/core/common/crash_key.h"
#include "jni/AwDebug_jni.h"

using base::android::ConvertJavaStringToUTF16;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace android_webview {

static jboolean JNI_AwDebug_DumpWithoutCrashing(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    const JavaParamRef<jstring>& dump_path) {
  // This may be called from any thread, and we might be in a state
  // where it is impossible to post tasks, so we have to be prepared
  // to do IO from this thread.
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  base::File target(base::FilePath(ConvertJavaStringToUTF8(env, dump_path)),
                    base::File::FLAG_OPEN_TRUNCATED | base::File::FLAG_READ |
                        base::File::FLAG_WRITE);
  if (!target.IsValid())
    return false;
  // breakpad_linux::HandleCrashDump will close this fd once it is done.
  return crash_reporter::DumpWithoutCrashingToFd(target.TakePlatformFile());
}

static void JNI_AwDebug_InitCrashKeysForWebViewTesting(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz) {
  crash_keys::InitCrashKeysForWebViewTesting();
}

static void JNI_AwDebug_SetWhiteListedKeyForTesting(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz) {
  static ::crash_reporter::CrashKeyString<32> crash_key(
      "AW_WHITELISTED_DEBUG_KEY");
  crash_key.Set("AW_DEBUG_VALUE");
}

static void JNI_AwDebug_SetNonWhiteListedKeyForTesting(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz) {
  static ::crash_reporter::CrashKeyString<32> crash_key(
      "AW_NONWHITELISTED_DEBUG_KEY");
  crash_key.Set("AW_DEBUG_VALUE");
}

}  // namespace android_webview
