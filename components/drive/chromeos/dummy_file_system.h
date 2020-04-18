// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_DUMMY_FILE_SYSTEM_H_
#define COMPONENTS_DRIVE_CHROMEOS_DUMMY_FILE_SYSTEM_H_

#include <stdint.h>

#include "components/drive/chromeos/file_system_interface.h"

namespace drive {

// Dummy implementation of FileSystemInterface. All functions do nothing.
class DummyFileSystem : public FileSystemInterface {
 public:
  ~DummyFileSystem() override = default;
  void AddObserver(FileSystemObserver* observer) override {}
  void RemoveObserver(FileSystemObserver* observer) override {}
  void CheckForUpdates() override {}
  void TransferFileFromLocalToRemote(
      const base::FilePath& local_src_file_path,
      const base::FilePath& remote_dest_file_path,
      const FileOperationCallback& callback) override {}
  void OpenFile(const base::FilePath& file_path,
                OpenMode open_mode,
                const std::string& mime_type,
                const OpenFileCallback& callback) override {}
  void Copy(const base::FilePath& src_file_path,
            const base::FilePath& dest_file_path,
            bool preserve_last_modified,
            const FileOperationCallback& callback) override {}
  void Move(const base::FilePath& src_file_path,
            const base::FilePath& dest_file_path,
            const FileOperationCallback& callback) override {}
  void Remove(const base::FilePath& file_path,
              bool is_recursive,
              const FileOperationCallback& callback) override {}
  void CreateDirectory(const base::FilePath& directory_path,
                       bool is_exclusive,
                       bool is_recursive,
                       const FileOperationCallback& callback) override {}
  void CreateFile(const base::FilePath& file_path,
                  bool is_exclusive,
                  const std::string& mime_type,
                  const FileOperationCallback& callback) override {}
  void TouchFile(const base::FilePath& file_path,
                 const base::Time& last_access_time,
                 const base::Time& last_modified_time,
                 const FileOperationCallback& callback) override {}
  void TruncateFile(const base::FilePath& file_path,
                    int64_t length,
                    const FileOperationCallback& callback) override {}
  void Pin(const base::FilePath& file_path,
           const FileOperationCallback& callback) override {}
  void Unpin(const base::FilePath& file_path,
             const FileOperationCallback& callback) override {}
  void GetFile(const base::FilePath& file_path,
               const GetFileCallback& callback) override {}
  void GetFileForSaving(const base::FilePath& file_path,
                        const GetFileCallback& callback) override {}
  base::Closure GetFileContent(
      const base::FilePath& file_path,
      const GetFileContentInitializedCallback& initialized_callback,
      const google_apis::GetContentCallback& get_content_callback,
      const FileOperationCallback& completion_callback) override;
  void GetResourceEntry(const base::FilePath& file_path,
                        const GetResourceEntryCallback& callback) override {}
  void ReadDirectory(
      const base::FilePath& file_path,
      const ReadDirectoryEntriesCallback& entries_callback,
      const FileOperationCallback& completion_callback) override {}
  void Search(const std::string& search_query,
              const GURL& next_link,
              const SearchCallback& callback) override {}
  void SearchMetadata(const std::string& query,
                      int options,
                      int at_most_num_matches,
                      MetadataSearchOrder order,
                      const SearchMetadataCallback& callback) override {}
  void SearchByHashes(const std::set<std::string>& hashes,
                      const SearchByHashesCallback& callback) override {}
  void GetAvailableSpace(const GetAvailableSpaceCallback& callback) override {}
  void GetShareUrl(const base::FilePath& file_path,
                   const GURL& embed_origin,
                   const GetShareUrlCallback& callback) override {}
  void GetMetadata(const GetFilesystemMetadataCallback& callback) override {}
  void MarkCacheFileAsMounted(const base::FilePath& drive_file_path,
                              const MarkMountedCallback& callback) override {}
  void MarkCacheFileAsUnmounted(
      const base::FilePath& cache_file_path,
      const FileOperationCallback& callback) override {}
  void IsCacheFileMarkedAsMounted(const base::FilePath& drive_file_path,
                                  const IsMountedCallback& callback) override {}
  void AddPermission(const base::FilePath& drive_file_path,
                     const std::string& email,
                     google_apis::drive::PermissionRole role,
                     const FileOperationCallback& callback) override {}
  void SetProperty(const base::FilePath& drive_file_path,
                   google_apis::drive::Property::Visibility visibility,
                   const std::string& key,
                   const std::string& value,
                   const FileOperationCallback& callback) override {}
  void Reset(const FileOperationCallback& callback) override {}
  void GetPathFromResourceId(const std::string& resource_id,
                             const GetFilePathCallback& callback) override {}
  void FreeDiskSpaceIfNeededFor(
      int64_t num_bytes,
      const FreeDiskSpaceCallback& callback) override {}
  void CalculateCacheSize(const CacheSizeCallback& callback) override {}
  void CalculateEvictableCacheSize(
      const CacheSizeCallback& callback) override {}
};

}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_DUMMY_FILE_SYSTEM_H_
