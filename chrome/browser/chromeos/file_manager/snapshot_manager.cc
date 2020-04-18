// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/file_manager/snapshot_manager.h"

#include "base/bind.h"
#include "base/containers/circular_deque.h"
#include "base/sys_info.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/chromeos/file_manager/app_id.h"
#include "chrome/browser/chromeos/file_manager/fileapi_util.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "google_apis/drive/task_util.h"
#include "storage/browser/blob/shareable_file_reference.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "third_party/cros_system_api/constants/cryptohome.h"

namespace file_manager {
namespace {

typedef base::Callback<void(int64_t)> GetNecessaryFreeSpaceCallback;

// Part of ComputeSpaceNeedToBeFreed.
int64_t ComputeSpaceNeedToBeFreedAfterGetMetadataAsync(
    const base::FilePath& path,
    int64_t snapshot_size) {
  int64_t free_size = base::SysInfo::AmountOfFreeDiskSpace(path);
  if (free_size < 0)
    return -1;

  // We need to keep cryptohome::kMinFreeSpaceInBytes free space even after
  // |snapshot_size| is occupied.
  free_size -= snapshot_size + cryptohome::kMinFreeSpaceInBytes;
  return (free_size < 0 ? -free_size : 0);
}

// Part of ComputeSpaceNeedToBeFreed.
void ComputeSpaceNeedToBeFreedAfterGetMetadata(
    const base::FilePath& path,
    const GetNecessaryFreeSpaceCallback& callback,
    base::File::Error result,
    const base::File::Info& file_info) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (result != base::File::FILE_OK) {
    callback.Run(-1);
    return;
  }

  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_BLOCKING},
      base::Bind(&ComputeSpaceNeedToBeFreedAfterGetMetadataAsync, path,
                 file_info.size),
      callback);
}

// Part of ComputeSpaceNeedToBeFreed.
void GetMetadataOnIOThread(const base::FilePath& path,
                           scoped_refptr<storage::FileSystemContext> context,
                           const storage::FileSystemURL& url,
                           const GetNecessaryFreeSpaceCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  context->operation_runner()->GetMetadata(
      url, storage::FileSystemOperation::GET_METADATA_FIELD_SIZE,
      base::Bind(&ComputeSpaceNeedToBeFreedAfterGetMetadata, path, callback));
}

// Computes the size of space that need to be __additionally__ made available
// in the |profile|'s data directory for taking the snapshot of |url|.
// Returns 0 if no additional space is required, or -1 in the case of an error.
void ComputeSpaceNeedToBeFreed(
    Profile* profile,
    scoped_refptr<storage::FileSystemContext> context,
    const storage::FileSystemURL& url,
    const GetNecessaryFreeSpaceCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&GetMetadataOnIOThread, profile->GetPath(), context, url,
                     google_apis::CreateRelayCallback(callback)));
}

// Part of CreateManagedSnapshot. Runs CreateSnapshotFile method of fileapi.
void CreateSnapshotFileOnIOThread(
    scoped_refptr<storage::FileSystemContext> context,
    const storage::FileSystemURL& url,
    const storage::FileSystemOperation::SnapshotFileCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  context->operation_runner()->CreateSnapshotFile(url, callback);
}

// Utility for destructing the bound |file_refs| on IO thread. This is meant
// to be used together with base::Bind. After this function finishes, the
// Bind callback should destruct the bound argument.
void FreeReferenceOnIOThread(
    const base::circular_deque<SnapshotManager::FileReferenceWithSizeInfo>&
        file_refs) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
}

}  // namespace

SnapshotManager::FileReferenceWithSizeInfo::FileReferenceWithSizeInfo(
    scoped_refptr<storage::ShareableFileReference> ref,
    int64_t size)
    : file_ref(ref), file_size(size) {}

SnapshotManager::FileReferenceWithSizeInfo::FileReferenceWithSizeInfo(
    const FileReferenceWithSizeInfo& other) = default;

SnapshotManager::FileReferenceWithSizeInfo::~FileReferenceWithSizeInfo() =
    default;

SnapshotManager::SnapshotManager(Profile* profile)
    : profile_(profile), weak_ptr_factory_(this) {
}

SnapshotManager::~SnapshotManager() {
  if (!file_refs_.empty()) {
    bool posted = content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&FreeReferenceOnIOThread, file_refs_));
    DCHECK(posted);
  }
}

void SnapshotManager::CreateManagedSnapshot(
    const base::FilePath& absolute_file_path,
    const LocalPathCallback& callback) {
  scoped_refptr<storage::FileSystemContext> context(
      util::GetFileSystemContextForExtensionId(profile_, kFileManagerAppId));
  DCHECK(context.get());

  GURL url;
  if (!util::ConvertAbsoluteFilePathToFileSystemUrl(
          profile_, absolute_file_path, kFileManagerAppId, &url)) {
    callback.Run(base::FilePath());
    return;
  }
  storage::FileSystemURL filesystem_url = context->CrackURL(url);

  ComputeSpaceNeedToBeFreed(profile_, context, filesystem_url,
      base::Bind(&SnapshotManager::CreateManagedSnapshotAfterSpaceComputed,
                 weak_ptr_factory_.GetWeakPtr(),
                 filesystem_url,
                 callback));
}

void SnapshotManager::CreateManagedSnapshotAfterSpaceComputed(
    const storage::FileSystemURL& filesystem_url,
    const LocalPathCallback& callback,
    int64_t needed_space) {
  scoped_refptr<storage::FileSystemContext> context(
      util::GetFileSystemContextForExtensionId(profile_, kFileManagerAppId));
  DCHECK(context.get());

  if (needed_space < 0) {
    callback.Run(base::FilePath());
    return;
  }

  // Free up to the required size.
  base::circular_deque<FileReferenceWithSizeInfo> to_free;
  while (needed_space > 0 && !file_refs_.empty()) {
    needed_space -= file_refs_.front().file_size;
    to_free.push_back(file_refs_.front());
    file_refs_.pop_front();
  }
  if (!to_free.empty()) {
    bool posted = content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&FreeReferenceOnIOThread, to_free));
    DCHECK(posted);
  }

  // If we still could not achieve the space requirement, abort with failure.
  if (needed_space > 0) {
    callback.Run(base::FilePath());
    return;
  }

  // Start creating the snapshot.
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&CreateSnapshotFileOnIOThread, context, filesystem_url,
                     google_apis::CreateRelayCallback(base::Bind(
                         &SnapshotManager::OnCreateSnapshotFile,
                         weak_ptr_factory_.GetWeakPtr(), callback))));
}

void SnapshotManager::OnCreateSnapshotFile(
    const LocalPathCallback& callback,
    base::File::Error result,
    const base::File::Info& file_info,
    const base::FilePath& platform_path,
    scoped_refptr<storage::ShareableFileReference> file_ref) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (result != base::File::FILE_OK) {
    callback.Run(base::FilePath());
    return;
  }

  file_refs_.push_back(
      FileReferenceWithSizeInfo(std::move(file_ref), file_info.size));
  callback.Run(platform_path);
}

}  // namespace file_manager
