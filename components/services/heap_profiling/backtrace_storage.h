// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_HEAP_PROFILING_BACKTRACE_STORAGE_H_
#define COMPONENTS_SERVICES_HEAP_PROFILING_BACKTRACE_STORAGE_H_

#include <unordered_set>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "components/services/heap_profiling/backtrace.h"

namespace heap_profiling {

// Backtraces are stored effectively as atoms, and this class is the backing
// store for the atoms. When you insert a backtrace, it will get de-duped with
// existing ones, one refcount added, and returned. When you're done with a
// backtrace, call Free() which will release the refcount. This may or may not
// release the underlying Backtrace itself, depending on whether other refs are
// held.
//
// This class is threadsafe.
class BacktraceStorage {
 public:
  // Instantiating this lock will prevent backtraces from being deleted from
  // the strorage for as long as it's alive. This class is moveable but not
  // copyable.
  class Lock {
   public:
    Lock();                                    // Doesn't take the lock.
    explicit Lock(BacktraceStorage* storage);  // Takes the lock.
    Lock(const Lock&) = delete;
    Lock(Lock&&);
    ~Lock();

    Lock& operator=(Lock&& other);
    Lock& operator=(const Lock&) = delete;
    bool IsLocked();

   private:
    BacktraceStorage* storage_;  // May be null if moved from.
  };

  BacktraceStorage();
  ~BacktraceStorage();

  // Adds the given backtrace to the storage and returns a key to it. If a
  // matching backtrace already exists, a key to the existing one will be
  // returned.
  //
  // The returned key will have a reference count associated with it, call
  // Free when the key is no longer needed.
  const Backtrace* Insert(std::vector<Address>&& bt);

  // Frees one reference to a backtrace.
  void Free(const Backtrace* bt);
  void Free(const std::vector<const Backtrace*>& bts);

 private:
  friend Lock;
  using Container = std::unordered_set<Backtrace>;

  // Called by the BacktraceStorage::Lock class.
  void LockStorage();
  void UnlockStorage();

  // Releases all backtraces in the vector assuming |lock| is already held
  // and |consumer_count| is zero.
  void ReleaseBacktracesLocked(const std::vector<const Backtrace*>& bts,
                               size_t shard_index);

  struct ContainerShard {
    ContainerShard();
    ~ContainerShard();

    // Container of de-duped, live backtraces. All modifications to |backtraces|
    // or the Backtrace elements owned by |backtraces| must be protected by
    // |lock|.
    Container backtraces;
    mutable base::Lock lock;

    // Protected by |lock|. This indicates the number of consumers that have
    // raw backtrace pointers owned by |backtraces|. As long as this count is
    // non-zero, Backtraces owned by |backtraces| cannot be modified or
    // destroyed. Elements can be inserted into |backtraces| even when this is
    // non-zero because existing raw backtrace pointers are stable.
    int consumer_count = 0;

    // When |consumer_count| is non-zero, no backtraces will be deleted from
    // the storage. Instead, they are accumulated here for releasing after
    // consumer_count becomes non-zero.
    std::vector<const Backtrace*> release_after_lock;

    DISALLOW_COPY_AND_ASSIGN(ContainerShard);
  };

  // Backtraces are sharded by fingerprint to reduce lock contention.
  std::vector<ContainerShard> shards_;
  DISALLOW_COPY_AND_ASSIGN(BacktraceStorage);
};

}  // namespace heap_profiling

#endif  // COMPONENTS_SERVICES_HEAP_PROFILING_BACKTRACE_STORAGE_H_
