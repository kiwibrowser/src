// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/dom_storage/session_storage_data_map.h"

#include "base/sys_info.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "content/common/dom_storage/dom_storage_types.h"

namespace content {

// static
scoped_refptr<SessionStorageDataMap> SessionStorageDataMap::Create(
    Listener* listener,
    scoped_refptr<SessionStorageMetadata::MapData> map_data,
    leveldb::mojom::LevelDBDatabase* database) {
  return base::WrapRefCounted(
      new SessionStorageDataMap(listener, std::move(map_data), database));
}

// static
scoped_refptr<SessionStorageDataMap> SessionStorageDataMap::CreateClone(
    Listener* listener,
    scoped_refptr<SessionStorageMetadata::MapData> map_data,
    LevelDBWrapperImpl* clone_from) {
  return base::WrapRefCounted(
      new SessionStorageDataMap(listener, std::move(map_data), clone_from));
}

std::vector<leveldb::mojom::BatchedOperationPtr>
SessionStorageDataMap::PrepareToCommit() {
  return std::vector<leveldb::mojom::BatchedOperationPtr>();
}

void SessionStorageDataMap::DidCommit(leveldb::mojom::DatabaseError error) {
  listener_->OnCommitResult(error);
}

SessionStorageDataMap::SessionStorageDataMap(
    Listener* listener,
    scoped_refptr<SessionStorageMetadata::MapData> map_data,
    leveldb::mojom::LevelDBDatabase* database)
    : listener_(listener),
      map_data_(std::move(map_data)),
      wrapper_impl_(std::make_unique<LevelDBWrapperImpl>(database,
                                                         map_data_->KeyPrefix(),
                                                         this,
                                                         GetOptions())),
      level_db_wrapper_ptr_(wrapper_impl_.get()) {
  DCHECK(listener_);
  DCHECK(map_data_);
  listener_->OnDataMapCreation(map_data_->MapNumberAsBytes(), this);
}

SessionStorageDataMap::SessionStorageDataMap(
    Listener* listener,
    scoped_refptr<SessionStorageMetadata::MapData> map_data,
    LevelDBWrapperImpl* forking_from)
    : listener_(listener),
      map_data_(std::move(map_data)),
      wrapper_impl_(forking_from->ForkToNewPrefix(map_data_->KeyPrefix(),
                                                  this,
                                                  GetOptions())),
      level_db_wrapper_ptr_(wrapper_impl_.get()) {
  DCHECK(listener_);
  DCHECK(map_data_);
  listener_->OnDataMapCreation(map_data_->MapNumberAsBytes(), this);
}

SessionStorageDataMap::~SessionStorageDataMap() {
  listener_->OnDataMapDestruction(map_data_->MapNumberAsBytes());
}

void SessionStorageDataMap::RemoveBindingReference() {
  DCHECK_GT(binding_count_, 0);
  --binding_count_;
  if (binding_count_ > 0)
    return;
  // Don't delete ourselves, but do schedule an immediate commit. Possible
  // deletion will happen under memory pressure or when another sessionstorage
  // area is opened.
  level_db_wrapper()->ScheduleImmediateCommit();
}

// static
LevelDBWrapperImpl::Options SessionStorageDataMap::GetOptions() {
  // Delay for a moment after a value is set in anticipation
  // of other values being set, so changes are batched.
  constexpr const base::TimeDelta kCommitDefaultDelaySecs =
      base::TimeDelta::FromSeconds(5);

  // To avoid excessive IO we apply limits to the amount of data being
  // written and the frequency of writes.
  LevelDBWrapperImpl::Options options;
  options.max_size = kPerStorageAreaQuota + kPerStorageAreaOverQuotaAllowance;
  options.default_commit_delay = kCommitDefaultDelaySecs;
  options.max_bytes_per_hour = kPerStorageAreaQuota;
  options.max_commits_per_hour = 60;
  options.cache_mode = LevelDBWrapperImpl::CacheMode::KEYS_ONLY_WHEN_POSSIBLE;
  return options;
}

}  // namespace content
