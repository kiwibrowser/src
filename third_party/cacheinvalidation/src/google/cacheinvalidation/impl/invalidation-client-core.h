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

#ifndef GOOGLE_CACHEINVALIDATION_IMPL_INVALIDATION_CLIENT_CORE_H_
#define GOOGLE_CACHEINVALIDATION_IMPL_INVALIDATION_CLIENT_CORE_H_

#include <stdint.h>

#include <string>
#include <utility>

#include "base/macros.h"
#include "google/cacheinvalidation/include/invalidation-client.h"
#include "google/cacheinvalidation/include/invalidation-listener.h"
#include "google/cacheinvalidation/deps/digest-function.h"
#include "google/cacheinvalidation/impl/client-protocol-namespace-fix.h"
#include "google/cacheinvalidation/impl/digest-store.h"
#include "google/cacheinvalidation/impl/exponential-backoff-delay-generator.h"
#include "google/cacheinvalidation/impl/protocol-handler.h"
#include "google/cacheinvalidation/impl/registration-manager.h"
#include "google/cacheinvalidation/impl/run-state.h"
#include "google/cacheinvalidation/impl/safe-storage.h"
#include "google/cacheinvalidation/impl/smearer.h"

namespace invalidation {

class InvalidationClientCore;

/* A task for acquiring tokens from the server. */
class AcquireTokenTask : public RecurringTask {
 public:
  explicit AcquireTokenTask(InvalidationClientCore* client);
  virtual ~AcquireTokenTask() {}

  // The actual implementation as required by the RecurringTask.
  virtual bool RunTask();

 private:
  /* The client that owns this task. */
  InvalidationClientCore* client_;
};

/* A task that schedules heartbeats when the registration summary at the client
 * is not in sync with the registration summary from the server.
 */
class RegSyncHeartbeatTask : public RecurringTask {
 public:
  explicit RegSyncHeartbeatTask(InvalidationClientCore* client);
  virtual ~RegSyncHeartbeatTask() {}

  // The actual implementation as required by the RecurringTask.
  virtual bool RunTask();

 private:
  /* The client that owns this task. */
  InvalidationClientCore* client_;
};

/* A task that writes the token to persistent storage. */
class PersistentWriteTask : public RecurringTask {
 public:
  explicit PersistentWriteTask(InvalidationClientCore* client);
  virtual ~PersistentWriteTask() {}

  // The actual implementation as required by the RecurringTask.
  virtual bool RunTask();

 private:
  /* Handles the result of a request to write to persistent storage.
   * |state| is the serialized state that was written.
   */
  void WriteCallback(const string& state, Status status);

  InvalidationClientCore* client_;

  /* The last client token that was written to to persistent state
   * successfully.
   */
  string last_written_token_;
};

/* A task for sending heartbeats to the server. */
class HeartbeatTask : public RecurringTask {
 public:
  explicit HeartbeatTask(InvalidationClientCore* client);
  virtual ~HeartbeatTask() {}

  // The actual implementation as required by the RecurringTask.
  virtual bool RunTask();
 private:
  /* The client that owns this task. */
  InvalidationClientCore* client_;

  /* Next time that the performance counters are sent to the server. */
  Time next_performance_send_time_;
};

/* The task that is scheduled to send batched messages to the server (when
 * needed).
 */
class BatchingTask : public RecurringTask {
 public:
  BatchingTask(ProtocolHandler *handler, Smearer* smearer,
      TimeDelta batching_delay);

  virtual ~BatchingTask() {}

  // The actual implementation as required by the RecurringTask.
  virtual bool RunTask();

 private:
  ProtocolHandler* protocol_handler_;
};

class InvalidationClientCore : public InvalidationClient,
                               public ProtocolListener {
 public:
  /* Modifies |config| to contain default parameters. */
  static void InitConfig(ClientConfigP *config);

  /* Modifies |config| to contain parameters set for unit tests. */
  static void InitConfigForTest(ClientConfigP *config);

  /* Creates the tasks used by the Ticl for token acquisition, heartbeats,
   * persistent writes and registration sync.
   */
  void CreateSchedulingTasks();

  /* Stores the client id that is used for squelching invalidations on the
   * server side.
   */
  void GetApplicationClientIdForTest(string* result) {
    application_client_id_.SerializeToString(result);
  }

  void GetClientTokenForTest(string* result) {
    *result = client_token_;
  }

  // Getters for testing.  No transfer of ownership occurs in any of these
  // methods.

  /* Returns the system resources. */
  SystemResources* GetResourcesForTest() {
    return resources_;
  }

  /* Returns the performance counters/statistics. */
  Statistics* GetStatisticsForTest() {
    return statistics_.get();
  }

  /* Returns the digest function used for computing digests for object
   * registrations.
   */
  DigestFunction* GetDigestFunctionForTest() {
    return digest_fn_.get();
  }

  /* Changes the existing delay for the network timeout delay in the operation
   * scheduler to be delay_ms.
   */
  void ChangeNetworkTimeoutDelayForTest(const TimeDelta& delay);

  /* Changes the existing delay for the heartbeat delay in the operation
   * scheduler to be delay_ms.
   */
  void ChangeHeartbeatDelayForTest(const TimeDelta& delay);

  /* Returns the next time a message is allowed to be sent to the server (could
   * be in the past).
   */
  int64_t GetNextMessageSendTimeMsForTest() {
    return protocol_handler_.GetNextMessageSendTimeMsForTest();
  }

  /* Returns true iff the client is currently started. */
  bool IsStartedForTest() {
    return ticl_state_.IsStarted();
  }

  /* Sets the digest store to be digest_store for testing purposes.
   *
   * REQUIRES: This method is called before the Ticl has been started.
   */
  void SetDigestStoreForTest(DigestStore<ObjectIdP>* digest_store) {
    CHECK(!resources_->IsStarted());
    registration_manager_.SetDigestStoreForTest(digest_store);
  }

  virtual void Start();

  virtual void Stop();

  virtual void Register(const ObjectId& object_id);

  virtual void Unregister(const ObjectId& object_id);

  virtual void Register(const vector<ObjectId>& object_ids) {
    PerformRegisterOperations(object_ids, RegistrationP_OpType_REGISTER);
  }

  virtual void Unregister(const vector<ObjectId>& object_ids) {
    PerformRegisterOperations(object_ids, RegistrationP_OpType_UNREGISTER);
  }

  /* Implementation of (un)registration.
   *
   * Arguments:
   * object_ids - object ids on which to operate
   * reg_op_type - whether to register or unregister
   */
  virtual void PerformRegisterOperations(
      const vector<ObjectId>& object_ids, RegistrationP::OpType reg_op_type);

  void PerformRegisterOperationsInternal(
      const vector<ObjectId>& object_ids, RegistrationP::OpType reg_op_type);

  virtual void Acknowledge(const AckHandle& acknowledge_handle);

  string ToString();

  /* Returns a randomly generated nonce. */
  static string GenerateNonce(Random* random);

  //
  // Protocol listener methods
  //

  /* Returns the current client token. */
  virtual string GetClientToken();

  virtual void GetRegistrationSummary(RegistrationSummary* summary) {
    registration_manager_.GetClientSummary(summary);
  }

  virtual void HandleMessageSent();

  /* Gets registration manager state as a serialized RegistrationManagerState.
   */
  void GetRegistrationManagerStateAsSerializedProto(string* result);

  /* Gets statistics as a serialized InfoMessage. */
  void GetStatisticsAsSerializedProto(string* result);

  /* The single key used to write all the Ticl state. */
  static const char* kClientTokenKey;
 protected:
   /* Constructs a client.
    *
    * Arguments:
    * resources - resources to use during execution
    * random - a random number generator (owned by this after the call)
    * client_type - client type code
    * client_name - application identifier for the client
    * config - configuration for the client
    */
  InvalidationClientCore(
      SystemResources* resources, Random* random, int client_type,
      const string& client_name, const ClientConfigP &config,
      const string& application_name);

  /* Returns the internal scheduler. */
  Scheduler* GetInternalScheduler() {
    return internal_scheduler_;
  }

  /* Returns the statistics. */
  Statistics* GetStatistics() {
    return statistics_.get();
  }

  /* Returns the listener. */
  virtual InvalidationListener* GetListener() = 0;
 private:
  // Friend classes so that they can access the scheduler, logger, smearer, etc.
  friend class AcquireTokenTask;
  friend class HeartbeatTask;
  friend class InvalidationClientFactoryTest;
  friend class PersistentWriteTask;
  friend class RegSyncHeartbeatTask;

  //
  // Private methods.
  //

  virtual void HandleNetworkStatusChange(bool is_online);

  /* Implementation of start on the internal thread with the persistent
   * serialized_state if any. Starts the TICL protocol and makes the TICL ready
   * to received registration, invalidations, etc
   */
  void StartInternal(const string& serialized_state);

  void AcknowledgeInternal(const AckHandle& acknowledge_handle);

  /* Set client_token to NULL and schedule acquisition of the token. */
  void ScheduleAcquireToken(const string& debug_string);

  /* Sends an info message to the server. If mustSendPerformanceCounters is
   * true, the performance counters are sent regardless of when they were sent
   * earlier.
   */
  void SendInfoMessageToServer(
      bool mustSendPerformanceCounters, bool request_server_summary);

  /* Sets the nonce to new_nonce.
   *
   * REQUIRES: new_nonce be empty or client_token_ be empty.  The goal is to
   * ensure that a nonce is never set unless there is no client token, unless
   * the nonce is being cleared.
   */
  void set_nonce(const string& new_nonce);

  /* Sets the client_token_ to new_client_token.
   *
   * REQUIRES: new_client_token be empty or nonce_ be empty.  The goal is to
   * ensure that a token is never set unless there is no nonce, unless the token
   * is being cleared.
   */
  void set_client_token(const string& new_client_token);

  /* Reads the Ticl state from persistent storage (if any) and calls
   * startInternal.
   */
  void ScheduleStartAfterReadingStateBlob();

  /* Handles the result of a request to read from persistent storage. */
  void ReadCallback(pair<Status, string> read_result);

  /* Finish starting the ticl and inform the listener that it is ready. */
  void FinishStartingTiclAndInformListener();

  /* Returns an exponential backoff generator with a max exponential factor
   * given by |config_.max_exponential_backoff_factor| and initial delay
   * |initial_delay|.
   * Space for the returned object is owned by the caller.
   */
  ExponentialBackoffDelayGenerator* CreateExpBackOffGenerator(
      const TimeDelta& initial_delay);

  /* Registers a message receiver and status change listener on |resources|. */
  void RegisterWithNetwork(SystemResources* resources);

  /* Handles inbound messages from the network. */
  void MessageReceiver(string message);

  /* Responds to changes in network connectivity. */
  void NetworkStatusReceiver(bool status);

  /* Handles a |message| from the server. */
  void HandleIncomingMessage(const string& message);

  /*
   * Handles a changed token. |header_token| is the token in the server message
   * header. |new_token| is a new token from the server; if empty, it indicates
   * a destroy-token message.
   */
  void HandleTokenChanged(const string& header_token, const string& new_token);

  /* Processes a server message |header|. */
  void HandleIncomingHeader(const ServerMessageHeader& header);

  /* Handles |invalidations| from the server. */
  void HandleInvalidations(
       const RepeatedPtrField<InvalidationP>& invalidations);

  /* Handles registration statusES from the server. */
  void HandleRegistrationStatus(
       const RepeatedPtrField<RegistrationStatus>& reg_status_list);

  /* Handles A registration sync request from the server. */
  void HandleRegistrationSyncRequest();

  /* Handles an info message request from the server. */
  void HandleInfoMessage(
       const RepeatedField<InfoRequestMessage_InfoType>& info_types);

  /* Handles an error message with |code| and |description| from the server. */
  void HandleErrorMessage(ErrorMessage::Code code, const string& description);

  /* Returns whether |server_token| matches the client token or nonce. */
  bool ValidateToken(const string& server_token);

  /* Converts an operation type reg_status to a
   * InvalidationListener::RegistrationState.
   */
  static InvalidationListener::RegistrationState ConvertOpTypeToRegState(
      RegistrationP::OpType reg_op_type);

  /* Resources for the Ticl. */
  SystemResources* resources_;  // Owned by application.

  /* Reference into the resources object for cleaner code. All Ticl code must be
   * scheduled on this scheduler.
   */
  Scheduler* internal_scheduler_;

  /* Logger reference into the resources object for cleaner code. */
  Logger* logger_;

  /* A storage layer which schedules the callbacks on the internal scheduler
   * thread.
   */
  scoped_ptr<SafeStorage> storage_;

  /* Statistics objects to track number of sent messages, etc. */
  scoped_ptr<Statistics> statistics_;

  /* Configuration for this instance. */
  ClientConfigP config_;

  /* Application identifier for this client. */
  ApplicationClientIdP application_client_id_;

  /* The function for computing the registration and persistence state digests.
   */
  scoped_ptr<DigestFunction> digest_fn_;

  /* Object maintaining the registration state for this client. */
  RegistrationManager registration_manager_;

  /* Used to validate messages */
  scoped_ptr<TiclMessageValidator> msg_validator_;

  /* A smearer to make sure that delays are randomized a little bit. */
  Smearer smearer_;

  /* Object handling low-level wire format interactions. */
  ProtocolHandler protocol_handler_;

  /* The state of the Ticl whether it has started or not. */
  RunState ticl_state_;

  /* Current client token known from the server. */
  string client_token_;

  // After the client starts, exactly one of nonce and clientToken is non-null.

  /* If not empty, nonce for pending identifier request. */
  string nonce_;

  /* Whether we should send registrations to the server or not. */
  // TODO(ghc): [cleanup] Make the server summary in the registration manager
  // nullable and replace this variable with a test for whether it's null or
  // not.
  bool should_send_registrations_;

  /* Whether the network is online.  Assume so when we start. */
  bool is_online_;

  /* Last time a message was sent to the server. */
  Time last_message_send_time_;

  /* A task for acquiring the token (if the client has no token). */
  scoped_ptr<AcquireTokenTask> acquire_token_task_;

  /* Task for checking if reg summary is out of sync and then sending a
   * heartbeat to the server.
   */
  scoped_ptr<RegSyncHeartbeatTask> reg_sync_heartbeat_task_;

  /* Task for writing the state blob to persistent storage. */
  scoped_ptr<PersistentWriteTask> persistent_write_task_;

  /* A task for periodic heartbeats. */
  scoped_ptr<HeartbeatTask> heartbeat_task_;

  /* Task to send all batched messages to the server. */
  scoped_ptr<BatchingTask> batching_task_;

  /* Random number generator for smearing, exp backoff, etc. */
  scoped_ptr<Random> random_;

  DISALLOW_COPY_AND_ASSIGN(InvalidationClientCore);
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_IMPL_INVALIDATION_CLIENT_CORE_H_
