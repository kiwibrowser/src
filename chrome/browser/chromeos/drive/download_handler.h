// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DRIVE_DOWNLOAD_HANDLER_H_
#define CHROME_BROWSER_CHROMEOS_DRIVE_DOWNLOAD_HANDLER_H_

#include <stdint.h>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/download/content/public/all_download_item_notifier.h"
#include "components/drive/file_errors.h"
#include "content/public/browser/download_manager_delegate.h"

class Profile;

namespace content {
class DownloadManager;
}

namespace download {
class DownloadItem;
}

namespace drive {

class FileSystemInterface;

// Observes downloads to temporary local drive folder. Schedules these
// downloads for upload to drive service.
class DownloadHandler : public download::AllDownloadItemNotifier::Observer {
 public:
  explicit DownloadHandler(FileSystemInterface* file_system);
  ~DownloadHandler() override;

  // Utility method to get DownloadHandler with profile.
  static DownloadHandler* GetForProfile(Profile* profile);

  // Become an observer of DownloadManager.
  void Initialize(content::DownloadManager* download_manager,
                  const base::FilePath& drive_tmp_download_path);

  // In addition to the DownloadManager passed to Initialize(), observe another
  // download manager. This should be called only for the DownloadManager of the
  // incognito version of the profile where |file_system_| resides.
  void ObserveIncognitoDownloadManager(
      content::DownloadManager* download_manager);

  // Callback used to return results from SubstituteDriveDownloadPath.
  // TODO(hashimoto): Report error with a FileError. crbug.com/171345
  typedef base::Callback<void(const base::FilePath&)>
      SubstituteDriveDownloadPathCallback;

  void SubstituteDriveDownloadPath(
      const base::FilePath& drive_path,
      download::DownloadItem* download,
      const SubstituteDriveDownloadPathCallback& callback);

  // Sets drive path, for example, '/special/drive/MyFolder/MyFile',
  // to external data in |download|. Also sets display name and
  // makes |download| a temporary.
  void SetDownloadParams(const base::FilePath& drive_path,
                         download::DownloadItem* download);

  // Gets the target drive path from external data in |download|.
  base::FilePath GetTargetPath(const download::DownloadItem* download);

  // Gets the downloaded drive cache file path from external data in |download|.
  base::FilePath GetCacheFilePath(const download::DownloadItem* download);

  // Checks if there is a Drive upload associated with |download|
  bool IsDriveDownload(const download::DownloadItem* download);

  // Checks a file corresponding to the download item exists in Drive.
  void CheckForFileExistence(const download::DownloadItem* download,
                             content::CheckForFileExistenceCallback callback);

  // Calculates request space for |downloads|.
  int64_t CalculateRequestSpace(
      const content::DownloadManager::DownloadVector& downloads);

  // Checks available storage space and free disk space if necessary. Actual
  // execution is delayed and rate limited.
  void FreeDiskSpaceIfNeeded();

  // Checks available storage space and free disk space if necessary. This is
  // executed immediately.
  void FreeDiskSpaceIfNeededImmediately();

  // Sets free disk space delay for testing.
  void SetFreeDiskSpaceDelayForTesting(const base::TimeDelta& delay);

 private:
  // AllDownloadItemNotifier::Observer overrides:
  void OnDownloadCreated(content::DownloadManager* manager,
                         download::DownloadItem* download) override;
  void OnDownloadUpdated(content::DownloadManager* manager,
                         download::DownloadItem* download) override;

  // Removes the download.
  void RemoveDownload(void* manager_id, int id);

  // Callback for FileSystem::CreateDirectory().
  // Used to implement SubstituteDriveDownloadPath().
  void OnCreateDirectory(const SubstituteDriveDownloadPathCallback& callback,
                         FileError error);

  // Starts the upload of a downloaded/downloading file.
  void UploadDownloadItem(content::DownloadManager* manager,
                          download::DownloadItem* download);

  // Sets |cache_file_path| as user data of the download item specified by |id|.
  void SetCacheFilePath(void* manager_id,
                        int id,
                        const base::FilePath* cache_file_path,
                        FileError error);

  // Gets a download manager, given a |manager_id| casted from the pointer to
  // the manager. This is used to validate the manager that may be deleted while
  // asynchronous task posting. Returns NULL if the manager is already gone.
  content::DownloadManager* GetDownloadManager(void* manager_id);

  FileSystemInterface* file_system_;  // Owned by DriveIntegrationService.

  // Observe the DownloadManager for new downloads.
  std::unique_ptr<download::AllDownloadItemNotifier> notifier_;
  std::unique_ptr<download::AllDownloadItemNotifier> notifier_incognito_;

  // Temporary download location directory.
  base::FilePath drive_tmp_download_path_;

  // True if there is pending FreeDiskSpaceIfNeeded call.
  bool has_pending_free_disk_space_;

  base::TimeDelta free_disk_space_delay_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<DownloadHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DownloadHandler);
};

}  // namespace drive

#endif  // CHROME_BROWSER_CHROMEOS_DRIVE_DOWNLOAD_HANDLER_H_
