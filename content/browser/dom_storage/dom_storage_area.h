// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_AREA_H_
#define CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_AREA_H_

#include <stddef.h>
#include <stdint.h>

#include <list>
#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/strings/nullable_string16.h"
#include "base/strings/string16.h"
#include "content/common/content_export.h"
#include "content/common/dom_storage/dom_storage_map.h"
#include "content/common/dom_storage/dom_storage_types.h"
#include "url/origin.h"

namespace base {
namespace trace_event {
class ProcessMemoryDump;
}
}

namespace content {

class DOMStorageDatabaseAdapter;
class DOMStorageTaskRunner;
class SessionStorageDatabase;

// Container for a per-origin Map of key/value pairs potentially
// backed by storage on disk and lazily commits changes to disk.
// See class comments for DOMStorageContextImpl for a larger overview.
class CONTENT_EXPORT DOMStorageArea
    : public base::RefCountedThreadSafe<DOMStorageArea> {

 public:
  static const base::FilePath::CharType kDatabaseFileExtension[];
  static base::FilePath DatabaseFileNameFromOrigin(const url::Origin& origin);
  static url::Origin OriginFromDatabaseFileName(
      const base::FilePath& file_name);

  // Commence aggressive flushing. This should be called early in the startup -
  // before any localStorage writing. Currently scheduled writes will not be
  // rescheduled and will be flushed at the scheduled time after which
  // aggressive flushing will commence.
  static void EnableAggressiveCommitDelay();

  // Session storage. Backed on disk if |session_storage_backing| is not NULL.
  DOMStorageArea(const std::string& namespace_id,
                 std::vector<std::string> original_namespace_ids,
                 const url::Origin& origin,
                 SessionStorageDatabase* session_storage_backing,
                 DOMStorageTaskRunner* task_runner);

  const url::Origin& origin() const { return origin_; }
  const std::string& namespace_id() const { return namespace_id_; }
  size_t map_memory_used() const { return map_ ? map_->memory_used() : 0; }

  // Writes a copy of the current set of values in the area to the |map|.
  void ExtractValues(DOMStorageValuesMap* map);

  unsigned Length();
  base::NullableString16 Key(unsigned index);
  base::NullableString16 GetItem(const base::string16& key);
  bool SetItem(const base::string16& key,
               const base::string16& value,
               const base::NullableString16& client_old_value,
               base::NullableString16* old_value);
  bool RemoveItem(const base::string16& key,
                  const base::NullableString16& client_old_value,
                  base::string16* old_value);
  bool Clear();
  void FastClear();

  DOMStorageArea* ShallowCopy(const std::string& destination_namespace_id);

  bool HasUncommittedChanges() const;
  void ScheduleImmediateCommit();
  void ClearShallowCopiedCommitBatches();

  // Stores only the keys in the in-memory cache when set to true. Changing this
  // behavior will reload the cache only when needed and not immediately.
  // Note: Do not use ExtractValues() frequently since will have a disk access
  // when only keys are stored.
  void SetCacheOnlyKeys(bool value);

  // Frees up memory when possible. Typically, this method returns
  // the object to its just constructed state, however if uncommitted
  // changes are pending, it does nothing.
  void PurgeMemory();

  // Schedules the commit of any unsaved changes and enters a
  // shutdown state such that the value getters and setters will
  // no longer do anything.
  void Shutdown();

  // Returns true if data needs to be loaded in memory.
  bool IsMapReloadNeeded();

  // Adds memory statistics to |pmd| for chrome://tracing.
  void OnMemoryDump(base::trace_event::ProcessMemoryDump* pmd);

 private:
  friend class DOMStorageAreaTest;
  FRIEND_TEST_ALL_PREFIXES(DOMStorageAreaParamTest, DOMStorageAreaBasics);
  FRIEND_TEST_ALL_PREFIXES(DOMStorageAreaTest, BackingDatabaseOpened);
  FRIEND_TEST_ALL_PREFIXES(DOMStorageAreaParamTest, ShallowCopyWithBacking);
  FRIEND_TEST_ALL_PREFIXES(DOMStorageAreaTest, SetCacheOnlyKeysWithoutBacking);
  FRIEND_TEST_ALL_PREFIXES(DOMStorageAreaTest, SetCacheOnlyKeysWithBacking);
  FRIEND_TEST_ALL_PREFIXES(DOMStorageAreaTest, TestDatabaseFilePath);
  FRIEND_TEST_ALL_PREFIXES(DOMStorageAreaParamTest, CommitTasks);
  FRIEND_TEST_ALL_PREFIXES(DOMStorageAreaParamTest, CommitChangesAtShutdown);
  FRIEND_TEST_ALL_PREFIXES(DOMStorageAreaParamTest, PurgeMemory);
  FRIEND_TEST_ALL_PREFIXES(DOMStorageAreaTest, RateLimiter);
  FRIEND_TEST_ALL_PREFIXES(DOMStorageContextImplTest, PersistentIds);
  FRIEND_TEST_ALL_PREFIXES(DOMStorageContextImplTest, PurgeMemory);
  friend class base::RefCountedThreadSafe<DOMStorageArea>;

  enum LoadState {
    LOAD_STATE_UNLOADED = 0,
    LOAD_STATE_KEYS_ONLY = 1,
    LOAD_STATE_KEYS_AND_VALUES = 2
  };

  // Used to rate limit commits.
  class CONTENT_EXPORT RateLimiter {
   public:
    RateLimiter(size_t desired_rate, base::TimeDelta time_quantum);

    void add_samples(size_t samples) {
      samples_ += samples;
    }

    // Computes the total time needed to process the total samples seen
    // at the desired rate.
    base::TimeDelta ComputeTimeNeeded() const;

    // Given the elapsed time since the start of the rate limiting session,
    // computes the delay needed to mimic having processed the total samples
    // seen at the desired rate.
    base::TimeDelta ComputeDelayNeeded(
        const base::TimeDelta elapsed_time) const;

   private:
    float rate_;
    float samples_;
    base::TimeDelta time_quantum_;
  };

  struct CONTENT_EXPORT CommitBatch
      : public base::RefCountedThreadSafe<CommitBatch> {
    bool clear_all_first;
    DOMStorageValuesMap changed_values;

    CommitBatch();
    size_t GetDataSize() const;

   private:
    friend class base::RefCountedThreadSafe<CommitBatch>;
    ~CommitBatch();
  };

  struct CONTENT_EXPORT CommitBatchHolder {
    enum Type { TYPE_CURRENT_BATCH, TYPE_IN_FLIGHT, TYPE_CLONE };
    Type type;
    scoped_refptr<CommitBatch> batch;

    CommitBatchHolder(Type, scoped_refptr<CommitBatch>);
    CommitBatchHolder(const CommitBatchHolder& other);
    ~CommitBatchHolder();
  };

  ~DOMStorageArea();

  // Loads the map for this origin into the |map_| if desired load state is not
  // met and into the |map| if it's not null.
  void LoadMapAndApplyUncommittedChangesIfNeeded(DOMStorageValuesMap* map);

  // If the cache state is inconsistent with the map storage, purges the map and
  // updates with new data if possible without reading from database. Noop if
  // the area has uncommitted changes.
  void UnloadMapIfDesired();

  // Post tasks to defer writing a batch of changed values to
  // disk on the commit sequence, and to call back on the primary
  // task sequence when complete.
  CommitBatch* CreateCommitBatchIfNeeded();
  const CommitBatchHolder* GetCurrentCommitBatch() const;
  bool HasCommitBatchInFlight() const;
  void PopulateCommitBatchValues();
  void StartCommitTimer();
  void OnCommitTimer();
  void PostCommitTask();
  void CommitChanges(const CommitBatch* commit_batch);
  void OnCommitComplete();
  base::TimeDelta ComputeCommitDelay() const;

  void ShutdownInCommitSequence();

  static bool s_aggressive_flushing_enabled_;

  std::string namespace_id_;
  std::vector<std::string> original_namespace_ids_;
  url::Origin origin_;
  scoped_refptr<DOMStorageTaskRunner> task_runner_;
  LoadState desired_load_state_;
  LoadState load_state_;
  scoped_refptr<DOMStorageMap> map_;
  std::unique_ptr<DOMStorageDatabaseAdapter> backing_;
  scoped_refptr<SessionStorageDatabase> session_storage_backing_;
  bool is_shutdown_;
  std::list<CommitBatchHolder> commit_batches_;
  base::TimeTicks start_time_;
  RateLimiter data_rate_limiter_;
  RateLimiter commit_rate_limiter_;

  DISALLOW_COPY_AND_ASSIGN(DOMStorageArea);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOM_STORAGE_DOM_STORAGE_AREA_H_
