// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/fileapi/arc_documents_provider_root.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_documents_provider_util.h"
#include "content/public/browser/browser_thread.h"
#include "url/gurl.h"

using content::BrowserThread;
using EntryList = storage::AsyncFileUtil::EntryList;

namespace arc {

namespace {

// Directory cache will be cleared this duration after it is built.
constexpr base::TimeDelta kCacheExpiration = base::TimeDelta::FromSeconds(60);

}  // namespace

// Represents the status of a document watcher.
struct ArcDocumentsProviderRoot::WatcherData {
  // ID of a watcher in the remote file system service.
  //
  // Valid IDs are represented by positive integers. An invalid watcher is
  // represented by |kInvalidWatcherId|, which occurs in several cases:
  //
  // - AddWatcher request is still in-flight. In this case, a valid ID is set
  //   to |inflight_request_id|.
  //
  // - The remote file system service notified us that it stopped and all
  //   watchers were forgotten. Such watchers are still tracked here, but they
  //   are not known by the remote service.
  int64_t id;

  // A unique ID of AddWatcher() request.
  //
  // While AddWatcher() is in-flight, a positive integer is set to this
  // variable, and |id| is |kInvalidWatcherId|. Otherwise it is set to
  // |kInvalidWatcherRequestId|.
  uint64_t inflight_request_id;
};

// Cache of directory contents.
struct ArcDocumentsProviderRoot::DirectoryCache {
  // Files under the directory.
  NameToDocumentMap mapping;

  // Timer to delete this cache.
  base::OneShotTimer clear_timer;
};

// static
const int64_t ArcDocumentsProviderRoot::kInvalidWatcherId = -1;
// static
const uint64_t ArcDocumentsProviderRoot::kInvalidWatcherRequestId = 0;
// static
const ArcDocumentsProviderRoot::WatcherData
    ArcDocumentsProviderRoot::kInvalidWatcherData = {kInvalidWatcherId,
                                                     kInvalidWatcherRequestId};

ArcDocumentsProviderRoot::ArcDocumentsProviderRoot(
    ArcFileSystemOperationRunner* runner,
    const std::string& authority,
    const std::string& root_document_id)
    : runner_(runner),
      authority_(authority),
      root_document_id_(root_document_id),
      weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  runner_->AddObserver(this);
}

ArcDocumentsProviderRoot::~ArcDocumentsProviderRoot() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  runner_->RemoveObserver(this);
}

void ArcDocumentsProviderRoot::GetFileInfo(const base::FilePath& path,
                                           GetFileInfoCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (path.IsAbsolute()) {
    std::move(callback).Run(base::File::FILE_ERROR_NOT_FOUND,
                            base::File::Info());
    return;
  }

  // Specially handle the root directory since Files app does not update the
  // list of file systems (left pane) until all volumes respond to GetMetadata
  // requests to root directories.
  if (path.empty()) {
    base::File::Info info;
    info.size = -1;
    info.is_directory = true;
    info.is_symbolic_link = false;
    info.last_modified = info.last_accessed = info.creation_time =
        base::Time::UnixEpoch();  // arbitrary
    std::move(callback).Run(base::File::FILE_OK, info);
    return;
  }

  base::FilePath basename = path.BaseName();
  base::FilePath parent = path.DirName();
  if (parent.value() == base::FilePath::kCurrentDirectory)
    parent = base::FilePath();

  ResolveToDocumentId(
      parent,
      base::Bind(&ArcDocumentsProviderRoot::GetFileInfoWithParentDocumentId,
                 weak_ptr_factory_.GetWeakPtr(), base::Passed(&callback),
                 basename));
}

void ArcDocumentsProviderRoot::ReadDirectory(const base::FilePath& path,
                                             ReadDirectoryCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ResolveToDocumentId(
      path, base::Bind(&ArcDocumentsProviderRoot::ReadDirectoryWithDocumentId,
                       weak_ptr_factory_.GetWeakPtr(),
                       base::Passed(std::move(callback))));
}

void ArcDocumentsProviderRoot::AddWatcher(
    const base::FilePath& path,
    const WatcherCallback& watcher_callback,
    const StatusCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (path_to_watcher_data_.count(path)) {
    callback.Run(base::File::FILE_ERROR_FAILED);
    return;
  }
  uint64_t watcher_request_id = next_watcher_request_id_++;
  path_to_watcher_data_.insert(
      std::make_pair(path, WatcherData{kInvalidWatcherId, watcher_request_id}));
  ResolveToDocumentId(
      path, base::Bind(&ArcDocumentsProviderRoot::AddWatcherWithDocumentId,
                       weak_ptr_factory_.GetWeakPtr(), path, watcher_request_id,
                       watcher_callback));

  // HACK: Invoke |callback| immediately.
  //
  // TODO(crbug.com/698624): Remove this hack. It was introduced because Files
  // app freezes until AddWatcher() finishes, but it should be handled in Files
  // app rather than here.
  callback.Run(base::File::FILE_OK);
}

void ArcDocumentsProviderRoot::RemoveWatcher(const base::FilePath& path,
                                             const StatusCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto iter = path_to_watcher_data_.find(path);
  if (iter == path_to_watcher_data_.end()) {
    callback.Run(base::File::FILE_ERROR_FAILED);
    return;
  }
  int64_t watcher_id = iter->second.id;
  path_to_watcher_data_.erase(iter);
  if (watcher_id == kInvalidWatcherId) {
    // This is an invalid watcher. No need to send a request to the remote
    // service.
    callback.Run(base::File::FILE_OK);
    return;
  }
  runner_->RemoveWatcher(
      watcher_id, base::BindOnce(&ArcDocumentsProviderRoot::OnWatcherRemoved,
                                 weak_ptr_factory_.GetWeakPtr(), callback));
}

void ArcDocumentsProviderRoot::ResolveToContentUrl(
    const base::FilePath& path,
    const ResolveToContentUrlCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ResolveToDocumentId(
      path,
      base::Bind(&ArcDocumentsProviderRoot::ResolveToContentUrlWithDocumentId,
                 weak_ptr_factory_.GetWeakPtr(), callback));
}

void ArcDocumentsProviderRoot::SetDirectoryCacheExpireSoonForTesting() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  directory_cache_expire_soon_ = true;
}

void ArcDocumentsProviderRoot::OnWatchersCleared() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Mark all watchers invalid.
  for (auto& entry : path_to_watcher_data_)
    entry.second = kInvalidWatcherData;
}

void ArcDocumentsProviderRoot::GetFileInfoWithParentDocumentId(
    GetFileInfoCallback callback,
    const base::FilePath& basename,
    const std::string& parent_document_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (parent_document_id.empty()) {
    std::move(callback).Run(base::File::FILE_ERROR_NOT_FOUND,
                            base::File::Info());
    return;
  }
  ReadDirectoryInternal(
      parent_document_id, false /* force_refresh */,
      base::Bind(&ArcDocumentsProviderRoot::GetFileInfoWithNameToDocumentMap,
                 weak_ptr_factory_.GetWeakPtr(), base::Passed(&callback),
                 basename));
}

void ArcDocumentsProviderRoot::GetFileInfoWithNameToDocumentMap(
    GetFileInfoCallback callback,
    const base::FilePath& basename,
    base::File::Error error,
    const NameToDocumentMap& mapping) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (error != base::File::FILE_OK) {
    std::move(callback).Run(error, base::File::Info());
    return;
  }

  auto iter = mapping.find(basename.value());
  if (iter == mapping.end()) {
    std::move(callback).Run(base::File::FILE_ERROR_NOT_FOUND,
                            base::File::Info());
    return;
  }

  const auto& document = iter->second;

  base::File::Info info;
  info.size = document->size;
  info.is_directory = document->mime_type == kAndroidDirectoryMimeType;
  info.is_symbolic_link = false;
  info.last_modified = info.last_accessed = info.creation_time =
      base::Time::FromJavaTime(document->last_modified);

  std::move(callback).Run(base::File::FILE_OK, info);
}

void ArcDocumentsProviderRoot::ReadDirectoryWithDocumentId(
    ReadDirectoryCallback callback,
    const std::string& document_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (document_id.empty()) {
    std::move(callback).Run(base::File::FILE_ERROR_NOT_FOUND, {});
    return;
  }
  ReadDirectoryInternal(
      document_id, true /* force_refresh */,
      base::Bind(&ArcDocumentsProviderRoot::ReadDirectoryWithNameToDocumentMap,
                 weak_ptr_factory_.GetWeakPtr(),
                 base::Passed(std::move(callback))));
}

void ArcDocumentsProviderRoot::ReadDirectoryWithNameToDocumentMap(
    ReadDirectoryCallback callback,
    base::File::Error error,
    const NameToDocumentMap& mapping) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (error != base::File::FILE_OK) {
    std::move(callback).Run(error, {});
    return;
  }

  std::vector<ThinFileInfo> files;
  for (const auto& pair : mapping) {
    const base::FilePath::StringType& name = pair.first;
    const mojom::DocumentPtr& document = pair.second;
    files.emplace_back(
        ThinFileInfo{name, document->document_id,
                     document->mime_type == kAndroidDirectoryMimeType,
                     base::Time::FromJavaTime(document->last_modified)});
  }
  std::move(callback).Run(base::File::FILE_OK, std::move(files));
}

void ArcDocumentsProviderRoot::AddWatcherWithDocumentId(
    const base::FilePath& path,
    uint64_t watcher_request_id,
    const WatcherCallback& watcher_callback,
    const std::string& document_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (IsWatcherInflightRequestCanceled(path, watcher_request_id))
    return;

  if (document_id.empty()) {
    DCHECK(path_to_watcher_data_.count(path));
    path_to_watcher_data_[path] = kInvalidWatcherData;
    return;
  }

  runner_->AddWatcher(
      authority_, document_id, watcher_callback,
      base::BindOnce(&ArcDocumentsProviderRoot::OnWatcherAdded,
                     weak_ptr_factory_.GetWeakPtr(), path, watcher_request_id));
}

void ArcDocumentsProviderRoot::OnWatcherAdded(const base::FilePath& path,
                                              uint64_t watcher_request_id,
                                              int64_t watcher_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (IsWatcherInflightRequestCanceled(path, watcher_request_id)) {
    runner_->RemoveWatcher(
        watcher_id,
        base::BindOnce(&ArcDocumentsProviderRoot::OnWatcherAddedButRemoved,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  DCHECK(path_to_watcher_data_.count(path));
  path_to_watcher_data_[path] =
      WatcherData{watcher_id < 0 ? kInvalidWatcherId : watcher_id,
                  kInvalidWatcherRequestId};
}

void ArcDocumentsProviderRoot::OnWatcherAddedButRemoved(bool success) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Ignore |success|.
}

void ArcDocumentsProviderRoot::OnWatcherRemoved(const StatusCallback& callback,
                                                bool success) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  callback.Run(success ? base::File::FILE_OK : base::File::FILE_ERROR_FAILED);
}

bool ArcDocumentsProviderRoot::IsWatcherInflightRequestCanceled(
    const base::FilePath& path,
    uint64_t watcher_request_id) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto iter = path_to_watcher_data_.find(path);
  return (iter == path_to_watcher_data_.end() ||
          iter->second.inflight_request_id != watcher_request_id);
}

void ArcDocumentsProviderRoot::ResolveToContentUrlWithDocumentId(
    const ResolveToContentUrlCallback& callback,
    const std::string& document_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (document_id.empty()) {
    callback.Run(GURL());
    return;
  }
  callback.Run(BuildDocumentUrl(authority_, document_id));
}

void ArcDocumentsProviderRoot::ResolveToDocumentId(
    const base::FilePath& path,
    const ResolveToDocumentIdCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  std::vector<base::FilePath::StringType> components;
  path.GetComponents(&components);
  ResolveToDocumentIdRecursively(root_document_id_, components, callback);
}

void ArcDocumentsProviderRoot::ResolveToDocumentIdRecursively(
    const std::string& document_id,
    const std::vector<base::FilePath::StringType>& components,
    const ResolveToDocumentIdCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (components.empty()) {
    callback.Run(document_id);
    return;
  }
  ReadDirectoryInternal(
      document_id, false /* force_refresh */,
      base::Bind(&ArcDocumentsProviderRoot::
                     ResolveToDocumentIdRecursivelyWithNameToDocumentMap,
                 weak_ptr_factory_.GetWeakPtr(), components, callback));
}

void ArcDocumentsProviderRoot::
    ResolveToDocumentIdRecursivelyWithNameToDocumentMap(
        const std::vector<base::FilePath::StringType>& components,
        const ResolveToDocumentIdCallback& callback,
        base::File::Error error,
        const NameToDocumentMap& mapping) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!components.empty());
  if (error != base::File::FILE_OK) {
    callback.Run(std::string());
    return;
  }
  auto iter = mapping.find(components[0]);
  if (iter == mapping.end()) {
    callback.Run(std::string());
    return;
  }
  ResolveToDocumentIdRecursively(iter->second->document_id,
                                 std::vector<base::FilePath::StringType>(
                                     components.begin() + 1, components.end()),
                                 callback);
}

void ArcDocumentsProviderRoot::ReadDirectoryInternal(
    const std::string& document_id,
    bool force_refresh,
    const ReadDirectoryInternalCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Use cache if possible. Note that we do not invalidate it immediately
  // even if we decide not to use it for now.
  if (!force_refresh) {
    auto iter = directory_cache_.find(document_id);
    if (iter != directory_cache_.end()) {
      callback.Run(base::File::FILE_OK, iter->second.mapping);
      return;
    }
  }

  auto& pending_callbacks = pending_callbacks_map_[document_id];
  bool read_in_flight = !pending_callbacks.empty();
  pending_callbacks.emplace_back(callback);

  if (read_in_flight) {
    // There is already an in-flight ReadDirectoryInternal() call, so
    // just enqueue the callback and return.
    return;
  }

  runner_->GetChildDocuments(
      authority_, document_id,
      base::BindOnce(
          &ArcDocumentsProviderRoot::ReadDirectoryInternalWithChildDocuments,
          weak_ptr_factory_.GetWeakPtr(), document_id));
}

void ArcDocumentsProviderRoot::ReadDirectoryInternalWithChildDocuments(
    const std::string& document_id,
    base::Optional<std::vector<mojom::DocumentPtr>> maybe_children) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto iter = pending_callbacks_map_.find(document_id);
  DCHECK(iter != pending_callbacks_map_.end());

  std::vector<ReadDirectoryInternalCallback> pending_callbacks =
      std::move(iter->second);
  DCHECK(!pending_callbacks.empty());

  pending_callbacks_map_.erase(iter);

  if (!maybe_children) {
    for (const auto& callback : pending_callbacks)
      callback.Run(base::File::FILE_ERROR_NOT_FOUND, NameToDocumentMap());
    return;
  }

  std::vector<mojom::DocumentPtr> children = std::move(maybe_children.value());

  // Sort entries to keep the mapping stable as far as possible.
  std::sort(children.begin(), children.end(),
            [](const mojom::DocumentPtr& a, const mojom::DocumentPtr& b) {
              return a->document_id < b->document_id;
            });

  NameToDocumentMap mapping;
  std::map<base::FilePath::StringType, int> suffix_counters;

  for (mojom::DocumentPtr& document : children) {
    base::FilePath::StringType filename = GetFileNameForDocument(document);

    if (mapping.count(filename) > 0) {
      // Resolve a conflict by adding a suffix.
      int& suffix_counter = suffix_counters[filename];
      while (true) {
        ++suffix_counter;
        std::string suffix = base::StringPrintf(" (%d)", suffix_counter);
        base::FilePath::StringType new_filename =
            base::FilePath(filename).InsertBeforeExtensionASCII(suffix).value();
        if (mapping.count(new_filename) == 0) {
          filename = new_filename;
          break;
        }
      }
    }

    DCHECK_EQ(0u, mapping.count(filename));

    mapping[filename] = std::move(document);
  }

  // This may create a new cache, or just update an existing cache.
  DirectoryCache& cache = directory_cache_[document_id];
  cache.mapping = std::move(mapping);
  cache.clear_timer.Start(
      FROM_HERE,
      directory_cache_expire_soon_ ? base::TimeDelta() : kCacheExpiration,
      base::Bind(&ArcDocumentsProviderRoot::ClearDirectoryCache,
                 weak_ptr_factory_.GetWeakPtr(), document_id));

  for (const auto& callback : pending_callbacks)
    callback.Run(base::File::FILE_OK, cache.mapping);
}

void ArcDocumentsProviderRoot::ClearDirectoryCache(
    const std::string& document_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  directory_cache_.erase(document_id);
}

}  // namespace arc
