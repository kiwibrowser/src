// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_FILE_CACHE_H_
#define COMPONENTS_DRIVE_CHROMEOS_FILE_CACHE_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <set>
#include <string>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/cancellation_flag.h"
#include "base/threading/thread_checker.h"
#include "build/build_config.h"
#include "components/drive/file_errors.h"
#include "components/drive/resource_metadata_storage.h"
#if defined(OS_CHROMEOS)
#include "third_party/cros_system_api/constants/cryptohome.h"
#endif

namespace base {
class ScopedClosureRunner;
class SequencedTaskRunner;
}  // namespace base

namespace drive {

namespace internal {

#if defined(OS_CHROMEOS)
const int64_t kMinFreeSpaceInBytes = cryptohome::kMinFreeSpaceInBytes;
#else
const int64_t kMinFreeSpaceInBytes = 512ull * 1024ull * 1024ull;  // 512MB
#endif

// Interface class used for getting the free disk space. Tests can inject an
// implementation that reports fake free disk space.
class FreeDiskSpaceGetterInterface {
 public:
  virtual ~FreeDiskSpaceGetterInterface() = default;
  virtual int64_t AmountOfFreeDiskSpace() = 0;
};

// FileCache is used to maintain cache states of FileSystem.
//
// All non-static public member functions, unless mentioned otherwise (see
// GetCacheFilePath() for example), should be run with |blocking_task_runner|.
class FileCache {
 public:
  // The file extended attribute assigned to Drive cache directory.
  static const char kGCacheFilesAttribute[];

  // Enum defining type of file operation e.g. copy or move, etc.
  enum FileOperationType {
    FILE_OPERATION_MOVE = 0,
    FILE_OPERATION_COPY,
  };

  // |cache_file_directory| stores cached files.
  //
  // |blocking_task_runner| indicates the blocking worker pool for cache
  // operations. All operations on this FileCache must be run on this runner.
  // Must not be null.
  //
  // |free_disk_space_getter| is used to inject a custom free disk space
  // getter for testing. NULL must be passed for production code.
  //
  // Must be called on the UI thread.
  FileCache(ResourceMetadataStorage* storage,
            const base::FilePath& cache_file_directory,
            base::SequencedTaskRunner* blocking_task_runner,
            FreeDiskSpaceGetterInterface* free_disk_space_getter);

  // Sets maximum number of evicted cache files for test.
  void SetMaxNumOfEvictedCacheFilesForTest(
      size_t max_num_of_evicted_cache_files);

  // Returns true if the given path is under drive cache directory, i.e.
  // <user_profile_dir>/GCache/v1
  //
  // Can be called on any thread.
  bool IsUnderFileCacheDirectory(const base::FilePath& path) const;

  // Frees up disk space to store a file with |num_bytes| size content, while
  // keeping drive::internal::kMinFreeSpaceInBytes bytes on the disk, if needed.
  // Returns true if we successfully manage to have enough space, otherwise
  // false.
  bool FreeDiskSpaceIfNeededFor(int64_t num_bytes);

  // Calculates and returns cache size. In error case, this returns 0.
  int64_t CalculateCacheSize();

  // Calculates and returns evictable cache size. In error case, this returns 0.
  int64_t CalculateEvictableCacheSize();

  // Checks if file corresponding to |id| exists in cache, and returns
  // FILE_ERROR_OK with |cache_file_path| storing the path to the file.
  // |cache_file_path| must not be null.
  FileError GetFile(const std::string& id, base::FilePath* cache_file_path);

  // Stores |source_path| as a cache of the remote content of the file
  // with |id| and |md5|.
  // Pass an empty string as MD5 to mark the entry as dirty.
  FileError Store(const std::string& id,
                  const std::string& md5,
                  const base::FilePath& source_path,
                  FileOperationType file_operation_type);

  // Pins the specified entry.
  FileError Pin(const std::string& id);

  // Unpins the specified entry.
  FileError Unpin(const std::string& id);

  // Sets the state of the cache entry corresponding to |id| as mounted.
  FileError MarkAsMounted(const std::string& id,
                          base::FilePath* cache_file_path);

  // Returns if a file corresponding to |id| is marked as mounted.
  bool IsMarkedAsMounted(const std::string& id);

  // Sets the state of the cache entry corresponding to file_path as unmounted.
  FileError MarkAsUnmounted(const base::FilePath& file_path);

  // Opens the cache file corresponding to |id| for write. |file_closer| should
  // be kept alive until writing finishes.
  // This method must be called before writing to cache files.
  FileError OpenForWrite(
      const std::string& id,
      std::unique_ptr<base::ScopedClosureRunner>* file_closer);

  // Returns true if the cache file corresponding to |id| is write-opened.
  bool IsOpenedForWrite(const std::string& id);

  // Calculates MD5 of the cache file and updates the stored value.
  FileError UpdateMd5(const std::string& id);

  // Clears dirty state of the specified entry.
  FileError ClearDirty(const std::string& id);

  // Removes the specified cache entry and delete cache files if available.
  FileError Remove(const std::string& id);

  // Removes all the files in the cache directory.
  bool ClearAll();

  // Initializes the cache. Returns true on success.
  bool Initialize();

  // Destroys this cache. This function posts a task to the blocking task
  // runner to safely delete the object.
  // Must be called on the UI thread.
  void Destroy();

  // Moves files in the cache directory which are not managed by FileCache to
  // |dest_directory|.
  // |recovered_cache_info| should contain cache info recovered from the trashed
  // metadata DB. It is used to ignore non-dirty files.
  bool RecoverFilesFromCacheDirectory(
      const base::FilePath& dest_directory,
      const ResourceMetadataStorage::RecoveredCacheInfoMap&
          recovered_cache_info);

 private:
  friend class FileCacheTest;

  ~FileCache();

  // Returns absolute path of the file if it were cached or to be cached.
  //
  // Can be called on any thread.
  base::FilePath GetCacheFilePath(const std::string& id) const;

  // Checks whether the current thread is on the right sequenced worker pool
  // with the right sequence ID. If not, DCHECK will fail.
  void AssertOnSequencedWorkerPool();

  // Destroys the cache on the blocking pool.
  void DestroyOnBlockingPool();

  // Returns available space, while keeping
  // drive::internal::kMinFreeSpaceInBytes bytes on the disk.
  int64_t GetAvailableSpace();

  // Renames cache files from old "prefix:id.md5" format to the new format.
  // TODO(hashimoto): Remove this method at some point.
  bool RenameCacheFilesToNewFormat();

  // Adds appropriate file attributes to the Drive cache directory and files in
  // it for crbug.com/533750. Returns true on success.
  // This also resolves inconsistency between cache files and metadata which can
  // be produced when cryptohome removed cache files or on abrupt shutdown.
  bool FixMetadataAndFileAttributes();

  // This method must be called after writing to a cache file.
  // Used to implement OpenForWrite().
  void CloseForWrite(const std::string& id);

  // Returns true if the cache entry can be evicted.
  bool IsEvictable(const std::string& id, const ResourceEntry& entry);

  const base::FilePath cache_file_directory_;

  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;

  base::CancellationFlag in_shutdown_;

  ResourceMetadataStorage* storage_;

  FreeDiskSpaceGetterInterface* free_disk_space_getter_;  // Not owned.

  // Maximum number of cache files which can be evicted by a single call of
  // FreeDiskSpaceIfNeededFor. That method takes O(n) memory space, we need to
  // set this value not to use up memory.
  size_t max_num_of_evicted_cache_files_;

  // IDs of files being write-opened.
  std::map<std::string, int> write_opened_files_;

  // IDs of files marked mounted.
  std::set<std::string> mounted_files_;

  THREAD_CHECKER(thread_checker_);

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  // This object should be accessed only on |blocking_task_runner_|.
  base::WeakPtrFactory<FileCache> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(FileCache);
};

}  // namespace internal
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_FILE_CACHE_H_
