// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "base/test/scoped_task_environment.h"
#include "device/fido/authenticator_get_assertion_response.h"
#include "device/fido/ctap_get_assertion_request.h"
#include "device/fido/fake_fido_discovery.h"
#include "device/fido/fido_constants.h"
#include "device/fido/fido_parsing_utils.h"
#include "device/fido/fido_test_data.h"
#include "device/fido/fido_transport_protocol.h"
#include "device/fido/get_assertion_request_handler.h"
#include "device/fido/mock_fido_device.h"
#include "device/fido/test_callback_receiver.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

namespace {

constexpr uint8_t kClientDataHash[] = {0x01, 0x02, 0x03};
constexpr char kRpId[] = "acme.com";

using TestGetAssertionRequestCallback = test::StatusAndValueCallbackReceiver<
    FidoReturnCode,
    base::Optional<AuthenticatorGetAssertionResponse>>;

}  // namespace

class FidoGetAssertionHandlerTest : public ::testing::Test {
 public:
  void ForgeNextHidDiscovery() {
    discovery_ = scoped_fake_discovery_factory_.ForgeNextHidDiscovery();
  }

  std::unique_ptr<GetAssertionRequestHandler> CreateGetAssertionHandler() {
    ForgeNextHidDiscovery();

    CtapGetAssertionRequest request_param(
        kRpId, fido_parsing_utils::Materialize(kClientDataHash));
    request_param.SetAllowList(
        {{CredentialType::kPublicKey,
          fido_parsing_utils::Materialize(
              test_data::kTestGetAssertionCredentialId)}});

    return std::make_unique<GetAssertionRequestHandler>(
        nullptr /* connector */,
        base::flat_set<FidoTransportProtocol>(
            {FidoTransportProtocol::kUsbHumanInterfaceDevice}),
        std::move(request_param), get_assertion_cb_.callback());
  }

  test::FakeFidoDiscovery* discovery() const { return discovery_; }

  TestGetAssertionRequestCallback& get_assertion_callback() {
    return get_assertion_cb_;
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_{
      base::test::ScopedTaskEnvironment::MainThreadType::MOCK_TIME};
  test::ScopedFakeFidoDiscoveryFactory scoped_fake_discovery_factory_;
  test::FakeFidoDiscovery* discovery_;
  TestGetAssertionRequestCallback get_assertion_cb_;
};

TEST_F(FidoGetAssertionHandlerTest, TestGetAssertionRequestOnSingleDevice) {
  auto request_handler = CreateGetAssertionHandler();
  discovery()->WaitForCallToStartAndSimulateSuccess();
  auto device = std::make_unique<MockFidoDevice>();

  EXPECT_CALL(*device, GetId()).WillRepeatedly(testing::Return("device0"));
  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetInfo,
      test_data::kTestAuthenticatorGetInfoResponse);
  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetAssertion,
      test_data::kTestGetAssertionResponse);

  discovery()->AddDevice(std::move(device));
  get_assertion_callback().WaitForCallback();

  EXPECT_EQ(FidoReturnCode::kSuccess, get_assertion_callback().status());
  EXPECT_TRUE(get_assertion_callback().value());
  EXPECT_TRUE(request_handler->is_complete());
}

// Test a scenario where the connected authenticator is a U2F device. Request
// be silently dropped and request should remain in incomplete state.
TEST_F(FidoGetAssertionHandlerTest, TestGetAssertionIncorrectGetInfoResponse) {
  auto request_handler = CreateGetAssertionHandler();
  discovery()->WaitForCallToStartAndSimulateSuccess();

  auto device = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device, GetId()).WillRepeatedly(testing::Return("device0"));
  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetInfo, base::nullopt);

  discovery()->AddDevice(std::move(device));
  scoped_task_environment_.FastForwardUntilNoTasksRemain();
  EXPECT_FALSE(request_handler->is_complete());
}

}  // namespace device
