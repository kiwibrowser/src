// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LEVELDB_WRAPPER_IMPL_H_
#define CONTENT_BROWSER_LEVELDB_WRAPPER_IMPL_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "content/common/content_export.h"
#include "content/common/leveldb_wrapper.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"

namespace base {
namespace trace_event {
class ProcessMemoryDump;
}
}

namespace content {

// This is a wrapper around a leveldb::mojom::LevelDBDatabase. Multiple
// interface
// pointers can be bound to the same object. The wrapper adds a couple of
// features not found directly in leveldb.
// 1) Adds the given prefix, if any, to all keys. This allows the sharing of one
//    database across many, possibly untrusted, consumers and ensuring that they
//    can't access each other's values.
// 2) Enforces a max_size constraint.
// 3) Informs observers when values scoped by prefix are modified.
// 4) Throttles requests to avoid overwhelming the disk.
//
// The wrapper supports two different caching modes.
class CONTENT_EXPORT LevelDBWrapperImpl : public mojom::LevelDBWrapper {
 public:
  using ValueMap = std::map<std::vector<uint8_t>, std::vector<uint8_t>>;
  using ValueMapCallback = base::OnceCallback<void(std::unique_ptr<ValueMap>)>;
  using Change =
      std::pair<std::vector<uint8_t>, base::Optional<std::vector<uint8_t>>>;
  using KeysOnlyMap = std::map<std::vector<uint8_t>, size_t>;

  class CONTENT_EXPORT Delegate {
   public:
    virtual ~Delegate();
    virtual void OnNoBindings() = 0;
    virtual std::vector<leveldb::mojom::BatchedOperationPtr>
    PrepareToCommit() = 0;
    virtual void DidCommit(leveldb::mojom::DatabaseError error) = 0;
    // Called during loading if no data was found. Needs to call |callback|.
    virtual void MigrateData(ValueMapCallback callback);
    // Called during loading to give delegate a chance to modify the data as
    // stored in the database.
    virtual std::vector<Change> FixUpData(const ValueMap& data);
    virtual void OnMapLoaded(leveldb::mojom::DatabaseError error);
  };

  enum class CacheMode {
    // The cache stores only keys (required to maintain max size constraints)
    // when there is only one client binding to save memory. The client is
    // asked to send old values on mutations for sending notifications to
    // observers.
    KEYS_ONLY_WHEN_POSSIBLE,
    // The cache always stores keys and values.
    KEYS_AND_VALUES
  };

  // Options provided to constructor.
  struct Options {
    CacheMode cache_mode = CacheMode::KEYS_AND_VALUES;

    // Max bytes of storage that can be used by key value pairs.
    size_t max_size = 0;
    // Minimum time between 2 commits to disk.
    base::TimeDelta default_commit_delay;
    // Maximum number of bytes written to disk in one hour.
    int max_bytes_per_hour = 0;
    // Maximum number of disk write batches in one hour.
    int max_commits_per_hour = 0;
  };

  // |Delegate::OnNoBindings| will be called when this object has no more
  // bindings and all pending modifications have been processed.
  LevelDBWrapperImpl(leveldb::mojom::LevelDBDatabase* database,
                     const std::string& prefix,
                     Delegate* delegate,
                     const Options& options);
  LevelDBWrapperImpl(leveldb::mojom::LevelDBDatabase* database,
                     std::vector<uint8_t> prefix,
                     Delegate* delegate,
                     const Options& options);

  ~LevelDBWrapperImpl() override;

  void Bind(mojom::LevelDBWrapperRequest request);

  // Forks, or copies, all data in this prefix to another prefix.
  // Note: this object (the parent) must stay alive until the forked wrapper
  // has been loaded (see initialized()).
  std::unique_ptr<LevelDBWrapperImpl> ForkToNewPrefix(
      const std::string& new_prefix,
      Delegate* delegate,
      const Options& options);
  std::unique_ptr<LevelDBWrapperImpl> ForkToNewPrefix(
      std::vector<uint8_t> new_prefix,
      Delegate* delegate,
      const Options& options);

  // Cancels all pending load tasks. Useful for emergency destructions. If the
  // wrapper is unloaded (initialized() returns false), this will DROP all
  // pending changes to the database, and any uninitialized wrappers created
  // through |ForkToNewPrefix| will stay BROKEN and unresponsive.
  void CancelAllPendingRequests();

  // The total bytes used by items which counts towards the quota.
  size_t storage_used() const { return storage_used_; }
  // The physical memory used by the cache.
  size_t memory_used() const { return memory_used_; }

  bool empty() const { return storage_used_ == 0; }

  // If this wrapper is loaded and sending changes to the database.
  bool initialized() const { return IsMapLoaded(); }

  CacheMode cache_mode() const { return cache_mode_; }

  // Tasks that are waiting for the map to be loaded.
  bool has_pending_load_tasks() const {
    return !on_load_complete_tasks_.empty();
  }

  bool has_changes_to_commit() const { return commit_batch_.get(); }

  const std::vector<uint8_t>& prefix() { return prefix_; }

  leveldb::mojom::LevelDBDatabase* database() { return database_; }

  // Commence aggressive flushing. This should be called early during startup,
  // before any localStorage writing. Currently scheduled writes will not be
  // rescheduled and will be flushed at the scheduled time after which
  // aggressive flushing will commence.
  static void EnableAggressiveCommitDelay();

  // Commits any uncommitted data to the database as soon as possible. This
  // usually means data will be committed immediately, but if we're currently
  // waiting on the result of initializing our map the commit won't happen
  // until the load has finished.
  void ScheduleImmediateCommit();

  // Clears the in-memory cache if currently no changes are pending. If there
  // are uncommitted changes this method does nothing.
  void PurgeMemory();

  // Adds memory statistics to |pmd| for memory infra.
  void OnMemoryDump(const std::string& name,
                    base::trace_event::ProcessMemoryDump* pmd);

  // Sets cache mode to either store only keys or keys and values. See
  // SetCacheMode().
  void SetCacheModeForTesting(CacheMode cache_mode);

  // Returns a pointer ID for use with HasObserver and RemoveObserver.
  mojo::InterfacePtrSetElementId AddObserver(
      mojom::LevelDBObserverAssociatedPtr observer);
  bool HasObserver(mojo::InterfacePtrSetElementId id);
  mojom::LevelDBObserverAssociatedPtr RemoveObserver(
      mojo::InterfacePtrSetElementId id);

  // LevelDBWrapper:
  void AddObserver(mojom::LevelDBObserverAssociatedPtrInfo observer) override;
  void Put(const std::vector<uint8_t>& key,
           const std::vector<uint8_t>& value,
           const base::Optional<std::vector<uint8_t>>& client_old_value,
           const std::string& source,
           PutCallback callback) override;
  void Delete(const std::vector<uint8_t>& key,
              const base::Optional<std::vector<uint8_t>>& client_old_value,
              const std::string& source,
              DeleteCallback callback) override;
  void DeleteAll(const std::string& source,
                 DeleteAllCallback callback) override;
  void Get(const std::vector<uint8_t>& key, GetCallback callback) override;
  void GetAll(
      mojom::LevelDBWrapperGetAllCallbackAssociatedPtrInfo complete_callback,
      GetAllCallback callback) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(LevelDBWrapperImplTest, GetAllAfterSetCacheMode);
  FRIEND_TEST_ALL_PREFIXES(LevelDBWrapperImplTest,
                           PutLoadsValuesAfterCacheModeUpgrade);
  FRIEND_TEST_ALL_PREFIXES(LevelDBWrapperImplTest, SetCacheModeConsistent);
  FRIEND_TEST_ALL_PREFIXES(LevelDBWrapperImplParamTest,
                           CommitOnDifferentCacheModes);

  // Used to rate limit commits.
  class RateLimiter {
   public:
    RateLimiter(size_t desired_rate, base::TimeDelta time_quantum);

    void add_samples(size_t samples) { samples_ += samples;  }

    // Computes the total time needed to process the total samples seen
    // at the desired rate.
    base::TimeDelta ComputeTimeNeeded() const;

    // Given the elapsed time since the start of the rate limiting session,
    // computes the delay needed to mimic having processed the total samples
    // seen at the desired rate.
    base::TimeDelta ComputeDelayNeeded(
        const base::TimeDelta elapsed_time) const;

    float rate() const { return rate_; }

   private:
    float rate_;
    float samples_;
    base::TimeDelta time_quantum_;
  };

  // There can be only one fork operation per commit batch.
  struct CommitBatch {
    bool clear_all_first;
    // Prefix copying is performed before applying changes.
    base::Optional<std::vector<uint8_t>> copy_to_prefix;
    // Used if the map_type_ is LOADED_KEYS_ONLY.
    std::map<std::vector<uint8_t>, std::vector<uint8_t>> changed_values;
    // Used if the map_type_ is LOADED_KEYS_AND_VALUES.
    std::set<std::vector<uint8_t>> changed_keys;

    CommitBatch();
    ~CommitBatch();
  };

  enum class MapState {
    UNLOADED,
    // Loading from the database connection.
    LOADING_FROM_DATABASE,
    // Loading from another LevelDBWrapperImpl that we have forked from.
    LOADING_FROM_FORK,
    LOADED_KEYS_ONLY,
    LOADED_KEYS_AND_VALUES
  };

  using LoadStateForForkCallback = base::OnceCallback<
      void(bool database_enabled, const ValueMap&, const KeysOnlyMap&)>;
  using ForkSourceEarlyDeathCallback =
      base::OnceCallback<void(std::vector<uint8_t> source_prefix)>;

  // Changes the cache mode of the wrapper. If applicable, this will change the
  // internal storage type after the next commit. The keys-only mode can only
  // be set only when there is one client binding. It automatically changes to
  // keys-and-values mode when more than one binding exists.
  // Notifications to observers when an item is mutated depends on the
  // |client_old_value| when in keys-only mode. Using GetAll during
  // keys-only mode will cause extra disk access.
  void SetCacheMode(CacheMode cache_mode);

  void OnConnectionError();

  // Always loads the |keys_values_map_|, sets the |map_state_| to
  // LOADED_KEYS_AND_VALUES, and calls through all the completion callbacks.
  //
  // Then if the |cache_mode_| is keys-only, it unloads the map to the
  // |keys_only_map_| and sets the |map_state_| to LOADED_KEYS_ONLY
  void LoadMap(base::OnceClosure completion_callback);
  void OnMapLoaded(leveldb::mojom::DatabaseError status,
                   std::vector<leveldb::mojom::KeyValuePtr> data);
  void OnGotMigrationData(std::unique_ptr<ValueMap> data);
  void CalculateStorageAndMemoryUsed();
  void OnLoadComplete();

  void CreateCommitBatchIfNeeded();
  void StartCommitTimer();
  base::TimeDelta ComputeCommitDelay() const;

  void CommitChanges();
  void OnCommitComplete(leveldb::mojom::DatabaseError error);

  void UnloadMapIfPossible();

  bool IsMapUpgradeNeeded() const {
    return map_state_ == MapState::LOADED_KEYS_ONLY &&
           cache_mode_ == CacheMode::KEYS_AND_VALUES;
  }

  bool IsMapLoaded() const {
    return map_state_ == MapState::LOADED_KEYS_ONLY ||
           map_state_ == MapState::LOADED_KEYS_AND_VALUES;
  }

  bool IsMapLoadedAndEmpty() const {
    return (map_state_ == MapState::LOADED_KEYS_ONLY &&
            keys_only_map_.empty()) ||
           (map_state_ == MapState::LOADED_KEYS_AND_VALUES &&
            keys_values_map_.empty());
  }

  void DoForkOperation(const base::WeakPtr<LevelDBWrapperImpl>& forked_wrapper);
  void OnForkStateLoaded(bool database_enabled,
                         const ValueMap& map,
                         const KeysOnlyMap& key_only_map);

  std::vector<uint8_t> prefix_;
  mojo::BindingSet<mojom::LevelDBWrapper> bindings_;
  mojo::AssociatedInterfacePtrSet<mojom::LevelDBObserver> observers_;
  Delegate* delegate_;
  leveldb::mojom::LevelDBDatabase* database_;

  // For commits to work correctly the map loaded state (keys vs keys & values)
  // must stay consistent for a given commit batch.
  MapState map_state_ = MapState::UNLOADED;
  CacheMode cache_mode_;
  ValueMap keys_values_map_;
  KeysOnlyMap keys_only_map_;
  // These are always consumed & cleared when the map is loaded.
  std::vector<base::OnceClosure> on_load_complete_tasks_;

  size_t storage_used_;
  size_t max_size_;
  size_t memory_used_;
  base::TimeTicks start_time_;
  base::TimeDelta default_commit_delay_;
  RateLimiter data_rate_limiter_;
  RateLimiter commit_rate_limiter_;
  int commit_batches_in_flight_ = 0;
  bool has_committed_data_ = false;
  std::unique_ptr<CommitBatch> commit_batch_;

  base::WeakPtrFactory<LevelDBWrapperImpl> weak_ptr_factory_;

  static bool s_aggressive_flushing_enabled_;

  DISALLOW_COPY_AND_ASSIGN(LevelDBWrapperImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LEVELDB_WRAPPER_IMPL_H_
