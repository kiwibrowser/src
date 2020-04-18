// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_CONTEXT_MOJO_H_
#define CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_CONTEXT_MOJO_H_

#include <stdint.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/trace_event/memory_dump_provider.h"
#include "content/browser/dom_storage/session_storage_data_map.h"
#include "content/browser/dom_storage/session_storage_metadata.h"
#include "content/browser/dom_storage/session_storage_namespace_impl_mojo.h"
#include "content/common/content_export.h"
#include "content/common/leveldb_wrapper.mojom.h"
#include "content/common/storage_partition_service.mojom.h"
#include "content/public/browser/session_storage_usage_info.h"
#include "services/file/public/mojom/file_system.mojom.h"
#include "url/origin.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace service_manager {
class Connector;
}  // namespace service_manager

namespace content {
struct SessionStorageUsageInfo;

// Used for mojo-based SessionStorage implementation.
// Created on the UI thread, but all further methods are called on the task
// runner passed to the constructor. Furthermore since destruction of this class
// can involve asynchronous steps, it can only be deleted by calling
// ShutdownAndDelete (on the correct task runner).
class CONTENT_EXPORT SessionStorageContextMojo
    : public base::trace_event::MemoryDumpProvider,
      public SessionStorageDataMap::Listener {
 public:
  using GetStorageUsageCallback =
      base::OnceCallback<void(std::vector<SessionStorageUsageInfo>)>;

  SessionStorageContextMojo(
      scoped_refptr<base::SequencedTaskRunner> task_runner,
      service_manager::Connector* connector,
      base::Optional<base::FilePath> local_partition_directory,
      std::string leveldb_name);

  void OpenSessionStorage(int process_id,
                          const std::string& namespace_id,
                          mojom::SessionStorageNamespaceRequest request);

  void CreateSessionNamespace(const std::string& namespace_id);
  void CloneSessionNamespace(const std::string& namespace_id_to_clone,
                             const std::string& clone_namespace_id);

  void DeleteSessionNamespace(const std::string& namespace_id,
                              bool should_persist);

  void Flush();

  void GetStorageUsage(GetStorageUsageCallback callback);

  void DeleteStorage(const url::Origin& origin,
                     const std::string& namespace_id);

  // Called when the owning BrowserContext is ending. Schedules the commit of
  // any unsaved changes then deletes this object. All data on disk (where there
  // was no call to |DeleteSessionNamespace| will stay on disk for later
  // restoring.
  void ShutdownAndDelete();

  // Clears any caches, to free up as much memory as possible. Next access to
  // storage for a particular origin will reload the data from the database.
  void PurgeMemory();

  // Clears unused leveldb wrappers, when thresholds are reached.
  void PurgeUnusedWrappersIfNeeded();

  // Any namespaces that have been loaded from disk and have not had a
  // corresponding CreateSessionNamespace() call will be deleted. Called after
  // startup. The calback is used for unittests, and is called after the
  // scavenging has finished (but not necessarily saved to disk). A null
  // callback is ok.
  void ScavengeUnusedNamespaces(base::OnceClosure done);

  // base::trace_event::MemoryDumpProvider implementation:
  bool OnMemoryDump(const base::trace_event::MemoryDumpArgs& args,
                    base::trace_event::ProcessMemoryDump* pmd) override;

  // SessionStorageLevelDBWrapper::Listener implementation:
  void OnDataMapCreation(const std::vector<uint8_t>& map_prefix,
                         SessionStorageDataMap* map) override;
  void OnDataMapDestruction(const std::vector<uint8_t>& map_prefix) override;
  void OnCommitResult(leveldb::mojom::DatabaseError error) override;

  // Sets the database for testing.
  void SetDatabaseForTesting(
      leveldb::mojom::LevelDBDatabaseAssociatedPtr database);

  leveldb::mojom::LevelDBDatabase* DatabaseForTesting() {
    return database_.get();
  }

  void FlushAreaForTesting(const std::string& namespace_id,
                           const url::Origin& origin);

 private:
  // Object deletion is done through |ShutdownAndDelete()|.
  ~SessionStorageContextMojo() override;

  scoped_refptr<SessionStorageMetadata::MapData> RegisterNewAreaMap(
      SessionStorageMetadata::NamespaceEntry namespace_entry,
      const url::Origin& origin);

  void RegisterShallowClonedNamespace(
      SessionStorageMetadata::NamespaceEntry source_namespace_entry,
      const std::string& new_namespace_id,
      const SessionStorageNamespaceImplMojo::OriginAreas& clone_from_areas);

  std::unique_ptr<SessionStorageNamespaceImplMojo>
  CreateSessionStorageNamespaceImplMojo(std::string namespace_id);

  void DoDatabaseDelete(const std::string& namespace_id);

  // Runs |callback| immediately if already connected to a database, otherwise
  // delays running |callback| untill after a connection has been established.
  // Initiates connecting to the database if no connection is in progress yet.
  void RunWhenConnected(base::OnceClosure callback);

  // Part of our asynchronous directory opening called from RunWhenConnected().
  void InitiateConnection(bool in_memory_only = false);
  void OnDirectoryOpened(base::File::Error err);
  void OnDatabaseOpened(bool in_memory, leveldb::mojom::DatabaseError status);
  void OnGotDatabaseVersion(leveldb::mojom::DatabaseError status,
                            const std::vector<uint8_t>& value);
  void OnGotNamespaces(
      base::OnceClosure done,
      std::vector<leveldb::mojom::BatchedOperationPtr> migration_operations,
      leveldb::mojom::DatabaseError status,
      std::vector<leveldb::mojom::KeyValuePtr> values);
  void OnGotNextMapId(base::OnceClosure done,
                      leveldb::mojom::DatabaseError status,
                      const std::vector<uint8_t>& map_id);
  void OnConnectionFinished();
  void DeleteAndRecreateDatabase(const char* histogram_name);
  void OnDBDestroyed(bool recreate_in_memory,
                     leveldb::mojom::DatabaseError status);

  void OnGotMetaData(GetStorageUsageCallback callback,
                     leveldb::mojom::DatabaseError status,
                     std::vector<leveldb::mojom::KeyValuePtr> data);

  void OnShutdownComplete(leveldb::mojom::DatabaseError error);

  void GetStatistics(size_t* total_cache_size, size_t* unused_wrapper_count);

  // These values are written to logs.  New enum values can be added, but
  // existing enums must never be renumbered or deleted and reused.
  enum class OpenResult {
    kDirectoryOpenFailed = 0,
    kDatabaseOpenFailed = 1,
    kInvalidVersion = 2,
    kVersionReadError = 3,
    kNamespacesReadError = 4,
    kSuccess = 6,
    kMaxValue = kSuccess
  };

  void LogDatabaseOpenResult(OpenResult result);

  // Since the session storage object hierarchy references iterators owned by
  // the metadata, make sure it is destroyed last on destruction.
  SessionStorageMetadata metadata_;

  std::unique_ptr<service_manager::Connector> connector_;
  const base::Optional<base::FilePath> partition_directory_path_;
  std::string leveldb_name_;

  enum ConnectionState {
    NO_CONNECTION,
    FETCHING_METADATA,
    CONNECTION_IN_PROGRESS,
    CONNECTION_FINISHED,
    CONNECTION_SHUTDOWN
  } connection_state_ = NO_CONNECTION;
  bool database_initialized_ = false;

  file::mojom::FileSystemPtr file_system_;
  filesystem::mojom::DirectoryPtr partition_directory_;

  base::trace_event::MemoryAllocatorDumpGuid memory_dump_id_;

  leveldb::mojom::LevelDBServicePtr leveldb_service_;
  leveldb::mojom::LevelDBDatabaseAssociatedPtr database_;
  bool tried_to_recreate_during_open_ = false;

  std::vector<base::OnceClosure> on_database_opened_callbacks_;

  // The removal of items from this map is managed by the refcounting in
  // SessionStorageDataMap.
  // Populated after the database is connected.
  std::map<std::vector<uint8_t>, SessionStorageDataMap*> data_maps_;
  // Populated on OpenSessionStorage calls.
  std::map<std::string, std::unique_ptr<SessionStorageNamespaceImplMojo>>
      namespaces_;

  // Scavenging only happens once.
  bool has_scavenged_ = false;
  // When namespaces are destroyed but marked as persistent, a scavenge should
  // not delete them. Cleared after ScavengeUnusedNamespaces is called.
  std::set<std::string> protected_namespaces_from_scavenge_;

  bool is_low_end_device_;
  // Counts consecutive commit errors. If this number reaches a threshold, the
  // whole database is thrown away.
  int commit_error_count_ = 0;
  bool tried_to_recover_from_commit_errors_ = false;

  // Name of an extra histogram to log open results to, if not null.
  const char* open_result_histogram_ = nullptr;

  base::WeakPtrFactory<SessionStorageContextMojo> weak_ptr_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOM_STORAGE_SESSION_STORAGE_CONTEXT_MOJO_H_
