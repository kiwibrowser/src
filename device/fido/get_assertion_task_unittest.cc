// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/get_assertion_task.h"

#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/test/scoped_task_environment.h"
#include "device/fido/authenticator_get_assertion_response.h"
#include "device/fido/ctap_get_assertion_request.h"
#include "device/fido/fido_constants.h"
#include "device/fido/fido_parsing_utils.h"
#include "device/fido/fido_test_data.h"
#include "device/fido/mock_fido_device.h"
#include "device/fido/test_callback_receiver.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;

namespace device {

namespace {

constexpr uint8_t kClientDataHash[] = {0x01, 0x02, 0x03};
constexpr char kRpId[] = "acme.com";

using TestGetAssertionTaskCallbackReceiver =
    ::device::test::StatusAndValueCallbackReceiver<
        CtapDeviceResponseCode,
        base::Optional<AuthenticatorGetAssertionResponse>>;

}  // namespace

class FidoGetAssertionTaskTest : public testing::Test {
 public:
  TestGetAssertionTaskCallbackReceiver& get_assertion_callback_receiver() {
    return cb_;
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  TestGetAssertionTaskCallbackReceiver cb_;
};

TEST_F(FidoGetAssertionTaskTest, TestGetAssertionSuccess) {
  auto device = std::make_unique<MockFidoDevice>();
  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetInfo,
      test_data::kTestAuthenticatorGetInfoResponse);
  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetAssertion,
      test_data::kTestGetAssertionResponse);

  CtapGetAssertionRequest request_param(
      kRpId, fido_parsing_utils::Materialize(kClientDataHash));
  request_param.SetAllowList({{CredentialType::kPublicKey,
                               fido_parsing_utils::Materialize(
                                   test_data::kTestGetAssertionCredentialId)}});

  auto task = std::make_unique<GetAssertionTask>(
      device.get(), std::move(request_param),
      get_assertion_callback_receiver().callback());

  get_assertion_callback_receiver().WaitForCallback();
  EXPECT_EQ(CtapDeviceResponseCode::kSuccess,
            get_assertion_callback_receiver().status());
  EXPECT_TRUE(get_assertion_callback_receiver().value());
  EXPECT_EQ(device->supported_protocol(), ProtocolVersion::kCtap);
  EXPECT_TRUE(device->device_info());
}

// Tests a scenario where the authenticator responds with credential ID that
// is not included in the allowed list.
TEST_F(FidoGetAssertionTaskTest, TestGetAssertionInvalidCredential) {
  auto device = std::make_unique<MockFidoDevice>();

  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetInfo,
      test_data::kTestAuthenticatorGetInfoResponse);
  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetAssertion,
      test_data::kTestGetAssertionResponse);

  auto task = std::make_unique<GetAssertionTask>(
      device.get(),
      CtapGetAssertionRequest(kRpId,
                              fido_parsing_utils::Materialize(kClientDataHash)),
      get_assertion_callback_receiver().callback());

  get_assertion_callback_receiver().WaitForCallback();
  EXPECT_EQ(CtapDeviceResponseCode::kCtap2ErrOther,
            get_assertion_callback_receiver().status());
  EXPECT_FALSE(get_assertion_callback_receiver().value());
  EXPECT_EQ(device->supported_protocol(), ProtocolVersion::kCtap);
  EXPECT_TRUE(device->device_info());
}

// Tests a scenario where authenticator responds without user entity in its
// response but client is expecting a resident key credential.
TEST_F(FidoGetAssertionTaskTest, TestGetAsserionIncorrectUserEntity) {
  auto device = std::make_unique<MockFidoDevice>();

  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetInfo,
      test_data::kTestAuthenticatorGetInfoResponse);
  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetAssertion,
      test_data::kTestGetAssertionResponse);

  auto task = std::make_unique<GetAssertionTask>(
      device.get(),
      CtapGetAssertionRequest(kRpId,
                              fido_parsing_utils::Materialize(kClientDataHash)),
      get_assertion_callback_receiver().callback());

  get_assertion_callback_receiver().WaitForCallback();
  EXPECT_EQ(device->supported_protocol(), ProtocolVersion::kCtap);
  EXPECT_TRUE(device->device_info());
  EXPECT_EQ(CtapDeviceResponseCode::kCtap2ErrOther,
            get_assertion_callback_receiver().status());
  EXPECT_FALSE(get_assertion_callback_receiver().value());
}

TEST_F(FidoGetAssertionTaskTest, TestGetAsserionIncorrectRpIdHash) {
  auto device = std::make_unique<MockFidoDevice>();

  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetInfo,
      test_data::kTestAuthenticatorGetInfoResponse);
  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetAssertion,
      test_data::kTestGetAssertionResponseWithIncorrectRpIdHash);

  auto task = std::make_unique<GetAssertionTask>(
      device.get(),
      CtapGetAssertionRequest(kRpId,
                              fido_parsing_utils::Materialize(kClientDataHash)),
      get_assertion_callback_receiver().callback());

  get_assertion_callback_receiver().WaitForCallback();
  EXPECT_EQ(device->supported_protocol(), ProtocolVersion::kCtap);
  EXPECT_TRUE(device->device_info());
  EXPECT_EQ(CtapDeviceResponseCode::kCtap2ErrOther,
            get_assertion_callback_receiver().status());
  EXPECT_FALSE(get_assertion_callback_receiver().value());
}

TEST_F(FidoGetAssertionTaskTest, TestIncorrectGetAssertionResponse) {
  auto device = std::make_unique<MockFidoDevice>();

  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetInfo,
      test_data::kTestAuthenticatorGetInfoResponse);
  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetAssertion, base::nullopt);

  auto task = std::make_unique<GetAssertionTask>(
      device.get(),
      CtapGetAssertionRequest(kRpId,
                              fido_parsing_utils::Materialize(kClientDataHash)),
      get_assertion_callback_receiver().callback());

  get_assertion_callback_receiver().WaitForCallback();
  EXPECT_EQ(device->supported_protocol(), ProtocolVersion::kCtap);
  EXPECT_TRUE(device->device_info());
  EXPECT_EQ(CtapDeviceResponseCode::kCtap2ErrOther,
            get_assertion_callback_receiver().status());
  EXPECT_FALSE(get_assertion_callback_receiver().value());
}

TEST_F(FidoGetAssertionTaskTest, TestIncompatibleUserVerificationSetting) {
  auto device = std::make_unique<MockFidoDevice>();

  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetInfo,
      test_data::kTestGetInfoResponseWithoutUvSupport);

  auto request = CtapGetAssertionRequest(
      kRpId, fido_parsing_utils::Materialize(kClientDataHash));
  request.SetUserVerification(UserVerificationRequirement::kRequired);

  auto task = std::make_unique<GetAssertionTask>(
      device.get(), std::move(request),
      get_assertion_callback_receiver().callback());

  get_assertion_callback_receiver().WaitForCallback();
  EXPECT_EQ(device->supported_protocol(), ProtocolVersion::kCtap);
  EXPECT_EQ(CtapDeviceResponseCode::kCtap2ErrOther,
            get_assertion_callback_receiver().status());
  EXPECT_FALSE(get_assertion_callback_receiver().value());
}

}  // namespace device
