// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SMB_CLIENT_SMB_FILE_SYSTEM_H_
#define CHROME_BROWSER_CHROMEOS_SMB_CLIENT_SMB_FILE_SYSTEM_H_

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/linked_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "chrome/browser/chromeos/file_system_provider/abort_callback.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_info.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_interface.h"
#include "chrome/browser/chromeos/file_system_provider/watcher.h"
#include "chrome/browser/chromeos/smb_client/smb_service.h"
#include "chrome/browser/chromeos/smb_client/smb_task_queue.h"
#include "chrome/browser/chromeos/smb_client/temp_file_manager.h"
#include "chromeos/dbus/smb_provider_client.h"
#include "storage/browser/fileapi/async_file_util.h"
#include "storage/browser/fileapi/watcher_manager.h"
#include "url/gurl.h"

namespace net {
class IOBuffer;
}  // namespace net

namespace chromeos {
namespace smb_client {

class RequestManager;

// Smb provided file system implementation. For communication with Smb
// filesystems.
// Smb is an application level protocol used by Windows and Samba fileservers.
// Allows Files App to mount smb filesystems.
class SmbFileSystem : public file_system_provider::ProvidedFileSystemInterface,
                      public base::SupportsWeakPtr<SmbFileSystem> {
 public:
  using UnmountCallback = base::OnceCallback<base::File::Error(
      const std::string&,
      file_system_provider::Service::UnmountReason)>;

  SmbFileSystem(
      const file_system_provider::ProvidedFileSystemInfo& file_system_info,
      UnmountCallback unmount_callback);
  ~SmbFileSystem() override;

  // ProvidedFileSystemInterface overrides.
  file_system_provider::AbortCallback RequestUnmount(
      storage::AsyncFileUtil::StatusCallback callback) override;

  file_system_provider::AbortCallback GetMetadata(
      const base::FilePath& entry_path,
      ProvidedFileSystemInterface::MetadataFieldMask fields,
      ProvidedFileSystemInterface::GetMetadataCallback callback) override;

  file_system_provider::AbortCallback GetActions(
      const std::vector<base::FilePath>& entry_paths,
      GetActionsCallback callback) override;

  file_system_provider::AbortCallback ExecuteAction(
      const std::vector<base::FilePath>& entry_paths,
      const std::string& action_id,
      storage::AsyncFileUtil::StatusCallback callback) override;

  file_system_provider::AbortCallback ReadDirectory(
      const base::FilePath& directory_path,
      storage::AsyncFileUtil::ReadDirectoryCallback callback) override;

  file_system_provider::AbortCallback OpenFile(
      const base::FilePath& file_path,
      file_system_provider::OpenFileMode mode,
      OpenFileCallback callback) override;

  file_system_provider::AbortCallback CloseFile(
      int file_handle,
      storage::AsyncFileUtil::StatusCallback callback) override;

  file_system_provider::AbortCallback ReadFile(
      int file_handle,
      net::IOBuffer* buffer,
      int64_t offset,
      int length,
      ReadChunkReceivedCallback callback) override;

  file_system_provider::AbortCallback CreateDirectory(
      const base::FilePath& directory_path,
      bool recursive,
      storage::AsyncFileUtil::StatusCallback callback) override;

  file_system_provider::AbortCallback CreateFile(
      const base::FilePath& file_path,
      storage::AsyncFileUtil::StatusCallback callback) override;

  file_system_provider::AbortCallback DeleteEntry(
      const base::FilePath& entry_path,
      bool recursive,
      storage::AsyncFileUtil::StatusCallback callback) override;

  file_system_provider::AbortCallback CopyEntry(
      const base::FilePath& source_path,
      const base::FilePath& target_path,
      storage::AsyncFileUtil::StatusCallback callback) override;

  file_system_provider::AbortCallback MoveEntry(
      const base::FilePath& source_path,
      const base::FilePath& target_path,
      storage::AsyncFileUtil::StatusCallback callback) override;

  file_system_provider::AbortCallback Truncate(
      const base::FilePath& file_path,
      int64_t length,
      storage::AsyncFileUtil::StatusCallback callback) override;

  file_system_provider::AbortCallback WriteFile(
      int file_handle,
      net::IOBuffer* buffer,
      int64_t offset,
      int length,
      storage::AsyncFileUtil::StatusCallback callback) override;

  file_system_provider::AbortCallback AddWatcher(
      const GURL& origin,
      const base::FilePath& entry_path,
      bool recursive,
      bool persistent,
      storage::AsyncFileUtil::StatusCallback callback,
      const storage::WatcherManager::NotificationCallback&
          notification_callback) override;

  void RemoveWatcher(const GURL& origin,
                     const base::FilePath& entry_path,
                     bool recursive,
                     storage::AsyncFileUtil::StatusCallback callback) override;

  const file_system_provider::ProvidedFileSystemInfo& GetFileSystemInfo()
      const override;

  file_system_provider::RequestManager* GetRequestManager() override;

  file_system_provider::Watchers* GetWatchers() override;

  const file_system_provider::OpenedFiles& GetOpenedFiles() const override;

  void AddObserver(
      file_system_provider::ProvidedFileSystemObserver* observer) override;

  void RemoveObserver(
      file_system_provider::ProvidedFileSystemObserver* observer) override;

  void Notify(
      const base::FilePath& entry_path,
      bool recursive,
      storage::WatcherManager::ChangeType change_type,
      std::unique_ptr<file_system_provider::ProvidedFileSystemObserver::Changes>
          changes,
      const std::string& tag,
      storage::AsyncFileUtil::StatusCallback callback) override;

  void Configure(storage::AsyncFileUtil::StatusCallback callback) override;

  base::WeakPtr<ProvidedFileSystemInterface> GetWeakPtr() override;

 private:
  void Abort(OperationId operation_id);

  // Initializes temp_file_manager_.
  void InitTempFileManager();

  // Calls InitTempFileManager() and executes |task|.
  void InitTempFileManagerAndExecuteTask(SmbTask task);

  // Calls WriteFile in SmbProviderClient.
  file_system_provider::AbortCallback CallWriteFile(
      int file_handle,
      const std::vector<uint8_t>& data,
      int64_t offset,
      int length,
      storage::AsyncFileUtil::StatusCallback callback);

  file_system_provider::AbortCallback CreateAbortCallback(
      OperationId operation_id);

  file_system_provider::AbortCallback CreateAbortCallback();

  void HandleRequestUnmountCallback(
      storage::AsyncFileUtil::StatusCallback callback,
      smbprovider::ErrorType error);

  void HandleRequestReadDirectoryCallback(
      storage::AsyncFileUtil::ReadDirectoryCallback callback,
      smbprovider::ErrorType error,
      const smbprovider::DirectoryEntryListProto& entries) const;

  void HandleRequestGetMetadataEntryCallback(
      ProvidedFileSystemInterface::MetadataFieldMask fields,
      ProvidedFileSystemInterface::GetMetadataCallback callback,
      smbprovider::ErrorType error,
      const smbprovider::DirectoryEntryProto& entry) const;

  void HandleRequestOpenFileCallback(OpenFileCallback callback,
                                     smbprovider::ErrorType error,
                                     int32_t file_id) const;

  void HandleStatusCallback(storage::AsyncFileUtil::StatusCallback callback,
                            smbprovider::ErrorType error) const;

  base::File::Error RunUnmountCallback(
      const std::string& file_system_id,
      file_system_provider::Service::UnmountReason reason);

  void HandleRequestReadFileCallback(int32_t length,
                                     scoped_refptr<net::IOBuffer> buffer,
                                     ReadChunkReceivedCallback callback,
                                     smbprovider::ErrorType error,
                                     const base::ScopedFD& fd) const;

  void HandleGetDeleteListCallback(
      storage::AsyncFileUtil::StatusCallback callback,
      OperationId operation_id,
      smbprovider::ErrorType list_error,
      const smbprovider::DeleteListProto& delete_list);

  void HandleDeleteEntryCallback(
      storage::AsyncFileUtil::StatusCallback callback,
      smbprovider::ErrorType list_error,
      bool is_last_entry,
      smbprovider::ErrorType delete_error) const;

  int32_t GetMountId() const;

  SmbProviderClient* GetSmbProviderClient() const;
  base::WeakPtr<SmbProviderClient> GetWeakSmbProviderClient() const;

  // Gets a new OperationId and adds |task| to the task_queue_ with it. Returns
  // an AbortCallback to abort the newly created operation.
  file_system_provider::AbortCallback EnqueueTaskAndGetCallback(SmbTask task);

  // Adds |task| to the task_queue_ for |operation_id|.
  void EnqueueTask(SmbTask task, OperationId operation_id);

  // Gets a new OperationId and adds |task| to the task_queue_ with it. Returns
  // the OperationId for the newly created Operation.
  OperationId EnqueueTaskAndGetOperationId(SmbTask task);

  const file_system_provider::ProvidedFileSystemInfo file_system_info_;
  // opened_files_ is marked const since is currently unsupported.
  const file_system_provider::OpenedFiles opened_files_;

  UnmountCallback unmount_callback_;
  std::unique_ptr<TempFileManager> temp_file_manager_;
  mutable SmbTaskQueue task_queue_;

  DISALLOW_COPY_AND_ASSIGN(SmbFileSystem);
};

}  // namespace smb_client
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_SMB_CLIENT_SMB_FILE_SYSTEM_H_
