// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_OPERATION_DELEGATE_H_
#define COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_OPERATION_DELEGATE_H_

#include "components/drive/file_errors.h"

namespace drive {

struct ClientContext;
class FileChange;

namespace file_system {

// Error type of sync client.
// Keep it synced with "DriveSyncErrorType" in file_manager_private.idl.
enum DriveSyncErrorType {
  // Request to delete a file without permission.
  DRIVE_SYNC_ERROR_DELETE_WITHOUT_PERMISSION,
  // Google Drive is temporary unavailable.
  DRIVE_SYNC_ERROR_SERVICE_UNAVAILABLE,
  // There is no server space to sync a file.
  DRIVE_SYNC_ERROR_NO_SERVER_SPACE,
  // Errors other than above ones. No fallback is provided for the error.
  DRIVE_SYNC_ERROR_MISC,
};

// Passes notifications from Drive operations back to the file system.
class OperationDelegate {
 public:
  // Sent when a content of a directory has been changed.
  // |directory_path| is a virtual directory path representing the
  // changed directory.
  virtual void OnFileChangedByOperation(const FileChange& changed_files) {}

  // Sent when an entry is updated and sync is needed. The passed |context| is
  // used for syncing.
  virtual void OnEntryUpdatedByOperation(const ClientContext& context,
                                         const std::string& local_id) {}

  // Sent when a specific drive sync error occurred.
  // |local_id| is the local ID of the resource entry.
  virtual void OnDriveSyncError(DriveSyncErrorType type,
                                const std::string& local_id) {}

  // Waits for the sync task to complete and runs the callback.
  // Returns false if no task is found for the spcecified ID.
  virtual bool WaitForSyncComplete(const std::string& local_id,
                                   const FileOperationCallback& callback);
};

}  // namespace file_system
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_OPERATION_DELEGATE_H_
