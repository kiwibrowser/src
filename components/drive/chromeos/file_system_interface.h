// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_INTERFACE_H_
#define COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_INTERFACE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "components/drive/chromeos/resource_metadata.h"
#include "components/drive/drive.pb.h"
#include "components/drive/file_system_metadata.h"
#include "google_apis/drive/base_requests.h"
#include "google_apis/drive/drive_api_requests.h"

namespace drive {

class FileSystemObserver;

// Information about search result returned by Search Async callback.
// This is data needed to create a file system entry that will be used by file
// browser.
struct SearchResultInfo {
  SearchResultInfo(const base::FilePath& path, bool is_directory)
      : path(path), is_directory(is_directory) {}

  base::FilePath path;
  bool is_directory;
};

// File path and its MD5 hash obtained from drive API.
struct HashAndFilePath {
  std::string hash;
  base::FilePath path;
};

// Specifies the order in which SearchMetadata() results are sorted.
enum class MetadataSearchOrder {
  // Most recently accessed file comes first.
  LAST_ACCESSED = 0,

  // Most recently modified file comes first.
  LAST_MODIFIED = 1,
};

// Struct to represent a search result for SearchMetadata().
struct MetadataSearchResult {
  MetadataSearchResult(const base::FilePath& path,
                       bool is_directory,
                       const std::string& highlighted_base_name,
                       const std::string& md5);
  MetadataSearchResult(const MetadataSearchResult& other);

  // The two members are used to create FileEntry object.
  base::FilePath path;
  bool is_directory;

  // The base name to be displayed in the UI. The parts matched the search
  // query are highlighted with <b> tag. Meta characters are escaped like &lt;
  //
  // Why HTML? we could instead provide matched ranges using pairs of
  // integers, but this is fragile as we'll eventually converting strings
  // from UTF-8 (StringValue in base/values.h uses std::string) to UTF-16
  // when sending strings from C++ to JavaScript.
  //
  // Why <b> instead of <strong>? Because <b> is shorter.
  std::string highlighted_base_name;

  // MD5 hash of the file.
  std::string md5;
};

typedef std::vector<MetadataSearchResult> MetadataSearchResultVector;

// Used to get a resource entry from the file system.
// If |error| is not FILE_ERROR_OK, |entry_info| is set to NULL.
typedef base::Callback<void(FileError error,
                            std::unique_ptr<ResourceEntry> entry)>
    GetResourceEntryCallback;

// Used to get files from the file system.
typedef base::Callback<void(FileError error,
                            const base::FilePath& file_path,
                            std::unique_ptr<ResourceEntry> entry)>
    GetFileCallback;

// Used to get file content from the file system.
// If the file content is available in local cache, |local_file| is filled with
// the path to the cache file. If the file content starts to be downloaded from
// the server, |local_file| is empty.
typedef base::Callback<void(FileError error,
                            const base::FilePath& local_file,
                            std::unique_ptr<ResourceEntry> entry)>
    GetFileContentInitializedCallback;

// Used to get list of entries under a directory.
typedef base::Callback<void(std::unique_ptr<ResourceEntryVector> entries)>
    ReadDirectoryEntriesCallback;

// Used to get drive content search results.
// If |error| is not FILE_ERROR_OK, |result_paths| is empty.
typedef base::Callback<void(
    FileError error,
    const GURL& next_link,
    std::unique_ptr<std::vector<SearchResultInfo>> result_paths)>
    SearchCallback;

// Callback for SearchMetadata(). On success, |error| is FILE_ERROR_OK, and
// |result| contains the search result.
typedef base::Callback<void(FileError error,
                            std::unique_ptr<MetadataSearchResultVector> result)>
    SearchMetadataCallback;

// Callback for SearchByHashesCallback. On success, vector contains hash and
// corresponding files. The vector can include multiple entries for one hash.
typedef base::Callback<void(FileError, const std::vector<HashAndFilePath>&)>
    SearchByHashesCallback;

// Used to open files from the file system. |file_path| is the path on the local
// file system for the opened file.
// If |close_callback| is not null, it must be called when the
// modification to the cache is done. Otherwise, Drive file system does not
// pick up the file for uploading.
// |close_callback| must not be called more than once.
typedef base::Callback<void(FileError error,
                            const base::FilePath& file_path,
                            const base::Closure& close_callback)>
    OpenFileCallback;

// Used to get available space for the account from Drive.
typedef base::Callback<void(FileError error,
                            int64_t bytes_total,
                            int64_t bytes_used)> GetAvailableSpaceCallback;

// Used to get the url to the sharing dialog.
typedef base::Callback<void(FileError error,
                            const GURL& share_url)> GetShareUrlCallback;

// Used to get filesystem metadata.
typedef base::Callback<void(const FileSystemMetadata&)>
    GetFilesystemMetadataCallback;

// Used to mark cached files mounted.
typedef base::Callback<void(FileError error,
                            const base::FilePath& file_path)>
    MarkMountedCallback;

// Used to check if a cached file is mounted.
typedef base::Callback<void(FileError error, bool is_mounted)>
    IsMountedCallback;

// Used to get file path.
typedef base::Callback<void(FileError error, const base::FilePath& file_path)>
    GetFilePathCallback;

// Used to free space.
typedef base::Callback<void(bool)> FreeDiskSpaceCallback;

// Used for returning result of calculated cache size.
typedef base::Callback<void(int64_t)> CacheSizeCallback;

// The mode of opening a file.
enum OpenMode {
  // Open the file if exists. If not, failed.
  OPEN_FILE,

  // Create a new file if not exists, and then open it. If exists, failed.
  CREATE_FILE,

  // Open the file if exists. If not, create a new file and then open it.
  OPEN_OR_CREATE_FILE,
};

// Option enum to control eligible entries for SearchMetadata().
// SEARCH_METADATA_ALL is the default to investigate all the entries.
// SEARCH_METADATA_EXCLUDE_HOSTED_DOCUMENTS excludes the hosted documents.
// SEARCH_METADATA_EXCLUDE_DIRECTORIES excludes the directories from the result.
// SEARCH_METADATA_SHARED_WITH_ME targets only "shared-with-me" entries.
// SEARCH_METADATA_OFFLINE targets only "offline" entries. This option can not
// be used with other options.
enum SearchMetadataOptions {
  SEARCH_METADATA_ALL = 0,
  SEARCH_METADATA_EXCLUDE_HOSTED_DOCUMENTS = 1,
  SEARCH_METADATA_EXCLUDE_DIRECTORIES = 1 << 1,
  SEARCH_METADATA_SHARED_WITH_ME = 1 << 2,
  SEARCH_METADATA_OFFLINE = 1 << 3,
};

// Drive file system abstraction layer.
// The interface is defined to make FileSystem mockable.
class FileSystemInterface {
 public:
  virtual ~FileSystemInterface() {}

  // Adds and removes the observer.
  virtual void AddObserver(FileSystemObserver* observer) = 0;
  virtual void RemoveObserver(FileSystemObserver* observer) = 0;

  // Checks for updates on the server.
  virtual void CheckForUpdates() = 0;

  // Initiates transfer of |local_src_file_path| to |remote_dest_file_path|.
  // |local_src_file_path| must be a file from the local file system.
  // |remote_dest_file_path| is the virtual destination path within Drive file
  // system.
  //
  // |callback| must not be null.
  virtual void TransferFileFromLocalToRemote(
      const base::FilePath& local_src_file_path,
      const base::FilePath& remote_dest_file_path,
      const FileOperationCallback& callback) = 0;

  // Retrieves a file at the virtual path |file_path| on the Drive file system
  // onto the cache, and mark it dirty. The local path to the cache file is
  // returned to |callback|. After opening the file, both read and write
  // on the file can be done with normal local file operations.
  // If |mime_type| is set and the file is newly created, the mime type is
  // set to the specified value. If |mime_type| is empty, it is guessed from
  // |file_path|.
  //
  // |callback| must not be null.
  virtual void OpenFile(const base::FilePath& file_path,
                        OpenMode open_mode,
                        const std::string& mime_type,
                        const OpenFileCallback& callback) = 0;

  // Copies |src_file_path| to |dest_file_path| on the file system.
  // |src_file_path| can be a hosted document (see limitations below).
  // |dest_file_path| is expected to be of the same type of |src_file_path|
  // (i.e. if |src_file_path| is a file, |dest_file_path| will be created as
  // a file).
  // If |preserve_last_modified| is set to true, the last modified time will be
  // preserved. This feature is only supported on Drive API v2 protocol because
  // GData WAPI doesn't support updating modification time.
  //
  // This method also has the following assumptions/limitations that may be
  // relaxed or addressed later:
  // - |src_file_path| cannot be a regular file (i.e. non-hosted document)
  //   or a directory.
  // - |dest_file_path| must not exist.
  // - The parent of |dest_file_path| must already exist.
  //
  // The file entries represented by |src_file_path| and the parent directory
  // of |dest_file_path| need to be present in the in-memory representation
  // of the file system.
  //
  // |callback| must not be null.
  virtual void Copy(const base::FilePath& src_file_path,
                    const base::FilePath& dest_file_path,
                    bool preserve_last_modified,
                    const FileOperationCallback& callback) = 0;

  // Moves |src_file_path| to |dest_file_path| on the file system.
  // |src_file_path| can be a file (regular or hosted document) or a directory.
  // |dest_file_path| is expected to be of the same type of |src_file_path|
  // (i.e. if |src_file_path| is a file, |dest_file_path| will be created as
  // a file).
  //
  // This method also has the following assumptions/limitations that may be
  // relaxed or addressed later:
  // - |dest_file_path| must not exist.
  // - The parent of |dest_file_path| must already exist.
  //
  // The file entries represented by |src_file_path| and the parent directory
  // of |dest_file_path| need to be present in the in-memory representation
  // of the file system.
  //
  // |callback| must not be null.
  virtual void Move(const base::FilePath& src_file_path,
                    const base::FilePath& dest_file_path,
                    const FileOperationCallback& callback) = 0;

  // Removes |file_path| from the file system.  If |is_recursive| is set and
  // |file_path| represents a directory, we will also delete all of its
  // contained children elements. The file entry represented by |file_path|
  // needs to be present in in-memory representation of the file system that
  // in order to be removed.
  //
  // |callback| must not be null.
  virtual void Remove(const base::FilePath& file_path,
                      bool is_recursive,
                      const FileOperationCallback& callback) = 0;

  // Creates new directory under |directory_path|. If |is_exclusive| is true,
  // an error is raised in case a directory is already present at the
  // |directory_path|. If |is_recursive| is true, the call creates parent
  // directories as needed just like mkdir -p does.
  //
  // |callback| must not be null.
  virtual void CreateDirectory(const base::FilePath& directory_path,
                               bool is_exclusive,
                               bool is_recursive,
                               const FileOperationCallback& callback) = 0;

  // Creates a file at |file_path|. If the flag |is_exclusive| is true, an
  // error is raised when a file already exists at the path. It is
  // an error if a directory or a hosted document is already present at the
  // path, or the parent directory of the path is not present yet.
  // If |mime_type| is set and the file is newly created, the mime type is
  // set to the specified value. If |mime_type| is empty, it is guessed from
  // |file_path|.
  //
  // |callback| must not be null.
  virtual void CreateFile(const base::FilePath& file_path,
                          bool is_exclusive,
                          const std::string& mime_type,
                          const FileOperationCallback& callback) = 0;

  // Touches the file at |file_path| by updating the timestamp to
  // |last_access_time| and |last_modified_time|.
  // Upon completion, invokes |callback|.
  // Note that, differently from unix touch command, this doesn't create a file
  // if the target file doesn't exist.
  //
  // |last_access_time|, |last_modified_time| and |callback| must not be null.
  virtual void TouchFile(const base::FilePath& file_path,
                         const base::Time& last_access_time,
                         const base::Time& last_modified_time,
                         const FileOperationCallback& callback) = 0;

  // Truncates the file content at |file_path| to the |length|.
  //
  // |callback| must not be null.
  virtual void TruncateFile(const base::FilePath& file_path,
                            int64_t length,
                            const FileOperationCallback& callback) = 0;

  // Pins a file at |file_path|.
  //
  // |callback| must not be null.
  virtual void Pin(const base::FilePath& file_path,
                   const FileOperationCallback& callback) = 0;

  // Unpins a file at |file_path|.
  //
  // |callback| must not be null.
  virtual void Unpin(const base::FilePath& file_path,
                     const FileOperationCallback& callback) = 0;

  // Makes sure that |file_path| in the file system is available in the local
  // cache. If the file is not cached, the file will be downloaded. The entry
  // needs to be present in the file system.
  //
  // Returns the cache path and entry info to |callback|. It must not be null.
  virtual void GetFile(const base::FilePath& file_path,
                       const GetFileCallback& callback) = 0;

  // Makes sure that |file_path| in the file system is available in the local
  // cache, and mark it as dirty. The next modification to the cache file is
  // watched and is automatically uploaded to the server. If the entry is not
  // present in the file system, it is created.
  //
  // Returns the cache path and entry info to |callback|. It must not be null.
  virtual void GetFileForSaving(const base::FilePath& file_path,
                                const GetFileCallback& callback) = 0;

  // Gets a file by the given |file_path| and returns a closure to cancel the
  // task.
  // Calls |initialized_callback| when either:
  //   1) The cached file (or JSON file for hosted file) is found, or
  //   2) Starting to download the file from drive server.
  // In case of 2), the given FilePath is empty, and |get_content_callback| is
  // called repeatedly with downloaded content following the
  // |initialized_callback| invocation.
  // |completion_callback| is invoked if an error is found, or the operation
  // is successfully done.
  // |initialized_callback|, |get_content_callback| and |completion_callback|
  // must not be null.
  virtual base::Closure GetFileContent(
      const base::FilePath& file_path,
      const GetFileContentInitializedCallback& initialized_callback,
      const google_apis::GetContentCallback& get_content_callback,
      const FileOperationCallback& completion_callback) = 0;

  // Finds an entry (a file or a directory) by |file_path|. This call will also
  // retrieve and refresh file system content from server and disk cache.
  //
  // |callback| must not be null.
  virtual void GetResourceEntry(const base::FilePath& file_path,
                                const GetResourceEntryCallback& callback) = 0;

  // Finds and reads a directory by |file_path|. This call will also retrieve
  // and refresh file system content from server and disk cache.
  // |entries_callback| can be a null callback when not interested in entries.
  //
  // |completion_callback| must not be null.
  virtual void ReadDirectory(
      const base::FilePath& file_path,
      const ReadDirectoryEntriesCallback& entries_callback,
      const FileOperationCallback& completion_callback) = 0;

  // Does server side content search for |search_query|.
  // If |next_link| is set, this is the search result url that will be
  // fetched. Search results will be returned as a list of results'
  // |SearchResultInfo| structs, which contains file's path and is_directory
  // flag.
  //
  // |callback| must not be null.
  virtual void Search(const std::string& search_query,
                      const GURL& next_link,
                      const SearchCallback& callback) = 0;

  // Searches the local resource metadata, and returns the entries
  // |at_most_num_matches| that contain |query| in their base names. Search is
  // done in a case-insensitive fashion. The eligible entries are selected based
  // on the given |options|, which is a bit-wise OR of SearchMetadataOptions.
  // SEARCH_METADATA_EXCLUDE_HOSTED_DOCUMENTS will be automatically added based
  // on the preference. |callback| must not be null. Must be called on UI
  // thread. Empty |query| matches any base name. i.e. returns everything.
  virtual void SearchMetadata(const std::string& query,
                              int options,
                              int at_most_num_matches,
                              MetadataSearchOrder order,
                              const SearchMetadataCallback& callback) = 0;

  // Searches the local resource metadata, and returns the entries that have the
  // given |hashes|. The list of resource entries are passed to |callback|. The
  // item of the list can be null if the corresponding file is not found.
  // |callback| must not be null.
  virtual void SearchByHashes(const std::set<std::string>& hashes,
                              const SearchByHashesCallback& callback) = 0;

  // Fetches the user's Account Metadata to find out current quota information
  // and returns it to the callback.
  virtual void GetAvailableSpace(const GetAvailableSpaceCallback& callback) = 0;

  // Fetches the url to the sharing dialog to be embedded in |embed_origin|,
  // for the specified file or directory. |callback| must not be null.
  virtual void GetShareUrl(
      const base::FilePath& file_path,
      const GURL& embed_origin,
      const GetShareUrlCallback& callback) = 0;

  // Returns miscellaneous metadata of the file system like the largest
  // timestamp. Used in chrome:drive-internals. |callback| must not be null.
  virtual void GetMetadata(
      const GetFilesystemMetadataCallback& callback) = 0;

  // Marks the cached file as mounted, and runs |callback| upon completion.
  // If succeeded, the cached file path will be passed to the |callback|.
  // |callback| must not be null.
  virtual void MarkCacheFileAsMounted(const base::FilePath& drive_file_path,
                                      const MarkMountedCallback& callback) = 0;

  // Checks if the cached file is marked as mounted, and passes the result to
  // |callback| upon completion. If the file was not found in the cache, the
  // result is false. |callback| must not be null.
  virtual void IsCacheFileMarkedAsMounted(
      const base::FilePath& drive_file_path,
      const IsMountedCallback& callback) = 0;

  // Marks the cached file as unmounted, and runs |callback| upon completion.
  // Note that this method expects that the |cached_file_path| is the path
  // returned by MarkCacheFileAsMounted().
  // |callback| must not be null.
  virtual void MarkCacheFileAsUnmounted(
      const base::FilePath& cache_file_path,
      const FileOperationCallback& callback) = 0;

  // Adds permission as |role| to |email| for the entry at |drive_file_path|.
  // |callback| must not be null.
  virtual void AddPermission(const base::FilePath& drive_file_path,
                             const std::string& email,
                             google_apis::drive::PermissionRole role,
                             const FileOperationCallback& callback) = 0;

  // Sets the |key| property on the file or directory at |drive_file_path| with
  // the specified |visibility|. If already exists, then it will be overwritten.
  virtual void SetProperty(const base::FilePath& drive_file_path,
                           google_apis::drive::Property::Visibility visibility,
                           const std::string& key,
                           const std::string& value,
                           const FileOperationCallback& callback) = 0;

  // Resets local data.
  virtual void Reset(const FileOperationCallback& callback) = 0;

  // Finds a path of an entry (a file or a directory) by |resource_id|.
  virtual void GetPathFromResourceId(const std::string& resource_id,
                                     const GetFilePathCallback& callback) = 0;

  // Free drive caches if needed to secure given available spaces. |callback|
  // takes whether given bytes are available or not.
  virtual void FreeDiskSpaceIfNeededFor(
      int64_t num_bytes,
      const FreeDiskSpaceCallback& callback) = 0;

  // Calculates total cache size.
  // |callback| must not be null.
  virtual void CalculateCacheSize(const CacheSizeCallback& callback) = 0;

  // Calculates evictable cache size.
  // |callback| must not be null.
  virtual void CalculateEvictableCacheSize(
      const CacheSizeCallback& callback) = 0;
};

}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_FILE_SYSTEM_INTERFACE_H_
