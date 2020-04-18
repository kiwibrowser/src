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

// Unit tests for the InvalidationClientImpl class.

#include <vector>

#include "google/cacheinvalidation/client_test_internal.pb.h"
#include "google/cacheinvalidation/types.pb.h"
#include "google/cacheinvalidation/include/invalidation-listener.h"
#include "google/cacheinvalidation/include/types.h"
#include "google/cacheinvalidation/deps/gmock.h"
#include "google/cacheinvalidation/deps/googletest.h"
#include "google/cacheinvalidation/deps/random.h"
#include "google/cacheinvalidation/deps/string_util.h"
#include "google/cacheinvalidation/impl/basic-system-resources.h"
#include "google/cacheinvalidation/impl/constants.h"
#include "google/cacheinvalidation/impl/invalidation-client-impl.h"
#include "google/cacheinvalidation/impl/statistics.h"
#include "google/cacheinvalidation/impl/throttle.h"
#include "google/cacheinvalidation/impl/ticl-message-validator.h"
#include "google/cacheinvalidation/test/deterministic-scheduler.h"
#include "google/cacheinvalidation/test/test-logger.h"
#include "google/cacheinvalidation/test/test-utils.h"

namespace invalidation {

using ::ipc::invalidation::ClientType_Type_TEST;
using ::ipc::invalidation::RegistrationManagerStateP;
using ::ipc::invalidation::ObjectSource_Type_TEST;
using ::ipc::invalidation::StatusP_Code_PERMANENT_FAILURE;
using ::testing::_;
using ::testing::AllOf;
using ::testing::DeleteArg;
using ::testing::DoAll;
using ::testing::ElementsAre;
using ::testing::EqualsProto;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::InvokeArgument;
using ::testing::Matcher;
using ::testing::Property;
using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::SaveArg;
using ::testing::SetArgPointee;
using ::testing::StrictMock;
using ::testing::proto::WhenDeserializedAs;

// Creates an action SaveArgToVector<k>(vector*) that saves the kth argument in
// |vec|.
ACTION_TEMPLATE(
    SaveArgToVector,
    HAS_1_TEMPLATE_PARAMS(int, k),
    AND_1_VALUE_PARAMS(vec)) {
  vec->push_back(std::get<k>(args));
}

// Given the ReadCallback of Storage::ReadKey as argument 1, invokes it with a
// permanent failure status code.
ACTION(InvokeReadCallbackFailure) {
  arg1->Run(pair<Status, string>(Status(Status::PERMANENT_FAILURE, ""), ""));
  delete arg1;
}

// Given the WriteCallback of Storage::WriteKey as argument 2, invokes it with
// a success status code.
ACTION(InvokeWriteCallbackSuccess) {
  arg2->Run(Status(Status::SUCCESS, ""));
  delete arg2;
}

// Tests the basic functionality of the invalidation client.
class InvalidationClientImplTest : public UnitTestBase {
 public:
  virtual ~InvalidationClientImplTest() {}

  // Performs setup for protocol handler unit tests, e.g. creating resource
  // components and setting up common expectations for certain mock objects.
  virtual void SetUp() {
    UnitTestBase::SetUp();
    InitCommonExpectations();  // Set up expectations for common mock operations


    // Clear throttle limits so that it does not interfere with any test.
    InvalidationClientImpl::InitConfig(&config);
    config.set_smear_percent(kDefaultSmearPercent);
    config.mutable_protocol_handler_config()->clear_rate_limit();

    // Set up the listener scheduler to run any runnable that it receives.
    EXPECT_CALL(*listener_scheduler, Schedule(_, _))
        .WillRepeatedly(InvokeAndDeleteClosure<1>());

    // Create the actual client.
    Random* random = new Random(InvalidationClientUtil::GetCurrentTimeMs(
        resources->internal_scheduler()));
    client.reset(new InvalidationClientImpl(
        resources.get(), random, ClientType_Type_TEST, "clientName", config,
        "InvClientTest", &listener));
  }

  // Starts the Ticl and ensures that the initialize message is sent. In
  // response, gives a tokencontrol message to the protocol handler and makes
  // sure that ready is called. client_messages is the list of messages expected
  // from the client. The 0th message corresponds to the initialization message
  // sent out by the client.
  void StartClient() {
    // Start the client.
    client.get()->Start();

    // Let the message be sent out.
    internal_scheduler->PassTime(
        GetMaxBatchingDelay(config.protocol_handler_config()));

    // Check that the message contains an initializeMessage.
    ClientToServerMessage client_message;
    client_message.ParseFromString(outgoing_messages[0]);
    ASSERT_TRUE(client_message.has_initialize_message());
    string nonce = client_message.initialize_message().nonce();

    // Create the token control message and hand it to the protocol handler.
    ServerToClientMessage sc_message;
    InitServerHeader(nonce, sc_message.mutable_header());
    string new_token = "new token";
    sc_message.mutable_token_control_message()->set_new_token(new_token);
    ProcessIncomingMessage(sc_message, MessageHandlingDelay());
  }

  // Sets the expectations so that the Ticl is ready to be started such that
  // |num_outgoing_messages| are expected to be sent by the ticl. These messages
  // will be saved in |outgoing_messages|.
  void SetExpectationsForTiclStart(int num_outgoing_msgs) {
    // Set up expectations for number of messages expected on the network.
    EXPECT_CALL(*network, SendMessage(_))
        .Times(num_outgoing_msgs)
        .WillRepeatedly(SaveArgToVector<0>(&outgoing_messages));

    // Expect the storage to perform a read key that we will fail.
    EXPECT_CALL(*storage, ReadKey(_, _))
        .WillOnce(InvokeReadCallbackFailure());

    // Expect the listener to indicate that it is ready and let it reissue
    // registrations.
    EXPECT_CALL(listener, Ready(Eq(client.get())));
    EXPECT_CALL(listener, ReissueRegistrations(Eq(client.get()), _, _));

    // Expect the storage layer to receive the write of the session token.
    EXPECT_CALL(*storage, WriteKey(_, _, _))
        .WillOnce(InvokeWriteCallbackSuccess());
  }

  //
  // Test state maintained for every test.
  //

  // Messages sent by the Ticl.
  vector<string> outgoing_messages;

  // Configuration for the protocol handler (uses defaults).
  ClientConfigP config;

  // The client being tested. Created fresh for each test function.
  scoped_ptr<InvalidationClientImpl> client;

  // A mock invalidation listener.
  StrictMock<MockInvalidationListener> listener;
};

// Starts the ticl and checks that appropriate calls are made on the listener
// and that a proper message is sent on the network.
TEST_F(InvalidationClientImplTest, Start) {
  SetExpectationsForTiclStart(1);
  StartClient();
}

// Tests that GenerateNonce generates a unique nonce on every call.
TEST_F(InvalidationClientImplTest, GenerateNonce) {
  // Create a random number generated seeded with the current time.
  scoped_ptr<Random> random;
  random.reset(new Random(InvalidationClientUtil::GetCurrentTimeMs(
      resources->internal_scheduler())));

  // Generate two nonces and make sure they are distinct. (The chances
  // of a collision should be vanishingly small since our correctness
  // relies upon no collisions.)
  string nonce1 = InvalidationClientCore::GenerateNonce(random.get());
  string nonce2 = InvalidationClientCore::GenerateNonce(random.get());
  ASSERT_NE(nonce1, nonce2);
}

// Starts the Ticl, registers for a few objects, gets success and ensures that
// the right listener methods are invoked.
TEST_F(InvalidationClientImplTest, Register) {
  SetExpectationsForTiclStart(2);

  // Set some expectations for registration status messages.
  vector<ObjectId> saved_oids;
  EXPECT_CALL(listener,
              InformRegistrationStatus(Eq(client.get()), _,
                                       InvalidationListener::REGISTERED))
      .Times(3)
      .WillRepeatedly(SaveArgToVector<1>(&saved_oids));

  // Start the Ticl.
  StartClient();

  // Synthesize a few test object ids.
  int num_objects = 3;
  vector<ObjectIdP> oid_protos;
  vector<ObjectId> oids;
  InitTestObjectIds(num_objects, &oid_protos);
  ConvertFromObjectIdProtos(oid_protos, &oids);

  // Register
  client.get()->Register(oids);

  // Let the message be sent out.
  internal_scheduler->PassTime(
      GetMaxBatchingDelay(config.protocol_handler_config()));

  // Give a registration status message to the protocol handler and wait for
  // the listener calls.
  ServerToClientMessage message;
  InitServerHeader(client.get()->GetClientToken(), message.mutable_header());
  vector<RegistrationStatus> registration_statuses;
  MakeRegistrationStatusesFromObjectIds(oid_protos, true, true,
                                        &registration_statuses);
  for (int i = 0; i < num_objects; ++i) {
    message.mutable_registration_status_message()
        ->add_registration_status()->CopyFrom(registration_statuses[i]);
  }

  // Give this message to the protocol handler.
  ProcessIncomingMessage(message, EndOfTestWaitTime());

  // Check the object ids.
  ASSERT_TRUE(CompareVectorsAsSets(saved_oids, oids));

  // Check the registration message.
  ClientToServerMessage client_msg;
  client_msg.ParseFromString(outgoing_messages[1]);
  ASSERT_TRUE(client_msg.has_registration_message());
  ASSERT_FALSE(client_msg.has_info_message());
  ASSERT_FALSE(client_msg.has_registration_sync_message());

  RegistrationMessage expected_msg;
  InitRegistrationMessage(oid_protos, true, &expected_msg);
  const RegistrationMessage& actual_msg = client_msg.registration_message();
  ASSERT_TRUE(CompareMessages(expected_msg, actual_msg));
}

// Tests that given invalidations from the server, the right listener methods
// are invoked. Ack the invalidations and make sure that the ack message is sent
// out. Include a payload in one invalidation and make sure the client does not
// include it in the ack.
TEST_F(InvalidationClientImplTest, Invalidations) {
    // Set some expectations for starting the client.
  SetExpectationsForTiclStart(2);

  // Synthesize a few test object ids.
  int num_objects = 3;
  vector<ObjectIdP> oid_protos;
  vector<ObjectId> oids;
  InitTestObjectIds(num_objects, &oid_protos);
  ConvertFromObjectIdProtos(oid_protos, &oids);

  // Set up listener invalidation calls.
  vector<InvalidationP> invalidations;
  vector<Invalidation> expected_invs;
  MakeInvalidationsFromObjectIds(oid_protos, &invalidations);
  // Put a payload in one of the invalidations.
  invalidations[0].set_payload("this is a payload");
  ConvertFromInvalidationProtos(invalidations, &expected_invs);

  // Set up expectations for the acks.
  vector<Invalidation> saved_invs;
  vector<AckHandle> ack_handles;

  EXPECT_CALL(listener, Invalidate(Eq(client.get()), _, _))
      .Times(3)
      .WillRepeatedly(DoAll(SaveArgToVector<1>(&saved_invs),
                            SaveArgToVector<2>(&ack_handles)));

  // Start the Ticl.
  StartClient();

  // Give this message to the protocol handler.
  ServerToClientMessage message;
  InitServerHeader(client.get()->GetClientToken(), message.mutable_header());
  InitInvalidationMessage(invalidations,
      message.mutable_invalidation_message());

  // Process the incoming invalidation message.
  ProcessIncomingMessage(message, MessageHandlingDelay());

  // Check the invalidations.
  ASSERT_TRUE(CompareVectorsAsSets(expected_invs, saved_invs));

  // Ack the invalidations now and wait for them to be sent out.
  for (int i = 0; i < num_objects; i++) {
    client.get()->Acknowledge(ack_handles[i]);
  }
  internal_scheduler->PassTime(
      GetMaxBatchingDelay(config.protocol_handler_config()));

  // Check that the ack message is as expected.
  ClientToServerMessage client_msg;
  client_msg.ParseFromString(outgoing_messages[1]);
  ASSERT_TRUE(client_msg.has_invalidation_ack_message());

  InvalidationMessage expected_msg;
  // The client should strip the payload from the invalidation.
  invalidations[0].clear_payload();
  InitInvalidationMessage(invalidations, &expected_msg);
  const InvalidationMessage& actual_msg =
      client_msg.invalidation_ack_message();
  ASSERT_TRUE(CompareMessages(expected_msg, actual_msg));
}

// Give a registration sync request message and an info request message to the
// client and wait for the sync message and the info message to go out.
TEST_F(InvalidationClientImplTest, ServerRequests) {
  // Set some expectations for starting the client.
  SetExpectationsForTiclStart(2);

  // Start the ticl.
  StartClient();

  // Make the server to client message.
  ServerToClientMessage message;
  InitServerHeader(client.get()->GetClientToken(), message.mutable_header());

  // Add a registration sync request message.
  message.mutable_registration_sync_request_message();

  // Add an info request message.
  message.mutable_info_request_message()->add_info_type(
      InfoRequestMessage_InfoType_GET_PERFORMANCE_COUNTERS);

  // Give it to the prototol handler.
  ProcessIncomingMessage(message, EndOfTestWaitTime());

  // Make sure that the message is as expected.
  ClientToServerMessage client_msg;
  client_msg.ParseFromString(outgoing_messages[1]);
  ASSERT_TRUE(client_msg.has_info_message());
  ASSERT_TRUE(client_msg.has_registration_sync_message());
}

// Tests that an incoming unknown failure message results in the app being
// informed about it.
TEST_F(InvalidationClientImplTest, IncomingErrorMessage) {
  SetExpectationsForTiclStart(1);

  // Set up listener expectation for error.
  EXPECT_CALL(listener, InformError(Eq(client.get()), _));

  // Start the ticl.
  StartClient();

  // Give the error message to the protocol handler.
  ServerToClientMessage message;
  InitServerHeader(client.get()->GetClientToken(), message.mutable_header());
  InitErrorMessage(ErrorMessage_Code_UNKNOWN_FAILURE, "Some error message",
      message.mutable_error_message());
  ProcessIncomingMessage(message, EndOfTestWaitTime());
}

// Tests that an incoming auth failure message results in the app being informed
// about it and the registrations being removed.
TEST_F(InvalidationClientImplTest, IncomingAuthErrorMessage) {
  SetExpectationsForTiclStart(2);

  // One object to register for.
  int num_objects = 1;
  vector<ObjectIdP> oid_protos;
  vector<ObjectId> oids;
  InitTestObjectIds(num_objects, &oid_protos);
  ConvertFromObjectIdProtos(oid_protos, &oids);

  // Expect error and registration failure from the ticl.
  EXPECT_CALL(listener, InformError(Eq(client.get()), _));
  EXPECT_CALL(listener, InformRegistrationFailure(Eq(client.get()), Eq(oids[0]),
      Eq(false), _));

  // Start the client.
  StartClient();

  // Register and let the message be sent out.
  client.get()->Register(oids[0]);
  internal_scheduler->PassTime(
      GetMaxBatchingDelay(config.protocol_handler_config()));

  // Give this message to the protocol handler.
  ServerToClientMessage message;
  InitServerHeader(client.get()->GetClientToken(), message.mutable_header());
  InitErrorMessage(ErrorMessage_Code_AUTH_FAILURE, "Auth error message",
      message.mutable_error_message());
  ProcessIncomingMessage(message, EndOfTestWaitTime());
}

// Tests that a registration that times out results in a reg sync message being
// sent out.
TEST_F(InvalidationClientImplTest, NetworkTimeouts) {
  // Set some expectations for starting the client.
  SetExpectationsForTiclStart(3);

  // One object to register for.
  int num_objects = 1;
  vector<ObjectIdP> oid_protos;
  vector<ObjectId> oids;
  InitTestObjectIds(num_objects, &oid_protos);
  ConvertFromObjectIdProtos(oid_protos, &oids);

  // Start the client.
  StartClient();

  // Register for an object.
  client.get()->Register(oids[0]);

  // Let the registration message be sent out.
  internal_scheduler->PassTime(
      GetMaxBatchingDelay(config.protocol_handler_config()));

  // Now let the network timeout occur and an info message be sent.
  TimeDelta timeout_delay = GetMaxDelay(config.network_timeout_delay_ms());
  internal_scheduler->PassTime(timeout_delay);

  // Check that the message sent out is an info message asking for the server's
  // summary.
  ClientToServerMessage client_msg2;
  client_msg2.ParseFromString(outgoing_messages[2]);
  ASSERT_TRUE(client_msg2.has_info_message());
  ASSERT_TRUE(
      client_msg2.info_message().server_registration_summary_requested());
  internal_scheduler->PassTime(EndOfTestWaitTime());
}

// Tests that an incoming message without registration summary does not
// cause the registration summary in the client to be changed.
TEST_F(InvalidationClientImplTest, NoRegistrationSummary) {
  // Test plan: Initialze the ticl, let it get a token with a ServerToClient
  // message that has no registration summary.

  // Set some expectations for starting the client and start the client.
  // Give it a summary with 1 reg.
  reg_summary.get()->set_num_registrations(1);
  SetExpectationsForTiclStart(1);
  StartClient();

  // Now give it an message with no summary. It should not reset to a summary
  // with zero registrations.
  reg_summary.reset(NULL);
  ServerToClientMessage message;
  InitServerHeader(client.get()->GetClientToken(), message.mutable_header());
  ProcessIncomingMessage(message, EndOfTestWaitTime());

  // Check that the registration manager state did not change.
  string manager_serial_state;
  client->GetRegistrationManagerStateAsSerializedProto(&manager_serial_state);
  RegistrationManagerStateP reg_manager_state;
  reg_manager_state.ParseFromString(manager_serial_state);

  // Check that the registration manager state's number of registrations is 1.
  TLOG(logger, INFO, "Reg manager state: %s",
       ProtoHelpers::ToString(reg_manager_state).c_str());
  ASSERT_EQ(1, reg_manager_state.server_summary().num_registrations());
}

// Tests that heartbeats are sent out as time advances.
TEST_F(InvalidationClientImplTest, Heartbeats) {
  // Set some expectations for starting the client.
  SetExpectationsForTiclStart(2);

  // Start the client.
  StartClient();

  // Now let the heartbeat occur and an info message be sent.
  TimeDelta heartbeat_delay = GetMaxDelay(config.heartbeat_interval_ms() +
      config.protocol_handler_config().batching_delay_ms());
  internal_scheduler->PassTime(heartbeat_delay);

  // Check that the heartbeat is sent and it does not ask for the server's
  // summary.
  ClientToServerMessage client_msg1;
  client_msg1.ParseFromString(outgoing_messages[1]);
  ASSERT_TRUE(client_msg1.has_info_message());
  ASSERT_FALSE(
      client_msg1.info_message().server_registration_summary_requested());
  internal_scheduler->PassTime(EndOfTestWaitTime());
}

}  // namespace invalidation
