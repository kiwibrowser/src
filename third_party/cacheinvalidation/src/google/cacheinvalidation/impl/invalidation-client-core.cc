// Copyright 2012 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Implementation of the Invalidation Client Library (Ticl).

#include "google/cacheinvalidation/impl/invalidation-client-core.h"

#include <stddef.h>

#include <sstream>

#include "google/cacheinvalidation/client_test_internal.pb.h"
#include "google/cacheinvalidation/deps/callback.h"
#include "google/cacheinvalidation/deps/random.h"
#include "google/cacheinvalidation/deps/sha1-digest-function.h"
#include "google/cacheinvalidation/deps/string_util.h"
#include "google/cacheinvalidation/impl/exponential-backoff-delay-generator.h"
#include "google/cacheinvalidation/impl/invalidation-client-util.h"
#include "google/cacheinvalidation/impl/log-macro.h"
#include "google/cacheinvalidation/impl/persistence-utils.h"
#include "google/cacheinvalidation/impl/proto-converter.h"
#include "google/cacheinvalidation/impl/proto-helpers.h"
#include "google/cacheinvalidation/impl/recurring-task.h"
#include "google/cacheinvalidation/impl/smearer.h"

namespace invalidation {

using ::ipc::invalidation::RegistrationManagerStateP;

const char* InvalidationClientCore::kClientTokenKey = "ClientToken";

// AcquireTokenTask

AcquireTokenTask::AcquireTokenTask(InvalidationClientCore* client)
    : RecurringTask(
        "AcquireToken",
        client->internal_scheduler_,
        client->logger_,
        &client->smearer_,
        client->CreateExpBackOffGenerator(TimeDelta::FromMilliseconds(
            client->config_.network_timeout_delay_ms())),
        Scheduler::NoDelay(),
        TimeDelta::FromMilliseconds(
            client->config_.network_timeout_delay_ms())),
      client_(client) {
  }

bool AcquireTokenTask::RunTask() {
  // If token is still not assigned (as expected), sends a request.
  // Otherwise, ignore.
  if (client_->client_token_.empty()) {
    // Allocate a nonce and send a message requesting a new token.
    client_->set_nonce(
        InvalidationClientCore::GenerateNonce(client_->random_.get()));

    client_->protocol_handler_.SendInitializeMessage(
        client_->application_client_id_, client_->nonce_,
        client_->batching_task_.get(),
        "AcquireToken");
    // Reschedule to check state, retry if necessary after timeout.
    return true;
  } else {
    return false;  // Don't reschedule.
  }
}

// RegSyncHeartbeatTask

RegSyncHeartbeatTask::RegSyncHeartbeatTask(InvalidationClientCore* client)
    : RecurringTask(
        "RegSyncHeartbeat",
        client->internal_scheduler_,
        client->logger_,
        &client->smearer_,
        client->CreateExpBackOffGenerator(TimeDelta::FromMilliseconds(
            client->config_.network_timeout_delay_ms())),
        TimeDelta::FromMilliseconds(
            client->config_.network_timeout_delay_ms()),
        TimeDelta::FromMilliseconds(
            client->config_.network_timeout_delay_ms())),
      client_(client) {
}

bool RegSyncHeartbeatTask::RunTask() {
  if (!client_->registration_manager_.IsStateInSyncWithServer()) {
    // Simply send an info message to ensure syncing happens.
    TLOG(client_->logger_, INFO, "Registration state not in sync with "
         "server: %s", client_->registration_manager_.ToString().c_str());
    client_->SendInfoMessageToServer(false, true /* request server summary */);
    return true;
  } else {
    TLOG(client_->logger_, INFO, "Not sending message since state is in sync");
    return false;
  }
}

// PersistentWriteTask

PersistentWriteTask::PersistentWriteTask(InvalidationClientCore* client)
    : RecurringTask(
        "PersistentWrite",
        client->internal_scheduler_,
        client->logger_,
        &client->smearer_,
        client->CreateExpBackOffGenerator(TimeDelta::FromMilliseconds(
            client->config_.write_retry_delay_ms())),
        Scheduler::NoDelay(),
        TimeDelta::FromMilliseconds(
            client->config_.write_retry_delay_ms())),
      client_(client) {
}

bool PersistentWriteTask::RunTask() {
  if (client_->client_token_.empty() ||
      (client_->client_token_ == last_written_token_)) {
    // No work to be done
    return false;  // Do not reschedule
  }

  // Persistent write needs to happen.
  PersistentTiclState state;
  state.set_client_token(client_->client_token_);
  string serialized_state;
  PersistenceUtils::SerializeState(state, client_->digest_fn_.get(),
      &serialized_state);
  client_->storage_->WriteKey(InvalidationClientCore::kClientTokenKey,
      serialized_state,
      NewPermanentCallback(this, &PersistentWriteTask::WriteCallback,
          client_->client_token_));
  return true;  // Reschedule after timeout to make sure that write does happen.
}

void PersistentWriteTask::WriteCallback(const string& token, Status status) {
  TLOG(client_->logger_, INFO, "Write state completed: %d, %s",
       status.IsSuccess(), status.message().c_str());
  if (status.IsSuccess()) {
    // Set lastWrittenToken to be the token that was written (NOT client_token_:
    // which could have changed while the write was happening).
    last_written_token_ = token;
  } else {
    client_->statistics_->RecordError(
        Statistics::ClientErrorType_PERSISTENT_WRITE_FAILURE);
  }
}

// HeartbeatTask

HeartbeatTask::HeartbeatTask(InvalidationClientCore* client)
    : RecurringTask(
        "Heartbeat",
        client->internal_scheduler_,
        client->logger_,
        &client->smearer_,
        NULL,
        TimeDelta::FromMilliseconds(
            client->config_.heartbeat_interval_ms()),
        Scheduler::NoDelay()),
      client_(client) {
  next_performance_send_time_ = client_->internal_scheduler_->GetCurrentTime() +
      smearer()->GetSmearedDelay(TimeDelta::FromMilliseconds(
          client_->config_.perf_counter_delay_ms()));
}

bool HeartbeatTask::RunTask() {
  // Send info message. If needed, send performance counters and reset the next
  // performance counter send time.
  TLOG(client_->logger_, INFO, "Sending heartbeat to server: %s",
       client_->ToString().c_str());
  Scheduler *scheduler = client_->internal_scheduler_;
  bool must_send_perf_counters =
      next_performance_send_time_ > scheduler->GetCurrentTime();
  if (must_send_perf_counters) {
    next_performance_send_time_ = scheduler->GetCurrentTime() +
        client_->smearer_.GetSmearedDelay(TimeDelta::FromMilliseconds(
            client_->config_.perf_counter_delay_ms()));
  }

  TLOG(client_->logger_, INFO, "Sending heartbeat to server: %s",
       client_->ToString().c_str());
  client_->SendInfoMessageToServer(must_send_perf_counters,
      !client_->registration_manager_.IsStateInSyncWithServer());
  return true;  // Reschedule.
}

BatchingTask::BatchingTask(
    ProtocolHandler *handler, Smearer* smearer, TimeDelta batching_delay)
    : RecurringTask(
        "Batching", handler->internal_scheduler_, handler->logger_, smearer,
        NULL,  batching_delay, Scheduler::NoDelay()),
        protocol_handler_(handler) {
}

bool BatchingTask::RunTask() {
  // Send message to server - the batching information is picked up in
  // SendMessageToServer.
  protocol_handler_->SendMessageToServer();
  return false;  // Don't reschedule.
}

InvalidationClientCore::InvalidationClientCore(
    SystemResources* resources, Random* random, int client_type,
    const string& client_name, const ClientConfigP& config,
    const string& application_name)
    : resources_(resources),
      internal_scheduler_(resources->internal_scheduler()),
      logger_(resources->logger()),
      storage_(new SafeStorage(resources->storage())),
      statistics_(new Statistics()),
      config_(config),
      digest_fn_(new Sha1DigestFunction()),
      registration_manager_(logger_, statistics_.get(), digest_fn_.get()),
      msg_validator_(new TiclMessageValidator(logger_)),
      smearer_(random, config.smear_percent()),
      protocol_handler_(config.protocol_handler_config(), resources, &smearer_,
          statistics_.get(), client_type, application_name, this,
          msg_validator_.get()),
      is_online_(true),
      random_(random) {
  storage_.get()->SetSystemResources(resources_);
  application_client_id_.set_client_name(client_name);
  application_client_id_.set_client_type(client_type);
  CreateSchedulingTasks();
  RegisterWithNetwork(resources);
  TLOG(logger_, INFO, "Created client: %s", ToString().c_str());
}

void InvalidationClientCore::RegisterWithNetwork(SystemResources* resources) {
  // Install ourselves as a receiver for server messages.
  resources->network()->SetMessageReceiver(
      NewPermanentCallback(this, &InvalidationClientCore::MessageReceiver));

  resources->network()->AddNetworkStatusReceiver(
      NewPermanentCallback(this,
                           &InvalidationClientCore::NetworkStatusReceiver));
}

void InvalidationClientCore::CreateSchedulingTasks() {
  acquire_token_task_.reset(new AcquireTokenTask(this));
  reg_sync_heartbeat_task_.reset(new RegSyncHeartbeatTask(this));
  persistent_write_task_.reset(new PersistentWriteTask(this));
  heartbeat_task_.reset(new HeartbeatTask(this));
  batching_task_.reset(new BatchingTask(&protocol_handler_,
      &smearer_,
      TimeDelta::FromMilliseconds(
          config_.protocol_handler_config().batching_delay_ms())));
}

void InvalidationClientCore::InitConfig(ClientConfigP* config) {
  ProtoHelpers::InitConfigVersion(config->mutable_version());
  ProtocolHandler::InitConfig(config->mutable_protocol_handler_config());
}

void InvalidationClientCore::InitConfigForTest(ClientConfigP* config) {
  ProtoHelpers::InitConfigVersion(config->mutable_version());
  config->set_network_timeout_delay_ms(2000);
  config->set_heartbeat_interval_ms(5000);
  config->set_write_retry_delay_ms(500);
  ProtocolHandler::InitConfigForTest(config->mutable_protocol_handler_config());
}

void InvalidationClientCore::Start() {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  if (ticl_state_.IsStarted()) {
    TLOG(logger_, SEVERE,
         "Ignoring start call since already started: client = %s",
         this->ToString().c_str());
    return;
  }

  // Initialize the nonce so that we can maintain the invariant that exactly
  // one of "nonce_" and "client_token_" is non-empty.
  set_nonce(InvalidationClientCore::GenerateNonce(random_.get()));

  TLOG(logger_, INFO, "Starting with C++ config: %s",
       ProtoHelpers::ToString(config_).c_str());

  // Read the state blob and then schedule startInternal once the value is
  // there.
  ScheduleStartAfterReadingStateBlob();
}

void InvalidationClientCore::StartInternal(const string& serialized_state) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";

  CHECK(resources_->IsStarted()) << "Resources must be started before starting "
      "the Ticl";

  // Initialize the session manager using the persisted client token.
  PersistentTiclState persistent_state;
  bool deserialized = false;
  if (!serialized_state.empty()) {
    deserialized = PersistenceUtils::DeserializeState(
        logger_, serialized_state, digest_fn_.get(), &persistent_state);
  }

  if (!serialized_state.empty() && !deserialized) {
    // In this case, we'll proceed as if we had no persistent state -- i.e.,
    // obtain a new client id from the server.
    statistics_->RecordError(
        Statistics::ClientErrorType_PERSISTENT_DESERIALIZATION_FAILURE);
    TLOG(logger_, SEVERE, "Failed deserializing persistent state: %s",
         ProtoHelpers::ToString(serialized_state).c_str());
  }
  if (deserialized) {
    // If we have persistent state, use the previously-stored token and send a
    // heartbeat to let the server know that we've restarted, since we may have
    // been marked offline.
    //
    // In the common case, the server will already have all of our
    // registrations, but we won't know for sure until we've gotten its summary.
    // We'll ask the application for all of its registrations, but to avoid
    // making the registrar redo the work of performing registrations that
    // probably already exist, we'll suppress sending them to the registrar.
    TLOG(logger_, INFO, "Restarting from persistent state: %s",
         ProtoHelpers::ToString(
             persistent_state.client_token()).c_str());
    set_nonce("");
    set_client_token(persistent_state.client_token());
    should_send_registrations_ = false;

    // Schedule an info message for the near future. We delay a little bit to
    // allow the application to reissue its registrations locally and avoid
    // triggering registration sync with the data center due to a hash mismatch.
    internal_scheduler_->Schedule(TimeDelta::FromMilliseconds(
        config_.initial_persistent_heartbeat_delay_ms()),
        NewPermanentCallback(this,
            &InvalidationClientCore::SendInfoMessageToServer, false, true));

    // We need to ensure that heartbeats are sent, regardless of whether we
    // start fresh or from persistent state.  The line below ensures that they
    // are scheduled in the persistent startup case.  For the other case, the
    // task is scheduled when we acquire a token.
    heartbeat_task_.get()->EnsureScheduled("Startup-after-persistence");
  } else {
    // If we had no persistent state or couldn't deserialize the state that we
    // had, start fresh.  Request a new client identifier.
    //
    // The server can't possibly have our registrations, so whatever we get
    // from the application we should send to the registrar.
    TLOG(logger_, INFO, "Starting with no previous state");
    should_send_registrations_ = true;
    ScheduleAcquireToken("Startup");
  }
  // InvalidationListener.Ready() is called when the ticl has acquired a
  // new token.
}

void InvalidationClientCore::Stop() {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  TLOG(logger_, WARNING, "Ticl being stopped: %s", ToString().c_str());
  if (ticl_state_.IsStarted()) {
    ticl_state_.Stop();
  }
}

void InvalidationClientCore::Register(const ObjectId& object_id) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  vector<ObjectId> object_ids;
  object_ids.push_back(object_id);
  PerformRegisterOperations(object_ids, RegistrationP_OpType_REGISTER);
}

void InvalidationClientCore::Unregister(const ObjectId& object_id) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  vector<ObjectId> object_ids;
  object_ids.push_back(object_id);
  PerformRegisterOperations(object_ids, RegistrationP_OpType_UNREGISTER);
}

void InvalidationClientCore::PerformRegisterOperations(
    const vector<ObjectId>& object_ids, RegistrationP::OpType reg_op_type) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  CHECK(!object_ids.empty()) << "Must specify some object id";

  if (ticl_state_.IsStopped()) {
    // The Ticl has been stopped. This might be some old registration op
    // coming in. Just ignore instead of crashing.
    TLOG(logger_, SEVERE, "Ticl stopped: register (%d) of %d objects ignored.",
         reg_op_type, object_ids.size());
    return;
  }
  if (!ticl_state_.IsStarted()) {
    // We must be in the NOT_STARTED state, since we can't be in STOPPED or
    // STARTED (since the previous if-check didn't succeeded, and isStarted uses
    // a != STARTED test).
    TLOG(logger_, SEVERE,
        "Ticl is not yet started; failing registration call; client = %s, "
         "num-objects = %d, op = %d",
        this->ToString().c_str(), object_ids.size(), reg_op_type);
    for (size_t i = 0; i < object_ids.size(); ++i) {
      const ObjectId& object_id = object_ids[i];
      GetListener()->InformRegistrationFailure(this, object_id, true,
                                               "Client not yet ready");
    }
    return;
  }

  vector<ObjectIdP> object_id_protos;
  for (size_t i = 0; i < object_ids.size(); ++i) {
    const ObjectId& object_id = object_ids[i];
    ObjectIdP object_id_proto;
    ProtoConverter::ConvertToObjectIdProto(object_id, &object_id_proto);
    Statistics::IncomingOperationType op_type =
        (reg_op_type == RegistrationP_OpType_REGISTER) ?
        Statistics::IncomingOperationType_REGISTRATION :
        Statistics::IncomingOperationType_UNREGISTRATION;
    statistics_->RecordIncomingOperation(op_type);
    TLOG(logger_, INFO, "Register %s, %d",
         ProtoHelpers::ToString(object_id_proto).c_str(), reg_op_type);
    object_id_protos.push_back(object_id_proto);
  }


  // Update the registration manager state, then have the protocol client send a
  // message.
  vector<ObjectIdP> object_id_protos_to_send;
  registration_manager_.PerformOperations(object_id_protos, reg_op_type,
      &object_id_protos_to_send);

  // Check whether we should suppress sending registrations because we don't
  // yet know the server's summary.
  if (should_send_registrations_ && (!object_id_protos_to_send.empty())) {
    protocol_handler_.SendRegistrations(
        object_id_protos_to_send, reg_op_type, batching_task_.get());
  }
  reg_sync_heartbeat_task_.get()->EnsureScheduled("PerformRegister");
}

void InvalidationClientCore::Acknowledge(const AckHandle& acknowledge_handle) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  if (acknowledge_handle.IsNoOp()) {
    // Nothing to do. We do not increment statistics here since this is a no op
    // handle and statistics can only be acccessed on the scheduler thread.
    return;
  }
  // Validate the ack handle.

  // 1. Parse the ack handle first.
  AckHandleP ack_handle;
  ack_handle.ParseFromString(acknowledge_handle.handle_data());
  if (!ack_handle.IsInitialized()) {
    TLOG(logger_, WARNING, "Bad ack handle : %s",
         ProtoHelpers::ToString(acknowledge_handle.handle_data()).c_str());
    statistics_->RecordError(
        Statistics::ClientErrorType_ACKNOWLEDGE_HANDLE_FAILURE);
    return;
  }

  // 2. Validate ack handle - it should have a valid invalidation.
  if (!ack_handle.has_invalidation()
      || !msg_validator_->IsValid(ack_handle.invalidation())) {
    TLOG(logger_, WARNING, "Incorrect ack handle: %s",
         ProtoHelpers::ToString(ack_handle).c_str());
    statistics_->RecordError(
        Statistics::ClientErrorType_ACKNOWLEDGE_HANDLE_FAILURE);
    return;
  }

  // Currently, only invalidations have non-trivial ack handle.
  InvalidationP* invalidation = ack_handle.mutable_invalidation();
  invalidation->clear_payload();  // Don't send the payload back.
  statistics_->RecordIncomingOperation(
      Statistics::IncomingOperationType_ACKNOWLEDGE);
  protocol_handler_.SendInvalidationAck(*invalidation, batching_task_.get());
}

string InvalidationClientCore::ToString() {
  return StringPrintf("Client: %s, %s, %s",
                      ProtoHelpers::ToString(application_client_id_).c_str(),
                      ProtoHelpers::ToString(client_token_).c_str(),
                     this->ticl_state_.ToString().c_str());
}

string InvalidationClientCore::GetClientToken() {
  CHECK(client_token_.empty() || nonce_.empty());
  TLOG(logger_, FINE, "Return client token = %s",
       ProtoHelpers::ToString(client_token_).c_str());
  return client_token_;
}

void InvalidationClientCore::HandleIncomingMessage(const string& message) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  statistics_->RecordReceivedMessage(
          Statistics::ReceivedMessageType_TOTAL);
  ParsedMessage parsed_message;
  if (!protocol_handler_.HandleIncomingMessage(message, &parsed_message)) {
    // Invalid message.
    return;
  }

  // Ensure we have either a matching token or a matching nonce.
  if (!ValidateToken(parsed_message.header.token())) {
    return;
  }

  // Handle a token control message, if present.
  if (parsed_message.token_control_message != NULL) {
    statistics_->RecordReceivedMessage(
        Statistics::ReceivedMessageType_TOKEN_CONTROL);
    HandleTokenChanged(parsed_message.header.token(),
        parsed_message.token_control_message->new_token());
  }

  // We might have lost our token or failed to acquire one. Ensure that we do
  // not proceed in either case.
  // Note that checking for the presence of a TokenControlMessage is *not*
  // sufficient: it might be a token-assign with the wrong nonce or a
  // token-destroy message, for example.
  if (client_token_.empty()) {
    return;
  }

  // Handle the messages received from the server by calling the appropriate
  // listener method.

  // In the beginning inform the listener about the header (the caller is
  // already prepared to handle the fact that the same header is given to
  // it multiple times).
  HandleIncomingHeader(parsed_message.header);

  if (parsed_message.invalidation_message != NULL) {
    statistics_->RecordReceivedMessage(
        Statistics::ReceivedMessageType_INVALIDATION);
    HandleInvalidations(parsed_message.invalidation_message->invalidation());
  }
  if (parsed_message.registration_status_message != NULL) {
    statistics_->RecordReceivedMessage(
        Statistics::ReceivedMessageType_REGISTRATION_STATUS);
    HandleRegistrationStatus(
        parsed_message.registration_status_message->registration_status());
  }
  if (parsed_message.registration_sync_request_message != NULL) {
    statistics_->RecordReceivedMessage(
        Statistics::ReceivedMessageType_REGISTRATION_SYNC_REQUEST);
    HandleRegistrationSyncRequest();
  }
  if (parsed_message.info_request_message != NULL) {
    statistics_->RecordReceivedMessage(
        Statistics::ReceivedMessageType_INFO_REQUEST);
    HandleInfoMessage(
        // Shouldn't have to do this, but the proto compiler generates bad code
        // for repeated enum fields.
        *reinterpret_cast<const RepeatedField<InfoRequestMessage_InfoType>* >(
            &parsed_message.info_request_message->info_type()));
  }
  if (parsed_message.error_message != NULL) {
    statistics_->RecordReceivedMessage(
        Statistics::ReceivedMessageType_ERROR);
    HandleErrorMessage(
        parsed_message.error_message->code(),
        parsed_message.error_message->description());
  }
}

void InvalidationClientCore::HandleTokenChanged(
    const string& header_token, const string& new_token) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";

  // The server is either supplying a new token in response to an
  // InitializeMessage, spontaneously destroying a token we hold, or
  // spontaneously upgrading a token we hold.

  if (!new_token.empty()) {
    // Note: header_token cannot be empty, so an empty nonce or client_token will
    // always be non-equal.
    bool header_token_matches_nonce = header_token == nonce_;
    bool header_token_matches_existing_token = header_token == client_token_;
    bool should_accept_token =
        header_token_matches_nonce || header_token_matches_existing_token;
    if (!should_accept_token) {
      TLOG(logger_, INFO, "Ignoring new token; %s does not match nonce = %s "
           "or existing token = %s",
           ProtoHelpers::ToString(new_token).c_str(),
           ProtoHelpers::ToString(nonce_).c_str(),
           ProtoHelpers::ToString(client_token_).c_str());
      return;
    }
    TLOG(logger_, INFO, "New token being assigned at client: %s, Old = %s",
        ProtoHelpers::ToString(new_token).c_str(),
        ProtoHelpers::ToString(client_token_).c_str());
    heartbeat_task_.get()->EnsureScheduled("Heartbeat-after-new-token");
    set_nonce("");
    set_client_token(new_token);
    persistent_write_task_.get()->EnsureScheduled("Write-after-new-token");
  } else {
    // Destroy the existing token.
    TLOG(logger_, INFO, "Destroying existing token: %s",
            ProtoHelpers::ToString(client_token_).c_str());
    ScheduleAcquireToken("Destroy");
  }
}

void InvalidationClientCore::ScheduleAcquireToken(const string& debug_string) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  set_client_token("");
  acquire_token_task_.get()->EnsureScheduled(debug_string);
}

void InvalidationClientCore::HandleInvalidations(
    const RepeatedPtrField<InvalidationP>& invalidations) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";

  for (int i = 0; i < invalidations.size(); ++i) {
    const InvalidationP& invalidation = invalidations.Get(i);
    AckHandleP ack_handle_proto;
    ack_handle_proto.mutable_invalidation()->CopyFrom(invalidation);
    string serialized;
    ack_handle_proto.SerializeToString(&serialized);
    AckHandle ack_handle(serialized);
    if (ProtoConverter::IsAllObjectIdP(invalidation.object_id())) {
      TLOG(logger_, INFO, "Issuing invalidate all");
      GetListener()->InvalidateAll(this, ack_handle);
    } else {
      // Regular object. Could be unknown version or not.
      Invalidation inv;
      ProtoConverter::ConvertFromInvalidationProto(invalidation, &inv);
      bool isSuppressed = invalidation.is_trickle_restart();
      TLOG(logger_, INFO, "Issuing invalidate: %s",
           ProtoHelpers::ToString(invalidation).c_str());

      // Issue invalidate if the invalidation had a known version AND either
      // no suppression has occurred or the client allows suppression.
      if (invalidation.is_known_version() &&
          (!isSuppressed || config_.allow_suppression())) {
        GetListener()->Invalidate(this, inv, ack_handle);
      } else {
        // Unknown version
        GetListener()->InvalidateUnknownVersion(this,
                                                inv.object_id(), ack_handle);
      }
    }
  }
}

void InvalidationClientCore::HandleRegistrationStatus(
    const RepeatedPtrField<RegistrationStatus>& reg_status_list) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";

  vector<bool> local_processing_statuses;
  registration_manager_.HandleRegistrationStatus(
      reg_status_list, &local_processing_statuses);
  CHECK(local_processing_statuses.size() ==
        static_cast<size_t>(reg_status_list.size())) <<
      "Not all registration statuses were processed";

  // Inform app about the success or failure of each registration based
  // on what the registration manager has indicated.
  for (int i = 0; i < reg_status_list.size(); ++i) {
    const RegistrationStatus& reg_status = reg_status_list.Get(i);
    bool was_success = local_processing_statuses[i];
    TLOG(logger_, FINE, "Process reg status: %s",
         ProtoHelpers::ToString(reg_status).c_str());

    ObjectId object_id;
    ProtoConverter::ConvertFromObjectIdProto(
        reg_status.registration().object_id(), &object_id);
    if (was_success) {
      // Server operation was both successful and agreed with what the client
      // wanted.
      RegistrationP::OpType reg_op_type = reg_status.registration().op_type();
      InvalidationListener::RegistrationState reg_state =
          ConvertOpTypeToRegState(reg_op_type);
      GetListener()->InformRegistrationStatus(this, object_id, reg_state);
    } else {
      // Server operation either failed or disagreed with client's intent (e.g.,
      // successful unregister, but the client wanted a registration).
      string description =
          (reg_status.status().code() == StatusP_Code_SUCCESS) ?
              "Registration discrepancy detected" :
              reg_status.status().description();
      bool is_permanent =
          (reg_status.status().code() == StatusP_Code_PERMANENT_FAILURE);
      GetListener()->InformRegistrationFailure(
          this, object_id, !is_permanent, description);
    }
  }
}

void InvalidationClientCore::HandleRegistrationSyncRequest() {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  // Send all the registrations in the reg sync message.
  // Generate a single subtree for all the registrations.
  RegistrationSubtree subtree;
  registration_manager_.GetRegistrations("", 0, &subtree);
  protocol_handler_.SendRegistrationSyncSubtree(subtree, batching_task_.get());
}

void InvalidationClientCore::HandleInfoMessage(
    const RepeatedField<InfoRequestMessage_InfoType>& info_types) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  bool must_send_performance_counters = false;
  for (int i = 0; i < info_types.size(); ++i) {
    must_send_performance_counters =
        (info_types.Get(i) ==
         InfoRequestMessage_InfoType_GET_PERFORMANCE_COUNTERS);
    if (must_send_performance_counters) {
      break;
    }
  }
  SendInfoMessageToServer(must_send_performance_counters,
                          !registration_manager_.IsStateInSyncWithServer());
}

void InvalidationClientCore::HandleErrorMessage(
      ErrorMessage::Code code,
      const string& description) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";

  // If it is an auth failure, we shut down the ticl.
  TLOG(logger_, SEVERE, "Received error message: %s, %s",
         ProtoHelpers::ToString(code).c_str(),
         description.c_str());

  // Translate the code to error reason.
  int reason;
  switch (code) {
    case ErrorMessage_Code_AUTH_FAILURE:
      reason = ErrorReason::AUTH_FAILURE;
      break;
    case ErrorMessage_Code_UNKNOWN_FAILURE:
      reason = ErrorReason::UNKNOWN_FAILURE;
      break;
    default:
      reason = ErrorReason::UNKNOWN_FAILURE;
      break;
  }
  // Issue an informError to the application.
  ErrorInfo error_info(reason, false, description, ErrorContext());
  GetListener()->InformError(this, error_info);

  // If this is an auth failure, remove registrations and stop the Ticl.
  // Otherwise do nothing.
  if (code != ErrorMessage_Code_AUTH_FAILURE) {
    return;
  }

  // If there are any registrations, remove them and issue registration
  // failure.
  vector<ObjectIdP> desired_registrations;
  registration_manager_.RemoveRegisteredObjects(&desired_registrations);
  TLOG(logger_, WARNING, "Issuing failure for %d objects",
       desired_registrations.size());
  for (size_t i = 0; i < desired_registrations.size(); ++i) {
    ObjectId object_id;
    ProtoConverter::ConvertFromObjectIdProto(
        desired_registrations[i], &object_id);
    GetListener()->InformRegistrationFailure(
        this, object_id, false, "Auth error");
  }
}

void InvalidationClientCore::HandleMessageSent() {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  last_message_send_time_ = internal_scheduler_->GetCurrentTime();
}

void InvalidationClientCore::HandleNetworkStatusChange(bool is_online) {
  // If we're back online and haven't sent a message to the server in a while,
  // send a heartbeat to make sure the server knows we're online.
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  bool was_online = is_online_;
  is_online_ = is_online;
  if (is_online && !was_online &&
      (internal_scheduler_->GetCurrentTime() >
       last_message_send_time_ + TimeDelta::FromMilliseconds(
           config_.offline_heartbeat_threshold_ms()))) {
    TLOG(logger_, INFO,
         "Sending heartbeat after reconnection; previous send was %s ms ago",
         SimpleItoa(
             (internal_scheduler_->GetCurrentTime() - last_message_send_time_)
             .InMilliseconds()).c_str());
    SendInfoMessageToServer(
        false, !registration_manager_.IsStateInSyncWithServer());
  }
}

void InvalidationClientCore::GetRegistrationManagerStateAsSerializedProto(
    string* result) {
  RegistrationManagerStateP reg_state;
  registration_manager_.GetClientSummary(reg_state.mutable_client_summary());
  registration_manager_.GetServerSummary(reg_state.mutable_server_summary());
  vector<ObjectIdP> registered_objects;
  registration_manager_.GetRegisteredObjectsForTest(&registered_objects);
  for (size_t i = 0; i < registered_objects.size(); ++i) {
    reg_state.add_registered_objects()->CopyFrom(registered_objects[i]);
  }
  reg_state.SerializeToString(result);
}

void InvalidationClientCore::GetStatisticsAsSerializedProto(
    string* result) {
  vector<pair<string, int> > properties;
  statistics_->GetNonZeroStatistics(&properties);
  InfoMessage info_message;
  for (size_t i = 0; i < properties.size(); ++i) {
    PropertyRecord* record = info_message.add_performance_counter();
    record->set_name(properties[i].first);
    record->set_value(properties[i].second);
  }
  info_message.SerializeToString(result);
}

void InvalidationClientCore::HandleIncomingHeader(
    const ServerMessageHeader& header) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  CHECK(nonce_.empty()) <<
      "Cannot process server header " << header.ToString() <<
      " with non-empty nonce " << nonce_;

  if (header.registration_summary() != NULL) {
    // We've received a summary from the server, so if we were suppressing
    // registrations, we should now allow them to go to the registrar.
    should_send_registrations_ = true;


    // Pass the registration summary to the registration manager. If we are now
    // in agreement with the server and we had any pending operations, we can
    // tell the listener that those operations have succeeded.
    vector<RegistrationP> upcalls;
    registration_manager_.InformServerRegistrationSummary(
        *header.registration_summary(), &upcalls);
    TLOG(logger_, FINE,
        "Receivced new server registration summary (%s); will make %d upcalls",
         ProtoHelpers::ToString(*header.registration_summary()).c_str(),
         upcalls.size());
    vector<RegistrationP>::iterator iter;
    for (iter = upcalls.begin(); iter != upcalls.end(); iter++) {
      const RegistrationP& registration = *iter;
      ObjectId object_id;
      ProtoConverter::ConvertFromObjectIdProto(registration.object_id(),
                                               &object_id);
      InvalidationListener::RegistrationState reg_state =
          ConvertOpTypeToRegState(registration.op_type());
      GetListener()->InformRegistrationStatus(this, object_id, reg_state);
    }
  }
}

bool InvalidationClientCore::ValidateToken(const string& server_token) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  if (!client_token_.empty()) {
    // Client token case.
    if (client_token_ != server_token) {
      TLOG(logger_, INFO, "Incoming message has bad token: %s, %s",
               ProtoHelpers::ToString(client_token_).c_str(),
               ProtoHelpers::ToString(server_token).c_str());
          statistics_->RecordError(Statistics::ClientErrorType_TOKEN_MISMATCH);
          return false;
    }
    return true;
  } else if (!nonce_.empty()) {
    // Nonce case.
    CHECK(!nonce_.empty()) << "Client token and nonce are both empty: "
        << client_token_ << ", " << nonce_;
    if (nonce_ != server_token) {
      statistics_->RecordError(Statistics::ClientErrorType_NONCE_MISMATCH);
      TLOG(logger_, INFO,
           "Rejecting server message with mismatched nonce: Client = %s, "
           "Server = %s", ProtoHelpers::ToString(nonce_).c_str(),
           ProtoHelpers::ToString(server_token).c_str());
      return false;
    } else {
      TLOG(logger_, INFO,
          "Accepting server message with matching nonce: %s",
          ProtoHelpers::ToString(nonce_).c_str());
      return true;
    }
  }
  // Neither token nor nonce; ignore message.
  return false;
}

void InvalidationClientCore::SendInfoMessageToServer(
    bool must_send_performance_counters, bool request_server_summary) {
  TLOG(logger_, INFO,
       "Sending info message to server; request server summary = %s",
       request_server_summary ? "true" : "false");
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";

  // Make sure that you have the latest registration summary.
  vector<pair<string, int> > performance_counters;
  ClientConfigP* config_to_send = NULL;
  if (must_send_performance_counters) {
    statistics_->GetNonZeroStatistics(&performance_counters);
    config_to_send = &config_;
  }
  protocol_handler_.SendInfoMessage(performance_counters, config_to_send,
      request_server_summary, batching_task_.get());
}

string InvalidationClientCore::GenerateNonce(Random* random) {
  // Return a nonce computed by converting a random 64-bit number to a string.
  return SimpleItoa(static_cast<int64_t>(random->RandUint64()));
}

void InvalidationClientCore::set_nonce(const string& new_nonce) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  CHECK(new_nonce.empty() || client_token_.empty()) <<
      "Tried to set nonce with existing token " << client_token_;
  nonce_ = new_nonce;
}

void InvalidationClientCore::set_client_token(const string& new_client_token) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  CHECK(new_client_token.empty() || nonce_.empty()) <<
      "Tried to set token with existing nonce " << nonce_;

  // If the ticl has not been started and we are getting a new token (either
  // from persistence or from the server, start the ticl and inform the
  // application.
  bool finish_starting_ticl = !ticl_state_.IsStarted() &&
      client_token_.empty() && !new_client_token.empty();
  client_token_ = new_client_token;

  if (finish_starting_ticl) {
    FinishStartingTiclAndInformListener();
  }
}

void InvalidationClientCore::FinishStartingTiclAndInformListener() {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  CHECK(!ticl_state_.IsStarted());

  ticl_state_.Start();
  GetListener()->Ready(this);

  // We are not currently persisting our registration digest, so regardless of
  // whether or not we are restarting from persistent state, we need to query
  // the application for all of its registrations.
  GetListener()->ReissueRegistrations(this,
                                      RegistrationManager::kEmptyPrefix, 0);
  TLOG(logger_, INFO, "Ticl started: %s", ToString().c_str());
}

void InvalidationClientCore::ScheduleStartAfterReadingStateBlob() {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  storage_->ReadKey(kClientTokenKey,
      NewPermanentCallback(this, &InvalidationClientCore::ReadCallback));
}

void InvalidationClientCore::ReadCallback(
    pair<Status, string> read_result) {
  string serialized_state;
  if (read_result.first.IsSuccess()) {
    serialized_state = read_result.second;
  } else {
    statistics_->RecordError(
        Statistics::ClientErrorType_PERSISTENT_READ_FAILURE);
    TLOG(logger_, WARNING, "Could not read state blob: %s",
         read_result.first.message().c_str());
  }
  // Call start now.
  internal_scheduler_->Schedule(
      Scheduler::NoDelay(),
      NewPermanentCallback(
          this, &InvalidationClientCore::StartInternal, serialized_state));
}

ExponentialBackoffDelayGenerator*
InvalidationClientCore::CreateExpBackOffGenerator(
    const TimeDelta& initial_delay) {
  return new ExponentialBackoffDelayGenerator(random_.get(), initial_delay,
      config_.max_exponential_backoff_factor());
}

InvalidationListener::RegistrationState
InvalidationClientCore::ConvertOpTypeToRegState(RegistrationP::OpType
    reg_op_type) {
  InvalidationListener::RegistrationState reg_state =
      reg_op_type == RegistrationP_OpType_REGISTER ?
      InvalidationListener::REGISTERED :
      InvalidationListener::UNREGISTERED;
  return reg_state;
}

void InvalidationClientCore::MessageReceiver(string message) {
  internal_scheduler_->Schedule(Scheduler::NoDelay(), NewPermanentCallback(
      this,
      &InvalidationClientCore::HandleIncomingMessage, message));
}

void InvalidationClientCore::NetworkStatusReceiver(bool status) {
  internal_scheduler_->Schedule(Scheduler::NoDelay(), NewPermanentCallback(
      this, &InvalidationClientCore::HandleNetworkStatusChange, status));
}


void InvalidationClientCore::ChangeNetworkTimeoutDelayForTest(
    const TimeDelta& delay) {
  config_.set_network_timeout_delay_ms(delay.InMilliseconds());
  CreateSchedulingTasks();
}

void InvalidationClientCore::ChangeHeartbeatDelayForTest(
    const TimeDelta& delay) {
  config_.set_heartbeat_interval_ms(delay.InMilliseconds());
  CreateSchedulingTasks();
}

}  // namespace invalidation
