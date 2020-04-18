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
// Helper classes for tests including a mock Scheduler, a mock network, a
// mock storage layer, and a mock listener.

#ifndef GOOGLE_CACHEINVALIDATION_TEST_TEST_UTILS_H_
#define GOOGLE_CACHEINVALIDATION_TEST_TEST_UTILS_H_

#include <stddef.h>

#include <tuple>

#include "google/cacheinvalidation/client_protocol.pb.h"
#include "google/cacheinvalidation/include/invalidation-listener.h"
#include "google/cacheinvalidation/include/types.h"
#include "google/cacheinvalidation/types.pb.h"
#include "google/cacheinvalidation/deps/gmock.h"
#include "google/cacheinvalidation/deps/string_util.h"
#include "google/cacheinvalidation/impl/basic-system-resources.h"
#include "google/cacheinvalidation/impl/constants.h"
#include "google/cacheinvalidation/impl/log-macro.h"
#include "google/cacheinvalidation/impl/protocol-handler.h"
#include "google/cacheinvalidation/impl/statistics.h"
#include "google/cacheinvalidation/test/deterministic-scheduler.h"

namespace invalidation {

using ::ipc::invalidation::ObjectSource_Type_TEST;
using ::ipc::invalidation::ClientHeader;
using ::ipc::invalidation::ClientVersion;
using ::ipc::invalidation::InvalidationP;
using ::ipc::invalidation::ObjectIdP;
using ::ipc::invalidation::ProtocolVersion;
using ::ipc::invalidation::RegistrationP_OpType_REGISTER;
using ::ipc::invalidation::RegistrationP_OpType_UNREGISTER;
using ::ipc::invalidation::RegistrationStatus;
using ::ipc::invalidation::RegistrationSummary;
using ::ipc::invalidation::ServerHeader;
using ::ipc::invalidation::ServerToClientMessage;
using ::ipc::invalidation::StatusP_Code_SUCCESS;
using ::ipc::invalidation::Version;
using ::google::protobuf::MessageLite;
using ::testing::_;
using ::testing::EqualsProto;
using ::testing::Matcher;
using ::testing::Property;
using ::testing::SaveArg;
using ::testing::StrictMock;

/* A random class whose RandDouble method always returns a given constant. */
class FakeRandom : public Random {
 public:
  // Constructs a fake random generator that always returns |return_value|,
  // which must be in the range [0, 1).
  explicit FakeRandom(double return_value)
      : Random(0), return_value_(return_value) {
    CHECK_GE(return_value_, 0.0);
    CHECK_LT(return_value_, 1.0);
  }

  virtual ~FakeRandom() {}

  virtual double RandDouble() {
    return return_value_;
  }

 private:
  double return_value_;
};

// A mock of the Scheduler interface.
class MockScheduler : public Scheduler {
 public:
  MOCK_METHOD2(Schedule, void(TimeDelta, Closure*));  // NOLINT
  MOCK_CONST_METHOD0(IsRunningOnThread, bool());
  MOCK_CONST_METHOD0(GetCurrentTime, Time());
  MOCK_METHOD1(SetSystemResources, void(SystemResources*));  // NOLINT
};

// A mock of the Network interface.
class MockNetwork : public NetworkChannel {
 public:
  MOCK_METHOD1(SendMessage, void(const string&));  // NOLINT
  MOCK_METHOD1(SetMessageReceiver, void(MessageCallback*));  // NOLINT
  MOCK_METHOD1(AddNetworkStatusReceiver, void(NetworkStatusCallback*));  // NOLINT
  MOCK_METHOD1(SetSystemResources, void(SystemResources*));  // NOLINT
};

// A mock of the Storage interface.
class MockStorage : public Storage {
 public:
  MOCK_METHOD3(WriteKey, void(const string&, const string&, WriteKeyCallback*));  // NOLINT
  MOCK_METHOD2(ReadKey, void(const string&, ReadKeyCallback*));  // NOLINT
  MOCK_METHOD2(DeleteKey, void(const string&, DeleteKeyCallback*));  // NOLINT
  MOCK_METHOD1(ReadAllKeys, void(ReadAllKeysCallback*));  // NOLINT
  MOCK_METHOD1(SetSystemResources, void(SystemResources*));  // NOLINT
};

// A mock of the InvalidationListener interface.
class MockInvalidationListener : public InvalidationListener {
 public:
  MOCK_METHOD1(Ready, void(InvalidationClient*));  // NOLINT

  MOCK_METHOD3(Invalidate,
      void(InvalidationClient *, const Invalidation&,  // NOLINT
           const AckHandle&));  // NOLINT

  MOCK_METHOD3(InvalidateUnknownVersion,
               void(InvalidationClient *, const ObjectId&,
                    const AckHandle&));  // NOLINT

  MOCK_METHOD2(InvalidateAll,
      void(InvalidationClient *, const AckHandle&));  // NOLINT

  MOCK_METHOD3(InformRegistrationStatus,
      void(InvalidationClient*, const ObjectId&, RegistrationState));  // NOLINT

  MOCK_METHOD4(InformRegistrationFailure,
      void(InvalidationClient*, const ObjectId&, bool, const string&));

  MOCK_METHOD3(ReissueRegistrations,
      void(InvalidationClient*, const string&, int));

  MOCK_METHOD2(InformError,
      void(InvalidationClient*, const ErrorInfo&));
};

// A base class for unit tests to share common methods and helper routines.
class UnitTestBase : public testing::Test {
 public:
  // Default smearing to be done to randomize delays. Zero to get
  // precise delays.
  static const int kDefaultSmearPercent = 0;

  // The token or nonce used by default for a client in client to server or
  // server to client messages.
  static const char *kClientToken;

  // When "waiting" at the end of a test to make sure nothing happens, how long
  // to wait.
  static TimeDelta EndOfTestWaitTime();

  // When "waiting" at the end of a test to make sure nothing happens, how long
  // to wait. This delay will not only allow to run the processing on the
  // workqueue but will also give some 'extra' time to the code to do other
  // (incorrect) activities if there is a bug, e.g., make an uneccessary
  // listener call, etc.
  static TimeDelta MessageHandlingDelay();

  // Populates |object_ids| with |count| object ids in the TEST id space, each
  // named oid<n>.
  static void InitTestObjectIds(int count, vector<ObjectIdP>* object_ids);

  // Converts object id protos in |oid_protos| to ObjecIds in |oids|.
  static void ConvertFromObjectIdProtos(const vector<ObjectIdP>& oid_protos,
      vector<ObjectId> *oids);

  // Converts invalidation protos in |inv_protos| to Invalidations in |invs|.
  static void ConvertFromInvalidationProtos(
      const vector<InvalidationP>& inv_protos, vector<Invalidation> *invs);

  // For each object id in |object_ids|, adds an invalidation to |invalidations|
  // for that object at an arbitrary version.
  static void MakeInvalidationsFromObjectIds(
      const vector<ObjectIdP>& object_ids,
      vector<InvalidationP>* invalidations);

  // For each object in |object_ids|, makes a SUCCESSful registration status for
  // that object, alternating between REGISTER and UNREGISTER.  The precise
  // contents of these messages are unimportant to the protocol handler; we just
  // need them to pass the message validator.
  static void MakeRegistrationStatusesFromObjectIds(
      const vector<ObjectIdP>& object_ids, bool is_reg, bool is_success,
      vector<RegistrationStatus>* registration_statuses);

  // Returns the maximum smeared delay possible for |delay_ms| given the
  // |Smearer|'s default smearing.
  static TimeDelta GetMaxDelay(int delay_ms);

  // Returns the maximum batching delay that a message will incur in the
  // protocol handler.
  static TimeDelta GetMaxBatchingDelay(const ProtocolHandlerConfigP& config);

  // Initializes |summary| with a registration summary for 0 objects.
  static void InitZeroRegistrationSummary(RegistrationSummary* summary);

  // Creates a matcher for the parts of the header that the test can predict.
  static Matcher<ClientHeader> ClientHeaderMatches(const ClientHeader* header);

  // Initialize |reg_message| to contain registrations for all objects in |oids|
  // with |is_reg| indicating whether the operation is register or unregister.
  static void InitRegistrationMessage(const vector<ObjectIdP>& oids,
      bool is_reg, RegistrationMessage *reg_message);

  // Initializes |inv_message| to contain the invalidations |invs|.
  static void InitInvalidationMessage(const vector<InvalidationP>& invs,
      InvalidationMessage *inv_message);

  // Initializes |error_message| to contain given the |error_code| and error
  // |description|.
  static void InitErrorMessage(ErrorMessage::Code error_code,
      const string& description, ErrorMessage* error_message);

  // Performs setup for protocol handler unit tests, e.g. creating resource
  // components and setting up common expectations for certain mock objects.
  virtual void SetUp();

  // Tears down the test, e.g., drains any schedulers if needed.
  virtual void TearDown();

  // Initializes the basic system resources, using mocks for various components.
  void InitSystemResources();

  // Sets up some common expectations for the system resources.
  void InitCommonExpectations();

  // Initializes a server header with the given token (registration summary is
  // picked up the internal state |reg_summary|).
  void InitServerHeader(const string& token, ServerHeader* header);

  // Gives a ServerToClientMessage |message| to the protocol handler and
  // passes time in the internal scheduler by |delay| waiting for processing to
  // be done.
  void ProcessIncomingMessage(const ServerToClientMessage& message,
      TimeDelta delay);

  // Returns true iff the messages are equal (with lists interpreted as sets).
  bool CompareMessages(
      const ::google::protobuf::MessageLite& expected,
      const ::google::protobuf::MessageLite& actual);

  // Checks that |vec1| and |vec2| contain the same number of elements
  // and each element in |vec1| is present in |vec2| and vice-versa (Uses the
  // == operator for comparing). Returns true iff it is the case. Note that this
  // method will return true for (aab, abb)
  template <class T>
  static bool CompareVectorsAsSets(const vector<T>& vec1,
      const vector<T>& vec2) {
    if (vec1.size() != vec2.size()) {
      return false;
    }
    for (size_t i = 0; i < vec1.size(); i++) {
      bool found = false;
      for (size_t j = 0; (j < vec2.size()) && !found; j++) {
        found = found || (vec1[i] == vec2[j]);
      }
      if (!found) {
        return false;
      }
    }
    return true;
  }

  //
  // Internal state
  //

  // The time at which the test started.  Initialized to an arbitrary value to
  // ensure that we don't depend on it starting at 0.
  Time start_time;

  // Components of BasicSystemResources.  It takes ownership of all of these,
  // and its destructor deletes them, so we need to create fresh ones for each
  // test.

  // Use a deterministic scheduler for the protocol handler's internals, since
  // we want precise control over when batching intervals expire.
  DeterministicScheduler* internal_scheduler;

  // DeterministicScheduler or MockScheduler depending on the test.
  MockScheduler* listener_scheduler;

  // Use a mock network to let us trap the protocol handler's message receiver
  // and its attempts to send messages.
  MockNetwork* network;

  // A logger.
  Logger* logger;

  // Storage shouldn't be used by the protocol handler, so use a strict mock to
  // catch any accidental calls.
  MockStorage* storage;

  // System resources (owned by the test).
  scoped_ptr<BasicSystemResources> resources;

  // Statistics object for counting occurrences of different types of events.
  scoped_ptr<Statistics> statistics;

  // Message callback installed by the protocol handler.  Captured by the mock
  // network.
  MessageCallback* message_callback;

  // Registration summary to be placed in messages from the client to the server
  // and vice-versa.
  scoped_ptr<RegistrationSummary> reg_summary;
};

// Creates an action InvokeAndDeleteClosure<k> that invokes the kth closure and
// deletes it after the Run method has been called.
ACTION_TEMPLATE(
    InvokeAndDeleteClosure,
    HAS_1_TEMPLATE_PARAMS(int, k),
    AND_0_VALUE_PARAMS()) {
  std::get<k>(args)->Run();
  delete std::get<k>(args);
}

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_TEST_TEST_UTILS_H_
