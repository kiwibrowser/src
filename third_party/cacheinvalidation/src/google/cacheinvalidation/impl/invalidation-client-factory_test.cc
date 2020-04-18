// Copyright 2013 Google Inc.
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

// Unit tests for the InvalidationClientFactory class.

#include "google/cacheinvalidation/include/invalidation-client-factory.h"
#include "google/cacheinvalidation/include/invalidation-listener.h"
#include "google/cacheinvalidation/include/types.h"
#include "google/cacheinvalidation/types.pb.h"
#include "google/cacheinvalidation/deps/googletest.h"
#include "google/cacheinvalidation/impl/basic-system-resources.h"
#include "google/cacheinvalidation/impl/constants.h"
#include "google/cacheinvalidation/impl/invalidation-client-impl.h"

#include "google/cacheinvalidation/test/test-utils.h"

namespace invalidation {

using ::ipc::invalidation::ClientType_Type_TEST;
using ::ipc::invalidation::ObjectSource_Type_TEST;
using ::testing::StrictMock;

// Test constants
static const char CLIENT_NAME[] = "demo-client-01";
static const char APPLICATION_NAME[] = "demo-app";

// Tests the basic functionality of the invalidation client factory.
class InvalidationClientFactoryTest : public UnitTestBase {
 public:
  virtual ~InvalidationClientFactoryTest() {}

  // Performs setup for client factory unit tests, e.g. creating resource
  // components and setting up common expectations for certain mock objects.
  virtual void SetUp() {
    UnitTestBase::SetUp();
    InitCommonExpectations();  // Set up expectations for common mock operations

    // Set up the listener scheduler to run any runnable that it receives.
    EXPECT_CALL(*listener_scheduler, Schedule(_, _))
        .WillRepeatedly(InvokeAndDeleteClosure<1>());
  }

  // Creates a client with the given value for allowSuppression.
  // The caller owns the storage.
  InvalidationClientImpl* CreateClient(bool allowSuppression) {
    InvalidationClientConfig config(ClientType_Type_TEST,
        CLIENT_NAME, APPLICATION_NAME, allowSuppression);
    return static_cast<InvalidationClientImpl*>(
        ClientFactory::Create(resources.get(), config, &listener));
  }

  // Verifies that a client has expected values for allowing suppression
  // and application client id.
  void CheckClientValid(const InvalidationClientImpl* client,
                        bool allowSuppression) {
    // Check that the the allow suppression flag was correctly set to
    // the expected value.
    ClientConfigP config = client->config_;
    ASSERT_EQ(allowSuppression, config.allow_suppression());

    // Check that the client type and client name were properly populated.
    ASSERT_EQ(ClientType_Type_TEST,
              client->application_client_id_.client_type());

    ASSERT_EQ(CLIENT_NAME,
              client->application_client_id_.client_name());
  }

  // The client being tested. Created fresh for each test function.
  scoped_ptr<InvalidationClientImpl> client;

  // A mock invalidation listener.
  StrictMock<MockInvalidationListener> listener;
};

// Tests that the deprecated CreateInvalidationClient overload
// correctly initializes the client to allow suppression.
TEST_F(InvalidationClientFactoryTest, TestCreateClient) {
  client.reset(static_cast<InvalidationClientImpl*>(
      CreateInvalidationClient(
          resources.get(),
          ClientType_Type_TEST,
          CLIENT_NAME,
          APPLICATION_NAME,
          &listener)));
  CheckClientValid(client.get(), true /* allowSuppression */);
}

// Tests CreateClient with allowSuppression = false.
TEST_F(InvalidationClientFactoryTest, TestCreateClientForTrickles) {
  bool allowSuppression = false;
  client.reset(CreateClient(allowSuppression));
  CheckClientValid(client.get(), allowSuppression);
}

// Tests CreateClient with allowSuppression = true.
TEST_F(InvalidationClientFactoryTest, testCreateClientForInvalidation) {
  bool allowSuppression = true;
  client.reset(CreateClient(allowSuppression));
  CheckClientValid(client.get(), allowSuppression);
}

}  // namespace invalidation
