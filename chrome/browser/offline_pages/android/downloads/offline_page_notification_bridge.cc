// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/android/downloads/offline_page_notification_bridge.h"

#include "base/android/jni_string.h"
#include "components/offline_pages/core/offline_page_feature.h"

#include "jni/OfflinePageNotificationBridge_jni.h"

using base::android::AttachCurrentThread;
using base::android::ConvertUTF16ToJavaString;
using base::android::ConvertUTF8ToJavaString;

namespace offline_pages {
namespace android {

void OfflinePageNotificationBridge::NotifyDownloadSuccessful(
    const OfflineItem& item) {
  JNIEnv* env = AttachCurrentThread();
  Java_OfflinePageNotificationBridge_notifyDownloadSuccessful(
      env, ConvertUTF8ToJavaString(env, item.id.id),
      ConvertUTF8ToJavaString(env, item.page_url.spec()),
      ConvertUTF8ToJavaString(env, item.title), item.total_size_bytes);
}

void OfflinePageNotificationBridge::NotifyDownloadFailed(
    const OfflineItem& item) {
  JNIEnv* env = AttachCurrentThread();
  Java_OfflinePageNotificationBridge_notifyDownloadFailed(
      env, ConvertUTF8ToJavaString(env, item.id.id),
      ConvertUTF8ToJavaString(env, item.page_url.spec()),
      ConvertUTF8ToJavaString(env, item.title),
      static_cast<jint>(item.fail_state));
}

void OfflinePageNotificationBridge::NotifyDownloadProgress(
    const OfflineItem& item) {
  JNIEnv* env = AttachCurrentThread();
  Java_OfflinePageNotificationBridge_notifyDownloadProgress(
      env, ConvertUTF8ToJavaString(env, item.id.id),
      ConvertUTF8ToJavaString(env, item.page_url.spec()),
      item.creation_time.ToJavaTime(), item.received_bytes,
      ConvertUTF8ToJavaString(env, item.title));
}

void OfflinePageNotificationBridge::NotifyDownloadPaused(
    const OfflineItem& item) {
  JNIEnv* env = AttachCurrentThread();
  Java_OfflinePageNotificationBridge_notifyDownloadPaused(
      env, ConvertUTF8ToJavaString(env, item.id.id),
      ConvertUTF8ToJavaString(env, item.title));
}

void OfflinePageNotificationBridge::NotifyDownloadInterrupted(
    const OfflineItem& item) {
  JNIEnv* env = AttachCurrentThread();
  Java_OfflinePageNotificationBridge_notifyDownloadInterrupted(
      env, ConvertUTF8ToJavaString(env, item.id.id),
      ConvertUTF8ToJavaString(env, item.title),
      static_cast<jint>(item.pending_state));
}

void OfflinePageNotificationBridge::NotifyDownloadCanceled(
    const OfflineItem& item) {
  JNIEnv* env = AttachCurrentThread();
  Java_OfflinePageNotificationBridge_notifyDownloadCanceled(
      env, ConvertUTF8ToJavaString(env, item.id.id));
}

bool OfflinePageNotificationBridge::MaybeSuppressNotification(
    const std::string& origin,
    const OfflineItem& item) {
  // Do not suppress notification if chrome.
  if (origin == "" || !IsOfflinePagesSuppressNotificationsEnabled())
    return false;
  JNIEnv* env = AttachCurrentThread();
  return Java_OfflinePageNotificationBridge_maybeSuppressNotification(
      env, ConvertUTF8ToJavaString(env, origin),
      ConvertUTF8ToJavaString(env, item.id.id));
}

void OfflinePageNotificationBridge::ShowDownloadingToast() {
  JNIEnv* env = AttachCurrentThread();
  Java_OfflinePageNotificationBridge_showDownloadingToast(env);
}

}  // namespace android
}  // namespace offline_pages
