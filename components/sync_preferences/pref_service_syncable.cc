// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_preferences/pref_service_syncable.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/strings/string_number_conversions.h"
#include "base/value_conversions.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/default_pref_store.h"
#include "components/prefs/in_memory_pref_store.h"
#include "components/prefs/overlay_user_pref_store.h"
#include "components/prefs/pref_notifier_impl.h"
#include "components/prefs/pref_registry.h"
#include "components/sync_preferences/pref_model_associator.h"
#include "components/sync_preferences/pref_service_syncable_observer.h"
#include "services/preferences/public/cpp/in_process_service_factory.h"
#include "services/preferences/public/cpp/persistent_pref_store_client.h"
#include "services/preferences/public/cpp/pref_registry_serializer.h"
#include "services/preferences/public/mojom/preferences.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace sync_preferences {

PrefServiceSyncable::PrefServiceSyncable(
    std::unique_ptr<PrefNotifierImpl> pref_notifier,
    std::unique_ptr<PrefValueStore> pref_value_store,
    scoped_refptr<PersistentPrefStore> user_prefs,
    scoped_refptr<user_prefs::PrefRegistrySyncable> pref_registry,
    const PrefModelAssociatorClient* pref_model_associator_client,
    base::RepeatingCallback<void(PersistentPrefStore::PrefReadError)>
        read_error_callback,
    bool async)
    : PrefService(std::move(pref_notifier),
                  std::move(pref_value_store),
                  std::move(user_prefs),
                  std::move(pref_registry),
                  std::move(read_error_callback),
                  async),
      pref_service_forked_(false),
      pref_sync_associator_(pref_model_associator_client, syncer::PREFERENCES),
      priority_pref_sync_associator_(pref_model_associator_client,
                                     syncer::PRIORITY_PREFERENCES) {
  pref_sync_associator_.SetPrefService(this);
  priority_pref_sync_associator_.SetPrefService(this);

  // Let PrefModelAssociators know about changes to preference values.
  pref_value_store_->set_callback(base::Bind(
      &PrefServiceSyncable::ProcessPrefChange, base::Unretained(this)));

  // Add already-registered syncable preferences to PrefModelAssociator.
  for (const auto& entry : *pref_registry_) {
    const std::string& path = entry.first;
    AddRegisteredSyncablePreference(path,
                                    pref_registry_->GetRegistrationFlags(path));
  }

  // Watch for syncable preferences registered after this point.
  static_cast<user_prefs::PrefRegistrySyncable*>(pref_registry_.get())
      ->SetSyncableRegistrationCallback(base::BindRepeating(
          &PrefServiceSyncable::AddRegisteredSyncablePreference,
          base::Unretained(this)));
}

PrefServiceSyncable::~PrefServiceSyncable() {
  // Remove our callback from the registry, since it may outlive us.
  user_prefs::PrefRegistrySyncable* registry =
      static_cast<user_prefs::PrefRegistrySyncable*>(pref_registry_.get());
  registry->SetSyncableRegistrationCallback(
      user_prefs::PrefRegistrySyncable::SyncableRegistrationCallback());
}

std::unique_ptr<PrefServiceSyncable>
PrefServiceSyncable::CreateIncognitoPrefService(
    PrefStore* incognito_extension_pref_store,
    const std::vector<const char*>& overlay_pref_names,
    std::unique_ptr<PrefValueStore::Delegate> delegate) {
  pref_service_forked_ = true;
  auto pref_notifier = std::make_unique<PrefNotifierImpl>();

  scoped_refptr<user_prefs::PrefRegistrySyncable> forked_registry =
      static_cast<user_prefs::PrefRegistrySyncable*>(pref_registry_.get())
          ->ForkForIncognito();

  auto overlay = base::MakeRefCounted<InMemoryPrefStore>();
  if (delegate) {
    delegate->InitIncognitoUserPrefs(overlay, user_pref_store_,
                                     overlay_pref_names);
    delegate->InitPrefRegistry(forked_registry.get());
  }
  auto incognito_pref_store = base::MakeRefCounted<OverlayUserPrefStore>(
      overlay.get(), user_pref_store_.get());

  for (const char* overlay_pref_name : overlay_pref_names)
    incognito_pref_store->RegisterOverlayPref(overlay_pref_name);

  auto pref_value_store = pref_value_store_->CloneAndSpecialize(
      nullptr,  // managed
      nullptr,  // supervised_user
      incognito_extension_pref_store,
      nullptr,  // command_line_prefs
      incognito_pref_store.get(),
      nullptr,  // recommended
      forked_registry->defaults().get(), pref_notifier.get(),
      std::move(delegate));
  return std::make_unique<PrefServiceSyncable>(
      std::move(pref_notifier), std::move(pref_value_store),
      std::move(incognito_pref_store), std::move(forked_registry),
      pref_sync_associator_.client(), read_error_callback_, false);
}

bool PrefServiceSyncable::IsSyncing() {
  return pref_sync_associator_.models_associated();
}

bool PrefServiceSyncable::IsPrioritySyncing() {
  return priority_pref_sync_associator_.models_associated();
}

bool PrefServiceSyncable::IsPrefSynced(const std::string& name) const {
  return pref_sync_associator_.IsPrefSynced(name) ||
         priority_pref_sync_associator_.IsPrefSynced(name);
}

void PrefServiceSyncable::AddObserver(PrefServiceSyncableObserver* observer) {
  observer_list_.AddObserver(observer);
}

void PrefServiceSyncable::RemoveObserver(
    PrefServiceSyncableObserver* observer) {
  observer_list_.RemoveObserver(observer);
}

syncer::SyncableService* PrefServiceSyncable::GetSyncableService(
    const syncer::ModelType& type) {
  if (type == syncer::PREFERENCES) {
    return &pref_sync_associator_;
  } else if (type == syncer::PRIORITY_PREFERENCES) {
    return &priority_pref_sync_associator_;
  } else {
    NOTREACHED() << "invalid model type: " << type;
    return nullptr;
  }
}

void PrefServiceSyncable::UpdateCommandLinePrefStore(
    PrefStore* cmd_line_store) {
  // If |pref_service_forked_| is true, then this PrefService and the forked
  // copies will be out of sync.
  DCHECK(!pref_service_forked_);
  PrefService::UpdateCommandLinePrefStore(cmd_line_store);
}

void PrefServiceSyncable::AddSyncedPrefObserver(const std::string& name,
                                                SyncedPrefObserver* observer) {
  pref_sync_associator_.AddSyncedPrefObserver(name, observer);
  priority_pref_sync_associator_.AddSyncedPrefObserver(name, observer);
}

void PrefServiceSyncable::RemoveSyncedPrefObserver(
    const std::string& name,
    SyncedPrefObserver* observer) {
  pref_sync_associator_.RemoveSyncedPrefObserver(name, observer);
  priority_pref_sync_associator_.RemoveSyncedPrefObserver(name, observer);
}

void PrefServiceSyncable::RegisterMergeDataFinishedCallback(
    const base::Closure& callback) {
  pref_sync_associator_.RegisterMergeDataFinishedCallback(callback);
}

void PrefServiceSyncable::AddRegisteredSyncablePreference(
    const std::string& path,
    uint32_t flags) {
  DCHECK(FindPreference(path));
  if (flags & user_prefs::PrefRegistrySyncable::SYNCABLE_PREF) {
    pref_sync_associator_.RegisterPref(path.c_str());
  } else if (flags & user_prefs::PrefRegistrySyncable::SYNCABLE_PRIORITY_PREF) {
    priority_pref_sync_associator_.RegisterPref(path.c_str());
  }
}

void PrefServiceSyncable::OnIsSyncingChanged() {
  for (auto& observer : observer_list_)
    observer.OnIsSyncingChanged();
}

void PrefServiceSyncable::ProcessPrefChange(const std::string& name) {
  pref_sync_associator_.ProcessPrefChange(name);
  priority_pref_sync_associator_.ProcessPrefChange(name);
}

}  // namespace sync_preferences
