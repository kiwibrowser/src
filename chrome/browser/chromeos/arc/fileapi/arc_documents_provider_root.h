// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_ARC_DOCUMENTS_PROVIDER_ROOT_H_
#define CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_ARC_DOCUMENTS_PROVIDER_ROOT_H_

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_file_system_operation_runner.h"
#include "components/arc/common/file_system.mojom.h"
#include "storage/browser/fileapi/async_file_util.h"
#include "storage/browser/fileapi/watcher_manager.h"

class GURL;

namespace arc {

// Represents a file system root in Android Documents Provider.
//
// All methods must be called on the UI thread.
// If this object is deleted while there are in-flight operations, callbacks
// for those operations will be never called.
class ArcDocumentsProviderRoot : public ArcFileSystemOperationRunner::Observer {
 public:
  struct ThinFileInfo {
    base::FilePath::StringType name;
    std::string document_id;
    bool is_directory;
    base::Time last_modified;
  };

  // TODO(crbug.com/755451): Use OnceCallback/RepeatingCallback.
  using GetFileInfoCallback = storage::AsyncFileUtil::GetFileInfoCallback;
  using ReadDirectoryCallback =
      base::OnceCallback<void(base::File::Error error,
                              std::vector<ThinFileInfo> files)>;
  using ChangeType = storage::WatcherManager::ChangeType;
  using WatcherCallback = base::Callback<void(ChangeType type)>;
  using StatusCallback = base::Callback<void(base::File::Error error)>;
  using ResolveToContentUrlCallback =
      base::Callback<void(const GURL& content_url)>;

  ArcDocumentsProviderRoot(ArcFileSystemOperationRunner* runner,
                           const std::string& authority,
                           const std::string& root_document_id);
  ~ArcDocumentsProviderRoot() override;

  // Queries information of a file just like AsyncFileUtil.GetFileInfo().
  void GetFileInfo(const base::FilePath& path, GetFileInfoCallback callback);

  // Queries a list of files under a directory just like
  // AsyncFileUtil.ReadDirectory().
  void ReadDirectory(const base::FilePath& path,
                     ReadDirectoryCallback callback);

  // Installs a document watcher.
  //
  // It is not allowed to install multiple watchers at the same file path;
  // if attempted, duplicated requests will fail.
  //
  // Currently, watchers can be installed only on directories, and only
  // directory content changes are notified. The result of installing a watcher
  // to a non-directory in unspecified.
  //
  // NOTES ABOUT CORRECTNESS AND CONSISTENCY:
  //
  // Document watchers are not always correct and they may miss some updates or
  // even notify incorrect update events for several reasons, such as:
  //
  //   - Directory moves: Currently a watcher will misbehave if the watched
  //     directory is moved to another location. This is acceptable for now
  //     since we whitelist MediaDocumentsProvider only.
  //   - Duplicated file name handling in this class: The same reason as
  //     directory moves. This may happen even with MediaDocumentsProvider,
  //     but the chance will not be very high.
  //   - File system operation races: For example, an watcher can be installed
  //     to a non-directory in a race condition.
  //   - Broken DocumentsProviders: For example, we get no notification if a
  //     document provider does not call setNotificationUri().
  //
  // However, consistency of installed watchers is guaranteed. That is, after
  // a watcher is installed on a file path X, an attempt to uninstall a watcher
  // at X will always succeed.
  //
  // Unfortunately it is too difficult (or maybe theoretically impossible) to
  // implement a perfect Android document watcher which never misses document
  // updates. So the current implementation gives up correctness, but instead
  // focuses on following two goals:
  //
  //   1. Keep the implementation simple, rather than trying hard to catch
  //      race conditions or minor cases. Even if we return wrong results, the
  //      worst consequence is just that users do not see the latest contents
  //      until they refresh UI.
  //
  //   2. Keep consistency of installed watchers so that the caller can avoid
  //      dangling watchers.
  void AddWatcher(const base::FilePath& path,
                  const WatcherCallback& watcher_callback,
                  const StatusCallback& callback);

  // Uninstalls a document watcher.
  // See the documentation of AddWatcher() above.
  void RemoveWatcher(const base::FilePath& path,
                     const StatusCallback& callback);

  // Resolves a file path into a content:// URL pointing to the file
  // on DocumentsProvider. Returns URL that can be passed to
  // ArcContentFileSystemFileSystemReader to read the content.
  // On errors, an invalid GURL is returned.
  void ResolveToContentUrl(const base::FilePath& path,
                           const ResolveToContentUrlCallback& callback);

  // Instructs to make directory caches expire "soon" after callbacks are
  // called, that is, when the message loop gets idle.
  void SetDirectoryCacheExpireSoonForTesting();

  // ArcFileSystemOperationRunner::Observer overrides:
  void OnWatchersCleared() override;

 private:
  struct WatcherData;
  struct DirectoryCache;

  static const int64_t kInvalidWatcherId;
  static const uint64_t kInvalidWatcherRequestId;
  static const WatcherData kInvalidWatcherData;

  // Mapping from a file name to a Document.
  using NameToDocumentMap =
      std::map<base::FilePath::StringType, mojom::DocumentPtr>;

  // TODO(nya): Use OnceCallback.
  using ResolveToDocumentIdCallback =
      base::Callback<void(const std::string& document_id)>;
  using ReadDirectoryInternalCallback =
      base::Callback<void(base::File::Error error,
                          const NameToDocumentMap& mapping)>;

  void GetFileInfoWithParentDocumentId(GetFileInfoCallback callback,
                                       const base::FilePath& basename,
                                       const std::string& parent_document_id);
  void GetFileInfoWithNameToDocumentMap(GetFileInfoCallback callback,
                                        const base::FilePath& basename,
                                        base::File::Error error,
                                        const NameToDocumentMap& mapping);

  void ReadDirectoryWithDocumentId(ReadDirectoryCallback callback,
                                   const std::string& document_id);
  void ReadDirectoryWithNameToDocumentMap(ReadDirectoryCallback callback,
                                          base::File::Error error,
                                          const NameToDocumentMap& mapping);

  void AddWatcherWithDocumentId(const base::FilePath& path,
                                uint64_t watcher_request_id,
                                const WatcherCallback& watcher_callback,
                                const std::string& document_id);
  void OnWatcherAdded(const base::FilePath& path,
                      uint64_t watcher_request_id,
                      int64_t watcher_id);
  void OnWatcherAddedButRemoved(bool success);

  void OnWatcherRemoved(const StatusCallback& callback, bool success);

  // Returns true if the specified watcher request has been canceled.
  // This function should be called only while the request is in-flight.
  bool IsWatcherInflightRequestCanceled(const base::FilePath& path,
                                        uint64_t watcher_request_id) const;

  void ResolveToContentUrlWithDocumentId(
      const ResolveToContentUrlCallback& callback,
      const std::string& document_id);

  // Resolves |path| to a document ID. Failures are indicated by an empty
  // document ID.
  void ResolveToDocumentId(const base::FilePath& path,
                           const ResolveToDocumentIdCallback& callback);
  void ResolveToDocumentIdRecursively(
      const std::string& document_id,
      const std::vector<base::FilePath::StringType>& components,
      const ResolveToDocumentIdCallback& callback);
  void ResolveToDocumentIdRecursivelyWithNameToDocumentMap(
      const std::vector<base::FilePath::StringType>& components,
      const ResolveToDocumentIdCallback& callback,
      base::File::Error error,
      const NameToDocumentMap& mapping);

  // Enumerates child documents of a directory specified by |document_id|.
  // If |force_refresh| is true, the backend is queried even if there is a
  // directory cache.
  // The result is returned as a NameToDocumentMap. It is valid only within
  // the callback and might get deleted immediately after the callback
  // returns.
  void ReadDirectoryInternal(const std::string& document_id,
                             bool force_refresh,
                             const ReadDirectoryInternalCallback& callback);
  void ReadDirectoryInternalWithChildDocuments(
      const std::string& document_id,
      base::Optional<std::vector<mojom::DocumentPtr>> maybe_children);

  // Clears a directory cache.
  void ClearDirectoryCache(const std::string& document_id);

  // |runner_| outlives this object. ArcDocumentsProviderRootMap, the owner of
  // this object, depends on ArcFileSystemOperationRunner in the
  // BrowserContextKeyedServiceFactory dependency graph.
  ArcFileSystemOperationRunner* const runner_;

  const std::string authority_;
  const std::string root_document_id_;

  bool directory_cache_expire_soon_ = false;

  // Cache of directory contents. Keys are document IDs of directories.
  std::map<std::string, DirectoryCache> directory_cache_;

  // Map from a document ID to callbacks pending for ReadDirectoryInternal()
  // calls.
  std::map<std::string, std::vector<ReadDirectoryInternalCallback>>
      pending_callbacks_map_;

  // Map from a file path to a watcher data.
  //
  // Note that we do not use a document ID as a key here to guarantee that
  // a watch installed by AddWatcher() can be always identified in
  // RemoveWatcher() with the same file path specified.
  // See the documentation of AddWatcher() for more details.
  std::map<base::FilePath, WatcherData> path_to_watcher_data_;

  uint64_t next_watcher_request_id_ = 1;

  base::WeakPtrFactory<ArcDocumentsProviderRoot> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcDocumentsProviderRoot);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_ARC_DOCUMENTS_PROVIDER_ROOT_H_
