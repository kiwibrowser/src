// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FILEAPI_FILEAPI_MESSAGE_FILTER_H_
#define CONTENT_BROWSER_FILEAPI_FILEAPI_MESSAGE_FILTER_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/containers/hash_tables.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/shared_memory.h"
#include "components/services/filesystem/public/interfaces/types.mojom.h"
#include "content/browser/streams/stream.h"
#include "content/browser/streams/stream_context.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_message_filter.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_operation_runner.h"
#include "storage/common/fileapi/file_system_types.h"

class GURL;

namespace base {
class FilePath;
class Time;
}

namespace storage {
class FileSystemURL;
class FileSystemOperationRunner;
struct FileSystemInfo;
}

namespace net {
class URLRequestContext;
class URLRequestContextGetter;
}  // namespace net

namespace storage {
class ShareableFileReference;
}

namespace content {
class ChildProcessSecurityPolicyImpl;
class ChromeBlobStorageContext;

// TODO(tyoshino): Factor out code except for IPC gluing from
// FileAPIMessageFilter into separate classes. See crbug.com/263741.
class CONTENT_EXPORT FileAPIMessageFilter : public BrowserMessageFilter {
 public:
  // Used by the renderer process host on the UI thread.
  FileAPIMessageFilter(int process_id,
                       net::URLRequestContextGetter* request_context_getter,
                       storage::FileSystemContext* file_system_context,
                       ChromeBlobStorageContext* blob_storage_context);
  // Used by the worker process host on the IO thread.
  FileAPIMessageFilter(int process_id,
                       net::URLRequestContext* request_context,
                       storage::FileSystemContext* file_system_context,
                       ChromeBlobStorageContext* blob_storage_context);

  // BrowserMessageFilter implementation.
  void OnChannelConnected(int32_t peer_pid) override;
  void OnChannelClosing() override;
  base::TaskRunner* OverrideTaskRunnerForMessage(
      const IPC::Message& message) override;
  bool OnMessageReceived(const IPC::Message& message) override;

 protected:
  ~FileAPIMessageFilter() override;

 private:
  typedef storage::FileSystemOperationRunner::OperationID OperationID;

  void OnOpenFileSystem(int request_id,
                        const GURL& origin_url,
                        storage::FileSystemType type);
  void OnResolveURL(int request_id,
                    const GURL& filesystem_url);
  void OnMove(int request_id,
              const GURL& src_path,
              const GURL& dest_path);
  void OnCopy(int request_id,
              const GURL& src_path,
              const GURL& dest_path);
  void OnRemove(int request_id, const GURL& path, bool recursive);
  void OnReadMetadata(int request_id, const GURL& path);
  void OnCreate(int request_id,
                const GURL& path,
                bool exclusive,
                bool is_directory,
                bool recursive);
  void OnExists(int request_id, const GURL& path, bool is_directory);
  void OnReadDirectory(int request_id, const GURL& path);
  void OnWrite(int request_id,
               const GURL& path,
               const std::string& blob_uuid,
               int64_t offset);
  void OnTruncate(int request_id, const GURL& path, int64_t length);
  void OnTouchFile(int request_id,
                   const GURL& path,
                   const base::Time& last_access_time,
                   const base::Time& last_modified_time);
  void OnCancel(int request_id, int request_to_cancel);
  void OnSyncGetPlatformPath(const GURL& path,
                             base::FilePath* platform_path);
  void OnCreateSnapshotFile(int request_id,
                            const GURL& path);
  void OnDidReceiveSnapshotFile(int request_id);

  // Callback functions to be used when each file operation is finished.
  void DidFinish(int request_id, base::File::Error result);
  void DidGetMetadata(int request_id,
                      base::File::Error result,
                      const base::File::Info& info);
  void DidGetMetadataForStreaming(int request_id,
                                  base::File::Error result,
                                  const base::File::Info& info);
  void DidReadDirectory(int request_id,
                        base::File::Error result,
                        std::vector<filesystem::mojom::DirectoryEntry> entries,
                        bool has_more);
  void DidWrite(int request_id,
                base::File::Error result,
                int64_t bytes,
                bool complete);
  void DidOpenFileSystem(int request_id,
                         const GURL& root,
                         const std::string& filesystem_name,
                         base::File::Error result);
  void DidResolveURL(int request_id,
                     base::File::Error result,
                     const storage::FileSystemInfo& info,
                     const base::FilePath& file_path,
                     storage::FileSystemContext::ResolvedEntryType type);
  void DidCreateSnapshot(
      int request_id,
      const storage::FileSystemURL& url,
      base::File::Error result,
      const base::File::Info& info,
      const base::FilePath& platform_path,
      scoped_refptr<storage::ShareableFileReference> file_ref);

  // Sends a FileSystemMsg_DidFail and returns false if |url| is invalid.
  bool ValidateFileSystemURL(int request_id, const storage::FileSystemURL& url);

  storage::FileSystemOperationRunner* operation_runner() {
    return operation_runner_.get();
  }

  int process_id_;

  storage::FileSystemContext* context_;
  ChildProcessSecurityPolicyImpl* security_policy_;

  // Keeps map from request_id to OperationID for ongoing operations.
  // (Primarily for Cancel operation)
  typedef std::map<int, OperationID> OperationsMap;
  OperationsMap operations_;

  // The getter holds the context until OnChannelConnected() can be called from
  // the IO thread, which will extract the net::URLRequestContext from it.
  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
  net::URLRequestContext* request_context_;

  scoped_refptr<ChromeBlobStorageContext> blob_storage_context_;

  std::unique_ptr<storage::FileSystemOperationRunner> operation_runner_;

  // Keep track of stream URLs registered in this process. Need to unregister
  // all of them when the renderer process dies.
  base::hash_set<std::string> stream_urls_;

  // Used to keep snapshot files alive while a DidCreateSnapshot
  // is being sent to the renderer.
  std::map<int, scoped_refptr<storage::ShareableFileReference> >
      in_transit_snapshot_files_;

  DISALLOW_COPY_AND_ASSIGN(FileAPIMessageFilter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FILEAPI_FILEAPI_MESSAGE_FILTER_H_
