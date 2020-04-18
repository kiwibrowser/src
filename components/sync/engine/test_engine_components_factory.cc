// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine/test_engine_components_factory.h"

#include "components/sync/engine_impl/cycle/sync_cycle_context.h"
#include "components/sync/syncable/in_memory_directory_backing_store.h"
#include "components/sync/syncable/invalid_directory_backing_store.h"
#include "components/sync/syncable/on_disk_directory_backing_store.h"
#include "components/sync/test/engine/fake_sync_scheduler.h"

namespace syncer {

TestEngineComponentsFactory::TestEngineComponentsFactory(
    const Switches& switches,
    StorageOption option,
    StorageOption* storage_used)
    : switches_(switches),
      storage_override_(option),
      storage_used_(storage_used) {}

TestEngineComponentsFactory::~TestEngineComponentsFactory() {}

std::unique_ptr<SyncScheduler> TestEngineComponentsFactory::BuildScheduler(
    const std::string& name,
    SyncCycleContext* context,
    CancelationSignal* cancelation_signal,
    bool ignore_auth_credentials) {
  return std::unique_ptr<SyncScheduler>(new FakeSyncScheduler());
}

std::unique_ptr<SyncCycleContext> TestEngineComponentsFactory::BuildContext(
    ServerConnectionManager* connection_manager,
    syncable::Directory* directory,
    ExtensionsActivity* monitor,
    const std::vector<SyncEngineEventListener*>& listeners,
    DebugInfoGetter* debug_info_getter,
    ModelTypeRegistry* model_type_registry,
    const std::string& invalidator_client_id,
    base::TimeDelta short_poll_interval,
    base::TimeDelta long_poll_interval) {
  // Tests don't wire up listeners.
  std::vector<SyncEngineEventListener*> empty_listeners;
  return std::unique_ptr<SyncCycleContext>(new SyncCycleContext(
      connection_manager, directory, monitor, empty_listeners,
      debug_info_getter, model_type_registry,
      switches_.encryption_method == ENCRYPTION_KEYSTORE,
      switches_.pre_commit_updates_policy ==
          FORCE_ENABLE_PRE_COMMIT_UPDATE_AVOIDANCE,
      invalidator_client_id, short_poll_interval, long_poll_interval));
}

std::unique_ptr<syncable::DirectoryBackingStore>
TestEngineComponentsFactory::BuildDirectoryBackingStore(
    StorageOption storage,
    const std::string& dir_name,
    const base::FilePath& backing_filepath) {
  if (storage_used_)
    *storage_used_ = storage;

  switch (storage_override_) {
    case STORAGE_IN_MEMORY:
      return std::unique_ptr<syncable::DirectoryBackingStore>(
          new syncable::InMemoryDirectoryBackingStore(dir_name));
    case STORAGE_ON_DISK:
      return std::unique_ptr<syncable::DirectoryBackingStore>(
          new syncable::OnDiskDirectoryBackingStore(dir_name,
                                                    backing_filepath));
    case STORAGE_INVALID:
      return std::unique_ptr<syncable::DirectoryBackingStore>(
          new syncable::InvalidDirectoryBackingStore());
  }
  NOTREACHED();
  return std::unique_ptr<syncable::DirectoryBackingStore>();
}

EngineComponentsFactory::Switches TestEngineComponentsFactory::GetSwitches()
    const {
  return switches_;
}

}  // namespace syncer
