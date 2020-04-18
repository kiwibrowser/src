// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/test/fake_server/fake_server.h"

#include <algorithm>
#include <limits>
#include <set>
#include <utility>

#include "base/guid.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_restrictions.h"
#include "components/sync/engine_impl/net/server_connection_manager.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"

using syncer::GetModelType;
using syncer::GetModelTypeFromSpecifics;
using syncer::LoopbackServer;
using syncer::LoopbackServerEntity;
using syncer::ModelType;
using syncer::ModelTypeSet;

namespace fake_server {

FakeServer::FakeServer()
    : authenticated_(true),
      error_type_(sync_pb::SyncEnums::SUCCESS),
      alternate_triggered_errors_(false),
      request_counter_(0),
      network_enabled_(true),
      weak_ptr_factory_(this) {
  base::ThreadRestrictions::SetIOAllowed(true);
  loopback_server_storage_ = std::make_unique<base::ScopedTempDir>();
  if (!loopback_server_storage_->CreateUniqueTempDir()) {
    NOTREACHED() << "Creating temp dir failed.";
  }
  loopback_server_ = std::make_unique<syncer::LoopbackServer>(
      loopback_server_storage_->GetPath().AppendASCII("profile.pb"));
  loopback_server_->set_observer_for_tests(this);
}

FakeServer::~FakeServer() {}

void FakeServer::HandleCommand(const std::string& request,
                               const base::Closure& completion_closure,
                               int* error_code,
                               int* response_code,
                               std::string* response) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!network_enabled_) {
    *error_code = net::ERR_FAILED;
    *response_code = net::ERR_FAILED;
    *response = std::string();
    completion_closure.Run();
    return;
  }
  request_counter_++;

  if (!authenticated_) {
    *error_code = 0;
    *response_code = net::HTTP_UNAUTHORIZED;
    *response = std::string();
    completion_closure.Run();
    return;
  }

  sync_pb::ClientToServerResponse response_proto;
  *response_code = 200;
  *error_code = 0;
  if (error_type_ != sync_pb::SyncEnums::SUCCESS &&
      ShouldSendTriggeredError()) {
    response_proto.set_error_code(error_type_);
  } else if (triggered_actionable_error_.get() && ShouldSendTriggeredError()) {
    sync_pb::ClientToServerResponse_Error* error =
        response_proto.mutable_error();
    error->CopyFrom(*(triggered_actionable_error_.get()));
  } else {
    sync_pb::ClientToServerMessage message;
    bool parsed = message.ParseFromString(request);
    DCHECK(parsed) << "Unable to parse the ClientToServerMessage.";
    switch (message.message_contents()) {
      case sync_pb::ClientToServerMessage::GET_UPDATES:
        last_getupdates_message_ = message;
        break;
      case sync_pb::ClientToServerMessage::COMMIT:
        last_commit_message_ = message;
        break;
      default:
        break;
        // Don't care.
    }
    *response_code = SendToLoopbackServer(request, response);
    if (*response_code == net::HTTP_OK) {
      InjectClientCommand(response);
    }
    completion_closure.Run();
    return;
  }

  response_proto.set_store_birthday(loopback_server_->GetStoreBirthday());
  *response = response_proto.SerializeAsString();
  completion_closure.Run();
}

int FakeServer::SendToLoopbackServer(const std::string& request,
                                     std::string* response) {
  int64_t response_code;
  syncer::HttpResponse::ServerConnectionCode server_status;
  base::ThreadRestrictions::SetIOAllowed(true);
  loopback_server_->HandleCommand(request, &server_status, &response_code,
                                  response);
  return static_cast<int>(response_code);
}

void FakeServer::InjectClientCommand(std::string* response) {
  sync_pb::ClientToServerResponse response_proto;
  bool parse_ok = response_proto.ParseFromString(*response);
  DCHECK(parse_ok) << "Unable to parse-back the server response";
  if (response_proto.error_code() == sync_pb::SyncEnums::SUCCESS) {
    *response_proto.mutable_client_command() = client_command_;
    *response = response_proto.SerializeAsString();
  }
}

bool FakeServer::GetLastCommitMessage(sync_pb::ClientToServerMessage* message) {
  if (!last_commit_message_.has_commit())
    return false;

  message->CopyFrom(last_commit_message_);
  return true;
}

bool FakeServer::GetLastGetUpdatesMessage(
    sync_pb::ClientToServerMessage* message) {
  if (!last_getupdates_message_.has_get_updates())
    return false;

  message->CopyFrom(last_getupdates_message_);
  return true;
}

void FakeServer::OverrideResponseType(
    LoopbackServer::ResponseTypeProvider response_type_override) {
  loopback_server_->OverrideResponseType(std::move(response_type_override));
}

std::unique_ptr<base::DictionaryValue>
FakeServer::GetEntitiesAsDictionaryValue() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return loopback_server_->GetEntitiesAsDictionaryValue();
}

std::vector<sync_pb::SyncEntity> FakeServer::GetSyncEntitiesByModelType(
    ModelType model_type) {
  DCHECK(thread_checker_.CalledOnValidThread());
  return loopback_server_->GetSyncEntitiesByModelType(model_type);
}

void FakeServer::InjectEntity(std::unique_ptr<LoopbackServerEntity> entity) {
  DCHECK(thread_checker_.CalledOnValidThread());
  loopback_server_->SaveEntity(std::move(entity));
}

bool FakeServer::ModifyEntitySpecifics(
    const std::string& id,
    const sync_pb::EntitySpecifics& updated_specifics) {
  return loopback_server_->ModifyEntitySpecifics(id, updated_specifics);
}

bool FakeServer::ModifyBookmarkEntity(
    const std::string& id,
    const std::string& parent_id,
    const sync_pb::EntitySpecifics& updated_specifics) {
  return loopback_server_->ModifyBookmarkEntity(id, parent_id,
                                                updated_specifics);
}

void FakeServer::ClearServerData() {
  DCHECK(thread_checker_.CalledOnValidThread());
  loopback_server_->ClearServerData();
}

void FakeServer::SetAuthenticated() {
  DCHECK(thread_checker_.CalledOnValidThread());
  authenticated_ = true;
}

void FakeServer::SetUnauthenticated() {
  DCHECK(thread_checker_.CalledOnValidThread());
  authenticated_ = false;
}

void FakeServer::SetClientCommand(
    const sync_pb::ClientCommand& client_command) {
  DCHECK(thread_checker_.CalledOnValidThread());
  client_command_ = client_command;
}

bool FakeServer::TriggerError(const sync_pb::SyncEnums::ErrorType& error_type) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (triggered_actionable_error_) {
    DVLOG(1) << "Only one type of error can be triggered at any given time.";
    return false;
  }

  error_type_ = error_type;
  return true;
}

bool FakeServer::TriggerActionableError(
    const sync_pb::SyncEnums::ErrorType& error_type,
    const std::string& description,
    const std::string& url,
    const sync_pb::SyncEnums::Action& action) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (error_type_ != sync_pb::SyncEnums::SUCCESS) {
    DVLOG(1) << "Only one type of error can be triggered at any given time.";
    return false;
  }

  sync_pb::ClientToServerResponse_Error* error =
      new sync_pb::ClientToServerResponse_Error();
  error->set_error_type(error_type);
  error->set_error_description(description);
  error->set_url(url);
  error->set_action(action);
  triggered_actionable_error_.reset(error);
  return true;
}

bool FakeServer::EnableAlternatingTriggeredErrors() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (error_type_ == sync_pb::SyncEnums::SUCCESS &&
      !triggered_actionable_error_) {
    DVLOG(1) << "No triggered error set. Alternating can't be enabled.";
    return false;
  }

  alternate_triggered_errors_ = true;
  // Reset the counter so that the the first request yields a triggered error.
  request_counter_ = 0;
  return true;
}

bool FakeServer::ShouldSendTriggeredError() const {
  if (!alternate_triggered_errors_)
    return true;

  // Check that the counter is odd so that we trigger an error on the first
  // request after alternating is enabled.
  return request_counter_ % 2 != 0;
}

void FakeServer::AddObserver(Observer* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  observers_.AddObserver(observer);
}

void FakeServer::RemoveObserver(Observer* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  observers_.RemoveObserver(observer);
}

void FakeServer::OnCommit(const std::string& committer_id,
                          syncer::ModelTypeSet committed_model_types) {
  for (auto& observer : observers_)
    observer.OnCommit(committer_id, committed_model_types);
}

void FakeServer::EnableNetwork() {
  DCHECK(thread_checker_.CalledOnValidThread());
  network_enabled_ = true;
}

void FakeServer::DisableNetwork() {
  DCHECK(thread_checker_.CalledOnValidThread());
  network_enabled_ = false;
}

base::WeakPtr<FakeServer> FakeServer::AsWeakPtr() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace fake_server
