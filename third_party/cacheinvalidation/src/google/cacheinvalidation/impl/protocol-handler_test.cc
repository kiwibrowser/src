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

// Unit tests for the ProtocolHandler class.

#include <stddef.h>
#include <stdint.h>

#include "google/cacheinvalidation/types.pb.h"
#include "google/cacheinvalidation/include/types.h"
#include "google/cacheinvalidation/deps/gmock.h"
#include "google/cacheinvalidation/deps/googletest.h"
#include "google/cacheinvalidation/deps/string_util.h"
#include "google/cacheinvalidation/impl/basic-system-resources.h"
#include "google/cacheinvalidation/impl/constants.h"
#include "google/cacheinvalidation/impl/invalidation-client-impl.h"
#include "google/cacheinvalidation/impl/protocol-handler.h"
#include "google/cacheinvalidation/impl/statistics.h"
#include "google/cacheinvalidation/impl/throttle.h"
#include "google/cacheinvalidation/impl/ticl-message-validator.h"
#include "google/cacheinvalidation/test/deterministic-scheduler.h"
#include "google/cacheinvalidation/test/test-logger.h"
#include "google/cacheinvalidation/test/test-utils.h"

namespace invalidation {

using ::ipc::invalidation::ClientType_Type_TEST;
using ::ipc::invalidation::ObjectSource_Type_TEST;
using ::testing::_;
using ::testing::AllOf;
using ::testing::ByRef;
using ::testing::DoAll;
using ::testing::ElementsAre;
using ::testing::EqualsProto;
using ::testing::Eq;
using ::testing::Matcher;
using ::testing::Property;
using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::SaveArg;
using ::testing::SetArgPointee;
using ::testing::StrictMock;
using ::testing::proto::WhenDeserializedAs;

/* Returns whether two headers are equal. */
bool HeaderEqual(const ServerMessageHeader& expected,
    const ServerMessageHeader& actual) {
  // If the token is different or if one of the registration summaries is NULL
  // and the other is non-NULL, return false.
  if (((expected.registration_summary() != NULL) !=
       (actual.registration_summary() != NULL)) ||
      (expected.token() != actual.token())) {
    return false;
  }

  // The tokens are the same and registration summaries are either both
  // null or non-null.
  return (expected.registration_summary() == NULL) ||
      ((expected.registration_summary()->num_registrations() ==
        actual.registration_summary()->num_registrations()) &&
       (expected.registration_summary()->registration_digest() ==
        actual.registration_summary()->registration_digest()));
}

// A mock of the ProtocolListener interface.
class MockProtocolListener : public ProtocolListener {
 public:
  MOCK_METHOD0(HandleMessageSent, void());

  MOCK_METHOD1(HandleNetworkStatusChange, void(bool));  // NOLINT

  MOCK_METHOD1(GetRegistrationSummary, void(RegistrationSummary*));  // NOLINT

  MOCK_METHOD0(GetClientToken, string());
};

// Tests the basic functionality of the protocol handler.
class ProtocolHandlerTest : public UnitTestBase {
 public:
  virtual ~ProtocolHandlerTest() {}

  // Performs setup for protocol handler unit tests, e.g. creating resource
  // components and setting up common expectations for certain mock objects.
  virtual void SetUp() {
    // Use a strict mock scheduler for the listener, since it shouldn't be used
    // at all by the protocol handler.
    UnitTestBase::SetUp();
    InitListenerExpectations();
    validator.reset(new TiclMessageValidator(logger));  // Create msg validator

    // Create the protocol handler object.
    random.reset(new Random(InvalidationClientUtil::GetCurrentTimeMs(
        resources.get()->internal_scheduler())));
    smearer.reset(new Smearer(random.get(), kDefaultSmearPercent));
    protocol_handler.reset(
        new ProtocolHandler(
            config, resources.get(), smearer.get(), statistics.get(),
            ClientType_Type_TEST, "unit-test", &listener, validator.get()));
    batching_task.reset(
        new BatchingTask(protocol_handler.get(), smearer.get(),
            TimeDelta::FromMilliseconds(config.batching_delay_ms())));
  }

  // Configuration for the protocol handler (uses defaults).
  ProtocolHandlerConfigP config;

  // The protocol handler being tested.  Created fresh for each test function.
  scoped_ptr<ProtocolHandler> protocol_handler;

  // A mock protocol listener.  We make this strict in order to have tight
  // control over the interactions between this and the protocol handler.
  // SetUp() installs expectations to allow GetClientToken() and
  // GetRegistrationSummary() to be called any time and to give them
  // reasonable behavior.
  StrictMock<MockProtocolListener> listener;

  // Ticl message validator.  We do not mock this, since the correctness of the
  // protocol handler depends on it.
  scoped_ptr<TiclMessageValidator> validator;

  // Token and registration summary for the mock listener to return when
  // the protocol handler requests them.
  string token;
  RegistrationSummary summary;

  // A smearer to randomize delays.
  scoped_ptr<Smearer> smearer;

  // A random number generator.
  scoped_ptr<Random> random;

  // Batching task for the protocol handler.
  scoped_ptr<BatchingTask> batching_task;

  void AddExpectationForHandleMessageSent() {
    EXPECT_CALL(listener, HandleMessageSent());
  }

  /*
   * Processes a |message| using the protocol handler, initializing
   * |parsed_message| with the result.
   *
   * Returns whether the message could be parsed.
   */
  bool ProcessMessage(ServerToClientMessage message,
      ParsedMessage* parsed_message) {
    string serialized;
    message.SerializeToString(&serialized);
    bool accepted = protocol_handler->HandleIncomingMessage(
        serialized, parsed_message);
    return accepted;
  }

 private:
  void InitListenerExpectations() {
    // When the handler asks the listener for the client token, return whatever
    // |token| currently is.
    EXPECT_CALL(listener, GetClientToken())
        .WillRepeatedly(ReturnPointee(&token));

    // If the handler asks the listener for a registration summary, respond by
    // supplying a fake summary.
    InitZeroRegistrationSummary(&summary);
    EXPECT_CALL(listener, GetRegistrationSummary(_))
        .WillRepeatedly(SetArgPointee<0>(summary));
  }
};

// Asks the protocol handler to send an initialize message.  Waits for the
// batching delay to pass.  Checks that appropriate calls are made on the
// listener and that a proper message is sent on the network.
TEST_F(ProtocolHandlerTest, SendInitializeOnly) {
  ApplicationClientIdP app_client_id;
  app_client_id.set_client_name("unit-test-client-id");
  app_client_id.set_client_type(ClientType_Type_TEST);

  // Client's token is initially empty.  Give it an arbitrary nonce.
  token = "";
  string nonce = "unit-test-nonce";

  // SendInitializeMessage checks that it's running on the work queue thread, so
  // we need to schedule the call.
  internal_scheduler->Schedule(
      Scheduler::NoDelay(),
      NewPermanentCallback(
          protocol_handler.get(), &ProtocolHandler::SendInitializeMessage,
          app_client_id, nonce, batching_task.get(), "Startup"));

  AddExpectationForHandleMessageSent();
  ClientToServerMessage expected_message;

  // Build the header.
  ClientHeader* header = expected_message.mutable_header();
  ProtoHelpers::InitProtocolVersion(header->mutable_protocol_version());
  header->mutable_registration_summary()->CopyFrom(summary);
  header->set_max_known_server_time_ms(0);
  header->set_message_id("1");

  // Note: because the batching task is smeared, we don't know what the client's
  // timestamp will be.  We omit it from this proto and do a partial match in
  // the EXPECT_CALL but also save the proto and check later that it doesn't
  // contain anything we don't expect.

  // Create the expected initialize message.
  InitializeMessage* initialize_message =
      expected_message.mutable_initialize_message();
  initialize_message->set_client_type(ClientType_Type_TEST);
  initialize_message->set_nonce(nonce);
  initialize_message->mutable_application_client_id()->CopyFrom(app_client_id);
  initialize_message->set_digest_serialization_type(
      InitializeMessage_DigestSerializationType_BYTE_BASED);

  string actual_serialized;
  EXPECT_CALL(
      *network,
      SendMessage(WhenDeserializedAs<ClientToServerMessage>(
          // Check that the deserialized message has the initialize message and
          // header fields we expect.
          AllOf(Property(&ClientToServerMessage::initialize_message,
                         EqualsProto(*initialize_message)),
                Property(&ClientToServerMessage::header,
                         ClientHeaderMatches(header))))))
      .WillOnce(SaveArg<0>(&actual_serialized));

  // The actual message won't be sent until after the batching delay, which is
  // smeared, so double it to be sure enough time will have passed.
  TimeDelta wait_time = GetMaxBatchingDelay(config);
  internal_scheduler->PassTime(wait_time);

  // By now we expect the message to have been sent, so we'll deserialize it
  // and check that it doesn't have anything we don't expect.
  ClientToServerMessage actual_message;
  actual_message.ParseFromString(actual_serialized);
  ASSERT_FALSE(actual_message.has_info_message());
  ASSERT_FALSE(actual_message.has_invalidation_ack_message());
  ASSERT_FALSE(actual_message.has_registration_message());
  ASSERT_FALSE(actual_message.has_registration_sync_message());
  ASSERT_GE(actual_message.header().client_time_ms(),
            InvalidationClientUtil::GetTimeInMillis(start_time));
  ASSERT_LE(actual_message.header().client_time_ms(),
            InvalidationClientUtil::GetTimeInMillis(start_time + wait_time));
}

// Tests the receipt of a token control message like what we'd expect in
// response to an initialize message.  Check that appropriate calls are made on
// the protocol listener.
TEST_F(ProtocolHandlerTest, ReceiveTokenControlOnly) {
  ServerToClientMessage message;
  ServerHeader* header = message.mutable_header();
  string nonce = "fake nonce";
  InitServerHeader(nonce, header);

  string new_token = "new token";
  message.mutable_token_control_message()->set_new_token(new_token);

  ServerMessageHeader expected_header;
  expected_header.InitFrom(&nonce, &header->registration_summary());
  ParsedMessage parsed_message;
  ProcessMessage(message, &parsed_message);
  ASSERT_TRUE(HeaderEqual(expected_header, parsed_message.header));
  ASSERT_TRUE(parsed_message.token_control_message != NULL);
}

// Test that the protocol handler correctly buffers multiple message types.
// Tell it to send registrations, then unregistrations (with some overlap in the
// sets of objects).  Then send some invalidation acks and finally a
// registration subtree.  Wait for the batching interval to pass, and then check
// that the message sent out contains everything we expect.
TEST_F(ProtocolHandlerTest, SendMultipleMessageTypes) {
  // Concoct some performance counters and config parameters, and ask to send
  // an info message with them.
  vector<pair<string, int> > perf_counters;
  perf_counters.push_back(make_pair("x", 3));
  perf_counters.push_back(make_pair("y", 81));
  ClientConfigP client_config;
  InvalidationClientImpl::InitConfig(&client_config);

  internal_scheduler->Schedule(
      Scheduler::NoDelay(),
      NewPermanentCallback(
          protocol_handler.get(), &ProtocolHandler::SendInfoMessage,
          perf_counters, &client_config, true, batching_task.get()));

  // Synthesize a few test object ids.
  vector<ObjectIdP> oids;
  InitTestObjectIds(3, &oids);

  // Register for the first two.
  vector<ObjectIdP> oid_vec;
  oid_vec.push_back(oids[0]);
  oid_vec.push_back(oids[1]);

  internal_scheduler->Schedule(
      Scheduler::NoDelay(),
      NewPermanentCallback(
          protocol_handler.get(), &ProtocolHandler::SendRegistrations,
          oid_vec, RegistrationP_OpType_REGISTER, batching_task.get()));

  // Then unregister for the second and third.  This overrides the registration
  // on oids[1].
  oid_vec.clear();
  oid_vec.push_back(oids[1]);
  oid_vec.push_back(oids[2]);
  internal_scheduler->Schedule(
      Scheduler::NoDelay(),
      NewPermanentCallback(
          protocol_handler.get(), &ProtocolHandler::SendRegistrations,
          oid_vec, RegistrationP_OpType_UNREGISTER, batching_task.get()));

  // Send a couple of invalidations.
  vector<InvalidationP> invalidations;
  MakeInvalidationsFromObjectIds(oids, &invalidations);
  invalidations.pop_back();
  for (size_t i = 0; i < invalidations.size(); ++i) {
    internal_scheduler->Schedule(
        Scheduler::NoDelay(),
        NewPermanentCallback(
            protocol_handler.get(), &ProtocolHandler::SendInvalidationAck,
            invalidations[i], batching_task.get()));
  }

  // Send a simple registration subtree.
  RegistrationSubtree subtree;
  subtree.add_registered_object()->CopyFrom(oids[0]);
  internal_scheduler->Schedule(
      Scheduler::NoDelay(),
      NewPermanentCallback(
          protocol_handler.get(), &ProtocolHandler::SendRegistrationSyncSubtree,
          subtree, batching_task.get()));

  AddExpectationForHandleMessageSent();

  token = "test token";

  // The message it sends should contain all of the expected information:
  ClientToServerMessage expected_message;

  // Header.
  ClientHeader* header = expected_message.mutable_header();
  ProtoHelpers::InitProtocolVersion(header->mutable_protocol_version());
  header->mutable_registration_summary()->CopyFrom(summary);
  header->set_client_token(token);
  header->set_max_known_server_time_ms(0);
  header->set_message_id("1");

  // Note: because the batching task is smeared, we don't know what the client's
  // timestamp will be.  We omit it from this proto and do a partial match in
  // the EXPECT_CALL but also save the proto and check later that it doesn't
  // contain anything we don't expect.

  // Registrations.
  RegistrationMessage* reg_message =
      expected_message.mutable_registration_message();
  RegistrationP* registration;
  registration = reg_message->add_registration();
  registration->mutable_object_id()->CopyFrom(oids[0]);
  registration->set_op_type(RegistrationP_OpType_REGISTER);

  registration = reg_message->add_registration();
  registration->mutable_object_id()->CopyFrom(oids[1]);
  registration->set_op_type(RegistrationP_OpType_UNREGISTER);

  registration = reg_message->add_registration();
  registration->mutable_object_id()->CopyFrom(oids[2]);
  registration->set_op_type(RegistrationP_OpType_UNREGISTER);

  // Registration sync message.
  expected_message.mutable_registration_sync_message()->add_subtree()
      ->CopyFrom(subtree);

  // Invalidation acks.
  InvalidationMessage* invalidation_message =
      expected_message.mutable_invalidation_ack_message();
  InitInvalidationMessage(invalidations, invalidation_message);

  // Info message.
  InfoMessage* info_message = expected_message.mutable_info_message();
  ProtoHelpers::InitClientVersion("unit-test", "unit-test",
      info_message->mutable_client_version());
  info_message->set_server_registration_summary_requested(true);
  info_message->mutable_client_config()->CopyFrom(client_config);
  PropertyRecord* prop_rec;
  for (uint32_t i = 0; i < perf_counters.size(); ++i) {
    prop_rec = info_message->add_performance_counter();
    prop_rec->set_name(perf_counters[i].first);
    prop_rec->set_value(perf_counters[i].second);
  }

  string actual_serialized;
  EXPECT_CALL(
      *network,
      SendMessage(
          WhenDeserializedAs<ClientToServerMessage>(
              // Check that the deserialized message has the invalidation acks,
              // registrations, info message, and header fields we expect.
              AllOf(Property(&ClientToServerMessage::invalidation_ack_message,
                             EqualsProto(*invalidation_message)),
                    Property(&ClientToServerMessage::registration_message,
                             EqualsProto(*reg_message)),
                    Property(&ClientToServerMessage::info_message,
                             EqualsProto(*info_message)),
                    Property(&ClientToServerMessage::header,
                             ClientHeaderMatches(header))))))
      .WillOnce(SaveArg<0>(&actual_serialized));

  TimeDelta wait_time = GetMaxBatchingDelay(config);
  internal_scheduler->PassTime(wait_time);

  ClientToServerMessage actual_message;
  actual_message.ParseFromString(actual_serialized);

  ASSERT_FALSE(actual_message.has_initialize_message());
  ASSERT_GE(actual_message.header().client_time_ms(),
            InvalidationClientUtil::GetTimeInMillis(start_time));
  ASSERT_LE(actual_message.header().client_time_ms(),
            InvalidationClientUtil::GetTimeInMillis(start_time + wait_time));
}

// Check that if the protocol handler receives a message with several sub-
// messages set, it makes all the appropriate calls on the listener.
TEST_F(ProtocolHandlerTest, IncomingCompositeMessage) {
  // Build up a message with a number of sub-messages in it:
  ServerToClientMessage message;

  // First the header.
  token = "test token";
  InitServerHeader(token, message.mutable_header());

  // Fabricate a few object ids for use in invalidations and registration
  // statuses.
  vector<ObjectIdP> object_ids;
  InitTestObjectIds(3, &object_ids);

  // Add invalidations.
  vector<InvalidationP> invalidations;
  MakeInvalidationsFromObjectIds(object_ids, &invalidations);
  for (int i = 0; i < 3; ++i) {
    message.mutable_invalidation_message()->add_invalidation()->CopyFrom(
        invalidations[i]);
  }

  // Add registration statuses.
  vector<RegistrationStatus> registration_statuses;
  MakeRegistrationStatusesFromObjectIds(object_ids, true, true,
                                        &registration_statuses);
  for (int i = 0; i < 3; ++i) {
    message.mutable_registration_status_message()
        ->add_registration_status()->CopyFrom(registration_statuses[i]);
  }

  // Add a registration sync request message.
  message.mutable_registration_sync_request_message();

  // Add an info request message.
  message.mutable_info_request_message()->add_info_type(
      InfoRequestMessage_InfoType_GET_PERFORMANCE_COUNTERS);

  // The header we expect the listener to be called with.
  ServerMessageHeader expected_header;
  expected_header.InitFrom(&token, &summary);

  ParsedMessage parsed_message;
  ProcessMessage(message, &parsed_message);
  ASSERT_TRUE(HeaderEqual(expected_header, parsed_message.header));
  ASSERT_TRUE(parsed_message.invalidation_message != NULL);
  ASSERT_TRUE(parsed_message.registration_status_message != NULL);
  ASSERT_TRUE(parsed_message.registration_sync_request_message != NULL);
  ASSERT_TRUE(parsed_message.info_request_message != NULL);
}

// Test that the protocol handler drops an invalid message.
TEST_F(ProtocolHandlerTest, InvalidInboundMessage) {
  // Make an invalid message (omit protocol version from header).
  ServerToClientMessage message;
  string token = "test token";
  ServerHeader* header = message.mutable_header();
  InitServerHeader(token, header);
  header->clear_protocol_version();

  // Add an info request message to check that it doesn't get processed.
  message.mutable_info_request_message()->add_info_type(
      InfoRequestMessage_InfoType_GET_PERFORMANCE_COUNTERS);
  ParsedMessage parsed_message;
  ProcessMessage(message, &parsed_message);
  ASSERT_EQ(1, statistics->GetClientErrorCounterForTest(
      Statistics::ClientErrorType_INCOMING_MESSAGE_FAILURE));
}

// Test that the protocol handler drops a message whose major version doesn't
// match what it understands.
TEST_F(ProtocolHandlerTest, MajorVersionMismatch) {
  // Make a message with a different protocol major version.
  ServerToClientMessage message;
  token = "test token";
  ServerHeader* header = message.mutable_header();
  InitServerHeader(token, header);
  header->mutable_protocol_version()->mutable_version()->set_major_version(1);

  // Add an info request message to check that it doesn't get processed.
  message.mutable_info_request_message()->add_info_type(
      InfoRequestMessage_InfoType_GET_PERFORMANCE_COUNTERS);

  ParsedMessage parsed_message;
  ProcessMessage(message, &parsed_message);
  ASSERT_EQ(1, statistics->GetClientErrorCounterForTest(
      Statistics::ClientErrorType_PROTOCOL_VERSION_FAILURE));
}

// Test that the protocol handler doesn't drop a message whose minor version
// doesn't match what it understands.
TEST_F(ProtocolHandlerTest, MinorVersionMismatch) {
  // Make a message with a different protocol minor version.
  ServerToClientMessage message;
  token = "test token";
  ServerHeader* header = message.mutable_header();
  InitServerHeader(token, header);
  header->mutable_protocol_version()->mutable_version()->set_minor_version(4);

  ServerMessageHeader expected_header;
  expected_header.InitFrom(&token, &summary);

  ParsedMessage parsed_message;
  ProcessMessage(message, &parsed_message);
  ASSERT_TRUE(HeaderEqual(expected_header, parsed_message.header));
  ASSERT_EQ(0, statistics->GetClientErrorCounterForTest(
      Statistics::ClientErrorType_PROTOCOL_VERSION_FAILURE));
}

// Test that the protocol handler honors a config message (even if the server
// token doesn't match) and does not call any listener methods.
TEST_F(ProtocolHandlerTest, ConfigMessage) {
  // Fabricate a config message.
  ServerToClientMessage message;
  token = "test token";
  InitServerHeader(token, message.mutable_header());
  token = "token-that-should-mismatch";

  int next_message_delay_ms = 2000 * 1000;
  message.mutable_config_change_message()->set_next_message_delay_ms(
      next_message_delay_ms);

  ParsedMessage parsed_message;
  ProcessMessage(message, &parsed_message);

  // Check that the protocol handler recorded receiving the config change
  // message, and that it has updated the next time it will send a message.
  ASSERT_EQ(1, statistics->GetReceivedMessageCounterForTest(
      Statistics::ReceivedMessageType_CONFIG_CHANGE));
  ASSERT_EQ(
      InvalidationClientUtil::GetTimeInMillis(
          start_time + TimeDelta::FromMilliseconds(next_message_delay_ms)),
      protocol_handler->GetNextMessageSendTimeMsForTest());

  // Request to send an info message, and check that it doesn't get sent.
  vector<pair<string, int> > empty_vector;
  internal_scheduler->Schedule(
      Scheduler::NoDelay(),
      NewPermanentCallback(
          protocol_handler.get(), &ProtocolHandler::SendInfoMessage,
          empty_vector, NULL, false, batching_task.get()));

  // Keep simulating passage of time until just before the quiet period ends.
  // Nothing should be sent.  (The mock network will catch any attempts to send
  // and fail the test.)
  internal_scheduler->PassTime(
      TimeDelta::FromMilliseconds(next_message_delay_ms - 1));
}

// Test that the protocol handler properly delivers an error message to the
// listener.
TEST_F(ProtocolHandlerTest, ErrorMessage) {
  // Fabricate an error message.
  ServerToClientMessage message;
  token = "test token";
  InitServerHeader(token, message.mutable_header());

  // Add an error message.
  ErrorMessage::Code error_code = ErrorMessage_Code_AUTH_FAILURE;
  string description = "invalid auth token";
  InitErrorMessage(error_code, description, message.mutable_error_message());
  ServerMessageHeader expected_header;
  expected_header.InitFrom(&token, &summary);

  // Deliver the message.
  ParsedMessage parsed_message;
  ProcessMessage(message, &parsed_message);
  ASSERT_TRUE(HeaderEqual(expected_header, parsed_message.header));
  ASSERT_TRUE(parsed_message.error_message != NULL);
}

// Tests that the protocol handler accepts a message from the server if the
// token doesn't match the client's (the caller is responsible for checking
// the token).
TEST_F(ProtocolHandlerTest, TokenMismatch) {
  // Create the server message with one token.
  token = "test token";
  ServerToClientMessage message;
  InitServerHeader(token, message.mutable_header());

  // Give the client a different token.
  token = "token-that-should-mismatch";

  // Deliver the message.
  ParsedMessage parsed_message;
  bool accepted = ProcessMessage(message, &parsed_message);
  ASSERT_TRUE(accepted);

  ASSERT_EQ(0, statistics->GetClientErrorCounterForTest(
      Statistics::ClientErrorType_TOKEN_MISMATCH));
}

// Tests that the protocol handler won't send out a non-initialize message if
// the client has no token.
TEST_F(ProtocolHandlerTest, TokenMissing) {
  token = "";
  vector<pair<string, int> > empty_vector;

  internal_scheduler->Schedule(
      Scheduler::NoDelay(),
      NewPermanentCallback(
          protocol_handler.get(),
          &ProtocolHandler::SendInfoMessage, empty_vector, NULL, true,
          batching_task.get()));

  internal_scheduler->PassTime(GetMaxBatchingDelay(config));

  ASSERT_EQ(1, statistics->GetClientErrorCounterForTest(
      Statistics::ClientErrorType_TOKEN_MISSING_FAILURE));
}

// Tests that the protocol handler won't send out a message that fails
// validation (in this case, an invalidation ack with a missing version).
TEST_F(ProtocolHandlerTest, InvalidOutboundMessage) {
  token = "test token";

  vector<ObjectIdP> object_ids;
  InitTestObjectIds(1, &object_ids);
  vector<InvalidationP> invalidations;
  MakeInvalidationsFromObjectIds(object_ids, &invalidations);
  invalidations[0].clear_version();

  internal_scheduler->Schedule(
      Scheduler::NoDelay(),
      NewPermanentCallback(
          protocol_handler.get(),
          &ProtocolHandler::SendInvalidationAck,
          invalidations[0],
          batching_task.get()));

  internal_scheduler->PassTime(GetMaxBatchingDelay(config));

  ASSERT_EQ(1, statistics->GetClientErrorCounterForTest(
      Statistics::ClientErrorType_OUTGOING_MESSAGE_FAILURE));
}

// Tests that the protocol handler drops an unparseable message.
TEST_F(ProtocolHandlerTest, UnparseableInboundMessage) {
  // Make an unparseable message.
  string serialized = "this can't be a valid protocol buffer!";
  ParsedMessage parsed_message;
  bool accepted = protocol_handler->HandleIncomingMessage(serialized,
                                                          &parsed_message);
  ASSERT_FALSE(accepted);
}

}  // namespace invalidation
