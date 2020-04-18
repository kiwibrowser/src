// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOWNLOAD_CHROME_DOWNLOAD_MANAGER_DELEGATE_H_
#define CHROME_BROWSER_DOWNLOAD_CHROME_DOWNLOAD_MANAGER_DELEGATE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/containers/flat_map.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "build/build_config.h"
#include "chrome/browser/download/download_path_reservation_tracker.h"
#include "chrome/browser/download/download_target_determiner_delegate.h"
#include "chrome/browser/download/download_target_info.h"
#include "chrome/browser/safe_browsing/download_protection/download_protection_service.h"
#include "chrome/browser/safe_browsing/download_protection/download_protection_util.h"
#include "components/download/public/common/download_danger_type.h"
#include "components/download/public/common/download_item.h"
#include "content/public/browser/download_manager_delegate.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "extensions/buildflags/buildflags.h"
#include "ui/gfx/native_widget_types.h"

#if defined(OS_ANDROID)
#include "chrome/browser/android/download/download_controller.h"
#include "chrome/browser/android/download/download_location_dialog_bridge.h"
#endif

class DownloadPrefs;
class Profile;

namespace content {
class DownloadManager;
}

namespace extensions {
class CrxInstaller;
}

// This is the Chrome side helper for the download system.
class ChromeDownloadManagerDelegate
    : public content::DownloadManagerDelegate,
      public content::NotificationObserver,
      public DownloadTargetDeterminerDelegate {
 public:
  explicit ChromeDownloadManagerDelegate(Profile* profile);
  ~ChromeDownloadManagerDelegate() override;

  // Should be called before the first call to ShouldCompleteDownload() to
  // disable SafeBrowsing checks for |item|.
  static void DisableSafeBrowsing(download::DownloadItem* item);

  void SetDownloadManager(content::DownloadManager* dm);
#if defined(OS_ANDROID)

  void ChooseDownloadLocation(
      gfx::NativeWindow native_window,
      int64_t total_bytes,
      DownloadLocationDialogType dialog_type,
      const base::FilePath& suggested_path,
      DownloadLocationDialogBridge::LocationCallback callback);

  void SetDownloadLocationDialogBridgeForTesting(
      DownloadLocationDialogBridge* bridge);
#endif

  // Callbacks passed to GetNextId() will not be called until the returned
  // callback is called.
  content::DownloadIdCallback GetDownloadIdReceiverCallback();

  // content::DownloadManagerDelegate
  void Shutdown() override;
  void GetNextId(const content::DownloadIdCallback& callback) override;
  bool DetermineDownloadTarget(
      download::DownloadItem* item,
      const content::DownloadTargetCallback& callback) override;
  bool ShouldOpenFileBasedOnExtension(const base::FilePath& path) override;
  bool ShouldCompleteDownload(download::DownloadItem* item,
                              const base::Closure& complete_callback) override;
  bool ShouldOpenDownload(
      download::DownloadItem* item,
      const content::DownloadOpenDelayedCallback& callback) override;
  bool InterceptDownloadIfApplicable(
      const GURL& url,
      const std::string& mime_type,
      const std::string& request_origin,
      content::WebContents* web_contents) override;
  bool GenerateFileHash() override;
  void GetSaveDir(content::BrowserContext* browser_context,
                  base::FilePath* website_save_dir,
                  base::FilePath* download_save_dir,
                  bool* skip_dir_check) override;
  void ChooseSavePath(
      content::WebContents* web_contents,
      const base::FilePath& suggested_path,
      const base::FilePath::StringType& default_extension,
      bool can_save_as_complete,
      const content::SavePackagePathPickedCallback& callback) override;
  void SanitizeSavePackageResourceName(base::FilePath* filename) override;
  void OpenDownload(download::DownloadItem* download) override;
  bool IsMostRecentDownloadItemAtFilePath(
      download::DownloadItem* download) override;
  void ShowDownloadInShell(download::DownloadItem* download) override;
  void CheckForFileExistence(
      download::DownloadItem* download,
      content::CheckForFileExistenceCallback callback) override;
  std::string ApplicationClientIdForFileScanning() const override;
  void CheckDownloadAllowed(
      const content::ResourceRequestInfo::WebContentsGetter&
          web_contents_getter,
      const GURL& url,
      const std::string& request_method,
      content::CheckDownloadAllowedCallback check_download_allowed_cb) override;

  // Opens a download using the platform handler. DownloadItem::OpenDownload,
  // which ends up being handled by OpenDownload(), will open a download in the
  // browser if doing so is preferred.
  void OpenDownloadUsingPlatformHandler(download::DownloadItem* download);

  DownloadPrefs* download_prefs() { return download_prefs_.get(); }

 protected:
  virtual safe_browsing::DownloadProtectionService*
      GetDownloadProtectionService();

  // DownloadTargetDeterminerDelegate. Protected for testing.
  void NotifyExtensions(download::DownloadItem* download,
                        const base::FilePath& suggested_virtual_path,
                        const NotifyExtensionsCallback& callback) override;
  void ReserveVirtualPath(
      download::DownloadItem* download,
      const base::FilePath& virtual_path,
      bool create_directory,
      DownloadPathReservationTracker::FilenameConflictAction conflict_action,
      const ReservedPathCallback& callback) override;
  void RequestConfirmation(download::DownloadItem* download,
                           const base::FilePath& suggested_virtual_path,
                           DownloadConfirmationReason reason,
                           const ConfirmationCallback& callback) override;
  void DetermineLocalPath(download::DownloadItem* download,
                          const base::FilePath& virtual_path,
                          const LocalPathCallback& callback) override;
  void CheckDownloadUrl(download::DownloadItem* download,
                        const base::FilePath& suggested_virtual_path,
                        const CheckDownloadUrlCallback& callback) override;
  void GetFileMimeType(const base::FilePath& path,
                       const GetFileMimeTypeCallback& callback) override;

#if defined(OS_ANDROID)
  virtual void OnDownloadCanceled(
      download::DownloadItem* download,
      DownloadController::DownloadCancelReason reason);
#endif

  // So that test classes that inherit from this for override purposes
  // can call back into the DownloadManager.
  content::DownloadManager* download_manager_;

 private:
  friend class base::RefCountedThreadSafe<ChromeDownloadManagerDelegate>;
  FRIEND_TEST_ALL_PREFIXES(ChromeDownloadManagerDelegateTest,
                           RequestConfirmation_Android);

  typedef std::vector<content::DownloadIdCallback> IdCallbackVector;

  // content::NotificationObserver implementation.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // Callback function after the DownloadProtectionService completes.
  void CheckClientDownloadDone(uint32_t download_id,
                               safe_browsing::DownloadCheckResult result);

  // Internal gateways for ShouldCompleteDownload().
  bool IsDownloadReadyForCompletion(
      download::DownloadItem* item,
      const base::Closure& internal_complete_callback);
  void ShouldCompleteDownloadInternal(
      uint32_t download_id,
      const base::Closure& user_complete_callback);

  // Sets the next download id based on download database records, and runs all
  // cached id callbacks.
  void SetNextId(uint32_t id);

  // Runs the |callback| with next id. Results in the download being started.
  void ReturnNextId(const content::DownloadIdCallback& callback);

  void OnDownloadTargetDetermined(
      int32_t download_id,
      const content::DownloadTargetCallback& callback,
      std::unique_ptr<DownloadTargetInfo> target_info);

  // Returns true if |path| should open in the browser.
  bool IsOpenInBrowserPreferreredForFile(const base::FilePath& path);

  // Return true if the downloaded file should be blocked based on the current
  // download restriction pref and |danger_type|.
  bool ShouldBlockFile(download::DownloadDangerType danger_type,
                       download::DownloadItem* item) const;

  void MaybeSendDangerousDownloadOpenedReport(download::DownloadItem* download,
                                              bool show_download_in_folder);

  void OnCheckDownloadAllowedComplete(
      content::CheckDownloadAllowedCallback check_download_allowed_cb,
      bool storage_permission_granted,
      bool allow);

#if defined(OS_ANDROID)
  // Called after a unique file name is generated in the case that there is a
  // TARGET_CONFLICT and the new file name should be displayed to the user.
  void GenerateUniqueFileNameDone(
      gfx::NativeWindow native_window,
      const DownloadTargetDeterminerDelegate::ConfirmationCallback& callback,
      PathValidationResult result,
      const base::FilePath& target_path);
#endif

  Profile* profile_;

#if defined(OS_ANDROID)
  std::unique_ptr<DownloadLocationDialogBridge> location_dialog_bridge_;
#endif

  // Incremented by one for each download, the first available download id is
  // assigned from history database or 1 when history database fails to
  // intialize.
  uint32_t next_download_id_;

  // The |GetNextId| callbacks that may be cached before loading the download
  // database.
  IdCallbackVector id_callbacks_;
  std::unique_ptr<DownloadPrefs> download_prefs_;

  // SequencedTaskRunner to check for file existence. A sequence is used so
  // that a large download history doesn't cause a large number of concurrent
  // disk operations.
  const scoped_refptr<base::SequencedTaskRunner> disk_access_task_runner_;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Maps from pending extension installations to DownloadItem IDs.
  typedef base::flat_map<extensions::CrxInstaller*,
                         content::DownloadOpenDelayedCallback>
      CrxInstallerMap;
  CrxInstallerMap crx_installers_;
#endif

  content::NotificationRegistrar registrar_;

  base::WeakPtrFactory<ChromeDownloadManagerDelegate> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ChromeDownloadManagerDelegate);
};

#endif  // CHROME_BROWSER_DOWNLOAD_CHROME_DOWNLOAD_MANAGER_DELEGATE_H_
