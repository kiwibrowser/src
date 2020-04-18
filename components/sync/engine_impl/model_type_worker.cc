// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/model_type_worker.h"

#include <stdint.h>

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/format_macros.h"
#include "base/guid.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/memory_usage_estimator.h"
#include "components/sync/base/cancelation_signal.h"
#include "components/sync/base/time.h"
#include "components/sync/engine/model_type_processor.h"
#include "components/sync/engine_impl/commit_contribution.h"
#include "components/sync/engine_impl/non_blocking_type_commit_contribution.h"
#include "components/sync/protocol/proto_memory_estimations.h"

namespace syncer {

ModelTypeWorker::ModelTypeWorker(
    ModelType type,
    const sync_pb::ModelTypeState& initial_state,
    bool trigger_initial_sync,
    std::unique_ptr<Cryptographer> cryptographer,
    NudgeHandler* nudge_handler,
    std::unique_ptr<ModelTypeProcessor> model_type_processor,
    DataTypeDebugInfoEmitter* debug_info_emitter,
    CancelationSignal* cancelation_signal)
    : type_(type),
      debug_info_emitter_(debug_info_emitter),
      model_type_state_(initial_state),
      model_type_processor_(std::move(model_type_processor)),
      cryptographer_(std::move(cryptographer)),
      nudge_handler_(nudge_handler),
      cancelation_signal_(cancelation_signal),
      weak_ptr_factory_(this) {
  DCHECK(model_type_processor_);

  // Request an initial sync if it hasn't been completed yet.
  if (trigger_initial_sync) {
    nudge_handler_->NudgeForInitialDownload(type_);
  }

  // This case handles the scenario where the processor has a serialized model
  // type state that has already done its initial sync, and is going to be
  // tracking metadata changes, however it does not have the most recent
  // encryption key name. The cryptographer was updated while the worker was not
  // around, and we're not going to receive the normal UpdateCryptographer() or
  // EncryptionAcceptedApplyUpdates() calls to drive this process.
  //
  // If |cryptographer_->is_ready()| is false, all the rest of this logic can be
  // safely skipped, since |UpdateCryptographer(...)| must be called first and
  // things should be driven normally after that.
  //
  // If |model_type_state_.initial_sync_done()| is false, |model_type_state_|
  // may still need to be updated, since UpdateCryptographer() is never going to
  // happen, but we can assume PassiveApplyUpdates(...) will push the state to
  // the processor, and we should not push it now. In fact, doing so now would
  // violate the processor's assumption that the first OnUpdateReceived is will
  // be changing initial sync done to true.
  if (cryptographer_ && cryptographer_->is_ready() &&
      UpdateEncryptionKeyName() && model_type_state_.initial_sync_done()) {
    ApplyPendingUpdates();
  }
}

ModelTypeWorker::~ModelTypeWorker() {
  model_type_processor_->DisconnectSync();
}

ModelType ModelTypeWorker::GetModelType() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return type_;
}

void ModelTypeWorker::UpdateCryptographer(
    std::unique_ptr<Cryptographer> cryptographer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(cryptographer);
  cryptographer_ = std::move(cryptographer);
  UpdateEncryptionKeyName();
  DecryptStoredEntities();
  NudgeIfReadyToCommit();
}

// UpdateHandler implementation.
bool ModelTypeWorker::IsInitialSyncEnded() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return model_type_state_.initial_sync_done();
}

void ModelTypeWorker::GetDownloadProgress(
    sync_pb::DataTypeProgressMarker* progress_marker) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  progress_marker->CopyFrom(model_type_state_.progress_marker());
}

void ModelTypeWorker::GetDataTypeContext(
    sync_pb::DataTypeContext* context) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  context->CopyFrom(model_type_state_.type_context());
}

SyncerError ModelTypeWorker::ProcessGetUpdatesResponse(
    const sync_pb::DataTypeProgressMarker& progress_marker,
    const sync_pb::DataTypeContext& mutated_context,
    const SyncEntityList& applicable_updates,
    StatusController* status) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // TODO(rlarocque): Handle data type context conflicts.
  *model_type_state_.mutable_type_context() = mutated_context;
  *model_type_state_.mutable_progress_marker() = progress_marker;

  UpdateCounters* counters = debug_info_emitter_->GetMutableUpdateCounters();
  counters->num_updates_received += applicable_updates.size();

  for (const sync_pb::SyncEntity* update_entity : applicable_updates) {
    if (update_entity->deleted()) {
      status->increment_num_tombstone_updates_downloaded_by(1);
      ++counters->num_tombstone_updates_received;
    }

    UpdateResponseData response_data;
    switch (PopulateUpdateResponseData(cryptographer_.get(), *update_entity,
                                       &response_data)) {
      case SUCCESS:
        pending_updates_.push_back(response_data);
        break;
      case DECRYPTION_PENDING:
        entries_pending_decryption_[update_entity->id_string()] = response_data;
        break;
      case FAILED_TO_DECRYPT:
        // Failed to decrypt the entity. Likely it is corrupt. Move on.
        break;
    }
  }

  debug_info_emitter_->EmitUpdateCountersUpdate();
  return SYNCER_OK;
}

// static
// |cryptographer| can be null.
// |response_data| must be not null.
ModelTypeWorker::DecryptionStatus ModelTypeWorker::PopulateUpdateResponseData(
    const Cryptographer* cryptographer,
    const sync_pb::SyncEntity& update_entity,
    UpdateResponseData* response_data) {
  response_data->response_version = update_entity.version();
  EntityData data;
  // Prepare the message for the model thread.
  data.id = update_entity.id_string();
  data.client_tag_hash = update_entity.client_defined_unique_tag();
  data.creation_time = ProtoTimeToTime(update_entity.ctime());
  data.modification_time = ProtoTimeToTime(update_entity.mtime());
  data.non_unique_name = update_entity.name();
  data.is_folder = update_entity.folder();
  data.parent_id = update_entity.parent_id_string();
  data.unique_position = update_entity.unique_position();
  data.server_defined_unique_tag = update_entity.server_defined_unique_tag();

  // Deleted entities must use the default instance of EntitySpecifics in
  // order for EntityData to correctly reflect that they are deleted.
  const sync_pb::EntitySpecifics& specifics =
      update_entity.deleted() ? sync_pb::EntitySpecifics::default_instance()
                              : update_entity.specifics();

  // Check if specifics are encrypted and try to decrypt if so.
  if (!specifics.has_encrypted()) {
    // No encryption.
    data.specifics = specifics;
    response_data->entity = data.PassToPtr();
    return SUCCESS;
  }
  if (cryptographer && cryptographer->CanDecrypt(specifics.encrypted())) {
    // Encrypted and we know the key.
    if (!DecryptSpecifics(*cryptographer, specifics, &data.specifics)) {
      return FAILED_TO_DECRYPT;
    }
    response_data->entity = data.PassToPtr();
    response_data->encryption_key_name = specifics.encrypted().key_name();
    return SUCCESS;
  }
  // Can't decrypt right now. Ask the entity tracker to handle it.
  data.specifics = specifics;
  response_data->entity = data.PassToPtr();
  return DECRYPTION_PENDING;
}

void ModelTypeWorker::ApplyUpdates(StatusController* status) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // This should only ever be called after one PassiveApplyUpdates.
  DCHECK(model_type_state_.initial_sync_done());
  // Download cycle is done, pass all updates to the processor.
  ApplyPendingUpdates();
}

void ModelTypeWorker::PassiveApplyUpdates(StatusController* status) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // This should only be called at the end of the very first download cycle.
  DCHECK(!model_type_state_.initial_sync_done());
  // Indicate to the processor that the initial download is done. The initial
  // sync technically isn't done yet but by the time this value is persisted to
  // disk on the model thread it will be.
  model_type_state_.set_initial_sync_done(true);
  ApplyPendingUpdates();
}

void ModelTypeWorker::EncryptionAcceptedMaybeApplyUpdates() {
  DCHECK(cryptographer_);
  DCHECK(cryptographer_->is_ready());

  // Only push the encryption to the processor if we're already connected.
  // Otherwise this information can wait for the initial sync's first apply.
  if (model_type_state_.initial_sync_done()) {
    // Reuse ApplyUpdates(...) to get its DCHECKs as well.
    ApplyUpdates(nullptr);
  }
}

void ModelTypeWorker::ApplyPendingUpdates() {
  if (BlockForEncryption())
    return;
  DVLOG(1) << ModelTypeToString(type_) << ": "
           << base::StringPrintf("Delivering %" PRIuS " applicable updates.",
                                 pending_updates_.size());

  DCHECK(entries_pending_decryption_.empty());

  model_type_processor_->OnUpdateReceived(model_type_state_, pending_updates_);

  UpdateCounters* counters = debug_info_emitter_->GetMutableUpdateCounters();
  counters->num_updates_applied += pending_updates_.size();
  debug_info_emitter_->EmitUpdateCountersUpdate();
  debug_info_emitter_->EmitStatusCountersUpdate();

  pending_updates_.clear();
}

void ModelTypeWorker::NudgeForCommit() {
  DCHECK(thread_checker_.CalledOnValidThread());
  has_local_changes_ = true;
  NudgeIfReadyToCommit();
}

void ModelTypeWorker::NudgeIfReadyToCommit() {
  if (has_local_changes_ && CanCommitItems())
    nudge_handler_->NudgeForCommit(GetModelType());
}

// CommitContributor implementation.
std::unique_ptr<CommitContribution> ModelTypeWorker::GetContribution(
    size_t max_entries) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(model_type_state_.initial_sync_done());
  // Early return if type is not ready to commit (initial sync isn't done or
  // cryptographer has pending keys).
  if (!CanCommitItems())
    return std::unique_ptr<CommitContribution>();

  // Client shouldn't be committing data to server when it hasn't processed all
  // updates it received.
  DCHECK(entries_pending_decryption_.empty());

  // Request model type for local changes.
  scoped_refptr<GetLocalChangesRequest> request =
      base::MakeRefCounted<GetLocalChangesRequest>(cancelation_signal_);
  // TODO(mamir): do we need to make this async?
  model_type_processor_->GetLocalChanges(
      max_entries, base::Bind(&GetLocalChangesRequest::SetResponse, request));
  request->WaitForResponse();
  CommitRequestDataList response;
  if (!request->WasCancelled())
    response = request->ExtractResponse();
  if (response.empty()) {
    has_local_changes_ = false;
    return std::unique_ptr<CommitContribution>();
  }

  DCHECK(response.size() <= max_entries);
  return std::make_unique<NonBlockingTypeCommitContribution>(
      GetModelType(), model_type_state_.type_context(), response, this,
      cryptographer_.get(), debug_info_emitter_,
      CommitOnlyTypes().Has(GetModelType()));
}

bool ModelTypeWorker::HasLocalChangesForTest() const {
  return has_local_changes_;
}

void ModelTypeWorker::OnCommitResponse(CommitResponseDataList* response_list) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Send the responses back to the model thread. It needs to know which
  // items have been successfully committed so it can save that information in
  // permanent storage.
  model_type_processor_->OnCommitCompleted(model_type_state_, *response_list);
}

void ModelTypeWorker::AbortMigration() {
  DCHECK(!model_type_state_.initial_sync_done());
  model_type_state_ = sync_pb::ModelTypeState();
  entries_pending_decryption_.clear();
  pending_updates_.clear();
  nudge_handler_->NudgeForInitialDownload(type_);
}

size_t ModelTypeWorker::EstimateMemoryUsage() const {
  using base::trace_event::EstimateMemoryUsage;
  size_t memory_usage = 0;
  memory_usage += EstimateMemoryUsage(model_type_state_);
  memory_usage += EstimateMemoryUsage(entries_pending_decryption_);
  memory_usage += EstimateMemoryUsage(pending_updates_);
  return memory_usage;
}

base::WeakPtr<ModelTypeWorker> ModelTypeWorker::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

bool ModelTypeWorker::IsTypeInitialized() const {
  return model_type_state_.initial_sync_done();
}

bool ModelTypeWorker::CanCommitItems() const {
  // We can only commit if we've received the initial update response and aren't
  // blocked by missing encryption keys.
  return IsTypeInitialized() && !BlockForEncryption();
}

bool ModelTypeWorker::BlockForEncryption() const {
  // Should be using encryption, but we do not have the keys.
  return cryptographer_ && !cryptographer_->is_ready();
}

bool ModelTypeWorker::UpdateEncryptionKeyName() {
  const std::string& new_key_name = cryptographer_->GetDefaultNigoriKeyName();
  const std::string& old_key_name = model_type_state_.encryption_key_name();
  if (old_key_name == new_key_name) {
    return false;
  }

  DVLOG(1) << ModelTypeToString(type_) << ": Updating encryption key "
           << old_key_name << " -> " << new_key_name;
  model_type_state_.set_encryption_key_name(new_key_name);
  return true;
}

void ModelTypeWorker::DecryptStoredEntities() {
  for (auto it = entries_pending_decryption_.begin();
       it != entries_pending_decryption_.end();) {
    const UpdateResponseData& encrypted_update = it->second;
    EntityDataPtr data = encrypted_update.entity;
    DCHECK(data->specifics.has_encrypted());

    sync_pb::EntitySpecifics specifics;
    if (!cryptographer_->CanDecrypt(data->specifics.encrypted()) ||
        !DecryptSpecifics(*cryptographer_, data->specifics, &specifics)) {
      ++it;
      continue;
    }

    UpdateResponseData decrypted_update;
    decrypted_update.response_version = encrypted_update.response_version;
    // Copy the encryption_key_name from data->specifics before it gets
    // overriden in data->UpdateSpecifics().
    decrypted_update.encryption_key_name =
        data->specifics.encrypted().key_name();
    decrypted_update.entity = data->UpdateSpecifics(specifics);
    pending_updates_.push_back(decrypted_update);
    it = entries_pending_decryption_.erase(it);
  }
}

// static
bool ModelTypeWorker::DecryptSpecifics(const Cryptographer& cryptographer,
                                       const sync_pb::EntitySpecifics& in,
                                       sync_pb::EntitySpecifics* out) {
  DCHECK(in.has_encrypted());
  DCHECK(cryptographer.CanDecrypt(in.encrypted()));

  std::string plaintext = cryptographer.DecryptToString(in.encrypted());
  if (plaintext.empty()) {
    LOG(ERROR) << "Failed to decrypt a decryptable entity";
    return false;
  }
  if (!out->ParseFromString(plaintext)) {
    LOG(ERROR) << "Failed to parse decrypted entity";
    return false;
  }
  return true;
}

GetLocalChangesRequest::GetLocalChangesRequest(
    CancelationSignal* cancelation_signal)
    : cancelation_signal_(cancelation_signal),
      response_accepted_(base::WaitableEvent::ResetPolicy::MANUAL,
                         base::WaitableEvent::InitialState::NOT_SIGNALED) {}

GetLocalChangesRequest::~GetLocalChangesRequest() {}

void GetLocalChangesRequest::OnSignalReceived() {
  response_accepted_.Signal();
}

void GetLocalChangesRequest::WaitForResponse() {
  if (!cancelation_signal_->TryRegisterHandler(this)) {
    return;
  }
  response_accepted_.Wait();
  cancelation_signal_->UnregisterHandler(this);
}

void GetLocalChangesRequest::SetResponse(
    CommitRequestDataList&& local_changes) {
  response_ = local_changes;
  response_accepted_.Signal();
}

bool GetLocalChangesRequest::WasCancelled() {
  return cancelation_signal_->IsSignalled();
}

CommitRequestDataList&& GetLocalChangesRequest::ExtractResponse() {
  return std::move(response_);
}

}  // namespace syncer
