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

// Client for interacting with low-level protocol messages.

#include "google/cacheinvalidation/impl/protocol-handler.h"

#include <stddef.h>

#include "google/cacheinvalidation/deps/string_util.h"
#include "google/cacheinvalidation/impl/constants.h"
#include "google/cacheinvalidation/impl/invalidation-client-core.h"
#include "google/cacheinvalidation/impl/log-macro.h"
#include "google/cacheinvalidation/impl/proto-helpers.h"
#include "google/cacheinvalidation/impl/recurring-task.h"

namespace invalidation {

using ::ipc::invalidation::ConfigChangeMessage;
using ::ipc::invalidation::InfoMessage;
using ::ipc::invalidation::InitializeMessage;
using ::ipc::invalidation::InitializeMessage_DigestSerializationType_BYTE_BASED;
using ::ipc::invalidation::InvalidationMessage;
using ::ipc::invalidation::PropertyRecord;
using ::ipc::invalidation::RegistrationMessage;
using ::ipc::invalidation::RegistrationSyncMessage;
using ::ipc::invalidation::ServerHeader;
using ::ipc::invalidation::ServerToClientMessage;
using ::ipc::invalidation::TokenControlMessage;

string ServerMessageHeader::ToString() const {
  return StringPrintf(
      "Token: %s, Summary: %s", ProtoHelpers::ToString(*token_).c_str(),
      ProtoHelpers::ToString(*registration_summary_).c_str());
}

void ParsedMessage::InitFrom(const ServerToClientMessage& raw_message) {
  base_message = raw_message;  // Does a deep copy.

  // For each field, assign it to the corresponding protobuf field if
  // present, else NULL.
  header.InitFrom(&base_message.header().client_token(),
     base_message.header().has_registration_summary() ?
          &base_message.header().registration_summary() : NULL);

  token_control_message = base_message.has_token_control_message() ?
      &base_message.token_control_message() : NULL;

  invalidation_message = base_message.has_invalidation_message() ?
      &base_message.invalidation_message() : NULL;

  registration_status_message =
      base_message.has_registration_status_message() ?
          &base_message.registration_status_message() : NULL;

  registration_sync_request_message =
      base_message.has_registration_sync_request_message() ?
          &base_message.registration_sync_request_message() : NULL;

  config_change_message = base_message.has_config_change_message() ?
      &base_message.config_change_message() : NULL;

  info_request_message = base_message.has_info_request_message() ?
      &base_message.info_request_message() : NULL;

  error_message = base_message.has_error_message() ?
      &base_message.error_message() : NULL;
}

ProtocolHandler::ProtocolHandler(
    const ProtocolHandlerConfigP& config, SystemResources* resources,
    Smearer* smearer, Statistics* statistics, int client_type,
    const string& application_name, ProtocolListener* listener,
    TiclMessageValidator* msg_validator)
    : logger_(resources->logger()),
      internal_scheduler_(resources->internal_scheduler()),
      network_(resources->network()),
      throttle_(config.rate_limit(), internal_scheduler_,
          NewPermanentCallback(this, &ProtocolHandler::SendMessageToServer)),
      listener_(listener),
      msg_validator_(msg_validator),
      message_id_(1),
      last_known_server_time_ms_(0),
      next_message_send_time_ms_(0),
      statistics_(statistics),
      batcher_(resources->logger(), statistics),
      client_type_(client_type) {
  // Initialize client version.
  ProtoHelpers::InitClientVersion(resources->platform(), application_name,
      &client_version_);
}

void ProtocolHandler::InitConfig(ProtocolHandlerConfigP* config) {
  // Add rate limits.

  // Allow at most 3 messages every 5 seconds.
  int window_ms = 5 * 1000;
  int num_messages_per_window = 3;

  ProtoHelpers::InitRateLimitP(window_ms, num_messages_per_window,
      config->add_rate_limit());
}

void ProtocolHandler::InitConfigForTest(ProtocolHandlerConfigP* config) {
  // No rate limits.
  int small_batch_delay_for_test = 200;
  config->set_batching_delay_ms(small_batch_delay_for_test);

  // At most one message per second.
  ProtoHelpers::InitRateLimitP(1000, 1, config->add_rate_limit());
  // At most six messages per minute.
  ProtoHelpers::InitRateLimitP(60 * 1000, 6, config->add_rate_limit());
}

bool ProtocolHandler::HandleIncomingMessage(const string& incoming_message,
      ParsedMessage* parsed_message) {
  ServerToClientMessage message;
  message.ParseFromString(incoming_message);
  if (!message.IsInitialized()) {
    TLOG(logger_, WARNING, "Incoming message is unparseable: %s",
         ProtoHelpers::ToString(incoming_message).c_str());
    return false;
  }

  // Validate the message. If this passes, we can blindly assume valid messages
  // from here on.
  TLOG(logger_, FINE, "Incoming message: %s",
       ProtoHelpers::ToString(message).c_str());

  if (!msg_validator_->IsValid(message)) {
    statistics_->RecordError(
        Statistics::ClientErrorType_INCOMING_MESSAGE_FAILURE);
    TLOG(logger_, SEVERE, "Received invalid message: %s",
         ProtoHelpers::ToString(message).c_str());
    return false;
  }

  // Check the version of the message.
  const ServerHeader& message_header = message.header();
  if (message_header.protocol_version().version().major_version() !=
      Constants::kProtocolMajorVersion) {
    statistics_->RecordError(
        Statistics::ClientErrorType_PROTOCOL_VERSION_FAILURE);
    TLOG(logger_, SEVERE, "Dropping message with incompatible version: %s",
         ProtoHelpers::ToString(message).c_str());
    return false;
  }

  // Check if it is a ConfigChangeMessage which indicates that messages should
  // no longer be sent for a certain duration. Perform this check before the
  // token is even checked.
  if (message.has_config_change_message()) {
    const ConfigChangeMessage& config_change_msg =
        message.config_change_message();
    statistics_->RecordReceivedMessage(
        Statistics::ReceivedMessageType_CONFIG_CHANGE);
    if (config_change_msg.has_next_message_delay_ms()) {
      // Validator has ensured that it is positive.
      next_message_send_time_ms_ = GetCurrentTimeMs() +
          config_change_msg.next_message_delay_ms();
    }
    return false;  // Ignore all other messages in the envelope.
  }

  if (message_header.server_time_ms() > last_known_server_time_ms_) {
    last_known_server_time_ms_ = message_header.server_time_ms();
  }
  parsed_message->InitFrom(message);
  return true;
}

bool ProtocolHandler::CheckServerToken(const string& server_token) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  const string& client_token = listener_->GetClientToken();

  // If we do not have a client token yet, there is nothing to compare. The
  // message must have an initialize message and the upper layer will do the
  // appropriate checks. Hence, we return true if client_token is empty.
  if (client_token.empty()) {
    // No token. Return true so that we'll attempt to deliver a token control
    // message (if any) to the listener in handleIncomingMessage.
    return true;
  }

  if (client_token != server_token) {
    // Bad token - reject whole message.  However, our channel can send us
    // messages intended for other clients belonging to the same user, so don't
    // log too loudly.
    TLOG(logger_, INFO, "Incoming message has bad token: %s, %s",
         ProtoHelpers::ToString(client_token).c_str(),
         ProtoHelpers::ToString(server_token).c_str());
    statistics_->RecordError(Statistics::ClientErrorType_TOKEN_MISMATCH);
    return false;
  }
  return true;
}

void ProtocolHandler::SendInitializeMessage(
    const ApplicationClientIdP& application_client_id,
    const string& nonce,
    BatchingTask* batching_task,
    const string& debug_string) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";

  if (application_client_id.client_type() != client_type_) {
    // This condition is not fatal, but it probably represents a bug somewhere
    // if it occurs.
    TLOG(logger_, WARNING, "Client type in application id does not match "
         "constructor-provided type: %s vs %s",
         ProtoHelpers::ToString(application_client_id).c_str(), client_type_);
  }

  // Simply store the message in pending_initialize_message_ and send it
  // when the batching task runs.
  InitializeMessage* message = new InitializeMessage();
  ProtoHelpers::InitInitializeMessage(application_client_id, nonce, message);
  TLOG(logger_, INFO, "Batching initialize message for client: %s, %s",
       debug_string.c_str(),
       ProtoHelpers::ToString(*message).c_str());
  batcher_.SetInitializeMessage(message);
  batching_task->EnsureScheduled(debug_string);
}

void ProtocolHandler::SendInfoMessage(
    const vector<pair<string, int> >& performance_counters,
    ClientConfigP* client_config,
    bool request_server_registration_summary,
    BatchingTask* batching_task) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";

  // Simply store the message in pending_info_message_ and send it
  // when the batching task runs.
  InfoMessage* message = new InfoMessage();
  message->mutable_client_version()->CopyFrom(client_version_);

  // Add configuration parameters.
  if (client_config != NULL) {
    message->mutable_client_config()->CopyFrom(*client_config);
  }

  // Add performance counters.
  for (size_t i = 0; i < performance_counters.size(); ++i) {
    PropertyRecord* counter = message->add_performance_counter();
    counter->set_name(performance_counters[i].first);
    counter->set_value(performance_counters[i].second);
  }

  // Indicate whether we want the server's registration summary sent back.
  message->set_server_registration_summary_requested(
      request_server_registration_summary);

  TLOG(logger_, INFO, "Batching info message for client: %s",
       ProtoHelpers::ToString(*message).c_str());
  batcher_.SetInfoMessage(message);
  batching_task->EnsureScheduled("Send-info");
}

void ProtocolHandler::SendRegistrations(
    const vector<ObjectIdP>& object_ids,
    RegistrationP::OpType reg_op_type,
    BatchingTask* batching_task) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  for (size_t i = 0; i < object_ids.size(); ++i) {
    batcher_.AddRegistration(object_ids[i], reg_op_type);
  }
  batching_task->EnsureScheduled("Send-registrations");
}

void ProtocolHandler::SendInvalidationAck(const InvalidationP& invalidation,
    BatchingTask* batching_task) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  // We could summarize acks if there are suppressing invalidations - we don't
  // since it is unlikely to be too beneficial here.
  batcher_.AddAck(invalidation);
  batching_task->EnsureScheduled("Send-ack");
}

void ProtocolHandler::SendRegistrationSyncSubtree(
    const RegistrationSubtree& reg_subtree,
    BatchingTask* batching_task) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  TLOG(logger_, INFO, "Adding subtree: %s",
       ProtoHelpers::ToString(reg_subtree).c_str());
  batcher_.AddRegSubtree(reg_subtree);
  batching_task->EnsureScheduled("Send-reg-sync");
}

void ProtocolHandler::SendMessageToServer() {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";

  if (next_message_send_time_ms_ > GetCurrentTimeMs()) {
    TLOG(logger_, WARNING, "In quiet period: not sending message to server: "
         "%s > %s",
         SimpleItoa(next_message_send_time_ms_).c_str(),
         SimpleItoa(GetCurrentTimeMs()).c_str());
    return;
  }

  const bool has_client_token(!listener_->GetClientToken().empty());
  ClientToServerMessage builder;
  if (!batcher_.ToBuilder(&builder, has_client_token)) {
    TLOG(logger_, WARNING, "Unable to build message");
    return;
  }
  ClientHeader* outgoing_header = builder.mutable_header();
  InitClientHeader(outgoing_header);

  // Validate the message and send it.
  ++message_id_;
  if (!msg_validator_->IsValid(builder)) {
    TLOG(logger_, SEVERE, "Tried to send invalid message: %s",
         ProtoHelpers::ToString(builder).c_str());
    statistics_->RecordError(
        Statistics::ClientErrorType_OUTGOING_MESSAGE_FAILURE);
    return;
  }

  TLOG(logger_, FINE, "Sending message to server: %s",
       ProtoHelpers::ToString(builder).c_str());
  statistics_->RecordSentMessage(Statistics::SentMessageType_TOTAL);
  string serialized;
  builder.SerializeToString(&serialized);
  network_->SendMessage(serialized);

  // Record that the message was sent. We do this inline to match what the
  // Java Ticl, which is constrained by Android requirements, does.
  listener_->HandleMessageSent();
}

void ProtocolHandler::InitClientHeader(ClientHeader* builder) {
  CHECK(internal_scheduler_->IsRunningOnThread()) << "Not on internal thread";
  ProtoHelpers::InitProtocolVersion(builder->mutable_protocol_version());
  builder->set_client_time_ms(GetCurrentTimeMs());
  builder->set_message_id(StringPrintf("%d", message_id_));
  builder->set_max_known_server_time_ms(last_known_server_time_ms_);
  builder->set_client_type(client_type_);
  listener_->GetRegistrationSummary(builder->mutable_registration_summary());
  const string& client_token = listener_->GetClientToken();
  if (!client_token.empty()) {
    TLOG(logger_, FINE, "Sending token on client->server message: %s",
         ProtoHelpers::ToString(client_token).c_str());
    builder->set_client_token(client_token);
  }
}

bool Batcher::ToBuilder(ClientToServerMessage* builder, bool has_client_token) {
  // Check if an initialize message needs to be sent.
  if (pending_initialize_message_.get() != NULL) {
    statistics_->RecordSentMessage(Statistics::SentMessageType_INITIALIZE);
    builder->mutable_initialize_message()->CopyFrom(
        *pending_initialize_message_);
    pending_initialize_message_.reset();
  }

  // Note: Even if an initialize message is being sent, we can send additional
  // messages such as registration messages, etc to the server. But if there is
  // no token and an initialize message is not being sent, we cannot send any
  // other message.

  if (!has_client_token && !builder->has_initialize_message()) {
    // Cannot send any message.
    TLOG(logger_, WARNING,
         "Cannot send message since no token and no initialize msg: %s",
         ProtoHelpers::ToString(*builder).c_str());
    statistics_->RecordError(Statistics::ClientErrorType_TOKEN_MISSING_FAILURE);
    return false;
  }

  // Check for pending batched operations and add to message builder if needed.

  // Add reg, acks, reg subtrees - clear them after adding.
  if (!pending_acked_invalidations_.empty()) {
    InitAckMessage(builder->mutable_invalidation_ack_message());
    statistics_->RecordSentMessage(
        Statistics::SentMessageType_INVALIDATION_ACK);
  }

  // Check regs.
  if (!pending_registrations_.empty()) {
    InitRegistrationMessage(builder->mutable_registration_message());
    statistics_->RecordSentMessage(Statistics::SentMessageType_REGISTRATION);
  }

  // Check reg substrees.
  if (!pending_reg_subtrees_.empty()) {
    RegistrationSyncMessage* sync_message =
        builder->mutable_registration_sync_message();
    set<RegistrationSubtree, ProtoCompareLess>::const_iterator iter;
    for (iter = pending_reg_subtrees_.begin();
         iter != pending_reg_subtrees_.end(); ++iter) {
      sync_message->add_subtree()->CopyFrom(*iter);
    }
    pending_reg_subtrees_.clear();
    statistics_->RecordSentMessage(
        Statistics::SentMessageType_REGISTRATION_SYNC);
  }

  // Check info message.
  if (pending_info_message_.get() != NULL) {
    statistics_->RecordSentMessage(Statistics::SentMessageType_INFO);
    builder->mutable_info_message()->CopyFrom(*pending_info_message_);
    pending_info_message_.reset();
  }
  return true;
}

void Batcher::InitRegistrationMessage(
    RegistrationMessage* reg_message) {
  CHECK(!pending_registrations_.empty());

  // Run through the pending_registrations map.
  map<ObjectIdP, RegistrationP::OpType, ProtoCompareLess>::iterator iter;
  for (iter = pending_registrations_.begin();
       iter != pending_registrations_.end(); ++iter) {
    ProtoHelpers::InitRegistrationP(iter->first, iter->second,
        reg_message->add_registration());
  }
  pending_registrations_.clear();
}

void Batcher::InitAckMessage(InvalidationMessage* ack_message) {
  CHECK(!pending_acked_invalidations_.empty());

  // Run through pending_acked_invalidations_ set.
  set<InvalidationP, ProtoCompareLess>::iterator iter;
  for (iter = pending_acked_invalidations_.begin();
       iter != pending_acked_invalidations_.end(); iter++) {
    ack_message->add_invalidation()->CopyFrom(*iter);
  }
  pending_acked_invalidations_.clear();
}

}  // namespace invalidation
