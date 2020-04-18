// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/fileapi/isolated_file_system_backend.h"

#include <stdint.h>

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "storage/browser/fileapi/async_file_util_adapter.h"
#include "storage/browser/fileapi/copy_or_move_file_validator.h"
#include "storage/browser/fileapi/dragged_file_util.h"
#include "storage/browser/fileapi/file_stream_reader.h"
#include "storage/browser/fileapi/file_stream_writer.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_operation.h"
#include "storage/browser/fileapi/file_system_operation_context.h"
#include "storage/browser/fileapi/isolated_context.h"
#include "storage/browser/fileapi/native_file_util.h"
#include "storage/browser/fileapi/transient_file_util.h"
#include "storage/browser/fileapi/watcher_manager.h"
#include "storage/common/fileapi/file_system_types.h"
#include "storage/common/fileapi/file_system_util.h"

namespace storage {

IsolatedFileSystemBackend::IsolatedFileSystemBackend(
    bool use_for_type_native_local,
    bool use_for_type_platform_app)
    : use_for_type_native_local_(use_for_type_native_local),
      use_for_type_platform_app_(use_for_type_platform_app),
      isolated_file_util_(new AsyncFileUtilAdapter(new LocalFileUtil())),
      dragged_file_util_(new AsyncFileUtilAdapter(new DraggedFileUtil())),
      transient_file_util_(new AsyncFileUtilAdapter(new TransientFileUtil())) {
}

IsolatedFileSystemBackend::~IsolatedFileSystemBackend() = default;

bool IsolatedFileSystemBackend::CanHandleType(FileSystemType type) const {
  switch (type) {
    case kFileSystemTypeIsolated:
    case kFileSystemTypeDragged:
    case kFileSystemTypeForTransientFile:
      return true;
    case kFileSystemTypeNativeLocal:
      return use_for_type_native_local_;
    case kFileSystemTypeNativeForPlatformApp:
      return use_for_type_platform_app_;
    default:
      return false;
  }
}

void IsolatedFileSystemBackend::Initialize(FileSystemContext* context) {
}

void IsolatedFileSystemBackend::ResolveURL(const FileSystemURL& url,
                                           OpenFileSystemMode mode,
                                           OpenFileSystemCallback callback) {
  // We never allow opening a new isolated FileSystem via usual ResolveURL.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), GURL(), std::string(),
                                base::File::FILE_ERROR_SECURITY));
}

AsyncFileUtil* IsolatedFileSystemBackend::GetAsyncFileUtil(
    FileSystemType type) {
  switch (type) {
    case kFileSystemTypeNativeLocal:
      return isolated_file_util_.get();
    case kFileSystemTypeDragged:
      return dragged_file_util_.get();
    case kFileSystemTypeForTransientFile:
      return transient_file_util_.get();
    default:
      NOTREACHED();
  }
  return NULL;
}

WatcherManager* IsolatedFileSystemBackend::GetWatcherManager(
    FileSystemType type) {
  return NULL;
}

CopyOrMoveFileValidatorFactory*
IsolatedFileSystemBackend::GetCopyOrMoveFileValidatorFactory(
    FileSystemType type, base::File::Error* error_code) {
  DCHECK(error_code);
  *error_code = base::File::FILE_OK;
  return NULL;
}

FileSystemOperation* IsolatedFileSystemBackend::CreateFileSystemOperation(
    const FileSystemURL& url,
    FileSystemContext* context,
    base::File::Error* error_code) const {
  return FileSystemOperation::Create(
      url, context, std::make_unique<FileSystemOperationContext>(context));
}

bool IsolatedFileSystemBackend::SupportsStreaming(
    const storage::FileSystemURL& url) const {
  return false;
}

bool IsolatedFileSystemBackend::HasInplaceCopyImplementation(
    storage::FileSystemType type) const {
  DCHECK(type == kFileSystemTypeNativeLocal || type == kFileSystemTypeDragged ||
         type == kFileSystemTypeForTransientFile);
  return false;
}

std::unique_ptr<storage::FileStreamReader>
IsolatedFileSystemBackend::CreateFileStreamReader(
    const FileSystemURL& url,
    int64_t offset,
    int64_t max_bytes_to_read,
    const base::Time& expected_modification_time,
    FileSystemContext* context) const {
  return std::unique_ptr<storage::FileStreamReader>(
      storage::FileStreamReader::CreateForLocalFile(
          context->default_file_task_runner(), url.path(), offset,
          expected_modification_time));
}

std::unique_ptr<FileStreamWriter>
IsolatedFileSystemBackend::CreateFileStreamWriter(
    const FileSystemURL& url,
    int64_t offset,
    FileSystemContext* context) const {
  return std::unique_ptr<FileStreamWriter>(FileStreamWriter::CreateForLocalFile(
      context->default_file_task_runner(), url.path(), offset,
      FileStreamWriter::OPEN_EXISTING_FILE));
}

FileSystemQuotaUtil* IsolatedFileSystemBackend::GetQuotaUtil() {
  // No quota support.
  return NULL;
}

const UpdateObserverList* IsolatedFileSystemBackend::GetUpdateObservers(
    FileSystemType type) const {
  return NULL;
}

const ChangeObserverList* IsolatedFileSystemBackend::GetChangeObservers(
    FileSystemType type) const {
  return NULL;
}

const AccessObserverList* IsolatedFileSystemBackend::GetAccessObservers(
    FileSystemType type) const {
  return NULL;
}

}  // namespace storage
