// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/numerics/safe_conversions.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "device/fido/authenticator_make_credential_response.h"
#include "device/fido/ctap_make_credential_request.h"
#include "device/fido/fido_constants.h"
#include "device/fido/fido_parsing_utils.h"
#include "device/fido/fido_test_data.h"
#include "device/fido/make_credential_task.h"
#include "device/fido/mock_fido_device.h"
#include "device/fido/test_callback_receiver.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;

namespace device {

namespace {

constexpr uint8_t kClientDataHash[] = {0x01, 0x02, 0x03};
constexpr uint8_t kUserId[] = {0x01, 0x02, 0x03};
constexpr char kRpId[] = "acme.com";

using TestMakeCredentialTaskCallback =
    ::device::test::StatusAndValueCallbackReceiver<
        CtapDeviceResponseCode,
        base::Optional<AuthenticatorMakeCredentialResponse>>;

}  // namespace

class FidoMakeCredentialTaskTest : public testing::Test {
 public:
  FidoMakeCredentialTaskTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {}

  std::unique_ptr<MakeCredentialTask> CreateMakeCredentialTask(
      FidoDevice* device) {
    PublicKeyCredentialRpEntity rp(kRpId);
    PublicKeyCredentialUserEntity user(
        fido_parsing_utils::Materialize(kUserId));
    return std::make_unique<MakeCredentialTask>(
        device,
        CtapMakeCredentialRequest(
            fido_parsing_utils::Materialize(kClientDataHash), std::move(rp),
            std::move(user),
            PublicKeyCredentialParams(
                std::vector<PublicKeyCredentialParams::CredentialInfo>(1))),
        AuthenticatorSelectionCriteria(), callback_receiver_.callback());
  }

  std::unique_ptr<MakeCredentialTask>
  CreateMakeCredentialTaskWithAuthenticatorSelectionCriteria(
      FidoDevice* device,
      AuthenticatorSelectionCriteria criteria) {
    PublicKeyCredentialRpEntity rp(kRpId);
    PublicKeyCredentialUserEntity user(
        fido_parsing_utils::Materialize(kUserId));
    return std::make_unique<MakeCredentialTask>(
        device,
        CtapMakeCredentialRequest(
            fido_parsing_utils::Materialize(kClientDataHash), std::move(rp),
            std::move(user),
            PublicKeyCredentialParams(
                std::vector<PublicKeyCredentialParams::CredentialInfo>(1))),
        std::move(criteria), callback_receiver_.callback());
  }

  TestMakeCredentialTaskCallback& make_credential_callback_receiver() {
    return callback_receiver_;
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  TestMakeCredentialTaskCallback callback_receiver_;
};

TEST_F(FidoMakeCredentialTaskTest, TestMakeCredentialSuccess) {
  auto device = std::make_unique<MockFidoDevice>();

  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetInfo,
      test_data::kTestAuthenticatorGetInfoResponse);
  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorMakeCredential,
      test_data::kTestMakeCredentialResponse);

  const auto task = CreateMakeCredentialTask(device.get());
  make_credential_callback_receiver().WaitForCallback();

  EXPECT_EQ(CtapDeviceResponseCode::kSuccess,
            make_credential_callback_receiver().status());
  EXPECT_TRUE(make_credential_callback_receiver().value());
  EXPECT_EQ(device->supported_protocol(), ProtocolVersion::kCtap);
  EXPECT_TRUE(device->device_info());
}

TEST_F(FidoMakeCredentialTaskTest, TestIncorrectAuthenticatorGetInfoResponse) {
  auto device = std::make_unique<MockFidoDevice>();

  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetInfo, base::nullopt);

  const auto task = CreateMakeCredentialTask(device.get());
  make_credential_callback_receiver().WaitForCallback();
  EXPECT_EQ(CtapDeviceResponseCode::kCtap2ErrOther,
            make_credential_callback_receiver().status());
  EXPECT_EQ(ProtocolVersion::kU2f, device->supported_protocol());
  EXPECT_FALSE(device->device_info());
}

TEST_F(FidoMakeCredentialTaskTest, TestMakeCredentialWithIncorrectRpIdHash) {
  auto device = std::make_unique<MockFidoDevice>();

  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetInfo,
      test_data::kTestAuthenticatorGetInfoResponse);
  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorMakeCredential,
      test_data::kTestMakeCredentialResponseWithIncorrectRpIdHash);

  const auto task = CreateMakeCredentialTask(device.get());
  make_credential_callback_receiver().WaitForCallback();

  EXPECT_EQ(CtapDeviceResponseCode::kCtap2ErrOther,
            make_credential_callback_receiver().status());
}

TEST_F(FidoMakeCredentialTaskTest,
       TestUserVerificationAuthenticatorSelectionCriteria) {
  auto device = std::make_unique<MockFidoDevice>();

  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetInfo,
      test_data::kTestGetInfoResponseWithoutUvSupport);

  const auto task = CreateMakeCredentialTaskWithAuthenticatorSelectionCriteria(
      device.get(),
      AuthenticatorSelectionCriteria(
          AuthenticatorSelectionCriteria::AuthenticatorAttachment::kAny,
          false /* require_resident_key */,
          UserVerificationRequirement::kRequired));
  make_credential_callback_receiver().WaitForCallback();

  EXPECT_EQ(CtapDeviceResponseCode::kCtap2ErrOther,
            make_credential_callback_receiver().status());
  EXPECT_EQ(ProtocolVersion::kCtap, device->supported_protocol());
  EXPECT_TRUE(device->device_info());
  EXPECT_EQ(AuthenticatorSupportedOptions::UserVerificationAvailability::
                kSupportedButNotConfigured,
            device->device_info()->options().user_verification_availability());
}

TEST_F(FidoMakeCredentialTaskTest,
       TestPlatformDeviceAuthenticatorSelectionCriteria) {
  auto device = std::make_unique<MockFidoDevice>();

  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetInfo,
      test_data::kTestGetInfoResponseCrossPlatformDevice);

  const auto task = CreateMakeCredentialTaskWithAuthenticatorSelectionCriteria(
      device.get(),
      AuthenticatorSelectionCriteria(
          AuthenticatorSelectionCriteria::AuthenticatorAttachment::kPlatform,
          false /* require_resident_key */,
          UserVerificationRequirement::kPreferred));
  make_credential_callback_receiver().WaitForCallback();

  EXPECT_EQ(CtapDeviceResponseCode::kCtap2ErrOther,
            make_credential_callback_receiver().status());
  EXPECT_EQ(ProtocolVersion::kCtap, device->supported_protocol());
  EXPECT_TRUE(device->device_info());
  EXPECT_FALSE(device->device_info()->options().is_platform_device());
}

TEST_F(FidoMakeCredentialTaskTest,
       TestResidentKeyAuthenticatorSelectionCriteria) {
  auto device = std::make_unique<MockFidoDevice>();

  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetInfo,
      test_data::kTestGetInfoResponseWithoutResidentKeySupport);

  const auto task = CreateMakeCredentialTaskWithAuthenticatorSelectionCriteria(
      device.get(),
      AuthenticatorSelectionCriteria(
          AuthenticatorSelectionCriteria::AuthenticatorAttachment::kAny,
          true /* require_resident_key */,
          UserVerificationRequirement::kPreferred));
  make_credential_callback_receiver().WaitForCallback();

  EXPECT_EQ(CtapDeviceResponseCode::kCtap2ErrOther,
            make_credential_callback_receiver().status());
  EXPECT_EQ(ProtocolVersion::kCtap, device->supported_protocol());
  EXPECT_TRUE(device->device_info());
  EXPECT_FALSE(device->device_info()->options().supports_resident_key());
}

TEST_F(FidoMakeCredentialTaskTest,
       TestSatisfyAllAuthenticatorSelectionCriteria) {
  auto device = std::make_unique<MockFidoDevice>();

  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetInfo,
      test_data::kTestAuthenticatorGetInfoResponse);
  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorMakeCredential,
      test_data::kTestMakeCredentialResponse);

  const auto task = CreateMakeCredentialTaskWithAuthenticatorSelectionCriteria(
      device.get(),
      AuthenticatorSelectionCriteria(
          AuthenticatorSelectionCriteria::AuthenticatorAttachment::kPlatform,
          true /* require_resident_key */,
          UserVerificationRequirement::kRequired));
  make_credential_callback_receiver().WaitForCallback();

  EXPECT_EQ(CtapDeviceResponseCode::kSuccess,
            make_credential_callback_receiver().status());
  EXPECT_TRUE(make_credential_callback_receiver().value());
  EXPECT_EQ(ProtocolVersion::kCtap, device->supported_protocol());
  EXPECT_TRUE(device->device_info());
  const auto& device_options = device->device_info()->options();
  EXPECT_TRUE(device_options.is_platform_device());
  EXPECT_TRUE(device_options.supports_resident_key());
  EXPECT_EQ(AuthenticatorSupportedOptions::UserVerificationAvailability::
                kSupportedAndConfigured,
            device_options.user_verification_availability());
}

TEST_F(FidoMakeCredentialTaskTest, TestIncompatibleUserVerificationSetting) {
  auto device = std::make_unique<MockFidoDevice>();

  device->ExpectCtap2CommandAndRespondWith(
      CtapRequestCommand::kAuthenticatorGetInfo,
      test_data::kTestGetInfoResponseWithoutUvSupport);

  const auto task = CreateMakeCredentialTaskWithAuthenticatorSelectionCriteria(
      device.get(),
      AuthenticatorSelectionCriteria(
          AuthenticatorSelectionCriteria::AuthenticatorAttachment::kAny,
          false /* require_resident_key */,
          UserVerificationRequirement::kRequired));
  make_credential_callback_receiver().WaitForCallback();
  EXPECT_EQ(ProtocolVersion::kCtap, device->supported_protocol());
  EXPECT_EQ(CtapDeviceResponseCode::kCtap2ErrOther,
            make_credential_callback_receiver().status());
  EXPECT_FALSE(make_credential_callback_receiver().value());
}

}  // namespace device
