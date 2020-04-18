// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/chromeos/file_cache.h"

#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/xattr.h>

#include <memory>
#include <queue>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback_helpers.h"
#include "base/files/file.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/sys_info.h"
#include "build/build_config.h"
#include "components/drive/drive.pb.h"
#include "components/drive/drive_api_util.h"
#include "components/drive/file_system_core_util.h"
#include "components/drive/resource_metadata_storage.h"
#include "google_apis/drive/task_util.h"
#include "net/base/filename_util.h"
#include "net/base/mime_sniffer.h"
#include "net/base/mime_util.h"

namespace drive {
namespace internal {
namespace {

typedef std::pair<base::File::Info, ResourceEntry> CacheInfo;
typedef long FileAttributes;  // NOLINT(runtime/int)

// Returns ID extracted from the path.
std::string GetIdFromPath(const base::FilePath& path) {
  return util::UnescapeCacheFileName(path.BaseName().AsUTF8Unsafe());
}

base::FilePath GetPathForId(const base::FilePath& cache_directory,
                            const std::string& id) {
  return cache_directory.Append(
      base::FilePath::FromUTF8Unsafe(util::EscapeCacheFileName(id)));
}

// Returns if the filesystem backing |path| supports file attributes.
// This will return false if the filesystem is for example tmpfs, which is used
// for ephemeral mode.
bool IsFileAttributesSupported(const base::FilePath& path) {
  if (getxattr(path.value().c_str(), "user.foo", nullptr, 0) >= 0) {
    return true;
  }
  return errno != ENOTSUP;
}

// Sets extended file attribute as |name| |value| pair.
bool SetExtendedFileAttributes(const base::FilePath& path,
    const std::string& name, const std::string& value) {
  return setxattr(path.value().c_str(), name.c_str(), value.c_str(),
      value.size() + 1, 0) == 0;
}

// Changes attributes of the file with |flags|, e.g. FS_NODUMP_FL (cryptohome
// will remove Drive caches with this attribute).
// See linux/fs.h for available flags, and chattr(1) which does similar thing.
// Returns whether operation succeeded.
bool SetFileAttributes(const base::FilePath& path, FileAttributes flags) {
  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid()) {
    PLOG(ERROR) << "Failed to open file: " << path.value();
    return false;
  }
  if (ioctl(file.GetPlatformFile(), FS_IOC_SETFLAGS, &flags) < 0) {
    PLOG(ERROR) << "ioctl: " << path.value();
    return false;
  }
  return true;
}

// Gets file attributes similarly to lsattr(1). Returns flags or -1 on error.
// See linux/fs.h for the definition of the returned flags e.g. FS_NODUMP_FL.
FileAttributes GetFileAttributes(const base::FilePath& path) {
  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid()) {
    PLOG(ERROR) << "Failed to open file: " << path.value();
    return -1;
  }
  FileAttributes flags = 0;
  if (ioctl(file.GetPlatformFile(), FS_IOC_GETFLAGS, &flags) < 0) {
    PLOG(ERROR) << "ioctl: " << path.value();
    return -1;
  }
  return flags;
}

// Marks the cache file to be removable by cryptohome, or do nothing if
// underlying filesystem doesn't support file attributes, as tmpfs for ephemeral
// mode.
bool SetRemovable(const base::FilePath& path) {
  // For ephemeral mode.
  if (!IsFileAttributesSupported(path)) {
    return true;
  }
  FileAttributes flags = GetFileAttributes(path);
  if (flags < 0) return false;
  if ((flags & FS_NODUMP_FL) == FS_NODUMP_FL) return true;

  return SetFileAttributes(path, flags | FS_NODUMP_FL);
}

// Marks the cache file to be unremovable by cryptohome, or do nothing if
// underlying filesystem doesn't support file attributes, as tmpfs for ephemeral
// mode.
bool UnsetRemovable(const base::FilePath& path) {
  // For ephemeral mode.
  if (!IsFileAttributesSupported(path)) {
    return true;
  }
  FileAttributes flags = GetFileAttributes(path);
  if (flags < 0) return false;
  if ((flags & FS_NODUMP_FL) == 0) return true;

  return SetFileAttributes(path, flags & ~FS_NODUMP_FL);
}

// Marks |path| as drive cache dir, or do nothing if underlying filesystem
// doesn't support file attributes, as tmpfs for ephemeral mode. Returns if the
// operation succeeded.
bool MarkAsDriveCacheDir(const base::FilePath& path) {
  // For ephemeral mode.
  if (!IsFileAttributesSupported(path)) {
    return true;
  }
  return SetRemovable(path)
      && SetExtendedFileAttributes(path, FileCache::kGCacheFilesAttribute, "");
}

class CacheInfoLatestCompare {
 public:
  bool operator()(const CacheInfo& info_a, const CacheInfo& info_b) {
    return info_a.first.last_accessed < info_b.first.last_accessed;
  }
};

// Returns true if the cache file is present.
bool IsPresent(const ResourceEntry& entry) {
  return entry.has_file_specific_info() &&
         entry.file_specific_info().has_cache_state() &&
         entry.file_specific_info().cache_state().is_present();
}

const size_t kMaxNumOfEvictedCacheFiles = 30000;

}  // namespace

// static
const char FileCache::kGCacheFilesAttribute[] = "user.GCacheFiles";

FileCache::FileCache(ResourceMetadataStorage* storage,
                     const base::FilePath& cache_file_directory,
                     base::SequencedTaskRunner* blocking_task_runner,
                     FreeDiskSpaceGetterInterface* free_disk_space_getter)
    : cache_file_directory_(cache_file_directory),
      blocking_task_runner_(blocking_task_runner),
      storage_(storage),
      free_disk_space_getter_(free_disk_space_getter),
      max_num_of_evicted_cache_files_(kMaxNumOfEvictedCacheFiles),
      weak_ptr_factory_(this) {
  DCHECK(blocking_task_runner_.get());
}

FileCache::~FileCache() {
  // Must be on the sequenced worker pool, as |metadata_| must be deleted on
  // the sequenced worker pool.
  AssertOnSequencedWorkerPool();
}

void FileCache::SetMaxNumOfEvictedCacheFilesForTest(
    size_t max_num_of_evicted_cache_files) {
  max_num_of_evicted_cache_files_ = max_num_of_evicted_cache_files;
}

base::FilePath FileCache::GetCacheFilePath(const std::string& id) const {
  return GetPathForId(cache_file_directory_, id);
}

void FileCache::AssertOnSequencedWorkerPool() {
  DCHECK(blocking_task_runner_->RunsTasksInCurrentSequence());
}

bool FileCache::IsUnderFileCacheDirectory(const base::FilePath& path) const {
  return cache_file_directory_.IsParent(path);
}

bool FileCache::FreeDiskSpaceIfNeededFor(int64_t num_bytes) {
  AssertOnSequencedWorkerPool();

  // Do nothing and return if we have enough space.
  if (GetAvailableSpace() >= num_bytes)
    return true;

  // Otherwise, try to free up the disk space.
  DVLOG(1) << "Freeing up disk space for " << num_bytes;

  // Remove all files which have no corresponding cache entries.
  base::FileEnumerator enumerator(cache_file_directory_,
                                  false,  // not recursive
                                  base::FileEnumerator::FILES);
  ResourceEntry entry;
  for (base::FilePath current = enumerator.Next(); !current.empty();
       current = enumerator.Next()) {
    const std::string id = GetIdFromPath(current);
    const FileError error = storage_->GetEntry(id, &entry);

    if (error == FILE_ERROR_NOT_FOUND)
      base::DeleteFile(current, false /* recursive */);
    else if (error != FILE_ERROR_OK)
      return false;
  }

  // Check available space again. If we have enough space here, do nothing.
  const int64_t available_space = GetAvailableSpace();
  if (available_space >= num_bytes)
    return true;

  const int64_t requested_space = num_bytes - available_space;

  // Put all entries in priority queue where latest entry becomes top.
  std::priority_queue<CacheInfo, std::vector<CacheInfo>, CacheInfoLatestCompare>
      cache_info_queue;
  std::unique_ptr<ResourceMetadataStorage::Iterator> it =
      storage_->GetIterator();
  for (; !it->IsAtEnd(); it->Advance()) {
    if (IsEvictable(it->GetID(), it->GetValue())) {
      const ResourceEntry& entry = it->GetValue();

      const base::FilePath& cache_path = GetCacheFilePath(entry.local_id());
      base::File::Info info;
      // If it fails to get file info of |cache_path|, use default value as its
      // file info. i.e. the file becomes least recently used one.
      base::GetFileInfo(cache_path, &info);

      CacheInfo cache_info = std::make_pair(info, entry);

      if (cache_info_queue.size() < max_num_of_evicted_cache_files_) {
        cache_info_queue.push(cache_info);
      } else if (cache_info_queue.size() >= max_num_of_evicted_cache_files_ &&
                 cache_info.first.last_accessed <
                     cache_info_queue.top().first.last_accessed) {
        // Do not enqueue more than max_num_of_evicted_cache_files_ not to use
        // up memory with this queue.
        cache_info_queue.pop();
        cache_info_queue.push(cache_info);
      }
    }
  }
  if (it->HasError())
    return false;

  // Copy entries to the vector. This becomes last-accessed desc order.
  std::vector<CacheInfo> cache_info_list;
  while (!cache_info_queue.empty()) {
    cache_info_list.push_back(cache_info_queue.top());
    cache_info_queue.pop();
  }

  // Update DB and delete files with accessing to the vector in ascending order.
  int64_t evicted_cache_size = 0;
  auto iter = cache_info_list.rbegin();
  while (evicted_cache_size < requested_space &&
         iter != cache_info_list.rend()) {
    const CacheInfo& cache_info = *iter;

    // Update DB.
    ResourceEntry entry = cache_info.second;
    entry.mutable_file_specific_info()->clear_cache_state();
    storage_->PutEntry(entry);

    // Delete cache file.
    const base::FilePath& path = GetCacheFilePath(entry.local_id());

    if (base::DeleteFile(path, false /* recursive */))
      evicted_cache_size += cache_info.first.size;

    ++iter;
  }

  // Check the disk space again.
  return GetAvailableSpace() >= num_bytes;
}

int64_t FileCache::CalculateCacheSize() {
  AssertOnSequencedWorkerPool();

  int64_t total_cache_size = 0;
  int64_t cache_size = 0;

  std::unique_ptr<ResourceMetadataStorage::Iterator> it =
      storage_->GetIterator();
  for (; !it->IsAtEnd(); it->Advance()) {
    if (IsPresent(it->GetValue()) &&
        base::GetFileSize(GetCacheFilePath(it->GetID()), &cache_size)) {
      DCHECK_GE(cache_size, 0);
      total_cache_size += cache_size;
    }
  }

  if (it->HasError())
    return 0;

  return total_cache_size;
}

int64_t FileCache::CalculateEvictableCacheSize() {
  AssertOnSequencedWorkerPool();

  int64_t evictable_cache_size = 0;
  int64_t cache_size = 0;

  std::unique_ptr<ResourceMetadataStorage::Iterator> it =
      storage_->GetIterator();
  for (; !it->IsAtEnd(); it->Advance()) {
    if (IsEvictable(it->GetID(), it->GetValue()) &&
        base::GetFileSize(GetCacheFilePath(it->GetID()), &cache_size)) {
      DCHECK_GE(cache_size, 0);
      evictable_cache_size += cache_size;
    }
  }

  if (it->HasError())
    return 0;

  return evictable_cache_size;
}

FileError FileCache::GetFile(const std::string& id,
                             base::FilePath* cache_file_path) {
  AssertOnSequencedWorkerPool();
  DCHECK(cache_file_path);

  ResourceEntry entry;
  FileError error = storage_->GetEntry(id, &entry);
  if (error != FILE_ERROR_OK)
    return error;
  if (!entry.file_specific_info().cache_state().is_present())
    return FILE_ERROR_NOT_FOUND;

  *cache_file_path = GetCacheFilePath(id);
  return FILE_ERROR_OK;
}

FileError FileCache::Store(const std::string& id,
                           const std::string& md5,
                           const base::FilePath& source_path,
                           FileOperationType file_operation_type) {
  AssertOnSequencedWorkerPool();

  ResourceEntry entry;
  FileError error = storage_->GetEntry(id, &entry);
  if (error != FILE_ERROR_OK)
    return error;

  int64_t file_size = 0;
  if (file_operation_type == FILE_OPERATION_COPY) {
    if (!base::GetFileSize(source_path, &file_size)) {
      LOG(WARNING) << "Couldn't get file size for: " << source_path.value();
      return FILE_ERROR_FAILED;
    }
  }
  if (!FreeDiskSpaceIfNeededFor(file_size))
    return FILE_ERROR_NO_LOCAL_SPACE;

  // If file is mounted, return error.
  if (mounted_files_.count(id))
    return FILE_ERROR_IN_USE;

  base::FilePath dest_path = GetCacheFilePath(id);
  bool success = false;
  switch (file_operation_type) {
    case FILE_OPERATION_MOVE:
      success = base::Move(source_path, dest_path);
      break;
    case FILE_OPERATION_COPY:
      success = base::CopyFile(source_path, dest_path);
      break;
    default:
      NOTREACHED();
  }

  if (!success) {
    LOG(ERROR) << "Failed to store: "
               << "source_path = " << source_path.value() << ", "
               << "dest_path = " << dest_path.value() << ", "
               << "file_operation_type = " << file_operation_type;
    return FILE_ERROR_FAILED;
  }

  // Now that file operations have completed, update metadata.
  FileCacheEntry* cache_state =
      entry.mutable_file_specific_info()->mutable_cache_state();
  cache_state->set_md5(md5);
  cache_state->set_is_present(true);
  if (md5.empty())
    cache_state->set_is_dirty(true);

  if (!cache_state->is_pinned() && !cache_state->is_dirty()) {
    if (!SetRemovable(dest_path))
      return FILE_ERROR_FAILED;
  } else {
    if (!UnsetRemovable(dest_path))
      return FILE_ERROR_FAILED;
  }

  return storage_->PutEntry(entry);
}

FileError FileCache::Pin(const std::string& id) {
  AssertOnSequencedWorkerPool();

  ResourceEntry entry;
  FileError error = storage_->GetEntry(id, &entry);
  if (error != FILE_ERROR_OK)
    return error;

  entry.mutable_file_specific_info()->mutable_cache_state()->set_is_pinned(
      true);

  base::FilePath file_path = GetCacheFilePath(entry.local_id());
  // Cache file can be created later.
  if (entry.file_specific_info().cache_state().is_present()) {
    if (!UnsetRemovable(file_path))
      return FILE_ERROR_FAILED;
  }

  return storage_->PutEntry(entry);
}

FileError FileCache::Unpin(const std::string& id) {
  AssertOnSequencedWorkerPool();

  // Unpinning a file means its entry must exist in cache.
  ResourceEntry entry;
  FileError error = storage_->GetEntry(id, &entry);
  if (error != FILE_ERROR_OK)
    return error;

  // Now that file operations have completed, update metadata.
  if (entry.file_specific_info().cache_state().is_present()) {
    entry.mutable_file_specific_info()->mutable_cache_state()->set_is_pinned(
        false);
    if (!entry.file_specific_info().cache_state().is_dirty()) {
      if (!SetRemovable(GetCacheFilePath(entry.local_id())))
        return FILE_ERROR_FAILED;
    }
  } else {
    // Remove the existing entry if we are unpinning a non-present file.
    entry.mutable_file_specific_info()->clear_cache_state();
  }
  error = storage_->PutEntry(entry);
  if (error != FILE_ERROR_OK)
    return error;

  // Now it's a chance to free up space if needed.
  FreeDiskSpaceIfNeededFor(0);

  return FILE_ERROR_OK;
}

bool FileCache::IsMarkedAsMounted(const std::string& id) {
  AssertOnSequencedWorkerPool();
  return mounted_files_.count(id);
}

FileError FileCache::MarkAsMounted(const std::string& id,
                                   base::FilePath* cache_file_path) {
  AssertOnSequencedWorkerPool();
  DCHECK(cache_file_path);

  // Get cache entry associated with the id and md5
  ResourceEntry entry;
  FileError error = storage_->GetEntry(id, &entry);
  if (error != FILE_ERROR_OK)
    return error;
  if (!entry.file_specific_info().cache_state().is_present())
    return FILE_ERROR_NOT_FOUND;

  if (mounted_files_.count(id))
    return FILE_ERROR_INVALID_OPERATION;

  base::FilePath path = GetCacheFilePath(id);

#if defined(OS_CHROMEOS)
  // Ensure the file is readable to cros_disks. See crbug.com/236994.
  if (!base::SetPosixFilePermissions(
          path,
          base::FILE_PERMISSION_READ_BY_USER |
          base::FILE_PERMISSION_WRITE_BY_USER |
          base::FILE_PERMISSION_READ_BY_GROUP |
          base::FILE_PERMISSION_READ_BY_OTHERS))
    return FILE_ERROR_FAILED;
#endif

  mounted_files_.insert(id);

  *cache_file_path = path;
  return FILE_ERROR_OK;
}

FileError FileCache::OpenForWrite(
    const std::string& id,
    std::unique_ptr<base::ScopedClosureRunner>* file_closer) {
  AssertOnSequencedWorkerPool();

  // Marking a file dirty means its entry and actual file blob must exist in
  // cache.
  ResourceEntry entry;
  FileError error = storage_->GetEntry(id, &entry);
  if (error != FILE_ERROR_OK)
    return error;
  if (!entry.file_specific_info().cache_state().is_present()) {
    LOG(WARNING) << "Can't mark dirty a file that wasn't cached: " << id;
    return FILE_ERROR_NOT_FOUND;
  }

  entry.mutable_file_specific_info()->mutable_cache_state()->set_is_dirty(true);
  if (!UnsetRemovable(GetCacheFilePath(entry.local_id())))
    return FILE_ERROR_FAILED;

  entry.mutable_file_specific_info()->mutable_cache_state()->clear_md5();
  error = storage_->PutEntry(entry);
  if (error != FILE_ERROR_OK)
    return error;

  write_opened_files_[id]++;
  *file_closer = std::make_unique<base::ScopedClosureRunner>(
      base::Bind(&google_apis::RunTaskWithTaskRunner, blocking_task_runner_,
                 base::Bind(&FileCache::CloseForWrite,
                            weak_ptr_factory_.GetWeakPtr(), id)));
  return FILE_ERROR_OK;
}

bool FileCache::IsOpenedForWrite(const std::string& id) {
  AssertOnSequencedWorkerPool();
  return write_opened_files_.count(id) != 0;
}

FileError FileCache::UpdateMd5(const std::string& id) {
  AssertOnSequencedWorkerPool();

  if (IsOpenedForWrite(id))
    return FILE_ERROR_IN_USE;

  ResourceEntry entry;
  FileError error = storage_->GetEntry(id, &entry);
  if (error != FILE_ERROR_OK)
    return error;
  if (!entry.file_specific_info().cache_state().is_present())
    return FILE_ERROR_NOT_FOUND;

  const std::string& md5 =
      util::GetMd5Digest(GetCacheFilePath(id), &in_shutdown_);
  if (in_shutdown_.IsSet())
    return FILE_ERROR_ABORT;
  if (md5.empty())
    return FILE_ERROR_NOT_FOUND;

  entry.mutable_file_specific_info()->mutable_cache_state()->set_md5(md5);
  return storage_->PutEntry(entry);
}

FileError FileCache::ClearDirty(const std::string& id) {
  AssertOnSequencedWorkerPool();

  if (IsOpenedForWrite(id))
    return FILE_ERROR_IN_USE;

  // Clearing a dirty file means its entry and actual file blob must exist in
  // cache.
  ResourceEntry entry;
  FileError error = storage_->GetEntry(id, &entry);
  if (error != FILE_ERROR_OK)
    return error;
  if (!entry.file_specific_info().cache_state().is_present()) {
    LOG(WARNING) << "Can't clear dirty state of a file that wasn't cached: "
                 << id;
    return FILE_ERROR_NOT_FOUND;
  }

  // If a file is not dirty (it should have been marked dirty via OpenForWrite),
  // clearing its dirty state is an invalid operation.
  if (!entry.file_specific_info().cache_state().is_dirty()) {
    LOG(WARNING) << "Can't clear dirty state of a non-dirty file: " << id;
    return FILE_ERROR_INVALID_OPERATION;
  }

  entry.mutable_file_specific_info()->mutable_cache_state()->set_is_dirty(
      false);
  if (!entry.file_specific_info().cache_state().is_pinned()) {
    if (!SetRemovable(GetCacheFilePath(entry.local_id())))
      return FILE_ERROR_FAILED;
  }

  return storage_->PutEntry(entry);
}

FileError FileCache::Remove(const std::string& id) {
  AssertOnSequencedWorkerPool();

  ResourceEntry entry;

  // If entry doesn't exist, nothing to do.
  FileError error = storage_->GetEntry(id, &entry);
  if (error == FILE_ERROR_NOT_FOUND)
    return FILE_ERROR_OK;
  if (error != FILE_ERROR_OK)
    return error;
  if (!entry.file_specific_info().has_cache_state())
    return FILE_ERROR_OK;

  // Cannot delete a mounted file.
  if (mounted_files_.count(id))
    return FILE_ERROR_IN_USE;

  // Delete the file.
  base::FilePath path = GetCacheFilePath(id);
  if (!base::DeleteFile(path, false /* recursive */))
    return FILE_ERROR_FAILED;

  // Now that all file operations have completed, remove from metadata.
  entry.mutable_file_specific_info()->clear_cache_state();
  return storage_->PutEntry(entry);
}

bool FileCache::ClearAll() {
  AssertOnSequencedWorkerPool();

  // Remove files.
  base::FileEnumerator enumerator(cache_file_directory_,
                                  false,  // not recursive
                                  base::FileEnumerator::FILES);
  for (base::FilePath file = enumerator.Next(); !file.empty();
       file = enumerator.Next())
    base::DeleteFile(file, false /* recursive */);

  return true;
}

bool FileCache::Initialize() {
  AssertOnSequencedWorkerPool();

  // Older versions do not clear MD5 when marking entries dirty.
  // Clear MD5 of all dirty entries to deal with old data.
  std::unique_ptr<ResourceMetadataStorage::Iterator> it =
      storage_->GetIterator();
  for (; !it->IsAtEnd(); it->Advance()) {
    if (it->GetValue().file_specific_info().cache_state().is_dirty()) {
      ResourceEntry new_entry(it->GetValue());
      new_entry.mutable_file_specific_info()->mutable_cache_state()->
          clear_md5();
      if (storage_->PutEntry(new_entry) != FILE_ERROR_OK)
        return false;
    }
  }
  if (it->HasError())
    return false;

  if (!RenameCacheFilesToNewFormat())
    return false;

  // Run this every time to resolve inconsistency between metadata
  // and file attributes which possibly occurs on abrupt power failure.
  if (!FixMetadataAndFileAttributes()) {
    return false;
  }

  return true;
}

void FileCache::Destroy() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  in_shutdown_.Set();

  // Destroy myself on the blocking pool.
  // Note that base::DeletePointer<> cannot be used as the destructor of this
  // class is private.
  blocking_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&FileCache::DestroyOnBlockingPool, base::Unretained(this)));
}

void FileCache::DestroyOnBlockingPool() {
  AssertOnSequencedWorkerPool();
  delete this;
}

bool FileCache::RecoverFilesFromCacheDirectory(
    const base::FilePath& dest_directory,
    const ResourceMetadataStorage::RecoveredCacheInfoMap&
        recovered_cache_info) {
  int file_number = 1;

  base::FileEnumerator enumerator(cache_file_directory_,
                                  false,  // not recursive
                                  base::FileEnumerator::FILES);
  for (base::FilePath current = enumerator.Next(); !current.empty();
       current = enumerator.Next()) {
    const std::string& id = GetIdFromPath(current);
    ResourceEntry entry;
    FileError error = storage_->GetEntry(id, &entry);
    if (error != FILE_ERROR_OK && error != FILE_ERROR_NOT_FOUND)
      return false;
    if (error == FILE_ERROR_OK &&
        entry.file_specific_info().cache_state().is_present()) {
      // This file is managed by FileCache, no need to recover it.
      continue;
    }

    // If a cache entry which is non-dirty and has matching MD5 is found in
    // |recovered_cache_entries|, it means the current file is already uploaded
    // to the server. Just delete it instead of recovering it.
    ResourceMetadataStorage::RecoveredCacheInfoMap::const_iterator it =
        recovered_cache_info.find(id);
    if (it != recovered_cache_info.end()) {
      // Due to the DB corruption, cache info might be recovered from old
      // revision. Perform MD5 check even when is_dirty is false just in case.
      if (!it->second.is_dirty &&
          it->second.md5 == util::GetMd5Digest(current, &in_shutdown_)) {
        base::DeleteFile(current, false /* recursive */);
        continue;
      }
    }

    // Read file contents to sniff mime type.
    std::vector<char> content(net::kMaxBytesToSniff);
    const int read_result =
        base::ReadFile(current, &content[0], content.size());
    if (read_result < 0) {
      LOG(WARNING) << "Cannot read: " << current.value();
      return false;
    }
    if (read_result == 0)  // Skip empty files.
      continue;

    // Use recovered file name if available, otherwise decide file name with
    // sniffed mime type.
    base::FilePath dest_base_name(FILE_PATH_LITERAL("file"));
    std::string mime_type;
    if (it != recovered_cache_info.end() && !it->second.title.empty()) {
      // We can use a file name recovered from the trashed DB.
      dest_base_name = base::FilePath::FromUTF8Unsafe(it->second.title);
    } else if (net::SniffMimeType(
                   &content[0], read_result, net::FilePathToFileURL(current),
                   std::string(), net::ForceSniffFileUrlsForHtml::kDisabled,
                   &mime_type) ||
               net::SniffMimeTypeFromLocalData(&content[0], read_result,
                                               &mime_type)) {
      // Change base name for common mime types.
      if (net::MatchesMimeType("image/*", mime_type)) {
        dest_base_name = base::FilePath(FILE_PATH_LITERAL("image"));
      } else if (net::MatchesMimeType("video/*", mime_type)) {
        dest_base_name = base::FilePath(FILE_PATH_LITERAL("video"));
      } else if (net::MatchesMimeType("audio/*", mime_type)) {
        dest_base_name = base::FilePath(FILE_PATH_LITERAL("audio"));
      }

      // Estimate extension from mime type.
      std::vector<base::FilePath::StringType> extensions;
      base::FilePath::StringType extension;
      if (net::GetPreferredExtensionForMimeType(mime_type, &extension))
        extensions.push_back(extension);
      else
        net::GetExtensionsForMimeType(mime_type, &extensions);

      // Add extension if possible.
      if (!extensions.empty())
        dest_base_name = dest_base_name.AddExtension(extensions[0]);
    }

    // Add file number to the file name and move.
    const base::FilePath& dest_path = dest_directory.Append(dest_base_name)
        .InsertBeforeExtensionASCII(base::StringPrintf("%08d", file_number++));
    if (!base::CreateDirectory(dest_directory) ||
        !base::Move(current, dest_path)) {
      LOG(WARNING) << "Failed to move: " << current.value()
                   << " to " << dest_path.value();
      return false;
    }
  }
  UMA_HISTOGRAM_COUNTS("Drive.NumberOfCacheFilesRecoveredAfterDBCorruption",
                       file_number - 1);
  return true;
}

FileError FileCache::MarkAsUnmounted(const base::FilePath& file_path) {
  AssertOnSequencedWorkerPool();
  DCHECK(IsUnderFileCacheDirectory(file_path));

  std::string id = GetIdFromPath(file_path);

  // Get the entry associated with the id.
  ResourceEntry entry;
  FileError error = storage_->GetEntry(id, &entry);
  if (error != FILE_ERROR_OK)
    return error;

  std::set<std::string>::iterator it = mounted_files_.find(id);
  if (it == mounted_files_.end())
    return FILE_ERROR_INVALID_OPERATION;

  mounted_files_.erase(it);
  return FILE_ERROR_OK;
}

int64_t FileCache::GetAvailableSpace() {
  int64_t free_space = 0;
  if (free_disk_space_getter_)
    free_space = free_disk_space_getter_->AmountOfFreeDiskSpace();
  else
    free_space = base::SysInfo::AmountOfFreeDiskSpace(cache_file_directory_);

  // Subtract this as if this portion does not exist.
  free_space -= drive::internal::kMinFreeSpaceInBytes;
  return free_space;
}

bool FileCache::RenameCacheFilesToNewFormat() {
  base::FileEnumerator enumerator(cache_file_directory_,
                                  false,  // not recursive
                                  base::FileEnumerator::FILES);
  for (base::FilePath current = enumerator.Next(); !current.empty();
       current = enumerator.Next()) {
    base::FilePath new_path = current.RemoveExtension();
    if (!new_path.Extension().empty()) {
      // Delete files with multiple extensions.
      if (!base::DeleteFile(current, false /* recursive */))
        return false;
      continue;
    }
    const std::string& id = GetIdFromPath(new_path);
    new_path = GetCacheFilePath(util::CanonicalizeResourceId(id));
    if (new_path != current && !base::Move(current, new_path))
      return false;
  }
  return true;
}

bool FileCache::FixMetadataAndFileAttributes() {
  std::unique_ptr<ResourceMetadataStorage::Iterator> it =
      storage_->GetIterator();

  for (; !it->IsAtEnd(); it->Advance()) {
    ResourceEntry entry = it->GetValue();
    FileCacheEntry* file_cache_entry =
        entry.mutable_file_specific_info()->mutable_cache_state();

    const base::FilePath filepath = GetPathForId(cache_file_directory_,
        entry.local_id());

    if (base::PathExists(filepath)) {
      if (file_cache_entry->is_present()) {
        // Update file attribues for cryptohome.
        if (file_cache_entry->is_pinned() || file_cache_entry->is_dirty()) {
          if (!UnsetRemovable(filepath)) return false;
        } else {
          if (!SetRemovable(filepath)) return false;
        }
      } else {
        // Delete file if the file is present but metadata says not.
        // It happens only on abrupt shutdown.
        LOG(WARNING)
            << "File is present but metadata's state was inconsistent.";

        if (!base::DeleteFile(filepath, false /* recursive */))
          return false;
      }
    } else {
      // Update metatadata if there is no file but metadata says there is.
      // It happens when cryptohome removed the file.
      // We don't clear is_pinned here, so that file download is restarted on
      // the following scenario:
      //  1. The file was pinned but not present.
      //  2. Then the file was downloaded and became present.
      //  3. Unclean shutdown happens, metadata update was saved to the disk,
      //     but the file move was not.
      if (file_cache_entry->is_present()) {
        file_cache_entry->set_is_present(false);
        file_cache_entry->set_is_dirty(false);
        file_cache_entry->clear_md5();
        if (storage_->PutEntry(entry) != FILE_ERROR_OK)
          return false;
      }
    }
  }

  return MarkAsDriveCacheDir(cache_file_directory_);
}

void FileCache::CloseForWrite(const std::string& id) {
  AssertOnSequencedWorkerPool();

  std::map<std::string, int>::iterator it = write_opened_files_.find(id);
  if (it == write_opened_files_.end())
    return;

  DCHECK_LT(0, it->second);
  --it->second;
  if (it->second == 0)
    write_opened_files_.erase(it);

  // Update last modified date.
  ResourceEntry entry;
  FileError error = storage_->GetEntry(id, &entry);
  if (error != FILE_ERROR_OK) {
    LOG(ERROR) << "Failed to get entry: " << id << ", "
               << FileErrorToString(error);
    return;
  }
  int64_t now = base::Time::Now().ToInternalValue();
  entry.mutable_file_info()->set_last_modified(now);
  entry.set_last_modified_by_me(now);
  error = storage_->PutEntry(entry);
  if (error != FILE_ERROR_OK) {
    LOG(ERROR) << "Failed to put entry: " << id << ", "
               << FileErrorToString(error);
  }
}

bool FileCache::IsEvictable(const std::string& id, const ResourceEntry& entry) {
  return IsPresent(entry) &&
         !entry.file_specific_info().cache_state().is_pinned() &&
         !entry.file_specific_info().cache_state().is_dirty() &&
         !mounted_files_.count(id);
}

}  // namespace internal
}  // namespace drive
