// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine/engine_components_factory_impl.h"

#include <map>
#include <utility>

#include "components/sync/engine_impl/backoff_delay_provider.h"
#include "components/sync/engine_impl/cycle/sync_cycle_context.h"
#include "components/sync/engine_impl/sync_scheduler_impl.h"
#include "components/sync/engine_impl/syncer.h"
#include "components/sync/syncable/on_disk_directory_backing_store.h"

using base::TimeDelta;

namespace {
const int kShortNudgeDelayDurationMS = 1;
}

namespace syncer {

EngineComponentsFactoryImpl::EngineComponentsFactoryImpl(
    const Switches& switches)
    : switches_(switches) {}

EngineComponentsFactoryImpl::~EngineComponentsFactoryImpl() {}

std::unique_ptr<SyncScheduler> EngineComponentsFactoryImpl::BuildScheduler(
    const std::string& name,
    SyncCycleContext* context,
    CancelationSignal* cancelation_signal,
    bool ignore_auth_credentials) {
  std::unique_ptr<BackoffDelayProvider> delay(
      BackoffDelayProvider::FromDefaults());

  if (switches_.backoff_override == BACKOFF_SHORT_INITIAL_RETRY_OVERRIDE) {
    delay.reset(BackoffDelayProvider::WithShortInitialRetryOverride());
  }

  std::unique_ptr<SyncSchedulerImpl> scheduler =
      std::make_unique<SyncSchedulerImpl>(name, delay.release(), context,
                                          new Syncer(cancelation_signal),
                                          ignore_auth_credentials);
  if (switches_.nudge_delay == NudgeDelay::SHORT_NUDGE_DELAY) {
    // Set the default nudge delay to 0 because the default is used as a floor
    // for override values, and we don't want the below override to be ignored.
    scheduler->SetDefaultNudgeDelay(TimeDelta::FromMilliseconds(0));
    // Only protocol types can have their delay customized.
    ModelTypeSet protocol_types = syncer::ProtocolTypes();
    std::map<ModelType, base::TimeDelta> nudge_delays;
    for (ModelTypeSet::Iterator it = protocol_types.First(); it.Good();
         it.Inc()) {
      nudge_delays[it.Get()] =
          TimeDelta::FromMilliseconds(kShortNudgeDelayDurationMS);
    }
    scheduler->OnReceivedCustomNudgeDelays(nudge_delays);
  }
  return std::move(scheduler);
}

std::unique_ptr<SyncCycleContext> EngineComponentsFactoryImpl::BuildContext(
    ServerConnectionManager* connection_manager,
    syncable::Directory* directory,
    ExtensionsActivity* extensions_activity,
    const std::vector<SyncEngineEventListener*>& listeners,
    DebugInfoGetter* debug_info_getter,
    ModelTypeRegistry* model_type_registry,
    const std::string& invalidation_client_id,
    base::TimeDelta short_poll_interval,
    base::TimeDelta long_poll_interval) {
  return std::make_unique<SyncCycleContext>(
      connection_manager, directory, extensions_activity, listeners,
      debug_info_getter, model_type_registry,
      switches_.encryption_method == ENCRYPTION_KEYSTORE,
      switches_.pre_commit_updates_policy ==
          FORCE_ENABLE_PRE_COMMIT_UPDATE_AVOIDANCE,
      invalidation_client_id, short_poll_interval, long_poll_interval);
}

std::unique_ptr<syncable::DirectoryBackingStore>
EngineComponentsFactoryImpl::BuildDirectoryBackingStore(
    StorageOption storage,
    const std::string& dir_name,
    const base::FilePath& backing_filepath) {
  if (storage == STORAGE_ON_DISK) {
    return std::unique_ptr<syncable::DirectoryBackingStore>(
        new syncable::OnDiskDirectoryBackingStore(dir_name, backing_filepath));
  } else {
    NOTREACHED();
    return std::unique_ptr<syncable::DirectoryBackingStore>();
  }
}

EngineComponentsFactory::Switches EngineComponentsFactoryImpl::GetSwitches()
    const {
  return switches_;
}

}  // namespace syncer
