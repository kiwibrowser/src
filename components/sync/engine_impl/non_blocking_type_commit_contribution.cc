// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine_impl/non_blocking_type_commit_contribution.h"

#include <algorithm>

#include "base/guid.h"
#include "base/values.h"
#include "components/sync/base/time.h"
#include "components/sync/base/unique_position.h"
#include "components/sync/engine/non_blocking_sync_common.h"
#include "components/sync/engine_impl/model_type_worker.h"
#include "components/sync/protocol/proto_value_conversions.h"

namespace syncer {

NonBlockingTypeCommitContribution::NonBlockingTypeCommitContribution(
    ModelType type,
    const sync_pb::DataTypeContext& context,
    const CommitRequestDataList& commit_requests,
    ModelTypeWorker* worker,
    Cryptographer* cryptographer,
    DataTypeDebugInfoEmitter* debug_info_emitter,
    bool only_commit_specifics)
    : type_(type),
      worker_(worker),
      cryptographer_(cryptographer),
      context_(context),
      commit_requests_(commit_requests),
      cleaned_up_(false),
      debug_info_emitter_(debug_info_emitter),
      only_commit_specifics_(only_commit_specifics) {}

NonBlockingTypeCommitContribution::~NonBlockingTypeCommitContribution() {
  DCHECK(cleaned_up_);
}

void NonBlockingTypeCommitContribution::AddToCommitMessage(
    sync_pb::ClientToServerMessage* msg) {
  sync_pb::CommitMessage* commit_message = msg->mutable_commit();
  entries_start_index_ = commit_message->entries_size();

  commit_message->mutable_entries()->Reserve(commit_message->entries_size() +
                                             commit_requests_.size());

  for (const auto& commit_request : commit_requests_) {
    sync_pb::SyncEntity* sync_entity = commit_message->add_entries();
    if (only_commit_specifics_) {
      DCHECK(!commit_request.entity->is_deleted());
      DCHECK(!cryptographer_);
      // Only send specifics to server for commit-only types.
      sync_entity->mutable_specifics()->CopyFrom(
          commit_request.entity->specifics);
    } else {
      PopulateCommitProto(commit_request, sync_entity);
      AdjustCommitProto(sync_entity);
    }
  }

  if (!context_.context().empty())
    commit_message->add_client_contexts()->CopyFrom(context_);

  CommitCounters* counters = debug_info_emitter_->GetMutableCommitCounters();
  counters->num_commits_attempted += commit_requests_.size();
}

SyncerError NonBlockingTypeCommitContribution::ProcessCommitResponse(
    const sync_pb::ClientToServerResponse& response,
    StatusController* status) {
  const sync_pb::CommitResponse& commit_response = response.commit();

  bool unknown_error = false;
  int transient_error_commits = 0;
  int conflicting_commits = 0;
  int successes = 0;

  CommitResponseDataList response_list;

  for (size_t i = 0; i < commit_requests_.size(); ++i) {
    const sync_pb::CommitResponse_EntryResponse& entry_response =
        commit_response.entryresponse(entries_start_index_ + i);

    switch (entry_response.response_type()) {
      case sync_pb::CommitResponse::INVALID_MESSAGE:
        LOG(ERROR) << "Server reports commit message is invalid.";
        unknown_error = true;
        break;
      case sync_pb::CommitResponse::CONFLICT:
        DVLOG(1) << "Server reports conflict for commit message.";
        ++conflicting_commits;
        status->increment_num_server_conflicts();
        break;
      case sync_pb::CommitResponse::SUCCESS: {
        ++successes;
        CommitResponseData response_data;
        const CommitRequestData& commit_request = commit_requests_[i];
        response_data.id = entry_response.id_string();
        response_data.response_version = entry_response.version();
        response_data.client_tag_hash = commit_request.entity->client_tag_hash;
        response_data.sequence_number = commit_request.sequence_number;
        response_data.specifics_hash = commit_request.specifics_hash;
        response_list.push_back(response_data);

        status->increment_num_successful_commits();
        if (commit_request.entity->specifics.has_bookmark()) {
          status->increment_num_successful_bookmark_commits();
        }

        break;
      }
      case sync_pb::CommitResponse::OVER_QUOTA:
      case sync_pb::CommitResponse::RETRY:
      case sync_pb::CommitResponse::TRANSIENT_ERROR:
        DLOG(WARNING) << "Entity commit blocked by transient error.";
        ++transient_error_commits;
        break;
      default:
        LOG(ERROR) << "Bad return from ProcessSingleCommitResponse.";
        unknown_error = true;
    }
  }

  CommitCounters* counters = debug_info_emitter_->GetMutableCommitCounters();
  counters->num_commits_success += successes;
  counters->num_commits_conflict += transient_error_commits;
  counters->num_commits_error += transient_error_commits;

  // Send whatever successful responses we did get back to our parent.
  // It's the schedulers job to handle the failures.
  worker_->OnCommitResponse(&response_list);

  // Let the scheduler know about the failures.
  if (unknown_error) {
    return SERVER_RETURN_UNKNOWN_ERROR;
  } else if (transient_error_commits > 0) {
    return SERVER_RETURN_TRANSIENT_ERROR;
  } else if (conflicting_commits > 0) {
    return SERVER_RETURN_CONFLICT;
  } else {
    return SYNCER_OK;
  }
}

void NonBlockingTypeCommitContribution::CleanUp() {
  cleaned_up_ = true;

  debug_info_emitter_->EmitCommitCountersUpdate();
  debug_info_emitter_->EmitStatusCountersUpdate();
}

size_t NonBlockingTypeCommitContribution::GetNumEntries() const {
  return commit_requests_.size();
}

// static
void NonBlockingTypeCommitContribution::PopulateCommitProto(
    const CommitRequestData& commit_entity,
    sync_pb::SyncEntity* commit_proto) {
  const EntityData& entity_data = commit_entity.entity.value();
  commit_proto->set_id_string(entity_data.id);
  commit_proto->set_client_defined_unique_tag(entity_data.client_tag_hash);
  commit_proto->set_version(commit_entity.base_version);
  commit_proto->set_deleted(entity_data.is_deleted());
  commit_proto->set_folder(entity_data.is_folder);
  commit_proto->set_name(entity_data.non_unique_name);

  if (!entity_data.is_deleted()) {
    // Handle bookmarks separately.
    if (entity_data.specifics.has_bookmark()) {
      // position_in_parent field is set only for legacy reasons.  See comments
      // in sync.proto for more information.
      commit_proto->set_position_in_parent(
          syncer::UniquePosition::FromProto(entity_data.unique_position)
              .ToInt64());
      commit_proto->mutable_unique_position()->CopyFrom(
          entity_data.unique_position);
      // TODO(mamir): check if parent_id_string needs to be populated for
      // non-deletions.
      if (!entity_data.parent_id.empty()) {
        commit_proto->set_parent_id_string(entity_data.parent_id);
      }
    }
    commit_proto->set_ctime(TimeToProtoTime(entity_data.creation_time));
    commit_proto->set_mtime(TimeToProtoTime(entity_data.modification_time));
    commit_proto->mutable_specifics()->CopyFrom(entity_data.specifics);
  }
}

void NonBlockingTypeCommitContribution::AdjustCommitProto(
    sync_pb::SyncEntity* commit_proto) {
  // Initial commits need our help to generate a client ID.
  if (commit_proto->version() == kUncommittedVersion) {
    DCHECK(commit_proto->id_string().empty()) << commit_proto->id_string();
    // TODO(crbug.com/516866): This is incorrect for bookmarks for two reasons:
    // 1) Won't be able to match previously committed bookmarks to the ones
    //    with server ID.
    // 2) Recommitting an item in a case of failing to receive commit response
    //    would result in generating a different client ID, which in turn
    //    would result in a duplication.
    // We should generate client ID on the frontend side instead.
    commit_proto->set_id_string(base::GenerateGUID());
    commit_proto->set_version(0);
  } else {
    DCHECK(!commit_proto->id_string().empty());
  }

  // Encrypt the specifics and hide the title if necessary.
  if (cryptographer_) {
    if (commit_proto->has_specifics()) {
      sync_pb::EntitySpecifics encrypted_specifics;
      bool result = cryptographer_->Encrypt(
          commit_proto->specifics(), encrypted_specifics.mutable_encrypted());
      DCHECK(result);
      commit_proto->mutable_specifics()->CopyFrom(encrypted_specifics);
    }
    commit_proto->set_name("encrypted");
  }

  // Always include enough specifics to identify the type. Do this even in
  // deletion requests, where the specifics are otherwise invalid.
  AddDefaultFieldValue(type_, commit_proto->mutable_specifics());
}

}  // namespace syncer
