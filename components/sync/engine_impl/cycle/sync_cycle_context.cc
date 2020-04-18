// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/cycle/sync_cycle_context.h"

#include "components/sync/base/extensions_activity.h"

namespace syncer {

SyncCycleContext::SyncCycleContext(
    ServerConnectionManager* connection_manager,
    syncable::Directory* directory,
    ExtensionsActivity* extensions_activity,
    const std::vector<SyncEngineEventListener*>& listeners,
    DebugInfoGetter* debug_info_getter,
    ModelTypeRegistry* model_type_registry,
    bool keystore_encryption_enabled,
    bool client_enabled_pre_commit_update_avoidance,
    const std::string& invalidator_client_id,
    base::TimeDelta short_poll_interval,
    base::TimeDelta long_poll_interval)
    : connection_manager_(connection_manager),
      directory_(directory),
      extensions_activity_(extensions_activity),
      notifications_enabled_(false),
      max_commit_batch_size_(kDefaultMaxCommitBatchSize),
      debug_info_getter_(debug_info_getter),
      model_type_registry_(model_type_registry),
      keystore_encryption_enabled_(keystore_encryption_enabled),
      invalidator_client_id_(invalidator_client_id),
      server_enabled_pre_commit_update_avoidance_(false),
      client_enabled_pre_commit_update_avoidance_(
          client_enabled_pre_commit_update_avoidance),
      cookie_jar_mismatch_(false),
      cookie_jar_empty_(false),
      short_poll_interval_(short_poll_interval),
      long_poll_interval_(long_poll_interval) {
  DCHECK(!short_poll_interval.is_zero());
  DCHECK(!long_poll_interval.is_zero());
  std::vector<SyncEngineEventListener*>::const_iterator it;
  for (it = listeners.begin(); it != listeners.end(); ++it)
    listeners_.AddObserver(*it);
}

SyncCycleContext::~SyncCycleContext() {}

ModelTypeSet SyncCycleContext::GetEnabledTypes() const {
  return model_type_registry_->GetEnabledTypes();
}

}  // namespace syncer
