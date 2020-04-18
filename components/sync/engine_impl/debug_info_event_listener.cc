// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/debug_info_event_listener.h"

#include <stddef.h>

#include "components/sync/base/cryptographer.h"

namespace syncer {

DebugInfoEventListener::DebugInfoEventListener()
    : events_dropped_(false),
      cryptographer_has_pending_keys_(false),
      cryptographer_ready_(false),
      weak_ptr_factory_(this) {}

DebugInfoEventListener::~DebugInfoEventListener() {}

void DebugInfoEventListener::OnSyncCycleCompleted(
    const SyncCycleSnapshot& snapshot) {
  DCHECK(thread_checker_.CalledOnValidThread());
  sync_pb::DebugEventInfo event_info;
  sync_pb::SyncCycleCompletedEventInfo* sync_completed_event_info =
      event_info.mutable_sync_cycle_completed_event_info();

  sync_completed_event_info->set_num_encryption_conflicts(
      snapshot.num_encryption_conflicts());
  sync_completed_event_info->set_num_hierarchy_conflicts(
      snapshot.num_hierarchy_conflicts());
  sync_completed_event_info->set_num_server_conflicts(
      snapshot.num_server_conflicts());

  sync_completed_event_info->set_num_updates_downloaded(
      snapshot.model_neutral_state().num_updates_downloaded_total);
  sync_completed_event_info->set_num_reflected_updates_downloaded(
      snapshot.model_neutral_state().num_reflected_updates_downloaded_total);
  sync_completed_event_info->set_get_updates_origin(
      snapshot.get_updates_origin());
  sync_completed_event_info->mutable_caller_info()->set_notifications_enabled(
      snapshot.notifications_enabled());

  // Fill the legacy GetUpdatesSource field. This is not used anymore, but it's
  // a required field so we still have to fill it with something.
  sync_completed_event_info->mutable_caller_info()->set_source(
      sync_pb::GetUpdatesCallerInfo::UNKNOWN);

  AddEventToQueue(event_info);
}

void DebugInfoEventListener::OnInitializationComplete(
    const WeakHandle<JsBackend>& js_backend,
    const WeakHandle<DataTypeDebugInfoListener>& debug_listener,
    bool success,
    ModelTypeSet restored_types) {
  DCHECK(thread_checker_.CalledOnValidThread());
  CreateAndAddEvent(sync_pb::SyncEnums::INITIALIZATION_COMPLETE);
}

void DebugInfoEventListener::OnConnectionStatusChange(ConnectionStatus status) {
  DCHECK(thread_checker_.CalledOnValidThread());
  CreateAndAddEvent(sync_pb::SyncEnums::CONNECTION_STATUS_CHANGE);
}

void DebugInfoEventListener::OnPassphraseRequired(
    PassphraseRequiredReason reason,
    const sync_pb::EncryptedData& pending_keys) {
  DCHECK(thread_checker_.CalledOnValidThread());
  CreateAndAddEvent(sync_pb::SyncEnums::PASSPHRASE_REQUIRED);
}

void DebugInfoEventListener::OnPassphraseAccepted() {
  DCHECK(thread_checker_.CalledOnValidThread());
  CreateAndAddEvent(sync_pb::SyncEnums::PASSPHRASE_ACCEPTED);
}

void DebugInfoEventListener::OnBootstrapTokenUpdated(
    const std::string& bootstrap_token,
    BootstrapTokenType type) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (type == PASSPHRASE_BOOTSTRAP_TOKEN) {
    CreateAndAddEvent(sync_pb::SyncEnums::BOOTSTRAP_TOKEN_UPDATED);
    return;
  }
  DCHECK_EQ(type, KEYSTORE_BOOTSTRAP_TOKEN);
  CreateAndAddEvent(sync_pb::SyncEnums::KEYSTORE_TOKEN_UPDATED);
}

void DebugInfoEventListener::OnEncryptedTypesChanged(
    ModelTypeSet encrypted_types,
    bool encrypt_everything) {
  DCHECK(thread_checker_.CalledOnValidThread());
  CreateAndAddEvent(sync_pb::SyncEnums::ENCRYPTED_TYPES_CHANGED);
}

void DebugInfoEventListener::OnEncryptionComplete() {
  DCHECK(thread_checker_.CalledOnValidThread());
  CreateAndAddEvent(sync_pb::SyncEnums::ENCRYPTION_COMPLETE);
}

void DebugInfoEventListener::OnCryptographerStateChanged(
    Cryptographer* cryptographer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  cryptographer_has_pending_keys_ = cryptographer->has_pending_keys();
  cryptographer_ready_ = cryptographer->is_ready();
}

void DebugInfoEventListener::OnPassphraseTypeChanged(
    PassphraseType type,
    base::Time explicit_passphrase_time) {
  DCHECK(thread_checker_.CalledOnValidThread());
  CreateAndAddEvent(sync_pb::SyncEnums::PASSPHRASE_TYPE_CHANGED);
}

void DebugInfoEventListener::OnLocalSetPassphraseEncryption(
    const SyncEncryptionHandler::NigoriState& nigori_state) {}

void DebugInfoEventListener::OnActionableError(
    const SyncProtocolError& sync_error) {
  DCHECK(thread_checker_.CalledOnValidThread());
  CreateAndAddEvent(sync_pb::SyncEnums::ACTIONABLE_ERROR);
}

void DebugInfoEventListener::OnMigrationRequested(ModelTypeSet types) {}

void DebugInfoEventListener::OnProtocolEvent(const ProtocolEvent& event) {}

void DebugInfoEventListener::OnNudgeFromDatatype(ModelType datatype) {
  DCHECK(thread_checker_.CalledOnValidThread());
  sync_pb::DebugEventInfo event_info;
  event_info.set_nudging_datatype(
      GetSpecificsFieldNumberFromModelType(datatype));
  AddEventToQueue(event_info);
}

void DebugInfoEventListener::GetDebugInfo(sync_pb::DebugInfo* debug_info) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK_LE(events_.size(), kMaxEntries);

  for (DebugEventInfoQueue::const_iterator iter = events_.begin();
       iter != events_.end(); ++iter) {
    sync_pb::DebugEventInfo* event_info = debug_info->add_events();
    event_info->CopyFrom(*iter);
  }

  debug_info->set_events_dropped(events_dropped_);
  debug_info->set_cryptographer_ready(cryptographer_ready_);
  debug_info->set_cryptographer_has_pending_keys(
      cryptographer_has_pending_keys_);
}

void DebugInfoEventListener::ClearDebugInfo() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK_LE(events_.size(), kMaxEntries);

  events_.clear();
  events_dropped_ = false;
}

base::WeakPtr<DataTypeDebugInfoListener> DebugInfoEventListener::GetWeakPtr() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return weak_ptr_factory_.GetWeakPtr();
}

void DebugInfoEventListener::OnDataTypeConfigureComplete(
    const std::vector<DataTypeConfigurationStats>& configuration_stats) {
  DCHECK(thread_checker_.CalledOnValidThread());

  for (size_t i = 0; i < configuration_stats.size(); ++i) {
    DCHECK(ProtocolTypes().Has(configuration_stats[i].model_type));
    const DataTypeAssociationStats& association_stats =
        configuration_stats[i].association_stats;

    sync_pb::DebugEventInfo association_event;
    sync_pb::DatatypeAssociationStats* datatype_stats =
        association_event.mutable_datatype_association_stats();
    datatype_stats->set_data_type_id(GetSpecificsFieldNumberFromModelType(
        configuration_stats[i].model_type));
    datatype_stats->set_num_local_items_before_association(
        association_stats.num_local_items_before_association);
    datatype_stats->set_num_sync_items_before_association(
        association_stats.num_sync_items_before_association);
    datatype_stats->set_num_local_items_after_association(
        association_stats.num_local_items_after_association);
    datatype_stats->set_num_sync_items_after_association(
        association_stats.num_sync_items_after_association);
    datatype_stats->set_num_local_items_added(
        association_stats.num_local_items_added);
    datatype_stats->set_num_local_items_deleted(
        association_stats.num_local_items_deleted);
    datatype_stats->set_num_local_items_modified(
        association_stats.num_local_items_modified);
    datatype_stats->set_num_sync_items_added(
        association_stats.num_sync_items_added);
    datatype_stats->set_num_sync_items_deleted(
        association_stats.num_sync_items_deleted);
    datatype_stats->set_num_sync_items_modified(
        association_stats.num_sync_items_modified);
    datatype_stats->set_local_version_pre_association(
        association_stats.local_version_pre_association);
    datatype_stats->set_sync_version_pre_association(
        association_stats.sync_version_pre_association);
    datatype_stats->set_had_error(association_stats.had_error);
    datatype_stats->set_association_wait_time_for_same_priority_us(
        association_stats.association_wait_time.InMicroseconds());
    datatype_stats->set_association_time_us(
        association_stats.association_time.InMicroseconds());
    datatype_stats->set_download_wait_time_us(
        configuration_stats[i].download_wait_time.InMicroseconds());
    datatype_stats->set_download_time_us(
        configuration_stats[i].download_time.InMicroseconds());
    datatype_stats->set_association_wait_time_for_high_priority_us(
        configuration_stats[i]
            .association_wait_time_for_high_priority.InMicroseconds());

    for (ModelTypeSet::Iterator it =
             configuration_stats[i]
                 .high_priority_types_configured_before.First();
         it.Good(); it.Inc()) {
      datatype_stats->add_high_priority_type_configured_before(
          GetSpecificsFieldNumberFromModelType(it.Get()));
    }

    for (ModelTypeSet::Iterator it =
             configuration_stats[i]
                 .same_priority_types_configured_before.First();
         it.Good(); it.Inc()) {
      datatype_stats->add_same_priority_type_configured_before(
          GetSpecificsFieldNumberFromModelType(it.Get()));
    }

    AddEventToQueue(association_event);
  }
}

void DebugInfoEventListener::CreateAndAddEvent(
    sync_pb::SyncEnums::SingletonDebugEventType type) {
  DCHECK(thread_checker_.CalledOnValidThread());
  sync_pb::DebugEventInfo event_info;
  event_info.set_singleton_event(type);
  AddEventToQueue(event_info);
}

void DebugInfoEventListener::AddEventToQueue(
    const sync_pb::DebugEventInfo& event_info) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (events_.size() >= kMaxEntries) {
    DVLOG(1) << "DebugInfoEventListener::AddEventToQueue Dropping an old event "
             << "because of full queue";

    events_.pop_front();
    events_dropped_ = true;
  }
  events_.push_back(event_info);
}

}  // namespace syncer
