// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media_galleries/fileapi/native_media_file_util.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_util.h"
#include "base/task_runner_util.h"
#include "chrome/browser/media_galleries/fileapi/media_path_filter.h"
#include "components/services/filesystem/public/interfaces/types.mojom.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/io_buffer.h"
#include "net/base/mime_sniffer.h"
#include "storage/browser/blob/shareable_file_reference.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_operation_context.h"
#include "storage/browser/fileapi/native_file_util.h"
#include "url/gurl.h"

namespace {

// Returns true if the current thread is capable of doing IO.
bool IsOnTaskRunnerThread(storage::FileSystemOperationContext* context) {
  return context->task_runner()->RunsTasksInCurrentSequence();
}

base::File::Error IsMediaHeader(const char* buf, size_t length) {
  if (length == 0)
    return base::File::FILE_ERROR_SECURITY;

  std::string mime_type;
  if (!net::SniffMimeTypeFromLocalData(buf, length, &mime_type))
    return base::File::FILE_ERROR_SECURITY;

  if (base::StartsWith(mime_type, "image/", base::CompareCase::SENSITIVE) ||
      base::StartsWith(mime_type, "audio/", base::CompareCase::SENSITIVE) ||
      base::StartsWith(mime_type, "video/", base::CompareCase::SENSITIVE) ||
      mime_type == "application/x-shockwave-flash") {
    return base::File::FILE_OK;
  }
  return base::File::FILE_ERROR_SECURITY;
}

void HoldFileRef(scoped_refptr<storage::ShareableFileReference> file_ref) {}

void DidOpenSnapshot(storage::AsyncFileUtil::CreateOrOpenCallback callback,
                     scoped_refptr<storage::ShareableFileReference> file_ref,
                     base::File file) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!file.IsValid()) {
    std::move(callback).Run(std::move(file), base::Closure());
    return;
  }
  std::move(callback).Run(std::move(file),
                          base::Bind(&HoldFileRef, std::move(file_ref)));
}

}  // namespace

NativeMediaFileUtil::NativeMediaFileUtil(MediaPathFilter* media_path_filter)
    : media_path_filter_(media_path_filter),
      weak_factory_(this) {
}

NativeMediaFileUtil::~NativeMediaFileUtil() {
}

// static
base::File::Error NativeMediaFileUtil::IsMediaFile(
    const base::FilePath& path) {
  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid())
    return file.error_details();

  char buffer[net::kMaxBytesToSniff];

  // Read as much as net::SniffMimeTypeFromLocalData() will bother looking at.
  int64_t len = file.Read(0, buffer, net::kMaxBytesToSniff);
  if (len < 0)
    return base::File::FILE_ERROR_FAILED;

  return IsMediaHeader(buffer, len);
}

// static
base::File::Error NativeMediaFileUtil::BufferIsMediaHeader(
    net::IOBuffer* buf, size_t length) {
  return IsMediaHeader(buf->data(), length);
}

// static
void NativeMediaFileUtil::CreatedSnapshotFileForCreateOrOpen(
    base::SequencedTaskRunner* media_task_runner,
    int file_flags,
    storage::AsyncFileUtil::CreateOrOpenCallback callback,
    base::File::Error result,
    const base::File::Info& file_info,
    const base::FilePath& platform_path,
    scoped_refptr<storage::ShareableFileReference> file_ref) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (result != base::File::FILE_OK) {
    std::move(callback).Run(base::File(), base::Closure());
    return;
  }
  base::PostTaskAndReplyWithResult(
      media_task_runner, FROM_HERE,
      base::BindOnce(&storage::NativeFileUtil::CreateOrOpen, platform_path,
                     file_flags),
      base::BindOnce(&DidOpenSnapshot, std::move(callback),
                     std::move(file_ref)));
}

void NativeMediaFileUtil::CreateOrOpen(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    int file_flags,
    CreateOrOpenCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  // Returns an error if any unsupported flag is found.
  if (file_flags & ~(base::File::FLAG_OPEN |
                     base::File::FLAG_READ |
                     base::File::FLAG_WRITE_ATTRIBUTES)) {
    std::move(callback).Run(base::File(base::File::FILE_ERROR_SECURITY),
                            base::Closure());
    return;
  }
  scoped_refptr<base::SequencedTaskRunner> task_runner = context->task_runner();
  CreateSnapshotFile(
      std::move(context), url,
      base::BindOnce(&NativeMediaFileUtil::CreatedSnapshotFileForCreateOrOpen,
                     base::RetainedRef(task_runner), file_flags,
                     std::move(callback)));
}

void NativeMediaFileUtil::EnsureFileExists(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    EnsureFileExistsCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  std::move(callback).Run(base::File::FILE_ERROR_SECURITY, false);
}

void NativeMediaFileUtil::CreateDirectory(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    bool exclusive,
    bool recursive,
    StatusCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  storage::FileSystemOperationContext* context_ptr = context.get();
  const bool success = context_ptr->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&NativeMediaFileUtil::CreateDirectoryOnTaskRunnerThread,
                     weak_factory_.GetWeakPtr(), std::move(context), url,
                     exclusive, recursive, std::move(callback)));
  DCHECK(success);
}

void NativeMediaFileUtil::GetFileInfo(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    int /* fields */,
    GetFileInfoCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  storage::FileSystemOperationContext* context_ptr = context.get();
  const bool success = context_ptr->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&NativeMediaFileUtil::GetFileInfoOnTaskRunnerThread,
                     weak_factory_.GetWeakPtr(), std::move(context), url,
                     std::move(callback)));
  DCHECK(success);
}

void NativeMediaFileUtil::ReadDirectory(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    ReadDirectoryCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  storage::FileSystemOperationContext* context_ptr = context.get();
  const bool success = context_ptr->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&NativeMediaFileUtil::ReadDirectoryOnTaskRunnerThread,
                     weak_factory_.GetWeakPtr(), std::move(context), url,
                     std::move(callback)));
  DCHECK(success);
}

void NativeMediaFileUtil::Touch(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    const base::Time& last_access_time,
    const base::Time& last_modified_time,
    StatusCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  std::move(callback).Run(base::File::FILE_ERROR_SECURITY);
}

void NativeMediaFileUtil::Truncate(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    int64_t length,
    StatusCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  std::move(callback).Run(base::File::FILE_ERROR_SECURITY);
}

void NativeMediaFileUtil::CopyFileLocal(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& src_url,
    const storage::FileSystemURL& dest_url,
    CopyOrMoveOption option,
    CopyFileProgressCallback progress_callback,
    StatusCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  storage::FileSystemOperationContext* context_ptr = context.get();
  const bool success = context_ptr->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &NativeMediaFileUtil::CopyOrMoveFileLocalOnTaskRunnerThread,
          weak_factory_.GetWeakPtr(), std::move(context), src_url, dest_url,
          option, true /* copy */, std::move(callback)));
  DCHECK(success);
}

void NativeMediaFileUtil::MoveFileLocal(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& src_url,
    const storage::FileSystemURL& dest_url,
    CopyOrMoveOption option,
    StatusCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  storage::FileSystemOperationContext* context_ptr = context.get();
  const bool success = context_ptr->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &NativeMediaFileUtil::CopyOrMoveFileLocalOnTaskRunnerThread,
          weak_factory_.GetWeakPtr(), std::move(context), src_url, dest_url,
          option, false /* copy */, std::move(callback)));
  DCHECK(success);
}

void NativeMediaFileUtil::CopyInForeignFile(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const base::FilePath& src_file_path,
    const storage::FileSystemURL& dest_url,
    StatusCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  storage::FileSystemOperationContext* context_ptr = context.get();
  const bool success = context_ptr->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&NativeMediaFileUtil::CopyInForeignFileOnTaskRunnerThread,
                     weak_factory_.GetWeakPtr(), std::move(context),
                     src_file_path, dest_url, std::move(callback)));
  DCHECK(success);
}

void NativeMediaFileUtil::DeleteFile(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    StatusCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  storage::FileSystemOperationContext* context_ptr = context.get();
  const bool success = context_ptr->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&NativeMediaFileUtil::DeleteFileOnTaskRunnerThread,
                     weak_factory_.GetWeakPtr(), std::move(context), url,
                     std::move(callback)));
  DCHECK(success);
}

// This is needed to support Copy and Move.
void NativeMediaFileUtil::DeleteDirectory(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    StatusCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  storage::FileSystemOperationContext* context_ptr = context.get();
  const bool success = context_ptr->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&NativeMediaFileUtil::DeleteDirectoryOnTaskRunnerThread,
                     weak_factory_.GetWeakPtr(), std::move(context), url,
                     std::move(callback)));
  DCHECK(success);
}

void NativeMediaFileUtil::DeleteRecursively(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    StatusCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  std::move(callback).Run(base::File::FILE_ERROR_INVALID_OPERATION);
}

void NativeMediaFileUtil::CreateSnapshotFile(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    CreateSnapshotFileCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  storage::FileSystemOperationContext* context_ptr = context.get();
  const bool success = context_ptr->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&NativeMediaFileUtil::CreateSnapshotFileOnTaskRunnerThread,
                     weak_factory_.GetWeakPtr(), std::move(context), url,
                     std::move(callback)));
  DCHECK(success);
}

void NativeMediaFileUtil::CreateDirectoryOnTaskRunnerThread(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    bool exclusive,
    bool recursive,
    StatusCallback callback) {
  DCHECK(IsOnTaskRunnerThread(context.get()));
  base::File::Error error =
      CreateDirectorySync(context.get(), url, exclusive, recursive);
  content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                   base::BindOnce(std::move(callback), error));
}

void NativeMediaFileUtil::GetFileInfoOnTaskRunnerThread(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    GetFileInfoCallback callback) {
  DCHECK(IsOnTaskRunnerThread(context.get()));
  base::File::Info file_info;
  base::File::Error error =
      GetFileInfoSync(context.get(), url, &file_info, NULL);
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(std::move(callback), error, file_info));
}

void NativeMediaFileUtil::ReadDirectoryOnTaskRunnerThread(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    ReadDirectoryCallback callback) {
  DCHECK(IsOnTaskRunnerThread(context.get()));
  EntryList entry_list;
  base::File::Error error =
      ReadDirectorySync(context.get(), url, &entry_list);
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(std::move(callback), error, entry_list,
                     false /* has_more */));
}

void NativeMediaFileUtil::CopyOrMoveFileLocalOnTaskRunnerThread(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& src_url,
    const storage::FileSystemURL& dest_url,
    CopyOrMoveOption option,
    bool copy,
    StatusCallback callback) {
  DCHECK(IsOnTaskRunnerThread(context.get()));
  base::File::Error error =
      CopyOrMoveFileSync(context.get(), src_url, dest_url, option, copy);
  content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                   base::BindOnce(std::move(callback), error));
}

void NativeMediaFileUtil::CopyInForeignFileOnTaskRunnerThread(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const base::FilePath& src_file_path,
    const storage::FileSystemURL& dest_url,
    StatusCallback callback) {
  DCHECK(IsOnTaskRunnerThread(context.get()));
  base::File::Error error =
      CopyInForeignFileSync(context.get(), src_file_path, dest_url);
  content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                   base::BindOnce(std::move(callback), error));
}

void NativeMediaFileUtil::DeleteFileOnTaskRunnerThread(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    StatusCallback callback) {
  DCHECK(IsOnTaskRunnerThread(context.get()));
  base::File::Error error = DeleteFileSync(context.get(), url);
  content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                   base::BindOnce(std::move(callback), error));
}

void NativeMediaFileUtil::DeleteDirectoryOnTaskRunnerThread(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    StatusCallback callback) {
  DCHECK(IsOnTaskRunnerThread(context.get()));
  base::File::Error error = DeleteDirectorySync(context.get(), url);
  content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                   base::BindOnce(std::move(callback), error));
}

void NativeMediaFileUtil::CreateSnapshotFileOnTaskRunnerThread(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    CreateSnapshotFileCallback callback) {
  DCHECK(IsOnTaskRunnerThread(context.get()));
  base::File::Info file_info;
  base::FilePath platform_path;
  scoped_refptr<storage::ShareableFileReference> file_ref;
  base::File::Error error = CreateSnapshotFileSync(
      context.get(), url, &file_info, &platform_path, &file_ref);
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(std::move(callback), error, file_info, platform_path,
                     file_ref));
}

base::File::Error NativeMediaFileUtil::CreateDirectorySync(
    storage::FileSystemOperationContext* context,
    const storage::FileSystemURL& url,
    bool exclusive,
    bool recursive) {
  base::FilePath file_path;
  base::File::Error error = GetLocalFilePath(context, url, &file_path);
  if (error != base::File::FILE_OK)
    return error;
  return storage::NativeFileUtil::CreateDirectory(
      file_path, exclusive, recursive);
}

base::File::Error NativeMediaFileUtil::CopyOrMoveFileSync(
    storage::FileSystemOperationContext* context,
    const storage::FileSystemURL& src_url,
    const storage::FileSystemURL& dest_url,
    CopyOrMoveOption option,
    bool copy) {
  DCHECK(IsOnTaskRunnerThread(context));
  base::FilePath src_file_path;
  base::File::Error error =
      GetFilteredLocalFilePathForExistingFileOrDirectory(
          context, src_url,
          base::File::FILE_ERROR_NOT_FOUND,
          &src_file_path);
  if (error != base::File::FILE_OK)
    return error;
  if (storage::NativeFileUtil::DirectoryExists(src_file_path))
    return base::File::FILE_ERROR_NOT_A_FILE;

  base::FilePath dest_file_path;
  error = GetLocalFilePath(context, dest_url, &dest_file_path);
  if (error != base::File::FILE_OK)
    return error;
  base::File::Info file_info;
  error = storage::NativeFileUtil::GetFileInfo(dest_file_path, &file_info);
  if (error != base::File::FILE_OK &&
      error != base::File::FILE_ERROR_NOT_FOUND) {
    return error;
  }
  if (error == base::File::FILE_OK && file_info.is_directory)
    return base::File::FILE_ERROR_INVALID_OPERATION;
  if (!media_path_filter_->Match(dest_file_path))
    return base::File::FILE_ERROR_SECURITY;

  return storage::NativeFileUtil::CopyOrMoveFile(
      src_file_path,
      dest_file_path,
      option,
      storage::NativeFileUtil::CopyOrMoveModeForDestination(dest_url, copy));
}

base::File::Error NativeMediaFileUtil::CopyInForeignFileSync(
    storage::FileSystemOperationContext* context,
    const base::FilePath& src_file_path,
    const storage::FileSystemURL& dest_url) {
  DCHECK(IsOnTaskRunnerThread(context));
  if (src_file_path.empty())
    return base::File::FILE_ERROR_INVALID_OPERATION;

  base::FilePath dest_file_path;
  base::File::Error error =
      GetFilteredLocalFilePath(context, dest_url, &dest_file_path);
  if (error != base::File::FILE_OK)
    return error;
  return storage::NativeFileUtil::CopyOrMoveFile(
      src_file_path,
      dest_file_path,
      storage::FileSystemOperation::OPTION_NONE,
      storage::NativeFileUtil::CopyOrMoveModeForDestination(dest_url,
                                                            true /* copy */));
}

base::File::Error NativeMediaFileUtil::GetFileInfoSync(
    storage::FileSystemOperationContext* context,
    const storage::FileSystemURL& url,
    base::File::Info* file_info,
    base::FilePath* platform_path) {
  DCHECK(context);
  DCHECK(IsOnTaskRunnerThread(context));
  DCHECK(file_info);

  base::FilePath file_path;
  base::File::Error error = GetLocalFilePath(context, url, &file_path);
  if (error != base::File::FILE_OK)
    return error;
  if (base::IsLink(file_path))
    return base::File::FILE_ERROR_NOT_FOUND;
  error = storage::NativeFileUtil::GetFileInfo(file_path, file_info);
  if (error != base::File::FILE_OK)
    return error;

  if (platform_path)
    *platform_path = file_path;
  if (file_info->is_directory ||
      media_path_filter_->Match(file_path)) {
    return base::File::FILE_OK;
  }
  return base::File::FILE_ERROR_NOT_FOUND;
}

base::File::Error NativeMediaFileUtil::GetLocalFilePath(
    storage::FileSystemOperationContext* context,
    const storage::FileSystemURL& url,
    base::FilePath* local_file_path) {
  DCHECK(local_file_path);
  DCHECK(url.is_valid());
  if (url.path().empty()) {
    // Root direcory case, which should not be accessed.
    return base::File::FILE_ERROR_ACCESS_DENIED;
  }
  *local_file_path = url.path();
  return base::File::FILE_OK;
}

base::File::Error NativeMediaFileUtil::ReadDirectorySync(
    storage::FileSystemOperationContext* context,
    const storage::FileSystemURL& url,
    EntryList* file_list) {
  DCHECK(IsOnTaskRunnerThread(context));
  DCHECK(file_list);
  DCHECK(file_list->empty());
  base::File::Info file_info;
  base::FilePath dir_path;
  base::File::Error error =
      GetFileInfoSync(context, url, &file_info, &dir_path);

  if (error != base::File::FILE_OK)
    return error;

  if (!file_info.is_directory)
    return base::File::FILE_ERROR_NOT_A_DIRECTORY;

  base::FileEnumerator file_enum(
      dir_path,
      false /* recursive */,
      base::FileEnumerator::FILES | base::FileEnumerator::DIRECTORIES);
  for (base::FilePath enum_path = file_enum.Next();
       !enum_path.empty();
       enum_path = file_enum.Next()) {
    // Skip symlinks.
    if (base::IsLink(enum_path))
      continue;

    base::FileEnumerator::FileInfo info = file_enum.GetInfo();

    // NativeMediaFileUtil skip criteria.
    if (MediaPathFilter::ShouldSkip(enum_path))
      continue;
    if (!info.IsDirectory() && !media_path_filter_->Match(enum_path))
      continue;

    file_list->emplace_back(enum_path.BaseName(),
                            info.IsDirectory()
                                ? filesystem::mojom::FsFileType::DIRECTORY
                                : filesystem::mojom::FsFileType::REGULAR_FILE);
  }

  return base::File::FILE_OK;
}

base::File::Error NativeMediaFileUtil::DeleteFileSync(
    storage::FileSystemOperationContext* context,
    const storage::FileSystemURL& url) {
  DCHECK(IsOnTaskRunnerThread(context));
  base::File::Info file_info;
  base::FilePath file_path;
  base::File::Error error =
      GetFileInfoSync(context, url, &file_info, &file_path);
  if (error != base::File::FILE_OK)
    return error;
  if (file_info.is_directory)
    return base::File::FILE_ERROR_NOT_A_FILE;
  return storage::NativeFileUtil::DeleteFile(file_path);
}

base::File::Error NativeMediaFileUtil::DeleteDirectorySync(
    storage::FileSystemOperationContext* context,
    const storage::FileSystemURL& url) {
  DCHECK(IsOnTaskRunnerThread(context));
  base::FilePath file_path;
  base::File::Error error = GetLocalFilePath(context, url, &file_path);
  if (error != base::File::FILE_OK)
    return error;
  return storage::NativeFileUtil::DeleteDirectory(file_path);
}

base::File::Error NativeMediaFileUtil::CreateSnapshotFileSync(
    storage::FileSystemOperationContext* context,
    const storage::FileSystemURL& url,
    base::File::Info* file_info,
    base::FilePath* platform_path,
    scoped_refptr<storage::ShareableFileReference>* file_ref) {
  DCHECK(IsOnTaskRunnerThread(context));
  base::File::Error error =
      GetFileInfoSync(context, url, file_info, platform_path);
  if (error == base::File::FILE_OK && file_info->is_directory)
    error = base::File::FILE_ERROR_NOT_A_FILE;
  if (error == base::File::FILE_OK)
    error = NativeMediaFileUtil::IsMediaFile(*platform_path);

  // We're just returning the local file information.
  *file_ref = scoped_refptr<storage::ShareableFileReference>();

  return error;
}

base::File::Error NativeMediaFileUtil::GetFilteredLocalFilePath(
    storage::FileSystemOperationContext* context,
    const storage::FileSystemURL& file_system_url,
    base::FilePath* local_file_path) {
  DCHECK(IsOnTaskRunnerThread(context));
  base::FilePath file_path;
  base::File::Error error =
      GetLocalFilePath(context, file_system_url, &file_path);
  if (error != base::File::FILE_OK)
    return error;
  if (!media_path_filter_->Match(file_path))
    return base::File::FILE_ERROR_SECURITY;

  *local_file_path = file_path;
  return base::File::FILE_OK;
}

base::File::Error
NativeMediaFileUtil::GetFilteredLocalFilePathForExistingFileOrDirectory(
    storage::FileSystemOperationContext* context,
    const storage::FileSystemURL& file_system_url,
    base::File::Error failure_error,
    base::FilePath* local_file_path) {
  DCHECK(IsOnTaskRunnerThread(context));
  base::FilePath file_path;
  base::File::Error error =
      GetLocalFilePath(context, file_system_url, &file_path);
  if (error != base::File::FILE_OK)
    return error;

  if (!base::PathExists(file_path))
    return failure_error;
  base::File::Info file_info;
  if (!base::GetFileInfo(file_path, &file_info))
    return base::File::FILE_ERROR_FAILED;

  if (!file_info.is_directory &&
      !media_path_filter_->Match(file_path)) {
    return failure_error;
  }

  *local_file_path = file_path;
  return base::File::FILE_OK;
}
