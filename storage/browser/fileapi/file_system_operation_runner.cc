// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/fileapi/file_system_operation_runner.h"

#include <stdint.h>

#include <memory>
#include <tuple>
#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/url_request/url_request_context.h"
#include "storage/browser/blob/blob_url_request_job_factory.h"
#include "storage/browser/blob/shareable_file_reference.h"
#include "storage/browser/fileapi/file_observers.h"
#include "storage/browser/fileapi/file_stream_writer.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_writer_delegate.h"

namespace storage {

using OperationID = FileSystemOperationRunner::OperationID;

class FileSystemOperationRunner::BeginOperationScoper
    : public base::SupportsWeakPtr<
          FileSystemOperationRunner::BeginOperationScoper> {
 public:
  BeginOperationScoper() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(BeginOperationScoper);
};

FileSystemOperationRunner::OperationHandle::OperationHandle() = default;
FileSystemOperationRunner::OperationHandle::OperationHandle(
    const OperationHandle& other) = default;
FileSystemOperationRunner::OperationHandle::~OperationHandle() = default;

FileSystemOperationRunner::~FileSystemOperationRunner() = default;

void FileSystemOperationRunner::Shutdown() {
  operations_.clear();
}

OperationID FileSystemOperationRunner::CreateFile(
    const FileSystemURL& url,
    bool exclusive,
    const StatusCallback& callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, callback, error);
    return handle.id;
  }
  PrepareForWrite(handle.id, url);
  operation_raw->CreateFile(
      url, exclusive,
      base::Bind(&FileSystemOperationRunner::DidFinish, AsWeakPtr(),
                 handle, callback));
  return handle.id;
}

OperationID FileSystemOperationRunner::CreateDirectory(
    const FileSystemURL& url,
    bool exclusive,
    bool recursive,
    const StatusCallback& callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, callback, error);
    return handle.id;
  }
  PrepareForWrite(handle.id, url);
  operation_raw->CreateDirectory(
      url, exclusive, recursive,
      base::Bind(&FileSystemOperationRunner::DidFinish, AsWeakPtr(),
                 handle, callback));
  return handle.id;
}

OperationID FileSystemOperationRunner::Copy(
    const FileSystemURL& src_url,
    const FileSystemURL& dest_url,
    CopyOrMoveOption option,
    ErrorBehavior error_behavior,
    const CopyProgressCallback& progress_callback,
    const StatusCallback& callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(dest_url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, callback, error);
    return handle.id;
  }
  PrepareForWrite(handle.id, dest_url);
  PrepareForRead(handle.id, src_url);
  operation_raw->Copy(src_url, dest_url, option, error_behavior,
                  progress_callback.is_null()
                      ? CopyProgressCallback()
                      : base::Bind(&FileSystemOperationRunner::OnCopyProgress,
                                   AsWeakPtr(), handle, progress_callback),
                  base::Bind(&FileSystemOperationRunner::DidFinish, AsWeakPtr(),
                             handle, callback));
  return handle.id;
}

OperationID FileSystemOperationRunner::Move(
    const FileSystemURL& src_url,
    const FileSystemURL& dest_url,
    CopyOrMoveOption option,
    const StatusCallback& callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(dest_url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, callback, error);
    return handle.id;
  }
  PrepareForWrite(handle.id, dest_url);
  PrepareForWrite(handle.id, src_url);
  operation_raw->Move(
      src_url, dest_url, option,
      base::Bind(&FileSystemOperationRunner::DidFinish, AsWeakPtr(),
                 handle, callback));
  return handle.id;
}

OperationID FileSystemOperationRunner::DirectoryExists(
    const FileSystemURL& url,
    const StatusCallback& callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, callback, error);
    return handle.id;
  }
  PrepareForRead(handle.id, url);
  operation_raw->DirectoryExists(
      url,
      base::Bind(&FileSystemOperationRunner::DidFinish, AsWeakPtr(),
                 handle, callback));
  return handle.id;
}

OperationID FileSystemOperationRunner::FileExists(
    const FileSystemURL& url,
    const StatusCallback& callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, callback, error);
    return handle.id;
  }
  PrepareForRead(handle.id, url);
  operation_raw->FileExists(
      url,
      base::Bind(&FileSystemOperationRunner::DidFinish, AsWeakPtr(),
                 handle, callback));
  return handle.id;
}

OperationID FileSystemOperationRunner::GetMetadata(
    const FileSystemURL& url,
    int fields,
    const GetMetadataCallback& callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidGetMetadata(handle, callback, error, base::File::Info());
    return handle.id;
  }
  PrepareForRead(handle.id, url);
  operation_raw->GetMetadata(url, fields,
                         base::Bind(&FileSystemOperationRunner::DidGetMetadata,
                                    AsWeakPtr(), handle, callback));
  return handle.id;
}

OperationID FileSystemOperationRunner::ReadDirectory(
    const FileSystemURL& url,
    const ReadDirectoryCallback& callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidReadDirectory(handle, std::move(callback), error,
                     std::vector<filesystem::mojom::DirectoryEntry>(), false);
    return handle.id;
  }
  PrepareForRead(handle.id, url);
  operation_raw->ReadDirectory(
      url, base::BindRepeating(&FileSystemOperationRunner::DidReadDirectory,
                               AsWeakPtr(), handle, callback));
  return handle.id;
}

OperationID FileSystemOperationRunner::Remove(
    const FileSystemURL& url, bool recursive,
    const StatusCallback& callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, callback, error);
    return handle.id;
  }
  PrepareForWrite(handle.id, url);
  operation_raw->Remove(
      url, recursive,
      base::Bind(&FileSystemOperationRunner::DidFinish, AsWeakPtr(),
                 handle, callback));
  return handle.id;
}

OperationID FileSystemOperationRunner::Write(
    const net::URLRequestContext* url_request_context,
    const FileSystemURL& url,
    std::unique_ptr<storage::BlobDataHandle> blob,
    int64_t offset,
    const WriteCallback& callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidWrite(handle, callback, error, 0, true);
    return handle.id;
  }

  std::unique_ptr<FileStreamWriter> writer(
      file_system_context_->CreateFileStreamWriter(url, offset));
  if (!writer) {
    // Write is not supported.
    DidWrite(handle, callback, base::File::FILE_ERROR_SECURITY, 0, true);
    return handle.id;
  }

  std::unique_ptr<FileWriterDelegate> writer_delegate(new FileWriterDelegate(
      std::move(writer), url.mount_option().flush_policy()));

  std::unique_ptr<net::URLRequest> blob_request(
      storage::BlobProtocolHandler::CreateBlobRequest(
          std::move(blob), url_request_context, writer_delegate.get()));

  PrepareForWrite(handle.id, url);
  operation_raw->Write(url, std::move(writer_delegate), std::move(blob_request),
                   base::Bind(&FileSystemOperationRunner::DidWrite, AsWeakPtr(),
                              handle, callback));
  return handle.id;
}

OperationID FileSystemOperationRunner::Truncate(
    const FileSystemURL& url,
    int64_t length,
    const StatusCallback& callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, callback, error);
    return handle.id;
  }
  PrepareForWrite(handle.id, url);
  operation_raw->Truncate(
      url, length,
      base::Bind(&FileSystemOperationRunner::DidFinish, AsWeakPtr(),
                 handle, callback));
  return handle.id;
}

void FileSystemOperationRunner::Cancel(
    OperationID id,
    const StatusCallback& callback) {
  if (base::ContainsKey(finished_operations_, id)) {
    DCHECK(!base::ContainsKey(stray_cancel_callbacks_, id));
    stray_cancel_callbacks_[id] = callback;
    return;
  }

  Operations::iterator found = operations_.find(id);
  if (found == operations_.end() || !found->second) {
    // There is no operation with |id|.
    callback.Run(base::File::FILE_ERROR_INVALID_OPERATION);
    return;
  }
  found->second->Cancel(callback);
}

OperationID FileSystemOperationRunner::TouchFile(
    const FileSystemURL& url,
    const base::Time& last_access_time,
    const base::Time& last_modified_time,
    const StatusCallback& callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, callback, error);
    return handle.id;
  }
  PrepareForWrite(handle.id, url);
  operation_raw->TouchFile(
      url, last_access_time, last_modified_time,
      base::Bind(&FileSystemOperationRunner::DidFinish, AsWeakPtr(),
                 handle, callback));
  return handle.id;
}

OperationID FileSystemOperationRunner::OpenFile(
    const FileSystemURL& url,
    int file_flags,
    const OpenFileCallback& callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidOpenFile(handle, callback, base::File(error), base::Closure());
    return handle.id;
  }
  if (file_flags &
      (base::File::FLAG_CREATE | base::File::FLAG_OPEN_ALWAYS |
       base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_OPEN_TRUNCATED |
       base::File::FLAG_WRITE | base::File::FLAG_EXCLUSIVE_WRITE |
       base::File::FLAG_DELETE_ON_CLOSE |
       base::File::FLAG_WRITE_ATTRIBUTES)) {
    PrepareForWrite(handle.id, url);
  } else {
    PrepareForRead(handle.id, url);
  }
  operation_raw->OpenFile(
      url, file_flags,
      base::Bind(&FileSystemOperationRunner::DidOpenFile, AsWeakPtr(),
                 handle, callback));
  return handle.id;
}

OperationID FileSystemOperationRunner::CreateSnapshotFile(
    const FileSystemURL& url,
    const SnapshotFileCallback& callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidCreateSnapshot(handle, callback, error, base::File::Info(),
                      base::FilePath(), NULL);
    return handle.id;
  }
  PrepareForRead(handle.id, url);
  operation_raw->CreateSnapshotFile(
      url,
      base::Bind(&FileSystemOperationRunner::DidCreateSnapshot, AsWeakPtr(),
                 handle, callback));
  return handle.id;
}

OperationID FileSystemOperationRunner::CopyInForeignFile(
    const base::FilePath& src_local_disk_path,
    const FileSystemURL& dest_url,
    const StatusCallback& callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(dest_url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, callback, error);
    return handle.id;
  }
  PrepareForWrite(handle.id, dest_url);
  operation_raw->CopyInForeignFile(
      src_local_disk_path, dest_url,
      base::Bind(&FileSystemOperationRunner::DidFinish, AsWeakPtr(),
                 handle, callback));
  return handle.id;
}

OperationID FileSystemOperationRunner::RemoveFile(
    const FileSystemURL& url,
    const StatusCallback& callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, callback, error);
    return handle.id;
  }
  PrepareForWrite(handle.id, url);
  operation_raw->RemoveFile(
      url,
      base::Bind(&FileSystemOperationRunner::DidFinish, AsWeakPtr(),
                 handle, callback));
  return handle.id;
}

OperationID FileSystemOperationRunner::RemoveDirectory(
    const FileSystemURL& url,
    const StatusCallback& callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, callback, error);
    return handle.id;
  }
  PrepareForWrite(handle.id, url);
  operation_raw->RemoveDirectory(
      url,
      base::Bind(&FileSystemOperationRunner::DidFinish, AsWeakPtr(),
                 handle, callback));
  return handle.id;
}

OperationID FileSystemOperationRunner::CopyFileLocal(
    const FileSystemURL& src_url,
    const FileSystemURL& dest_url,
    CopyOrMoveOption option,
    const CopyFileProgressCallback& progress_callback,
    const StatusCallback& callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(src_url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, callback, error);
    return handle.id;
  }
  PrepareForRead(handle.id, src_url);
  PrepareForWrite(handle.id, dest_url);
  operation_raw->CopyFileLocal(
      src_url, dest_url, option, progress_callback,
      base::Bind(&FileSystemOperationRunner::DidFinish, AsWeakPtr(),
                 handle, callback));
  return handle.id;
}

OperationID FileSystemOperationRunner::MoveFileLocal(
    const FileSystemURL& src_url,
    const FileSystemURL& dest_url,
    CopyOrMoveOption option,
    const StatusCallback& callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(src_url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, callback, error);
    return handle.id;
  }
  PrepareForWrite(handle.id, src_url);
  PrepareForWrite(handle.id, dest_url);
  operation_raw->MoveFileLocal(
      src_url, dest_url, option,
      base::Bind(&FileSystemOperationRunner::DidFinish, AsWeakPtr(),
                 handle, callback));
  return handle.id;
}

base::File::Error FileSystemOperationRunner::SyncGetPlatformPath(
    const FileSystemURL& url,
    base::FilePath* platform_path) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation(
      file_system_context_->CreateFileSystemOperation(url, &error));
  if (!operation.get())
    return error;
  return operation->SyncGetPlatformPath(url, platform_path);
}

FileSystemOperationRunner::FileSystemOperationRunner(
    FileSystemContext* file_system_context)
    : file_system_context_(file_system_context) {}

void FileSystemOperationRunner::DidFinish(
    const OperationHandle& handle,
    const StatusCallback& callback,
    base::File::Error rv) {
  if (handle.scope) {
    finished_operations_.insert(handle.id);
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&FileSystemOperationRunner::DidFinish,
                                  AsWeakPtr(), handle, callback, rv));
    return;
  }
  callback.Run(rv);
  FinishOperation(handle.id);
}

void FileSystemOperationRunner::DidGetMetadata(
    const OperationHandle& handle,
    const GetMetadataCallback& callback,
    base::File::Error rv,
    const base::File::Info& file_info) {
  if (handle.scope) {
    finished_operations_.insert(handle.id);
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&FileSystemOperationRunner::DidGetMetadata, AsWeakPtr(),
                       handle, callback, rv, file_info));
    return;
  }
  callback.Run(rv, file_info);
  FinishOperation(handle.id);
}

void FileSystemOperationRunner::DidReadDirectory(
    const OperationHandle& handle,
    const ReadDirectoryCallback& callback,
    base::File::Error rv,
    std::vector<filesystem::mojom::DirectoryEntry> entries,
    bool has_more) {
  if (handle.scope) {
    finished_operations_.insert(handle.id);
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&FileSystemOperationRunner::DidReadDirectory,
                                  AsWeakPtr(), handle, callback, rv,
                                  std::move(entries), has_more));
    return;
  }
  callback.Run(rv, std::move(entries), has_more);
  if (rv != base::File::FILE_OK || !has_more)
    FinishOperation(handle.id);
}

void FileSystemOperationRunner::DidWrite(const OperationHandle& handle,
                                         const WriteCallback& callback,
                                         base::File::Error rv,
                                         int64_t bytes,
                                         bool complete) {
  if (handle.scope) {
    finished_operations_.insert(handle.id);
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&FileSystemOperationRunner::DidWrite, AsWeakPtr(),
                       handle, callback, rv, bytes, complete));
    return;
  }
  callback.Run(rv, bytes, complete);
  if (rv != base::File::FILE_OK || complete)
    FinishOperation(handle.id);
}

void FileSystemOperationRunner::DidOpenFile(
    const OperationHandle& handle,
    const OpenFileCallback& callback,
    base::File file,
    base::OnceClosure on_close_callback) {
  if (handle.scope) {
    finished_operations_.insert(handle.id);
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&FileSystemOperationRunner::DidOpenFile, AsWeakPtr(),
                       handle, callback, std::move(file),
                       std::move(on_close_callback)));
    return;
  }
  callback.Run(std::move(file), std::move(on_close_callback));
  FinishOperation(handle.id);
}

void FileSystemOperationRunner::DidCreateSnapshot(
    const OperationHandle& handle,
    const SnapshotFileCallback& callback,
    base::File::Error rv,
    const base::File::Info& file_info,
    const base::FilePath& platform_path,
    scoped_refptr<storage::ShareableFileReference> file_ref) {
  if (handle.scope) {
    finished_operations_.insert(handle.id);
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&FileSystemOperationRunner::DidCreateSnapshot,
                                  AsWeakPtr(), handle, callback, rv, file_info,
                                  platform_path, std::move(file_ref)));
    return;
  }
  callback.Run(rv, file_info, platform_path, std::move(file_ref));
  FinishOperation(handle.id);
}

void FileSystemOperationRunner::OnCopyProgress(
    const OperationHandle& handle,
    const CopyProgressCallback& callback,
    FileSystemOperation::CopyProgressType type,
    const FileSystemURL& source_url,
    const FileSystemURL& dest_url,
    int64_t size) {
  if (handle.scope) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&FileSystemOperationRunner::OnCopyProgress, AsWeakPtr(),
                       handle, callback, type, source_url, dest_url, size));
    return;
  }
  callback.Run(type, source_url, dest_url, size);
}

void FileSystemOperationRunner::PrepareForWrite(OperationID id,
                                                const FileSystemURL& url) {
  if (file_system_context_->GetUpdateObservers(url.type())) {
    file_system_context_->GetUpdateObservers(url.type())
        ->Notify(&FileUpdateObserver::OnStartUpdate, url);
  }
  write_target_urls_[id].insert(url);
}

void FileSystemOperationRunner::PrepareForRead(OperationID id,
                                               const FileSystemURL& url) {
  if (file_system_context_->GetAccessObservers(url.type())) {
    file_system_context_->GetAccessObservers(url.type())
        ->Notify(&FileAccessObserver::OnAccess, url);
  }
}

FileSystemOperationRunner::OperationHandle
FileSystemOperationRunner::BeginOperation(
    std::unique_ptr<FileSystemOperation> operation,
    base::WeakPtr<BeginOperationScoper> scope) {
  OperationHandle handle;
  handle.id = next_operation_id_++;
  operations_.emplace(handle.id, std::move(operation));
  handle.scope = scope;
  return handle;
}

void FileSystemOperationRunner::FinishOperation(OperationID id) {
  OperationToURLSet::iterator found = write_target_urls_.find(id);
  if (found != write_target_urls_.end()) {
    const FileSystemURLSet& urls = found->second;
    for (const FileSystemURL& url : urls) {
      if (file_system_context_->GetUpdateObservers(url.type())) {
        file_system_context_->GetUpdateObservers(url.type())
            ->Notify(&FileUpdateObserver::OnEndUpdate, url);
      }
    }
    write_target_urls_.erase(found);
  }

  operations_.erase(id);
  finished_operations_.erase(id);

  // Dispatch stray cancel callback if exists.
  std::map<OperationID, StatusCallback>::iterator found_cancel =
      stray_cancel_callbacks_.find(id);
  if (found_cancel != stray_cancel_callbacks_.end()) {
    // This cancel has been requested after the operation has finished,
    // so report that we failed to stop it.
    found_cancel->second.Run(base::File::FILE_ERROR_INVALID_OPERATION);
    stray_cancel_callbacks_.erase(found_cancel);
  }
}

}  // namespace storage
