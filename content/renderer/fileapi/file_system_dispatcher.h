// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_FILEAPI_FILE_SYSTEM_DISPATCHER_H_
#define CONTENT_RENDERER_FILEAPI_FILE_SYSTEM_DISPATCHER_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/containers/id_map.h"
#include "base/macros.h"
#include "base/process/process.h"
#include "components/services/filesystem/public/interfaces/types.mojom.h"
#include "ipc/ipc_listener.h"
#include "ipc/ipc_platform_file.h"
#include "storage/common/fileapi/file_system_types.h"
#include "storage/common/quota/quota_limit_type.h"

namespace base {
class FilePath;
}

namespace storage {
struct FileSystemInfo;
}

class GURL;

namespace content {

// Dispatches and sends file system related messages sent to/from a child
// process from/to the main browser process.  There is one instance
// per child process.  Messages are dispatched on the main child thread.
class FileSystemDispatcher : public IPC::Listener {
 public:
  typedef base::Callback<void(base::File::Error error)> StatusCallback;
  typedef base::Callback<void(const base::File::Info& file_info)>
      MetadataCallback;
  typedef base::Callback<void(const base::File::Info& file_info,
                              const base::FilePath& platform_path,
                              int request_id)>
      CreateSnapshotFileCallback;

  typedef base::Callback<void(
      const std::vector<filesystem::mojom::DirectoryEntry>& entries,
      bool has_more)>
      ReadDirectoryCallback;
  typedef base::Callback<void(const std::string& name, const GURL& root)>
      OpenFileSystemCallback;
  typedef base::Callback<void(const storage::FileSystemInfo& info,
                              const base::FilePath& file_path,
                              bool is_directory)>
      ResolveURLCallback;

  typedef base::Callback<void(int64_t bytes, bool complete)> WriteCallback;
  typedef base::Callback<void(base::PlatformFile file,
                              int file_open_id,
                              storage::QuotaLimitType quota_policy)>
      OpenFileCallback;

  FileSystemDispatcher();
  ~FileSystemDispatcher() override;

  // IPC::Listener implementation.
  bool OnMessageReceived(const IPC::Message& msg) override;

  void OpenFileSystem(const GURL& origin_url,
                      storage::FileSystemType type,
                      const OpenFileSystemCallback& success_callback,
                      const StatusCallback& error_callback);
  void ResolveURL(const GURL& filesystem_url,
                  const ResolveURLCallback& success_callback,
                  const StatusCallback& error_callback);
  void Move(const GURL& src_path,
            const GURL& dest_path,
            const StatusCallback& callback);
  void Copy(const GURL& src_path,
            const GURL& dest_path,
            const StatusCallback& callback);
  void Remove(const GURL& path, bool recursive, const StatusCallback& callback);
  void ReadMetadata(const GURL& path,
                    const MetadataCallback& success_callback,
                    const StatusCallback& error_callback);
  void CreateFile(const GURL& path,
                  bool exclusive,
                  const StatusCallback& callback);
  void CreateDirectory(const GURL& path,
                       bool exclusive,
                       bool recursive,
                       const StatusCallback& callback);
  void Exists(const GURL& path,
              bool for_directory,
              const StatusCallback& callback);
  void ReadDirectory(const GURL& path,
                     const ReadDirectoryCallback& success_callback,
                     const StatusCallback& error_callback);
  void Truncate(const GURL& path,
                int64_t offset,
                int* request_id_out,
                const StatusCallback& callback);
  void Write(const GURL& path,
             const std::string& blob_id,
             int64_t offset,
             int* request_id_out,
             const WriteCallback& success_callback,
             const StatusCallback& error_callback);
  void Cancel(int request_id_to_cancel, const StatusCallback& callback);
  void TouchFile(const GURL& file_path,
                 const base::Time& last_access_time,
                 const base::Time& last_modified_time,
                 const StatusCallback& callback);

  // The caller must send FileSystemHostMsg_DidReceiveSnapshot message
  // with |request_id| passed to |success_callback| after the snapshot file
  // is successfully received.
  void CreateSnapshotFile(const GURL& file_path,
                          const CreateSnapshotFileCallback& success_callback,
                          const StatusCallback& error_callback);

 private:
  class CallbackDispatcher;

  // Message handlers.
  void OnDidOpenFileSystem(int request_id,
                           const std::string& name,
                           const GURL& root);
  void OnDidResolveURL(int request_id,
                       const storage::FileSystemInfo& info,
                       const base::FilePath& file_path,
                       bool is_directory);
  void OnDidSucceed(int request_id);
  void OnDidReadMetadata(int request_id,
                         const base::File::Info& file_info);
  void OnDidCreateSnapshotFile(int request_id,
                               const base::File::Info& file_info,
                               const base::FilePath& platform_path);
  void OnDidReadDirectory(
      int request_id,
      const std::vector<filesystem::mojom::DirectoryEntry>& entries,
      bool has_more);
  void OnDidFail(int request_id, base::File::Error error_code);
  void OnDidWrite(int request_id, int64_t bytes, bool complete);

  base::IDMap<std::unique_ptr<CallbackDispatcher>> dispatchers_;

  DISALLOW_COPY_AND_ASSIGN(FileSystemDispatcher);
};

}  // namespace content

#endif  // CONTENT_RENDERER_FILEAPI_FILE_SYSTEM_DISPATCHER_H_
