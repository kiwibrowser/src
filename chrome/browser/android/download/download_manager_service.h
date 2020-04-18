// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_DOWNLOAD_DOWNLOAD_MANAGER_SERVICE_H_
#define CHROME_BROWSER_ANDROID_DOWNLOAD_DOWNLOAD_MANAGER_SERVICE_H_

#include <map>
#include <string>

#include "base/android/scoped_java_ref.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "chrome/browser/android/download/download_controller.h"
#include "chrome/browser/download/download_history.h"
#include "components/download/content/public/all_download_item_notifier.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

using base::android::JavaParamRef;

namespace download {
class DownloadItem;
}

// Native side of DownloadManagerService.java. The native object is owned by its
// Java object.
class DownloadManagerService
    : public download::AllDownloadItemNotifier::Observer,
      public DownloadHistory::Observer,
      public content::NotificationObserver {
 public:
  static void OnDownloadCanceled(
      download::DownloadItem* download,
      DownloadController::DownloadCancelReason reason);

  static DownloadManagerService* GetInstance();

  static base::android::ScopedJavaLocalRef<jobject> CreateJavaDownloadInfo(
      JNIEnv* env,
      download::DownloadItem* item);

  DownloadManagerService();
  ~DownloadManagerService() override;

  // Called to Initialize this object.
  void Init(JNIEnv* env, jobject obj);

  // Called to open a download item whose GUID is equal to |jdownload_guid|.
  void OpenDownload(JNIEnv* env,
                    jobject obj,
                    const JavaParamRef<jstring>& jdownload_guid,
                    bool is_off_the_record,
                    jint source);

  // Called to resume downloading the item that has GUID equal to
  // |jdownload_guid|..
  void ResumeDownload(JNIEnv* env,
                      jobject obj,
                      const JavaParamRef<jstring>& jdownload_guid,
                      bool is_off_the_record);

  // Called to cancel a download item that has GUID equal to |jdownload_guid|.
  // If the DownloadItem is not yet created, retry after a while.
  void CancelDownload(JNIEnv* env,
                      jobject obj,
                      const JavaParamRef<jstring>& jdownload_guid,
                      bool is_off_the_record);

  // Called to pause a download item that has GUID equal to |jdownload_guid|.
  // If the DownloadItem is not yet created, do nothing as it is already paused.
  void PauseDownload(JNIEnv* env,
                     jobject obj,
                     const JavaParamRef<jstring>& jdownload_guid,
                     bool is_off_the_record);

  // Called to remove a download item that has GUID equal to |jdownload_guid|.
  void RemoveDownload(JNIEnv* env,
                      jobject obj,
                      const JavaParamRef<jstring>& jdownload_guid,
                      bool is_off_the_record);

  // Returns whether or not the given download can be opened by the browser.
  bool IsDownloadOpenableInBrowser(JNIEnv* env,
                                   jobject obj,
                                   const JavaParamRef<jstring>& jdownload_guid,
                                   bool is_off_the_record);

  // Called to request that the DownloadManagerService return data about all
  // downloads in the user's history.
  void GetAllDownloads(JNIEnv* env,
                       const JavaParamRef<jobject>& obj,
                       bool is_off_the_record);

  // Called to check if the files associated with any downloads have been
  // removed by an external action.
  void CheckForExternallyRemovedDownloads(JNIEnv* env,
                                          const JavaParamRef<jobject>& obj,
                                          bool is_off_the_record);

  // Called to update the last access time associated with a download.
  void UpdateLastAccessTime(JNIEnv* env,
                            const JavaParamRef<jobject>& obj,
                            const JavaParamRef<jstring>& jdownload_guid,
                            bool is_off_the_record);

  // DownloadHistory::Observer methods.
  void OnHistoryQueryComplete() override;

  // AllDownloadItemNotifier::Observer methods.
  void OnDownloadCreated(content::DownloadManager* manager,
                         download::DownloadItem* item) override;
  void OnDownloadUpdated(content::DownloadManager* manager,
                         download::DownloadItem* item) override;
  void OnDownloadRemoved(content::DownloadManager* manager,
                         download::DownloadItem* item) override;

  // content::NotificationObserver methods.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

 protected:
  // Called to get the content::DownloadManager instance.
  virtual content::DownloadManager* GetDownloadManager(bool is_off_the_record);

 private:
  // For testing.
  friend class DownloadManagerServiceTest;
  friend struct base::DefaultSingletonTraits<DownloadManagerService>;

  // Helper function to start the download resumption.
  void ResumeDownloadInternal(const std::string& download_guid,
                              bool is_off_the_record);

  // Helper function to cancel a download.
  void CancelDownloadInternal(const std::string& download_guid,
                              bool is_off_the_record);

  // Helper function to pause a download.
  void PauseDownloadInternal(const std::string& download_guid,
                             bool is_off_the_record);

  // Helper function to remove a download.
  void RemoveDownloadInternal(const std::string& download_guid,
                              bool is_off_the_record);

  // Helper function to send info about all downloads to the Java-side.
  void GetAllDownloadsInternal(bool is_off_the_record);

  // Called to notify the java side that download resumption failed.
  void OnResumptionFailed(const std::string& download_guid);

  void OnResumptionFailedInternal(const std::string& download_guid);

  typedef base::Callback<void(bool)> ResumeCallback;
  void set_resume_callback_for_testing(const ResumeCallback& resume_cb) {
    resume_callback_for_testing_ = resume_cb;
  }

  // Reference to the Java object.
  base::android::ScopedJavaGlobalRef<jobject> java_ref_;

  bool is_history_query_complete_;

  enum PendingGetDownloadsFlags {
    NONE = 0,
    REGULAR = 1 << 0,
    OFF_THE_RECORD = 1 << 1,
  };
  int pending_get_downloads_actions_;

  enum DownloadAction {
    RESUME,
    PAUSE,
    CANCEL,
    REMOVE,
    UNKNOWN
  };
  using PendingDownloadActions = std::map<std::string, DownloadAction>;
  PendingDownloadActions pending_actions_;

  void EnqueueDownloadAction(const std::string& download_guid,
                             DownloadAction action);

  ResumeCallback resume_callback_for_testing_;

  // The Registrar used to register for notifications.
  content::NotificationRegistrar registrar_;

  std::unique_ptr<download::AllDownloadItemNotifier> original_notifier_;
  std::unique_ptr<download::AllDownloadItemNotifier> off_the_record_notifier_;

  DISALLOW_COPY_AND_ASSIGN(DownloadManagerService);
};

#endif  // CHROME_BROWSER_ANDROID_DOWNLOAD_DOWNLOAD_MANAGER_SERVICE_H_
