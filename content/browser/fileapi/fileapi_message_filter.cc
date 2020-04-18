// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/fileapi/fileapi_message_filter.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/metrics/user_metrics.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_util.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/services/filesystem/public/interfaces/types.mojom.h"
#include "content/browser/bad_message.h"
#include "content/browser/blob_storage/chrome_blob_storage_context.h"
#include "content/browser/child_process_security_policy_impl.h"
#include "content/browser/fileapi/browser_file_system_helper.h"
#include "content/common/fileapi/file_system_messages.h"
#include "content/common/fileapi/webblob_messages.h"
#include "content/public/browser/browser_thread.h"
#include "ipc/ipc_platform_file.h"
#include "net/base/mime_util.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "storage/browser/blob/blob_data_builder.h"
#include "storage/browser/blob/blob_storage_context.h"
#include "storage/browser/blob/shareable_file_reference.h"
#include "storage/browser/fileapi/file_observers.h"
#include "storage/browser/fileapi/file_permission_policy.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/isolated_context.h"
#include "storage/common/fileapi/file_system_info.h"
#include "storage/common/fileapi/file_system_types.h"
#include "storage/common/fileapi/file_system_util.h"
#include "url/gurl.h"

using storage::FileSystemFileUtil;
using storage::FileSystemBackend;
using storage::FileSystemOperation;
using storage::FileSystemURL;
using storage::BlobDataBuilder;
using storage::BlobStorageContext;

namespace content {

namespace {

const uint32_t kFileApiFilteredMessageClasses[] = {FileSystemMsgStart,
                                                   BlobMsgStart};

void RevokeFilePermission(int child_id, const base::FilePath& path) {
  ChildProcessSecurityPolicyImpl::GetInstance()->RevokeAllPermissionsForFile(
    child_id, path);
}

}  // namespace

FileAPIMessageFilter::FileAPIMessageFilter(
    int process_id,
    net::URLRequestContextGetter* request_context_getter,
    storage::FileSystemContext* file_system_context,
    ChromeBlobStorageContext* blob_storage_context)
    : BrowserMessageFilter(kFileApiFilteredMessageClasses,
                           arraysize(kFileApiFilteredMessageClasses)),
      process_id_(process_id),
      context_(file_system_context),
      security_policy_(ChildProcessSecurityPolicyImpl::GetInstance()),
      request_context_getter_(request_context_getter),
      request_context_(nullptr),
      blob_storage_context_(blob_storage_context) {
  DCHECK(context_);
  DCHECK(request_context_getter_.get());
  DCHECK(blob_storage_context);
}

FileAPIMessageFilter::FileAPIMessageFilter(
    int process_id,
    net::URLRequestContext* request_context,
    storage::FileSystemContext* file_system_context,
    ChromeBlobStorageContext* blob_storage_context)
    : BrowserMessageFilter(kFileApiFilteredMessageClasses,
                           arraysize(kFileApiFilteredMessageClasses)),
      process_id_(process_id),
      context_(file_system_context),
      security_policy_(ChildProcessSecurityPolicyImpl::GetInstance()),
      request_context_(request_context),
      blob_storage_context_(blob_storage_context) {
  DCHECK(context_);
  DCHECK(request_context_);
  DCHECK(blob_storage_context);
}

void FileAPIMessageFilter::OnChannelConnected(int32_t peer_pid) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (request_context_getter_.get()) {
    DCHECK(!request_context_);
    request_context_ = request_context_getter_->GetURLRequestContext();
    request_context_getter_ = nullptr;
    DCHECK(request_context_);
  }

  operation_runner_ = context_->CreateFileSystemOperationRunner();
}

void FileAPIMessageFilter::OnChannelClosing() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  in_transit_snapshot_files_.clear();

  operation_runner_.reset();
  operations_.clear();
}

base::TaskRunner* FileAPIMessageFilter::OverrideTaskRunnerForMessage(
    const IPC::Message& message) {
  if (message.type() == FileSystemHostMsg_SyncGetPlatformPath::ID)
    return context_->default_file_task_runner();
  return nullptr;
}

bool FileAPIMessageFilter::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(FileAPIMessageFilter, message)
    IPC_MESSAGE_HANDLER(FileSystemHostMsg_OpenFileSystem, OnOpenFileSystem)
    IPC_MESSAGE_HANDLER(FileSystemHostMsg_ResolveURL, OnResolveURL)
    IPC_MESSAGE_HANDLER(FileSystemHostMsg_Move, OnMove)
    IPC_MESSAGE_HANDLER(FileSystemHostMsg_Copy, OnCopy)
    IPC_MESSAGE_HANDLER(FileSystemHostMsg_Remove, OnRemove)
    IPC_MESSAGE_HANDLER(FileSystemHostMsg_ReadMetadata, OnReadMetadata)
    IPC_MESSAGE_HANDLER(FileSystemHostMsg_Create, OnCreate)
    IPC_MESSAGE_HANDLER(FileSystemHostMsg_Exists, OnExists)
    IPC_MESSAGE_HANDLER(FileSystemHostMsg_ReadDirectory, OnReadDirectory)
    IPC_MESSAGE_HANDLER(FileSystemHostMsg_Write, OnWrite)
    IPC_MESSAGE_HANDLER(FileSystemHostMsg_Truncate, OnTruncate)
    IPC_MESSAGE_HANDLER(FileSystemHostMsg_TouchFile, OnTouchFile)
    IPC_MESSAGE_HANDLER(FileSystemHostMsg_CancelWrite, OnCancel)
    IPC_MESSAGE_HANDLER(FileSystemHostMsg_CreateSnapshotFile,
                        OnCreateSnapshotFile)
    IPC_MESSAGE_HANDLER(FileSystemHostMsg_DidReceiveSnapshotFile,
                        OnDidReceiveSnapshotFile)
    IPC_MESSAGE_HANDLER(FileSystemHostMsg_SyncGetPlatformPath,
                        OnSyncGetPlatformPath)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

FileAPIMessageFilter::~FileAPIMessageFilter() {}

void FileAPIMessageFilter::OnOpenFileSystem(int request_id,
                                            const GURL& origin_url,
                                            storage::FileSystemType type) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (type == storage::kFileSystemTypeTemporary) {
    RecordAction(base::UserMetricsAction("OpenFileSystemTemporary"));
  } else if (type == storage::kFileSystemTypePersistent) {
    RecordAction(base::UserMetricsAction("OpenFileSystemPersistent"));
  }
  storage::OpenFileSystemMode mode =
      storage::OPEN_FILE_SYSTEM_CREATE_IF_NONEXISTENT;
  context_->OpenFileSystem(
      origin_url, type, mode,
      base::BindOnce(&FileAPIMessageFilter::DidOpenFileSystem, this,
                     request_id));
}

void FileAPIMessageFilter::OnResolveURL(
    int request_id,
    const GURL& filesystem_url) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  FileSystemURL url(context_->CrackURL(filesystem_url));
  if (!ValidateFileSystemURL(request_id, url))
    return;
  if (!security_policy_->CanReadFileSystemFile(process_id_, url)) {
    Send(new FileSystemMsg_DidFail(request_id,
                                   base::File::FILE_ERROR_SECURITY));
    return;
  }

  context_->ResolveURL(url, base::BindOnce(&FileAPIMessageFilter::DidResolveURL,
                                           this, request_id));
}

void FileAPIMessageFilter::OnMove(
    int request_id, const GURL& src_path, const GURL& dest_path) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  FileSystemURL src_url(context_->CrackURL(src_path));
  FileSystemURL dest_url(context_->CrackURL(dest_path));
  if (!ValidateFileSystemURL(request_id, src_url) ||
      !ValidateFileSystemURL(request_id, dest_url)) {
    return;
  }
  if (!security_policy_->CanReadFileSystemFile(process_id_, src_url) ||
      !security_policy_->CanDeleteFileSystemFile(process_id_, src_url) ||
      !security_policy_->CanCreateFileSystemFile(process_id_, dest_url)) {
    Send(new FileSystemMsg_DidFail(request_id,
                                   base::File::FILE_ERROR_SECURITY));
    return;
  }

  operations_[request_id] = operation_runner()->Move(
      src_url,
      dest_url,
      storage::FileSystemOperation::OPTION_NONE,
      base::Bind(&FileAPIMessageFilter::DidFinish, this, request_id));
}

void FileAPIMessageFilter::OnCopy(
    int request_id, const GURL& src_path, const GURL& dest_path) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  FileSystemURL src_url(context_->CrackURL(src_path));
  FileSystemURL dest_url(context_->CrackURL(dest_path));
  if (!ValidateFileSystemURL(request_id, src_url) ||
      !ValidateFileSystemURL(request_id, dest_url)) {
    return;
  }
  if (!security_policy_->CanReadFileSystemFile(process_id_, src_url) ||
      !security_policy_->CanCopyIntoFileSystemFile(process_id_, dest_url)) {
    Send(new FileSystemMsg_DidFail(request_id,
                                   base::File::FILE_ERROR_SECURITY));
    return;
  }

  operations_[request_id] = operation_runner()->Copy(
      src_url, dest_url, storage::FileSystemOperation::OPTION_NONE,
      FileSystemOperation::ERROR_BEHAVIOR_ABORT,
      storage::FileSystemOperationRunner::CopyProgressCallback(),
      base::Bind(&FileAPIMessageFilter::DidFinish, this, request_id));
}

void FileAPIMessageFilter::OnRemove(
    int request_id, const GURL& path, bool recursive) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  FileSystemURL url(context_->CrackURL(path));
  if (!ValidateFileSystemURL(request_id, url))
    return;
  if (!security_policy_->CanDeleteFileSystemFile(process_id_, url)) {
    Send(new FileSystemMsg_DidFail(request_id,
                                   base::File::FILE_ERROR_SECURITY));
    return;
  }

  operations_[request_id] = operation_runner()->Remove(
      url, recursive,
      base::Bind(&FileAPIMessageFilter::DidFinish, this, request_id));
}

void FileAPIMessageFilter::OnReadMetadata(
    int request_id, const GURL& path) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  FileSystemURL url(context_->CrackURL(path));
  if (!ValidateFileSystemURL(request_id, url))
    return;
  if (!security_policy_->CanReadFileSystemFile(process_id_, url)) {
    Send(new FileSystemMsg_DidFail(request_id,
                                   base::File::FILE_ERROR_SECURITY));
    return;
  }

  operations_[request_id] = operation_runner()->GetMetadata(
      url, FileSystemOperation::GET_METADATA_FIELD_IS_DIRECTORY |
               FileSystemOperation::GET_METADATA_FIELD_SIZE |
               FileSystemOperation::GET_METADATA_FIELD_LAST_MODIFIED,
      base::Bind(&FileAPIMessageFilter::DidGetMetadata, this, request_id));
}

void FileAPIMessageFilter::OnCreate(
    int request_id, const GURL& path, bool exclusive,
    bool is_directory, bool recursive) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  FileSystemURL url(context_->CrackURL(path));
  if (!ValidateFileSystemURL(request_id, url))
    return;
  if (!security_policy_->CanCreateFileSystemFile(process_id_, url)) {
    Send(new FileSystemMsg_DidFail(request_id,
                                   base::File::FILE_ERROR_SECURITY));
    return;
  }

  if (is_directory) {
    operations_[request_id] = operation_runner()->CreateDirectory(
        url, exclusive, recursive,
        base::Bind(&FileAPIMessageFilter::DidFinish, this, request_id));
  } else {
    operations_[request_id] = operation_runner()->CreateFile(
        url, exclusive,
        base::Bind(&FileAPIMessageFilter::DidFinish, this, request_id));
  }
}

void FileAPIMessageFilter::OnExists(
    int request_id, const GURL& path, bool is_directory) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  FileSystemURL url(context_->CrackURL(path));
  if (!ValidateFileSystemURL(request_id, url))
    return;
  if (!security_policy_->CanReadFileSystemFile(process_id_, url)) {
    Send(new FileSystemMsg_DidFail(request_id,
                                   base::File::FILE_ERROR_SECURITY));
    return;
  }

  if (is_directory) {
    operations_[request_id] = operation_runner()->DirectoryExists(
        url,
        base::Bind(&FileAPIMessageFilter::DidFinish, this, request_id));
  } else {
    operations_[request_id] = operation_runner()->FileExists(
        url,
        base::Bind(&FileAPIMessageFilter::DidFinish, this, request_id));
  }
}

void FileAPIMessageFilter::OnReadDirectory(
    int request_id, const GURL& path) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  FileSystemURL url(context_->CrackURL(path));
  if (!ValidateFileSystemURL(request_id, url))
    return;
  if (!security_policy_->CanReadFileSystemFile(process_id_, url)) {
    Send(new FileSystemMsg_DidFail(request_id,
                                   base::File::FILE_ERROR_SECURITY));
    return;
  }

  operations_[request_id] = operation_runner()->ReadDirectory(
      url, base::BindRepeating(&FileAPIMessageFilter::DidReadDirectory, this,
                               request_id));
}

void FileAPIMessageFilter::OnWrite(int request_id,
                                   const GURL& path,
                                   const std::string& blob_uuid,
                                   int64_t offset) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!request_context_) {
    // We can't write w/o a request context, trying to do so will crash.
    NOTREACHED();
    return;
  }

  FileSystemURL url(context_->CrackURL(path));
  if (!ValidateFileSystemURL(request_id, url))
    return;
  if (!security_policy_->CanWriteFileSystemFile(process_id_, url)) {
    Send(new FileSystemMsg_DidFail(request_id,
                                   base::File::FILE_ERROR_SECURITY));
    return;
  }

  std::unique_ptr<storage::BlobDataHandle> blob =
      blob_storage_context_->context()->GetBlobDataFromUUID(blob_uuid);

  operations_[request_id] = operation_runner()->Write(
      request_context_, url, std::move(blob), offset,
      base::Bind(&FileAPIMessageFilter::DidWrite, this, request_id));
}

void FileAPIMessageFilter::OnTruncate(int request_id,
                                      const GURL& path,
                                      int64_t length) {
  FileSystemURL url(context_->CrackURL(path));
  if (!ValidateFileSystemURL(request_id, url))
    return;
  if (!security_policy_->CanWriteFileSystemFile(process_id_, url)) {
    Send(new FileSystemMsg_DidFail(request_id,
                                   base::File::FILE_ERROR_SECURITY));
    return;
  }

  operations_[request_id] = operation_runner()->Truncate(
      url, length,
      base::Bind(&FileAPIMessageFilter::DidFinish, this, request_id));
}

void FileAPIMessageFilter::OnTouchFile(
    int request_id,
    const GURL& path,
    const base::Time& last_access_time,
    const base::Time& last_modified_time) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  FileSystemURL url(context_->CrackURL(path));
  if (!ValidateFileSystemURL(request_id, url))
    return;
  if (!security_policy_->CanCreateFileSystemFile(process_id_, url)) {
    Send(new FileSystemMsg_DidFail(request_id,
                                   base::File::FILE_ERROR_SECURITY));
    return;
  }

  operations_[request_id] = operation_runner()->TouchFile(
      url, last_access_time, last_modified_time,
      base::Bind(&FileAPIMessageFilter::DidFinish, this, request_id));
}

void FileAPIMessageFilter::OnCancel(
    int request_id,
    int request_id_to_cancel) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  OperationsMap::iterator found = operations_.find(request_id_to_cancel);
  if (found != operations_.end()) {
    // The cancel will eventually send both the write failure and the cancel
    // success.
    operation_runner()->Cancel(
        found->second,
        base::Bind(&FileAPIMessageFilter::DidFinish, this, request_id));
  } else {
    // The write already finished; report that we failed to stop it.
    Send(new FileSystemMsg_DidFail(
        request_id, base::File::FILE_ERROR_INVALID_OPERATION));
  }
}

void FileAPIMessageFilter::OnSyncGetPlatformPath(
    const GURL& path, base::FilePath* platform_path) {
  SyncGetPlatformPath(context_, process_id_, path, platform_path);
}

void FileAPIMessageFilter::OnCreateSnapshotFile(
    int request_id, const GURL& path) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  FileSystemURL url(context_->CrackURL(path));

  // Make sure if this file can be read by the renderer as this is
  // called when the renderer is about to create a new File object
  // (for reading the file).
  if (!ValidateFileSystemURL(request_id, url))
    return;
  if (!security_policy_->CanReadFileSystemFile(process_id_, url)) {
    Send(new FileSystemMsg_DidFail(request_id,
                                   base::File::FILE_ERROR_SECURITY));
    return;
  }

  FileSystemBackend* backend = context_->GetFileSystemBackend(url.type());
  if (backend->SupportsStreaming(url)) {
    operations_[request_id] = operation_runner()->GetMetadata(
        url, FileSystemOperation::GET_METADATA_FIELD_IS_DIRECTORY |
                 FileSystemOperation::GET_METADATA_FIELD_SIZE |
                 FileSystemOperation::GET_METADATA_FIELD_LAST_MODIFIED,
        base::Bind(&FileAPIMessageFilter::DidGetMetadataForStreaming, this,
                   request_id));
  } else {
    operations_[request_id] = operation_runner()->CreateSnapshotFile(
        url,
        base::Bind(&FileAPIMessageFilter::DidCreateSnapshot,
                   this, request_id, url));
  }
}

void FileAPIMessageFilter::OnDidReceiveSnapshotFile(int request_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  in_transit_snapshot_files_.erase(request_id);
}

void FileAPIMessageFilter::DidFinish(int request_id,
                                     base::File::Error result) {
  if (result == base::File::FILE_OK)
    Send(new FileSystemMsg_DidSucceed(request_id));
  else
    Send(new FileSystemMsg_DidFail(request_id, result));
  operations_.erase(request_id);
}

void FileAPIMessageFilter::DidGetMetadata(
    int request_id,
    base::File::Error result,
    const base::File::Info& info) {
  if (result == base::File::FILE_OK)
    Send(new FileSystemMsg_DidReadMetadata(request_id, info));
  else
    Send(new FileSystemMsg_DidFail(request_id, result));
  operations_.erase(request_id);
}

void FileAPIMessageFilter::DidGetMetadataForStreaming(
    int request_id,
    base::File::Error result,
    const base::File::Info& info) {
  if (result == base::File::FILE_OK) {
    // For now, streaming Blobs are implemented as a successful snapshot file
    // creation with an empty path.
    Send(new FileSystemMsg_DidCreateSnapshotFile(request_id, info,
                                                 base::FilePath()));
  } else {
    Send(new FileSystemMsg_DidFail(request_id, result));
  }
  operations_.erase(request_id);
}

void FileAPIMessageFilter::DidReadDirectory(
    int request_id,
    base::File::Error result,
    std::vector<filesystem::mojom::DirectoryEntry> entries,
    bool has_more) {
  if (result == base::File::FILE_OK) {
    if (!entries.empty() || !has_more)
      Send(new FileSystemMsg_DidReadDirectory(request_id, std::move(entries),
                                              has_more));
  } else {
    DCHECK(!has_more);
    Send(new FileSystemMsg_DidFail(request_id, result));
  }
  if (!has_more)
    operations_.erase(request_id);
}

void FileAPIMessageFilter::DidWrite(int request_id,
                                    base::File::Error result,
                                    int64_t bytes,
                                    bool complete) {
  if (result == base::File::FILE_OK) {
    Send(new FileSystemMsg_DidWrite(request_id, bytes, complete));
    if (complete)
      operations_.erase(request_id);
  } else {
    Send(new FileSystemMsg_DidFail(request_id, result));
    operations_.erase(request_id);
  }
}

void FileAPIMessageFilter::DidOpenFileSystem(int request_id,
                                             const GURL& root,
                                             const std::string& filesystem_name,
                                             base::File::Error result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (result == base::File::FILE_OK) {
    DCHECK(root.is_valid());
    Send(new FileSystemMsg_DidOpenFileSystem(
        request_id, filesystem_name, root));
  } else {
    Send(new FileSystemMsg_DidFail(request_id, result));
  }
  // For OpenFileSystem we do not create a new operation, so no unregister here.
}

void FileAPIMessageFilter::DidResolveURL(
    int request_id,
    base::File::Error result,
    const storage::FileSystemInfo& info,
    const base::FilePath& file_path,
    storage::FileSystemContext::ResolvedEntryType type) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (result == base::File::FILE_OK &&
      type == storage::FileSystemContext::RESOLVED_ENTRY_NOT_FOUND)
    result = base::File::FILE_ERROR_NOT_FOUND;

  if (result == base::File::FILE_OK) {
    DCHECK(info.root_url.is_valid());
    Send(new FileSystemMsg_DidResolveURL(
        request_id,
        info,
        file_path,
        type == storage::FileSystemContext::RESOLVED_ENTRY_DIRECTORY));
  } else {
    Send(new FileSystemMsg_DidFail(request_id, result));
  }
  // For ResolveURL we do not create a new operation, so no unregister here.
}

void FileAPIMessageFilter::DidCreateSnapshot(
    int request_id,
    const storage::FileSystemURL& url,
    base::File::Error result,
    const base::File::Info& info,
    const base::FilePath& platform_path,
    scoped_refptr<storage::ShareableFileReference> /* unused */) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  operations_.erase(request_id);

  if (result != base::File::FILE_OK) {
    Send(new FileSystemMsg_DidFail(request_id, result));
    return;
  }

  scoped_refptr<storage::ShareableFileReference> file_ref =
      storage::ShareableFileReference::Get(platform_path);
  if (!security_policy_->CanReadFile(process_id_, platform_path)) {
    // Give per-file read permission to the snapshot file if it hasn't it yet.
    // In order for the renderer to be able to read the file via File object,
    // it must be granted per-file read permission for the file's platform
    // path. By now, it has already been verified that the renderer has
    // sufficient permissions to read the file, so giving per-file permission
    // here must be safe.
    security_policy_->GrantReadFile(process_id_, platform_path);

    // Revoke all permissions for the file when the last ref of the file
    // is dropped.
    if (!file_ref.get()) {
      // Create a reference for temporary permission handling.
      file_ref = storage::ShareableFileReference::GetOrCreate(
          platform_path,
          storage::ShareableFileReference::DONT_DELETE_ON_FINAL_RELEASE,
          context_->default_file_task_runner());
    }
    file_ref->AddFinalReleaseCallback(
        base::BindOnce(&RevokeFilePermission, process_id_));
  }

  if (file_ref.get()) {
    // This ref is held until OnDidReceiveSnapshotFile is called.
    in_transit_snapshot_files_[request_id] = file_ref;
  }

  // Return the file info and platform_path.
  Send(new FileSystemMsg_DidCreateSnapshotFile(
      request_id, info, platform_path));
}

bool FileAPIMessageFilter::ValidateFileSystemURL(
    int request_id,
    const storage::FileSystemURL& url) {
  if (!FileSystemURLIsValid(context_, url)) {
    Send(new FileSystemMsg_DidFail(request_id,
                                   base::File::FILE_ERROR_INVALID_URL));
    return false;
  }

  // Deny access to files in PluginPrivate FileSystem from JavaScript.
  // TODO(nhiroki): Move this filter somewhere else since this is not for
  // validation.
  if (url.type() == storage::kFileSystemTypePluginPrivate) {
    Send(new FileSystemMsg_DidFail(request_id,
                                   base::File::FILE_ERROR_SECURITY));
    return false;
  }

  return true;
}

}  // namespace content
