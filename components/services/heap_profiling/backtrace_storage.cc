// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/heap_profiling/backtrace_storage.h"

#include "base/logging.h"
#include "components/services/heap_profiling/backtrace.h"

namespace heap_profiling {

namespace {
constexpr size_t kShardCount = 64;
}  // namespace

BacktraceStorage::Lock::Lock() : storage_(nullptr) {}

BacktraceStorage::Lock::Lock(BacktraceStorage* storage) : storage_(storage) {
  storage_->LockStorage();
}

BacktraceStorage::Lock::Lock(Lock&& other) : storage_(other.storage_) {
  other.storage_ = nullptr;  // Prevent the other from unlocking.
}

BacktraceStorage::Lock::~Lock() {
  if (storage_)
    storage_->UnlockStorage();
}

BacktraceStorage::Lock& BacktraceStorage::Lock::operator=(Lock&& other) {
  if (storage_)
    storage_->UnlockStorage();
  storage_ = other.storage_;
  other.storage_ = nullptr;
  return *this;
}

bool BacktraceStorage::Lock::IsLocked() {
  return storage_ != nullptr;
}

BacktraceStorage::BacktraceStorage() : shards_(kShardCount) {}

BacktraceStorage::~BacktraceStorage() {}

const Backtrace* BacktraceStorage::Insert(std::vector<Address>&& bt) {
  Backtrace backtrace(std::move(bt));
  size_t shard_index = backtrace.fingerprint() % kShardCount;
  ContainerShard& shard = shards_[shard_index];

  base::AutoLock lock(shard.lock);
  auto iter = shard.backtraces.insert(std::move(backtrace)).first;
  iter->AddRef();
  return &*iter;
}

void BacktraceStorage::Free(const Backtrace* bt) {
  size_t shard_index = bt->fingerprint() % kShardCount;
  ContainerShard& shard = shards_[shard_index];
  base::AutoLock lock(shard.lock);

  if (shard.consumer_count) {
    shard.release_after_lock.push_back(bt);
  } else {
    if (!bt->Release())
      shard.backtraces.erase(*bt);
  }
}

void BacktraceStorage::Free(const std::vector<const Backtrace*>& bts) {
  // Separate backtraces by shard using the fingerprint.
  std::vector<const Backtrace*> backtraces_by_shard[kShardCount];
  for (size_t i = 0; i < kShardCount; ++i) {
    backtraces_by_shard[i].reserve(bts.size() / kShardCount + 1);
  }
  for (const Backtrace* bt : bts) {
    size_t shard_index = bt->fingerprint() % kShardCount;
    backtraces_by_shard[shard_index].push_back(bt);
  }

  for (size_t i = 0; i < kShardCount; ++i) {
    ContainerShard& shard = shards_[i];
    base::AutoLock lock(shard.lock);

    if (shard.consumer_count) {
      shard.release_after_lock.insert(shard.release_after_lock.end(),
                                      backtraces_by_shard[i].begin(),
                                      backtraces_by_shard[i].end());
    } else {
      ReleaseBacktracesLocked(backtraces_by_shard[i], i);
    }
  }
}

void BacktraceStorage::LockStorage() {
  for (size_t i = 0; i < kShardCount; ++i) {
    base::AutoLock lock(shards_[i].lock);
    shards_[i].consumer_count++;
  }
}

void BacktraceStorage::UnlockStorage() {
  for (size_t i = 0; i < kShardCount; ++i) {
    ContainerShard& shard = shards_[i];
    base::AutoLock lock(shard.lock);
    DCHECK(shard.consumer_count > 0);
    shard.consumer_count--;

    if (shard.consumer_count == 0) {
      ReleaseBacktracesLocked(shard.release_after_lock, i);
      shard.release_after_lock.clear();
      shard.release_after_lock.shrink_to_fit();
    }
  }
}

void BacktraceStorage::ReleaseBacktracesLocked(
    const std::vector<const Backtrace*>& bts,
    size_t shard_index) {
  ContainerShard& shard = shards_[shard_index];

  shard.lock.AssertAcquired();
  DCHECK_EQ(0, shard.consumer_count);

  for (const Backtrace* bt : bts) {
    if (!bt->Release())
      shard.backtraces.erase(*bt);
  }
}

BacktraceStorage::ContainerShard::ContainerShard() = default;
BacktraceStorage::ContainerShard::~ContainerShard() = default;

}  // namespace heap_profiling
