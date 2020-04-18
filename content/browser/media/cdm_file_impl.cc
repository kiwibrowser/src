// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/cdm_file_impl.h"

#include <set>
#include <utility>

#include "base/callback.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/task_scheduler/post_task.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_operation_context.h"
#include "storage/browser/fileapi/file_system_url.h"
#include "storage/browser/quota/quota_manager.h"
#include "storage/common/fileapi/file_system_types.h"

namespace content {

namespace {

// The CDM interface has a restriction that file names can not begin with _,
// so use it to prefix temporary files.
const char kTemporaryFilePrefix[] = "_";

std::string GetTempFileName(const std::string& file_name) {
  DCHECK(!base::StartsWith(file_name, kTemporaryFilePrefix,
                           base::CompareCase::SENSITIVE));
  return kTemporaryFilePrefix + file_name;
}

// The file system is different for each CDM and each origin. So track files
// in use based on (file system ID, origin, file name).
struct FileLockKey {
  FileLockKey(const std::string& file_system_id,
              const url::Origin& origin,
              const std::string& file_name)
      : file_system_id(file_system_id), origin(origin), file_name(file_name) {}
  ~FileLockKey() = default;

  // Allow use as a key in std::set.
  bool operator<(const FileLockKey& other) const {
    return std::tie(file_system_id, origin, file_name) <
           std::tie(other.file_system_id, other.origin, other.file_name);
  }

  std::string file_system_id;
  url::Origin origin;
  std::string file_name;
};

// File map shared by all CdmFileImpl objects to prevent read/write race.
// A lock must be acquired before opening a file to ensure that the file is not
// currently in use. The lock must be held until the file is closed.
class FileLockMap {
 public:
  FileLockMap() = default;
  ~FileLockMap() = default;

  // Acquire a lock on the file represented by |key|. Returns true if |key|
  // is not currently in use, false otherwise.
  bool AcquireFileLock(const FileLockKey& key) {
    DVLOG(3) << __func__ << " file: " << key.file_name;
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

    // Add a new entry. If |key| already has an entry, insert() tells so
    // with the second piece of the returned value and does not modify
    // the original.
    return file_lock_map_.insert(key).second;
  }

  // Tests whether a lock is held on |key| or not. Returns true if |key|
  // is currently locked, false otherwise.
  bool IsFileLockHeld(const FileLockKey& key) {
    DVLOG(3) << __func__ << " file: " << key.file_name;
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

    // Lock is held if there is an entry for |key|.
    return file_lock_map_.count(key) > 0;
  }

  // Release the lock held on the file represented by |key|. If
  // |on_close_callback| has been set, run it before releasing the lock.
  void ReleaseFileLock(const FileLockKey& key) {
    DVLOG(3) << __func__ << " file: " << key.file_name;
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

    auto entry = file_lock_map_.find(key);
    if (entry == file_lock_map_.end()) {
      NOTREACHED() << "Unable to release lock on file " << key.file_name;
      return;
    }

    file_lock_map_.erase(entry);
  }

 private:
  // Note that this map is never deleted. As entries are removed when a file
  // is closed, it should never get too large.
  std::set<FileLockKey> file_lock_map_;

  THREAD_CHECKER(thread_checker_);
  DISALLOW_COPY_AND_ASSIGN(FileLockMap);
};

// The FileLockMap is a global lock map shared by all CdmFileImpl instances.
FileLockMap* GetFileLockMap() {
  static auto* file_lock_map = new FileLockMap();
  return file_lock_map;
}

}  // namespace

CdmFileImpl::CdmFileImpl(
    const std::string& file_name,
    const url::Origin& origin,
    const std::string& file_system_id,
    const std::string& file_system_root_uri,
    scoped_refptr<storage::FileSystemContext> file_system_context)
    : file_name_(file_name),
      temp_file_name_(GetTempFileName(file_name_)),
      origin_(origin),
      file_system_id_(file_system_id),
      file_system_root_uri_(file_system_root_uri),
      file_system_context_(file_system_context),
      weak_factory_(this) {
  DVLOG(3) << __func__ << " " << file_name_;
}

CdmFileImpl::~CdmFileImpl() {
  DVLOG(3) << __func__ << " " << file_name_;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // If a file open was started but hasn't completed by now, run the callback
  // and report an error.
  if (pending_open_callback_) {
    std::move(pending_open_callback_)
        .Run(base::File(base::File::FILE_ERROR_ABORT));
  }

  if (lock_state_ == LockState::kFileAndTempFileLocked) {
    // Temporary file is open, so close and release it.
    if (temporary_file_on_close_callback_)
      std::move(temporary_file_on_close_callback_).Run();
    ReleaseFileLock(temp_file_name_);
  }
  if (lock_state_ != LockState::kNone) {
    // Original file is open, so close and release it.
    if (on_close_callback_)
      std::move(on_close_callback_).Run();
    ReleaseFileLock(file_name_);
  }
}

void CdmFileImpl::Initialize(OpenFileCallback callback) {
  DVLOG(3) << __func__ << " file: " << file_name_;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(LockState::kNone, lock_state_);
  DCHECK(!pending_open_callback_);

  // Grab the lock on |file_name_|. The lock will be held until this object is
  // destructed.
  if (!AcquireFileLock(file_name_)) {
    DVLOG(3) << "File " << file_name_ << " is already in use.";
    std::move(callback).Run(base::File(base::File::FILE_ERROR_IN_USE));
    return;
  }

  // We have the lock on |file_name_|. Now open the file for reading. Since
  // we don't know if this file exists or not, provide FLAG_OPEN_ALWAYS to
  // create the file if it doesn't exist.
  lock_state_ = LockState::kFileLocked;
  pending_open_callback_ = std::move(callback);
  OpenFile(file_name_, base::File::FLAG_OPEN_ALWAYS | base::File::FLAG_READ,
           base::BindOnce(&CdmFileImpl::OnFileOpenedForReading,
                          weak_factory_.GetWeakPtr()));
}

void CdmFileImpl::OpenFile(const std::string& file_name,
                           uint32_t file_flags,
                           CreateOrOpenCallback callback) {
  DVLOG(3) << __func__ << " file: " << file_name << ", flags: " << file_flags;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_NE(LockState::kNone, lock_state_);
  DCHECK(IsFileLockHeld(file_name));
  DCHECK(pending_open_callback_);

  storage::FileSystemURL file_url = CreateFileSystemURL(file_name);
  storage::AsyncFileUtil* file_util = file_system_context_->GetAsyncFileUtil(
      storage::kFileSystemTypePluginPrivate);
  auto operation_context =
      std::make_unique<storage::FileSystemOperationContext>(
          file_system_context_.get());
  operation_context->set_allowed_bytes_growth(storage::QuotaManager::kNoLimit);
  DVLOG(3) << "Opening " << file_url.DebugString();

  file_util->CreateOrOpen(std::move(operation_context), file_url, file_flags,
                          std::move(callback));
}

void CdmFileImpl::OnFileOpenedForReading(base::File file,
                                         base::OnceClosure on_close_callback) {
  DVLOG(3) << __func__ << " file: " << file_name_;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(LockState::kFileLocked, lock_state_);
  DCHECK(pending_open_callback_);

  if (!file.IsValid()) {
    // File is invalid. Note that the lock on |file_name_| is kept until this
    // object is destructed.
    DLOG(WARNING) << "Unable to open file " << file_name_ << ", error: "
                  << base::File::ErrorToString(file.error_details());
    std::move(pending_open_callback_).Run(std::move(file));
    return;
  }

  // When the file is closed, |on_close_callback| will be run.
  on_close_callback_ = std::move(on_close_callback);
  std::move(pending_open_callback_).Run(std::move(file));
}

void CdmFileImpl::OpenFileForWriting(OpenFileForWritingCallback callback) {
  DVLOG(3) << __func__ << " " << file_name_;

  // Fail if this is called out of order. We must have opened the original
  // file, and there should be no call in progress.
  if (lock_state_ != LockState::kFileLocked || pending_open_callback_) {
    std::move(callback).Run(
        base::File(base::File::FILE_ERROR_INVALID_OPERATION));
    return;
  }

  // Grab a lock on the temporary file. The lock will be held until this
  // new file is renamed in CommitWrite() (or this object is
  // destructed).
  if (!AcquireFileLock(temp_file_name_)) {
    DVLOG(3) << "File " << temp_file_name_ << " is already in use.";
    std::move(callback).Run(base::File(base::File::FILE_ERROR_IN_USE));
    return;
  }

  // We now have locks on both |file_name_| and |temp_file_name_|. Open the
  // temporary file for writing. Specifying FLAG_CREATE_ALWAYS which will
  // overwrite any existing file.
  lock_state_ = LockState::kFileAndTempFileLocked;
  pending_open_callback_ = std::move(callback);
  OpenFile(temp_file_name_,
           base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE,
           base::BindOnce(&CdmFileImpl::OnTempFileOpenedForWriting,
                          weak_factory_.GetWeakPtr()));
}

void CdmFileImpl::OnTempFileOpenedForWriting(
    base::File file,
    base::OnceClosure on_close_callback) {
  DVLOG(3) << __func__ << " file: " << file_name_;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(LockState::kFileAndTempFileLocked, lock_state_);
  DCHECK(pending_open_callback_);

  if (!file.IsValid()) {
    DLOG(WARNING) << "Unable to open file " << temp_file_name_ << ", error: "
                  << base::File::ErrorToString(file.error_details());
    lock_state_ = LockState::kFileLocked;
    ReleaseFileLock(temp_file_name_);
    std::move(pending_open_callback_).Run(std::move(file));
    return;
  }

  temporary_file_on_close_callback_ = std::move(on_close_callback);
  std::move(pending_open_callback_).Run(std::move(file));
}

void CdmFileImpl::CommitWrite(CommitWriteCallback callback) {
  DVLOG(3) << __func__ << " " << temp_file_name_ << " to " << file_name_;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(LockState::kFileAndTempFileLocked, lock_state_);
  DCHECK(IsFileLockHeld(file_name_));
  DCHECK(IsFileLockHeld(temp_file_name_));

  // TODO(jrummell): Verify that the written file does not exceed the file
  // size limit of 32MB. If it does simply delete the written file and fail.

  // Fail if this is called out of order. We must have opened both the original
  // and the temporary file, and there should be no call in progress.
  if (lock_state_ != LockState::kFileAndTempFileLocked ||
      pending_open_callback_) {
    std::move(callback).Run(
        base::File(base::File::FILE_ERROR_INVALID_OPERATION));
    return;
  }

  if (on_close_callback_)
    std::move(on_close_callback_).Run();
  if (temporary_file_on_close_callback_)
    std::move(temporary_file_on_close_callback_).Run();

  // OpenFile() will be called after the file is renamed, so save |callback|.
  pending_open_callback_ = std::move(callback);

  storage::FileSystemURL src_file_url = CreateFileSystemURL(temp_file_name_);
  storage::FileSystemURL dest_file_url = CreateFileSystemURL(file_name_);
  storage::AsyncFileUtil* file_util = file_system_context_->GetAsyncFileUtil(
      storage::kFileSystemTypePluginPrivate);
  auto operation_context =
      std::make_unique<storage::FileSystemOperationContext>(
          file_system_context_.get());
  DVLOG(3) << "Renaming " << src_file_url.DebugString() << " to "
           << dest_file_url.DebugString();
  file_util->MoveFileLocal(
      std::move(operation_context), src_file_url, dest_file_url,
      storage::FileSystemOperation::OPTION_NONE,
      base::BindOnce(&CdmFileImpl::OnFileRenamed, weak_factory_.GetWeakPtr()));
}

void CdmFileImpl::OnFileRenamed(base::File::Error move_result) {
  DVLOG(3) << __func__;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(LockState::kFileAndTempFileLocked, lock_state_);
  DCHECK(pending_open_callback_);

  // Temporary file has been renamed, so we can release the lock on it.
  ReleaseFileLock(temp_file_name_);
  lock_state_ = LockState::kFileLocked;

  // Was the rename successful?
  if (move_result != base::File::FILE_OK) {
    std::move(pending_open_callback_).Run(base::File(move_result));
    return;
  }

  // Reopen the original file for reading. Specifying FLAG_OPEN as the file
  // has to exist or something's wrong.
  OpenFile(file_name_, base::File::FLAG_OPEN | base::File::FLAG_READ,
           base::BindOnce(&CdmFileImpl::OnFileOpenedForReading,
                          weak_factory_.GetWeakPtr()));
}

storage::FileSystemURL CdmFileImpl::CreateFileSystemURL(
    const std::string& file_name) {
  return file_system_context_->CrackURL(
      GURL(file_system_root_uri_ + file_name));
}

bool CdmFileImpl::AcquireFileLock(const std::string& file_name) {
  FileLockKey file_lock_key(file_system_id_, origin_, file_name);
  return GetFileLockMap()->AcquireFileLock(file_lock_key);
}

bool CdmFileImpl::IsFileLockHeld(const std::string& file_name) {
  FileLockKey file_lock_key(file_system_id_, origin_, file_name);
  return GetFileLockMap()->IsFileLockHeld(file_lock_key);
}

void CdmFileImpl::ReleaseFileLock(const std::string& file_name) {
  FileLockKey file_lock_key(file_system_id_, origin_, file_name);
  GetFileLockMap()->ReleaseFileLock(file_lock_key);
}

}  // namespace content
