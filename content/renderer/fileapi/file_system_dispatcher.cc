// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/fileapi/file_system_dispatcher.h"

#include <memory>
#include <utility>

#include "base/callback.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/process/process.h"
#include "components/services/filesystem/public/interfaces/types.mojom.h"
#include "content/child/child_thread_impl.h"
#include "content/common/fileapi/file_system_messages.h"
#include "storage/common/fileapi/file_system_info.h"

namespace content {

class FileSystemDispatcher::CallbackDispatcher {
 public:
  static std::unique_ptr<CallbackDispatcher> Create(
      const StatusCallback& callback) {
    auto dispatcher = base::WrapUnique(new CallbackDispatcher);
    dispatcher->status_callback_ = callback;
    dispatcher->error_callback_ = callback;
    return dispatcher;
  }
  static std::unique_ptr<CallbackDispatcher> Create(
      const MetadataCallback& callback,
      const StatusCallback& error_callback) {
    auto dispatcher = base::WrapUnique(new CallbackDispatcher);
    dispatcher->metadata_callback_ = callback;
    dispatcher->error_callback_ = error_callback;
    return dispatcher;
  }
  static std::unique_ptr<CallbackDispatcher> Create(
      const CreateSnapshotFileCallback& callback,
      const StatusCallback& error_callback) {
    auto dispatcher = base::WrapUnique(new CallbackDispatcher);
    dispatcher->snapshot_callback_ = callback;
    dispatcher->error_callback_ = error_callback;
    return dispatcher;
  }
  static std::unique_ptr<CallbackDispatcher> Create(
      const ReadDirectoryCallback& callback,
      const StatusCallback& error_callback) {
    auto dispatcher = base::WrapUnique(new CallbackDispatcher);
    dispatcher->directory_callback_ = callback;
    dispatcher->error_callback_ = error_callback;
    return dispatcher;
  }
  static std::unique_ptr<CallbackDispatcher> Create(
      const OpenFileSystemCallback& callback,
      const StatusCallback& error_callback) {
    auto dispatcher = base::WrapUnique(new CallbackDispatcher);
    dispatcher->filesystem_callback_ = callback;
    dispatcher->error_callback_ = error_callback;
    return dispatcher;
  }
  static std::unique_ptr<CallbackDispatcher> Create(
      const ResolveURLCallback& callback,
      const StatusCallback& error_callback) {
    auto dispatcher = base::WrapUnique(new CallbackDispatcher);
    dispatcher->resolve_callback_ = callback;
    dispatcher->error_callback_ = error_callback;
    return dispatcher;
  }
  static std::unique_ptr<CallbackDispatcher> Create(
      const WriteCallback& callback,
      const StatusCallback& error_callback) {
    auto dispatcher = base::WrapUnique(new CallbackDispatcher);
    dispatcher->write_callback_ = callback;
    dispatcher->error_callback_ = error_callback;
    return dispatcher;
  }

  ~CallbackDispatcher() {}

  void DidSucceed() { status_callback_.Run(base::File::FILE_OK); }

  void DidFail(base::File::Error error_code) {
    error_callback_.Run(error_code);
  }

  void DidReadMetadata(
      const base::File::Info& file_info) {
    metadata_callback_.Run(file_info);
  }

  void DidCreateSnapshotFile(
      const base::File::Info& file_info,
      const base::FilePath& platform_path,
      int request_id) {
    snapshot_callback_.Run(file_info, platform_path, request_id);
  }

  void DidReadDirectory(
      const std::vector<filesystem::mojom::DirectoryEntry>& entries,
      bool has_more) {
    directory_callback_.Run(entries, has_more);
  }

  void DidOpenFileSystem(const std::string& name,
                         const GURL& root) {
    filesystem_callback_.Run(name, root);
  }

  void DidResolveURL(const storage::FileSystemInfo& info,
                     const base::FilePath& file_path,
                     bool is_directory) {
    resolve_callback_.Run(info, file_path, is_directory);
  }

  void DidWrite(int64_t bytes, bool complete) {
    write_callback_.Run(bytes, complete);
  }

 private:
  CallbackDispatcher() {}

  StatusCallback status_callback_;
  MetadataCallback metadata_callback_;
  CreateSnapshotFileCallback snapshot_callback_;
  ReadDirectoryCallback directory_callback_;
  OpenFileSystemCallback filesystem_callback_;
  ResolveURLCallback resolve_callback_;
  WriteCallback write_callback_;

  StatusCallback error_callback_;

  DISALLOW_COPY_AND_ASSIGN(CallbackDispatcher);
};

FileSystemDispatcher::FileSystemDispatcher() {
}

FileSystemDispatcher::~FileSystemDispatcher() {
  // Make sure we fire all the remaining callbacks.
  for (base::IDMap<std::unique_ptr<CallbackDispatcher>>::iterator iter(
           &dispatchers_);
       !iter.IsAtEnd(); iter.Advance()) {
    int request_id = iter.GetCurrentKey();
    CallbackDispatcher* dispatcher = iter.GetCurrentValue();
    DCHECK(dispatcher);
    dispatcher->DidFail(base::File::FILE_ERROR_ABORT);
    dispatchers_.Remove(request_id);
  }
}

bool FileSystemDispatcher::OnMessageReceived(const IPC::Message& msg) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(FileSystemDispatcher, msg)
    IPC_MESSAGE_HANDLER(FileSystemMsg_DidOpenFileSystem, OnDidOpenFileSystem)
    IPC_MESSAGE_HANDLER(FileSystemMsg_DidResolveURL, OnDidResolveURL)
    IPC_MESSAGE_HANDLER(FileSystemMsg_DidSucceed, OnDidSucceed)
    IPC_MESSAGE_HANDLER(FileSystemMsg_DidReadDirectory, OnDidReadDirectory)
    IPC_MESSAGE_HANDLER(FileSystemMsg_DidReadMetadata, OnDidReadMetadata)
    IPC_MESSAGE_HANDLER(FileSystemMsg_DidCreateSnapshotFile,
                        OnDidCreateSnapshotFile)
    IPC_MESSAGE_HANDLER(FileSystemMsg_DidFail, OnDidFail)
    IPC_MESSAGE_HANDLER(FileSystemMsg_DidWrite, OnDidWrite)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void FileSystemDispatcher::OpenFileSystem(
    const GURL& origin_url,
    storage::FileSystemType type,
    const OpenFileSystemCallback& success_callback,
    const StatusCallback& error_callback) {
  int request_id = dispatchers_.Add(
      CallbackDispatcher::Create(success_callback, error_callback));
  ChildThreadImpl::current()->Send(new FileSystemHostMsg_OpenFileSystem(
      request_id, origin_url, type));
}

void FileSystemDispatcher::ResolveURL(
    const GURL& filesystem_url,
    const ResolveURLCallback& success_callback,
    const StatusCallback& error_callback) {
  int request_id = dispatchers_.Add(
      CallbackDispatcher::Create(success_callback, error_callback));
  ChildThreadImpl::current()->Send(new FileSystemHostMsg_ResolveURL(
          request_id, filesystem_url));
}

void FileSystemDispatcher::Move(
    const GURL& src_path,
    const GURL& dest_path,
    const StatusCallback& callback) {
  int request_id = dispatchers_.Add(CallbackDispatcher::Create(callback));
  ChildThreadImpl::current()->Send(new FileSystemHostMsg_Move(
          request_id, src_path, dest_path));
}

void FileSystemDispatcher::Copy(
    const GURL& src_path,
    const GURL& dest_path,
    const StatusCallback& callback) {
  int request_id = dispatchers_.Add(CallbackDispatcher::Create(callback));
  ChildThreadImpl::current()->Send(new FileSystemHostMsg_Copy(
      request_id, src_path, dest_path));
}

void FileSystemDispatcher::Remove(
    const GURL& path,
    bool recursive,
    const StatusCallback& callback) {
  int request_id = dispatchers_.Add(CallbackDispatcher::Create(callback));
  ChildThreadImpl::current()->Send(
      new FileSystemHostMsg_Remove(request_id, path, recursive));
}

void FileSystemDispatcher::ReadMetadata(
    const GURL& path,
    const MetadataCallback& success_callback,
    const StatusCallback& error_callback) {
  int request_id = dispatchers_.Add(
      CallbackDispatcher::Create(success_callback, error_callback));
  ChildThreadImpl::current()->Send(
      new FileSystemHostMsg_ReadMetadata(request_id, path));
}

void FileSystemDispatcher::CreateFile(
    const GURL& path,
    bool exclusive,
    const StatusCallback& callback) {
  int request_id = dispatchers_.Add(CallbackDispatcher::Create(callback));
  ChildThreadImpl::current()->Send(new FileSystemHostMsg_Create(
      request_id, path, exclusive,
      false /* is_directory */, false /* recursive */));
}

void FileSystemDispatcher::CreateDirectory(
    const GURL& path,
    bool exclusive,
    bool recursive,
    const StatusCallback& callback) {
  int request_id = dispatchers_.Add(CallbackDispatcher::Create(callback));
  ChildThreadImpl::current()->Send(new FileSystemHostMsg_Create(
      request_id, path, exclusive, true /* is_directory */, recursive));
}

void FileSystemDispatcher::Exists(
    const GURL& path,
    bool is_directory,
    const StatusCallback& callback) {
  int request_id = dispatchers_.Add(CallbackDispatcher::Create(callback));
  ChildThreadImpl::current()->Send(
      new FileSystemHostMsg_Exists(request_id, path, is_directory));
}

void FileSystemDispatcher::ReadDirectory(
    const GURL& path,
    const ReadDirectoryCallback& success_callback,
    const StatusCallback& error_callback) {
  int request_id = dispatchers_.Add(
      CallbackDispatcher::Create(success_callback, error_callback));
  ChildThreadImpl::current()->Send(
      new FileSystemHostMsg_ReadDirectory(request_id, path));
}

void FileSystemDispatcher::Truncate(const GURL& path,
                                    int64_t offset,
                                    int* request_id_out,
                                    const StatusCallback& callback) {
  int request_id = dispatchers_.Add(CallbackDispatcher::Create(callback));
  ChildThreadImpl::current()->Send(
      new FileSystemHostMsg_Truncate(request_id, path, offset));

  if (request_id_out)
    *request_id_out = request_id;
}

void FileSystemDispatcher::Write(const GURL& path,
                                 const std::string& blob_id,
                                 int64_t offset,
                                 int* request_id_out,
                                 const WriteCallback& success_callback,
                                 const StatusCallback& error_callback) {
  int request_id = dispatchers_.Add(
      CallbackDispatcher::Create(success_callback, error_callback));
  ChildThreadImpl::current()->Send(
      new FileSystemHostMsg_Write(request_id, path, blob_id, offset));

  if (request_id_out)
    *request_id_out = request_id;
}

void FileSystemDispatcher::Cancel(
    int request_id_to_cancel,
    const StatusCallback& callback) {
  int request_id = dispatchers_.Add(CallbackDispatcher::Create(callback));
  ChildThreadImpl::current()->Send(new FileSystemHostMsg_CancelWrite(
      request_id, request_id_to_cancel));
}

void FileSystemDispatcher::TouchFile(
    const GURL& path,
    const base::Time& last_access_time,
    const base::Time& last_modified_time,
    const StatusCallback& callback) {
  int request_id = dispatchers_.Add(CallbackDispatcher::Create(callback));
  ChildThreadImpl::current()->Send(
      new FileSystemHostMsg_TouchFile(
          request_id, path, last_access_time, last_modified_time));
}

void FileSystemDispatcher::CreateSnapshotFile(
    const GURL& file_path,
    const CreateSnapshotFileCallback& success_callback,
    const StatusCallback& error_callback) {
  int request_id = dispatchers_.Add(
      CallbackDispatcher::Create(success_callback, error_callback));
  ChildThreadImpl::current()->Send(
      new FileSystemHostMsg_CreateSnapshotFile(
          request_id, file_path));
}

void FileSystemDispatcher::OnDidOpenFileSystem(int request_id,
                                               const std::string& name,
                                               const GURL& root) {
  DCHECK(root.is_valid());
  CallbackDispatcher* dispatcher = dispatchers_.Lookup(request_id);
  DCHECK(dispatcher);
  dispatcher->DidOpenFileSystem(name, root);
  dispatchers_.Remove(request_id);
}

void FileSystemDispatcher::OnDidResolveURL(int request_id,
                                           const storage::FileSystemInfo& info,
                                           const base::FilePath& file_path,
                                           bool is_directory) {
  DCHECK(info.root_url.is_valid());
  CallbackDispatcher* dispatcher = dispatchers_.Lookup(request_id);
  DCHECK(dispatcher);
  dispatcher->DidResolveURL(info, file_path, is_directory);
  dispatchers_.Remove(request_id);
}

void FileSystemDispatcher::OnDidSucceed(int request_id) {
  CallbackDispatcher* dispatcher = dispatchers_.Lookup(request_id);
  DCHECK(dispatcher);
  dispatcher->DidSucceed();
  dispatchers_.Remove(request_id);
}

void FileSystemDispatcher::OnDidReadMetadata(
    int request_id, const base::File::Info& file_info) {
  CallbackDispatcher* dispatcher = dispatchers_.Lookup(request_id);
  DCHECK(dispatcher);
  dispatcher->DidReadMetadata(file_info);
  dispatchers_.Remove(request_id);
}

void FileSystemDispatcher::OnDidCreateSnapshotFile(
    int request_id, const base::File::Info& file_info,
    const base::FilePath& platform_path) {
  CallbackDispatcher* dispatcher = dispatchers_.Lookup(request_id);
  DCHECK(dispatcher);
  dispatcher->DidCreateSnapshotFile(file_info, platform_path, request_id);
  dispatchers_.Remove(request_id);
}

void FileSystemDispatcher::OnDidReadDirectory(
    int request_id,
    const std::vector<filesystem::mojom::DirectoryEntry>& entries,
    bool has_more) {
  CallbackDispatcher* dispatcher = dispatchers_.Lookup(request_id);
  DCHECK(dispatcher);
  dispatcher->DidReadDirectory(entries, has_more);
  if (!has_more)
    dispatchers_.Remove(request_id);
}

void FileSystemDispatcher::OnDidFail(
    int request_id, base::File::Error error_code) {
  CallbackDispatcher* dispatcher = dispatchers_.Lookup(request_id);
  DCHECK(dispatcher);
  dispatcher->DidFail(error_code);
  dispatchers_.Remove(request_id);
}

void FileSystemDispatcher::OnDidWrite(int request_id,
                                      int64_t bytes,
                                      bool complete) {
  CallbackDispatcher* dispatcher = dispatchers_.Lookup(request_id);
  DCHECK(dispatcher);
  dispatcher->DidWrite(bytes, complete);
  if (complete)
    dispatchers_.Remove(request_id);
}

}  // namespace content
