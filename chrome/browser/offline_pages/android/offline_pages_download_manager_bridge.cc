// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/android/offline_pages_download_manager_bridge.h"

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "jni/OfflinePagesDownloadManagerBridge_jni.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace offline_pages {
namespace android {

bool OfflinePagesDownloadManagerBridge::IsDownloadManagerInstalled() {
  JNIEnv* env = base::android::AttachCurrentThread();
  jboolean is_installed =
      Java_OfflinePagesDownloadManagerBridge_isAndroidDownloadManagerInstalled(
          env);
  return is_installed;
}

int64_t OfflinePagesDownloadManagerBridge::AddCompletedDownload(
    const std::string& title,
    const std::string& description,
    const std::string& path,
    int64_t length,
    const std::string& uri,
    const std::string& referer) {
  JNIEnv* env = base::android::AttachCurrentThread();
  // Convert strings to jstring references.
  ScopedJavaLocalRef<jstring> j_title =
      base::android::ConvertUTF8ToJavaString(env, title);
  ScopedJavaLocalRef<jstring> j_description =
      base::android::ConvertUTF8ToJavaString(env, description);
  ScopedJavaLocalRef<jstring> j_path =
      base::android::ConvertUTF8ToJavaString(env, path);
  ScopedJavaLocalRef<jstring> j_uri =
      base::android::ConvertUTF8ToJavaString(env, uri);
  ScopedJavaLocalRef<jstring> j_referer =
      base::android::ConvertUTF8ToJavaString(env, referer);

  return Java_OfflinePagesDownloadManagerBridge_addCompletedDownload(
      env, j_title, j_description, j_path, length, j_uri, j_referer);
}

int OfflinePagesDownloadManagerBridge::Remove(
    const std::vector<int64_t>& android_download_manager_ids) {
  JNIEnv* env = base::android::AttachCurrentThread();
  // Build a JNI array with our ID data.
  ScopedJavaLocalRef<jlongArray> j_ids =
      base::android::ToJavaLongArray(env, android_download_manager_ids);

  return Java_OfflinePagesDownloadManagerBridge_remove(env, j_ids);
}

}  // namespace android
}  // namespace offline_pages
