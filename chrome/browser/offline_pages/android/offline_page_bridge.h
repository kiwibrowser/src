// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_ANDROID_OFFLINE_PAGE_BRIDGE_H_
#define CHROME_BROWSER_OFFLINE_PAGES_ANDROID_OFFLINE_PAGE_BRIDGE_H_

#include <stdint.h>

#include "base/android/jni_android.h"
#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/supports_user_data.h"
#include "components/offline_pages/core/offline_page_item.h"
#include "components/offline_pages/core/offline_page_model.h"

namespace content {
class BrowserContext;
class WebContents;
}

namespace offline_pages {
namespace android {

/**
 * Bridge between C++ and Java for exposing native implementation of offline
 * pages model in managed code.
 */
class OfflinePageBridge : public OfflinePageModel::Observer,
                          public base::SupportsUserData::Data {
 public:
  static base::android::ScopedJavaLocalRef<jobject> ConvertToJavaOfflinePage(
      JNIEnv* env,
      const OfflinePageItem& offline_page);

  static std::string GetEncodedOriginApp(
      const content::WebContents* web_contents);

  OfflinePageBridge(JNIEnv* env,
                    content::BrowserContext* browser_context,
                    OfflinePageModel* offline_page_model);
  ~OfflinePageBridge() override;

  // OfflinePageModel::Observer implementation.
  void OfflinePageModelLoaded(OfflinePageModel* model) override;
  void OfflinePageAdded(OfflinePageModel* model,
                        const OfflinePageItem& added_page) override;
  void OfflinePageDeleted(
      const OfflinePageModel::DeletedPageInfo& page_info) override;

  void GetAllPages(JNIEnv* env,
                   const base::android::JavaParamRef<jobject>& obj,
                   const base::android::JavaParamRef<jobject>& j_result_obj,
                   const base::android::JavaParamRef<jobject>& j_callback_obj);

  void GetPageByOfflineId(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      jlong offline_id,
      const base::android::JavaParamRef<jobject>& j_callback_obj);

  void DeletePagesByClientId(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobjectArray>& j_namespaces_array,
      const base::android::JavaParamRef<jobjectArray>& j_ids_array,
      const base::android::JavaParamRef<jobject>& j_callback_obj);

  void DeletePagesByClientIdAndOrigin(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobjectArray>& j_namespaces_array,
      const base::android::JavaParamRef<jobjectArray>& j_ids_array,
      const base::android::JavaParamRef<jstring>& j_origin,
      const base::android::JavaParamRef<jobject>& j_callback_obj);

  void DeletePagesByOfflineId(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jlongArray>& j_offline_ids_array,
      const base::android::JavaParamRef<jobject>& j_callback_obj);

  void GetPagesByClientId(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& j_result_obj,
      const base::android::JavaParamRef<jobjectArray>& j_namespaces_array,
      const base::android::JavaParamRef<jobjectArray>& j_ids_array,
      const base::android::JavaParamRef<jobject>& j_callback_obj);

  void GetPagesByRequestOrigin(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& j_result_obj,
      const base::android::JavaParamRef<jstring>& j_request_origin,
      const base::android::JavaParamRef<jobject>& j_callback_obj);

  void GetPagesByNamespace(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& j_result_obj,
      const base::android::JavaParamRef<jstring>& j_namespace,
      const base::android::JavaParamRef<jobject>& j_callback_obj);

  void SelectPageForOnlineUrl(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& j_online_url,
      int tab_id,
      const base::android::JavaParamRef<jobject>& j_callback_obj);

  void SavePage(JNIEnv* env,
                const base::android::JavaParamRef<jobject>& obj,
                const base::android::JavaParamRef<jobject>& j_callback_obj,
                const base::android::JavaParamRef<jobject>& j_web_contents,
                const base::android::JavaParamRef<jstring>& j_namespace,
                const base::android::JavaParamRef<jstring>& j_client_id,
                const base::android::JavaParamRef<jstring>& j_origin);

  void SavePageLater(JNIEnv* env,
                     const base::android::JavaParamRef<jobject>& obj,
                     const base::android::JavaParamRef<jobject>& j_callback_obj,
                     const base::android::JavaParamRef<jstring>& url,
                     const base::android::JavaParamRef<jstring>& j_namespace,
                     const base::android::JavaParamRef<jstring>& j_client_id,
                     const base::android::JavaParamRef<jstring>& j_origin,
                     jboolean user_requested);

  void PublishInternalPageByOfflineId(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const jlong j_offline_id,
      const base::android::JavaParamRef<jobject>& j_published_callback);

  void PublishInternalPageByGuid(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& j_guid,
      const base::android::JavaParamRef<jobject>& j_published_callback);

  jboolean IsShowingOfflinePreview(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& j_web_contents);

  jboolean IsShowingDownloadButtonInErrorPage(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& j_web_contents);

  base::android::ScopedJavaLocalRef<jstring> GetOfflinePageHeaderForReload(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& j_web_contents);

  void GetRequestsInQueue(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& j_callback_obj);

  void RemoveRequestsFromQueue(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jlongArray>& j_request_ids_array,
      const base::android::JavaParamRef<jobject>& j_callback_obj);

  void WillCloseTab(JNIEnv* env,
                    const base::android::JavaParamRef<jobject>& obj,
                    const base::android::JavaParamRef<jobject>& j_web_contents);
  void ScheduleDownload(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& j_web_contents,
      const base::android::JavaParamRef<jstring>& j_namespace,
      const base::android::JavaParamRef<jstring>& j_url,
      int ui_action,
      const base::android::JavaParamRef<jstring>& j_origin);

  base::android::ScopedJavaGlobalRef<jobject> java_ref() { return java_ref_; }

  jboolean IsOfflinePage(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& j_web_contents);

  jboolean IsInPrivateDirectory(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& j_file_path);

  jboolean IsUserRequestedDownloadNamespace(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& j_name_space);

  base::android::ScopedJavaLocalRef<jobject> GetOfflinePage(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& j_web_contents);

  void CheckForNewOfflineContent(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const jlong j_timestamp_millis,
      const base::android::JavaParamRef<jobject>& j_callback_obj);

  void GetLaunchUrlByOfflineId(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      jlong j_offline_id,
      const base::android::JavaParamRef<jobject>& j_callback_obj);

  void GetLoadUrlParamsForOpeningMhtmlFileOrContent(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& j_url,
      const base::android::JavaParamRef<jobject>& j_callback_obj);

  jboolean IsShowingTrustedOfflinePage(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& j_web_contents);

  void AcquireFileAccessPermission(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jobject>& j_web_contents,
      const base::android::JavaParamRef<jobject>& j_callback_obj);

 private:
  void GetPageByOfflineIdDone(
      const base::android::ScopedJavaGlobalRef<jobject>& j_callback_obj,
      const OfflinePageItem* offline_page);

  void GetSizeAndComputeDigestDone(
      const base::android::ScopedJavaGlobalRef<jobject>& j_callback_obj,
      const GURL& intent_url,
      std::pair<int64_t, std::string> size_and_digest);

  void GetPageBySizeAndDigestDone(
      const base::android::ScopedJavaGlobalRef<jobject>& j_callback_obj,
      const GURL& intent_url,
      const OfflinePageItem* offline_page);

  void NotifyIfDoneLoading() const;

  base::android::ScopedJavaLocalRef<jobject> CreateClientId(
      JNIEnv* env,
      const ClientId& clientId) const;

  void PublishInternalArchive(
      const base::android::ScopedJavaGlobalRef<jobject>& j_callback_obj,
      const OfflinePageItem* offline_page);

  base::android::ScopedJavaGlobalRef<jobject> java_ref_;
  // Not owned.
  content::BrowserContext* browser_context_;
  // Not owned.
  OfflinePageModel* offline_page_model_;

  base::WeakPtrFactory<OfflinePageBridge> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OfflinePageBridge);
};

}  // namespace android
}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_ANDROID_OFFLINE_PAGE_BRIDGE_H_
