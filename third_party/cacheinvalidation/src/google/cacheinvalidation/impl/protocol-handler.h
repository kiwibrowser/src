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

// A layer for interacting with low-level protocol messages.

#ifndef GOOGLE_CACHEINVALIDATION_IMPL_PROTOCOL_HANDLER_H_
#define GOOGLE_CACHEINVALIDATION_IMPL_PROTOCOL_HANDLER_H_

#include <stdint.h>

#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "google/cacheinvalidation/include/system-resources.h"
#include "google/cacheinvalidation/deps/scoped_ptr.h"
#include "google/cacheinvalidation/impl/client-protocol-namespace-fix.h"
#include "google/cacheinvalidation/impl/invalidation-client-util.h"
#include "google/cacheinvalidation/impl/proto-helpers.h"
#include "google/cacheinvalidation/impl/recurring-task.h"
#include "google/cacheinvalidation/impl/statistics.h"
#include "google/cacheinvalidation/impl/smearer.h"
#include "google/cacheinvalidation/impl/throttle.h"
#include "google/cacheinvalidation/impl/ticl-message-validator.h"

namespace invalidation {

class ProtocolHandler;

using INVALIDATION_STL_NAMESPACE::make_pair;
using INVALIDATION_STL_NAMESPACE::map;
using INVALIDATION_STL_NAMESPACE::pair;
using INVALIDATION_STL_NAMESPACE::set;
using INVALIDATION_STL_NAMESPACE::string;

/*
 * Representation of a message header for use in a server message.
 */
struct ServerMessageHeader {
 public:
  ServerMessageHeader() {
  }

  /* Initializes an instance. Note that this call *does not* make copies of
   * the pointed-to data. Instances are always allocated inside a ParsedMessage,
   * and the containing ParsedMessage owns the data.
   *
   * Arguments:
   *     init_token - server-sent token.
   *     init_registration_summary - summary over server registration state.
   *     If num_registations is not set, means no registration summary was
   *     received from the server.
   */
  void InitFrom(const string* init_token,
      const RegistrationSummary* init_registration_summary) {
    token_ = init_token;
    registration_summary_ = init_registration_summary;
  }

  const string& token() const {
    return *token_;
  }

  // Returns the registration summary if any.
  const RegistrationSummary* registration_summary() const {
    return registration_summary_;
  }

  // Returns a human-readable representation of this object for debugging.
  string ToString() const;

 private:
  // Server-sent token.
  const string* token_;

  // Summary of the client's registration state at the server.
  const RegistrationSummary* registration_summary_;

  DISALLOW_COPY_AND_ASSIGN(ServerMessageHeader);
};

/*
 * Representation of a message receiver for the server. Such a message is
 * guaranteed to be valid (i.e. checked by the message validator), but
 * the session token is NOT checked.
 */
struct ParsedMessage {
 public:
  ParsedMessage() {
  }

  ServerMessageHeader header;

  /*
   * Each of these fields points to a field in the base_message
   * ServerToClientMessage protobuf. It is non-null iff the corresponding hasYYY
   * method in the protobuf would return true.
   */
  const TokenControlMessage* token_control_message;
  const InvalidationMessage* invalidation_message;
  const RegistrationStatusMessage* registration_status_message;
  const RegistrationSyncRequestMessage* registration_sync_request_message;
  const ConfigChangeMessage* config_change_message;
  const InfoRequestMessage* info_request_message;
  const ErrorMessage* error_message;

  /*
   * Initializes an instance from a |raw_message|. This function makes a copy of
   * the message internally.
   */
  void InitFrom(const ServerToClientMessage& raw_message);

 private:
  ServerToClientMessage base_message;
  DISALLOW_COPY_AND_ASSIGN(ParsedMessage);
};

/*
 * Class that batches messages to be sent to the data center.
 */
class Batcher {
 public:
  Batcher(Logger* logger, Statistics* statistics)
      : logger_(logger), statistics_(statistics) {}

  /* Sets the initialize |message| to be sent to the server. */
  void SetInitializeMessage(const InitializeMessage* message) {
    pending_initialize_message_.reset(message);
  }

  /* Sets the info |message| to be sent to the server. */
  void SetInfoMessage(const InfoMessage* message) {
    pending_info_message_.reset(message);
  }

  /* Adds a registration on |object_id| to be sent to the server. */
  void AddRegistration(const ObjectIdP& object_id,
                       const RegistrationP::OpType& reg_op_type) {
    pending_registrations_[object_id] = reg_op_type;
  }

  /* Adds an acknowledgment of |invalidation| to be sent to the server. */
  void AddAck(const InvalidationP& invalidation) {
    pending_acked_invalidations_.insert(invalidation);
  }

  /* Adds a registration subtree |reg_subtree| to be sent to the server. */
  void AddRegSubtree(const RegistrationSubtree& reg_subtree) {
    pending_reg_subtrees_.insert(reg_subtree);
  }

  /*
   * Builds a message from the batcher state and resets the batcher. Returns
   * whether the message could be built.
   *
   * Note that the returned message does NOT include a header.
   */
  bool ToBuilder(ClientToServerMessage* builder,
      bool has_client_token);

  /*
   * Initializes a registration message based on registrations from
   * |pending_registrations|.
   *
   * REQUIRES: pending_registrations.size() > 0
   */
  void InitRegistrationMessage(RegistrationMessage* reg_message);

  /* Initializes an invalidation ack message based on acks from
   * |pending_acked_invalidations|.
   * <p>
   * REQUIRES: pending_acked_invalidations.size() > 0
   */
  void InitAckMessage(InvalidationMessage* ack_message);

 private:
  Logger* const logger_;

  Statistics* const statistics_;

  /* Set of pending registrations stored as a map for overriding later
   * operations.
   */
  map<ObjectIdP, RegistrationP::OpType, ProtoCompareLess>
      pending_registrations_;

  /* Set of pending invalidation acks. */
  set<InvalidationP, ProtoCompareLess> pending_acked_invalidations_;

  /* Set of pending registration sub trees for registration sync. */
  set<RegistrationSubtree, ProtoCompareLess> pending_reg_subtrees_;

  /* Pending initialization message to send to the server, if any. */
  scoped_ptr<const InitializeMessage> pending_initialize_message_;

  /* Pending info message to send to the server, if any. */
  scoped_ptr<const InfoMessage> pending_info_message_;

  DISALLOW_COPY_AND_ASSIGN(Batcher);
};

/* Listener for protocol events. The protocol client calls these methods when
 * a message is received from the server. It guarantees that the call will be
 * made on the internal thread that the SystemResources provides. When the
 * protocol listener is called, the token has been checked and message
 * validation has been completed (using the {@link TiclMessageValidator2}).
 * That is, all of the methods below can assume that the nonce is null and the
 * server token is valid.
 */
class ProtocolListener {
 public:
  ProtocolListener() {}
  virtual ~ProtocolListener() {}

  /* Records that a message was sent to the server at the current time. */
  virtual void HandleMessageSent() = 0;

  /* Handles a change in network connectivity. */
  virtual void HandleNetworkStatusChange(bool is_online) = 0;

  /* Stores a summary of the current desired registrations. */
  virtual void GetRegistrationSummary(RegistrationSummary* summary) = 0;

  /* Returns the current server-assigned client token, if any. */
  virtual string GetClientToken() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(ProtocolListener);
};

// Forward-declare the BatchingTask so that send* methods can take it.
class BatchingTask;

/* Parses messages from the server and calls appropriate functions on the
 * ProtocolListener to handle various types of message content.  Also buffers
 * message data from the client and constructs and sends messages to the server.
 */
class ProtocolHandler {
 public:
  /* Creates an instance.
   *
   * config - configuration for the client
   * resources - resources to use
   * smearer - a smearer to randomize delays
   * statistics - track information about messages sent/received, etc
   * client_type - client typecode
   * application_name - name of the application using the library (for
   *     debugging/monitoring)
   * listener - callback for protocol events
   * msg_validator - validator for protocol messages
   * Caller continues to own space for smearer.
   */
  ProtocolHandler(const ProtocolHandlerConfigP& config,
                  SystemResources* resources,
                  Smearer* smearer, Statistics* statistics,
                  int client_type, const string& application_name,
                  ProtocolListener* listener,
                  TiclMessageValidator* msg_validator);

  /* Initializes |config| with default protocol handler config parameters. */
  static void InitConfig(ProtocolHandlerConfigP* config);

  /* Initializes |config| with protocol handler config parameters for unit
   * tests.
   */
  static void InitConfigForTest(ProtocolHandlerConfigP* config);

  /* Returns the next time a message is allowed to be sent to the server.
   * Typically, this will be in the past, meaning that the client is free to
   * send a message at any time.
   */
  int64_t GetNextMessageSendTimeMsForTest() {
    return next_message_send_time_ms_;
  }

  /* Sends a message to the server to request a client token.
   *
   * Arguments:
   * client_type - client type code as assigned by the notification system's
   *     backend
   * application_client_id - application-specific client id
   * nonce - nonce for the request
   * batching_task - recurring task to trigger batching. No ownership taken.
   * debug_string - information to identify the caller
   */
  void SendInitializeMessage(
      const ApplicationClientIdP& application_client_id,
      const string& nonce,
      BatchingTask* batching_task,
      const string& debug_string);

  /* Sends an info message to the server with the performance counters supplied
   * in performance_counters and the config supplies in client_config (which
   * could be null).
   */
  void SendInfoMessage(const vector<pair<string, int> >& performance_counters,
                       ClientConfigP* client_config,
                       bool request_server_registration_summary,
                       BatchingTask* batching_task);

  /* Sends a registration request to the server.
   *
   * Arguments:
   * object_ids - object ids on which to (un)register
   * reg_op_type - whether to register or unregister
   * batching_task - recurring task to trigger batching. No ownership taken.
   */
  void SendRegistrations(const vector<ObjectIdP>& object_ids,
                         RegistrationP::OpType reg_op_type,
                         BatchingTask* batching_task);

  /* Sends an acknowledgement for invalidation to the server. */
  void SendInvalidationAck(const InvalidationP& invalidation,
                           BatchingTask* batching_task);

  /* Sends a single registration subtree to the server.
   *
   * Arguments:
   * reg_subtree - subtree to send
   * batching_task - recurring task to trigger batching. No ownership taken.
   */
  void SendRegistrationSyncSubtree(const RegistrationSubtree& reg_subtree,
                                   BatchingTask* batching_task);

  /* Sends pending data to the server (e.g., registrations, acks, registration
   * sync messages).
   *
   * REQUIRES: caller do no further work after the method returns.
   */
  void SendMessageToServer();

  /*
   * Handles a message from the server. If the message can be processed (i.e.,
   * is valid, is of the right version, and is not a silence message), returns
   * a ParsedMessage representing it. Otherwise, returns NULL.
   *
   * This class intercepts and processes silence messages. In this case, it will
   * discard any other data in the message.
   *
   * Note that this method does not check the session token of any message.
   */
  bool HandleIncomingMessage(const string& incoming_message,
                             ParsedMessage* parsed_message);

 private:
  /* Verifies that server_token matches the token currently held by the client.
   */
  bool CheckServerToken(const string& server_token);

  /* Stores the header to include on a message to the server. */
  void InitClientHeader(ClientHeader* header);

  // Returns the current time in milliseconds.
  int64_t GetCurrentTimeMs() {
    return InvalidationClientUtil::GetCurrentTimeMs(internal_scheduler_);
  }

  friend class BatchingTask;

  // Information about the client, e.g., application name, OS, etc.

  ClientVersion client_version_;

  // A logger.
  Logger* logger_;

  // Scheduler for the client's internal processing.
  Scheduler* internal_scheduler_;

  // Network channel for sending and receiving messages to and from the server.
  NetworkChannel* network_;

  // A throttler to prevent the client from sending too many messages in a given
  // interval.
  Throttle throttle_;

  // The protocol listener.
  ProtocolListener* listener_;

  // Checks that messages (inbound and outbound) conform to basic validity
  // constraints.
  TiclMessageValidator* msg_validator_;

  /* A debug message id that is added to every message to the server. */
  int message_id_;

  // State specific to a client. If we want to support multiple clients, this
  // could be in a map or could be eliminated (e.g., no batching).

  /* The last known time from the server. */
  int64_t last_known_server_time_ms_;

  /* The next time before which a message cannot be sent to the server. If
   * this is less than current time, a message can be sent at any time.
   */
  int64_t next_message_send_time_ms_;

  /* Statistics objects to track number of sent messages, etc. */
  Statistics* statistics_;

  // Batches messages to be sent to the server.
  Batcher batcher_;

  // Type code for the client.
  int client_type_;

  DISALLOW_COPY_AND_ASSIGN(ProtocolHandler);
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_IMPL_PROTOCOL_HANDLER_H_
