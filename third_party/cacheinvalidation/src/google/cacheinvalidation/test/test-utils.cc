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

//
// Helper classes for tests including a mock Scheduler, a mock network and a
// mock storage layer.

#include <stdint.h>

#include <set>
#include <tuple>

#include "google/cacheinvalidation/impl/proto-converter.h"
#include "google/cacheinvalidation/test/test-utils.h"
#include "google/cacheinvalidation/test/test-logger.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"

namespace invalidation {

using ::google::protobuf::io::StringOutputStream;
using ::testing::DeleteArg;
using ::testing::StrictMock;

// Creates an Action InvokeNetworkStatusCallback<k>() that calls the Run method
// on the kth argument with value |true|.
ACTION_TEMPLATE(
    InvokeNetworkStatusCallback,
    HAS_1_TEMPLATE_PARAMS(int, k),
    AND_0_VALUE_PARAMS()) {
  std::get<k>(args)->Run(true);
}

const char* UnitTestBase::kClientToken = "Dummy";

void UnitTestBase::SetUp() {
  // Start time at an arbitrary point, just to make sure we don't depend on it
  // being 0.
  start_time = Time() + TimeDelta::FromDays(9);
  statistics.reset(new Statistics());
  reg_summary.reset(new RegistrationSummary());
  InitZeroRegistrationSummary(reg_summary.get());
  InitSystemResources();  // Set up system resources
  message_callback = NULL;

  // Start the scheduler and resources.
  internal_scheduler->StartScheduler();
  resources->Start();
}

void UnitTestBase::TearDown() {
  if (message_callback != NULL) {
    delete message_callback;
    message_callback = NULL;
  }
}

void UnitTestBase::InitSystemResources() {
  logger = new TestLogger();

  // Use a deterministic scheduler for the protocol handler's internals, since
  // we want precise control over when batching intervals expire.
  internal_scheduler = new DeterministicScheduler(logger);
  internal_scheduler->SetInitialTime(start_time);

  // Use a mock network to let us trap the protocol handler's message receiver
  // and its attempts to send messages.
  network = new StrictMock<MockNetwork>();
  listener_scheduler = new StrictMock<MockScheduler>();

  // Storage shouldn't be used by the protocol handler, so use a strict mock
  // to catch any accidental calls.
  storage = new StrictMock<MockStorage>();

  // The BasicSystemResources will set itself in the components.
  EXPECT_CALL(*network, SetSystemResources(_));
  EXPECT_CALL(*storage, SetSystemResources(_));
  EXPECT_CALL(*listener_scheduler, SetSystemResources(_));

  // Create the actual resources.
  resources.reset(new BasicSystemResources(
      logger, internal_scheduler, listener_scheduler, network, storage,
      "unit-test"));
}

void UnitTestBase::InitCommonExpectations() {
  // When we construct the protocol handler, it will set a message receiver on
  // the network. Intercept the call and save the callback.
  EXPECT_CALL(*network, SetMessageReceiver(_))
      .WillOnce(SaveArg<0>(&message_callback));

  // It will also add a network status receiver.  The network channel takes
  // ownership. Invoke it once with |true| just to exercise that code path,
  // then delete it since we won't need it anymore.
  EXPECT_CALL(*network, AddNetworkStatusReceiver(_))
      .WillOnce(DoAll(InvokeNetworkStatusCallback<0>(), DeleteArg<0>()));
}

void UnitTestBase::InitRegistrationMessage(const vector<ObjectIdP>& oids,
    bool is_reg, RegistrationMessage *reg_message) {
  RegistrationP::OpType op_type = is_reg ?
      RegistrationP::REGISTER : RegistrationP::UNREGISTER;
  for (size_t i = 0; i < oids.size(); i++) {
    ProtoHelpers::InitRegistrationP(
        oids[i], op_type, reg_message->add_registration());
  }
}

void UnitTestBase::InitErrorMessage(ErrorMessage::Code error_code,
    const string& description, ErrorMessage* error_message) {
  error_message->set_code(error_code);
  error_message->set_description(description);
}

void UnitTestBase::InitInvalidationMessage(const vector<InvalidationP>& invs,
    InvalidationMessage *inv_message) {
  for (size_t i = 0; i < invs.size(); ++i) {
    inv_message->add_invalidation()->CopyFrom(invs[i]);
  }
}

TimeDelta UnitTestBase::EndOfTestWaitTime() {
  return TimeDelta::FromSeconds(50);
}

TimeDelta UnitTestBase::MessageHandlingDelay() {
  return TimeDelta::FromMilliseconds(50);
}

void UnitTestBase::InitTestObjectIds(int count, vector<ObjectIdP>* object_ids) {
  for (int i = 0; i < count; ++i) {
    ObjectIdP object_id;
    object_id.set_source(ObjectSource_Type_TEST);
    object_id.set_name(StringPrintf("oid%d", i));
    object_ids->push_back(object_id);
  }
}

void UnitTestBase::ConvertFromObjectIdProtos(
    const vector<ObjectIdP>& oid_protos, vector<ObjectId> *oids) {
  for (size_t i = 0; i < oid_protos.size(); ++i) {
    ObjectId object_id;
    ProtoConverter::ConvertFromObjectIdProto(oid_protos[i], &object_id);
    oids->push_back(object_id);
  }
}

void UnitTestBase::ConvertFromInvalidationProtos(
      const vector<InvalidationP>& inv_protos, vector<Invalidation> *invs) {
  for (size_t i = 0; i < inv_protos.size(); ++i) {
    Invalidation inv;
    ProtoConverter::ConvertFromInvalidationProto(inv_protos[i], &inv);
    invs->push_back(inv);
  }
}

void UnitTestBase::MakeInvalidationsFromObjectIds(
    const vector<ObjectIdP>& object_ids,
    vector<InvalidationP>* invalidations) {
  for (size_t i = 0; i < object_ids.size(); ++i) {
    InvalidationP invalidation;
    invalidation.mutable_object_id()->CopyFrom(object_ids[i]);
    invalidation.set_is_known_version(true);
    invalidation.set_is_trickle_restart(true);

    // Pick an arbitrary version number; it shouldn't really matter, but we
    // try not to make them correlated too much with the object name.
    invalidation.set_version(100 + ((i * 19) % 31));
    invalidations->push_back(invalidation);
  }
}

void UnitTestBase::MakeRegistrationStatusesFromObjectIds(
    const vector<ObjectIdP>& object_ids, bool is_reg, bool is_success,
    vector<RegistrationStatus>* registration_statuses) {
  for (size_t i = 0; i < object_ids.size(); ++i) {
    RegistrationStatus registration_status;
    registration_status.mutable_registration()->mutable_object_id()->CopyFrom(
        object_ids[i]);
    registration_status.mutable_registration()->set_op_type(
        is_reg ? RegistrationP::REGISTER : RegistrationP::UNREGISTER);
    registration_status.mutable_status()->set_code(
        is_success ? StatusP::SUCCESS : StatusP::TRANSIENT_FAILURE);
    registration_statuses->push_back(registration_status);
  }
}

TimeDelta UnitTestBase::GetMaxDelay(int delay_ms) {
  int64_t extra_delay_ms = (delay_ms * kDefaultSmearPercent) / 100.0;
  return TimeDelta::FromMilliseconds(extra_delay_ms + delay_ms);
}

TimeDelta UnitTestBase::GetMaxBatchingDelay(
    const ProtocolHandlerConfigP& config) {
  return GetMaxDelay(config.batching_delay_ms());
}

void UnitTestBase::InitZeroRegistrationSummary(RegistrationSummary* summary) {
  summary->set_num_registrations(0);
  // "\3329..." can trigger MSVC to warn about "octal escape sequence terminated
  // by decimal number", so break the string between the two to avoid the
  // warning.
  string zero_digest(
      "\332" "9\243\356^kK\r2U\277\357\225`\030\220\257\330\007\t");
  summary->set_registration_digest(zero_digest);
}

void UnitTestBase::InitServerHeader(const string& token, ServerHeader* header) {
  ProtoHelpers::InitProtocolVersion(header->mutable_protocol_version());
  header->set_client_token(token);
  if (reg_summary.get() != NULL) {
    header->mutable_registration_summary()->CopyFrom(*reg_summary.get());
  }

  // Use arbitrary server time and message id, since they don't matter.
  header->set_server_time_ms(314159265);
  header->set_message_id("message-id-for-test");
}

bool UnitTestBase::CompareMessages(
    const ::google::protobuf::MessageLite& expected,
    const ::google::protobuf::MessageLite& actual) {
  // TODO: Fill in proper implementation.
  return true;
}

void UnitTestBase::ProcessIncomingMessage(const ServerToClientMessage& message,
    TimeDelta delay) {
  string serialized;
  message.SerializeToString(&serialized);
  message_callback->Run(serialized);
  internal_scheduler->PassTime(delay);
}

Matcher<ClientHeader> UnitTestBase::ClientHeaderMatches(
    const ClientHeader* header) {
  return AllOf(Property(&ClientHeader::protocol_version,
                        EqualsProto(header->protocol_version())),
               Property(&ClientHeader::registration_summary,
                        EqualsProto(header->registration_summary())),
               Property(&ClientHeader::max_known_server_time_ms,
                        header->max_known_server_time_ms()),
               Property(&ClientHeader::message_id,
                        header->message_id()));
}

}  // namespace invalidation
