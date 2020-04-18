// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/u2f_sign.h"

#include <utility>

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "crypto/ec_private_key.h"
#include "device/fido/authenticator_get_assertion_response.h"
#include "device/fido/fake_fido_discovery.h"
#include "device/fido/fido_constants.h"
#include "device/fido/fido_parsing_utils.h"
#include "device/fido/fido_test_data.h"
#include "device/fido/fido_transport_protocol.h"
#include "device/fido/mock_fido_device.h"
#include "device/fido/test_callback_receiver.h"
#include "device/fido/virtual_u2f_device.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;

namespace device {

namespace {

std::vector<uint8_t> GetTestCredentialRawIdBytes() {
  return fido_parsing_utils::Materialize(test_data::kU2fSignKeyHandle);
}

std::vector<uint8_t> GetU2fSignCommandWithCorrectCredential() {
  return fido_parsing_utils::Materialize(test_data::kU2fSignCommandApdu);
}

using TestSignCallback = ::device::test::StatusAndValueCallbackReceiver<
    FidoReturnCode,
    base::Optional<AuthenticatorGetAssertionResponse>>;

}  // namespace

class U2fSignTest : public ::testing::Test {
 public:
  U2fSignTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {}

  void ForgeNextHidDiscovery() {
    discovery_ = scoped_fake_discovery_factory_.ForgeNextHidDiscovery();
  }

  std::unique_ptr<U2fSign> CreateSignRequest() {
    return CreateSignRequestWithKeys({GetTestCredentialRawIdBytes()});
  }

  std::unique_ptr<U2fSign> CreateSignRequestWithKeys(
      std::vector<std::vector<uint8_t>> registered_keys) {
    ForgeNextHidDiscovery();
    return std::make_unique<U2fSign>(
        nullptr /* connector */,
        base::flat_set<FidoTransportProtocol>(
            {FidoTransportProtocol::kUsbHumanInterfaceDevice}),
        std::move(registered_keys),
        fido_parsing_utils::Materialize(test_data::kChallengeParameter),
        fido_parsing_utils::Materialize(test_data::kApplicationParameter),
        base::nullopt /* alt_application_parameter*/,
        sign_callback_receiver_.callback());
  }

  test::FakeFidoDiscovery* discovery() const { return discovery_; }
  TestSignCallback& sign_callback_receiver() { return sign_callback_receiver_; }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  test::ScopedFakeFidoDiscoveryFactory scoped_fake_discovery_factory_;
  test::FakeFidoDiscovery* discovery_;
  TestSignCallback sign_callback_receiver_;
  base::flat_set<FidoTransportProtocol> protocols_;
};

TEST_F(U2fSignTest, TestSignSuccess) {
  auto request = CreateSignRequest();
  request->Start();
  discovery()->WaitForCallToStartAndSimulateSuccess();

  auto device = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device, GetId()).WillRepeatedly(testing::Return("device"));
  EXPECT_CALL(*device,
              DeviceTransactPtr(GetU2fSignCommandWithCorrectCredential(), _))
      .WillOnce(testing::Invoke(MockFidoDevice::NoErrorSign));
  EXPECT_CALL(*device, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device));

  sign_callback_receiver().WaitForCallback();
  EXPECT_EQ(FidoReturnCode::kSuccess, sign_callback_receiver().status());

  // Correct key was sent so a sign response is expected.
  EXPECT_THAT(sign_callback_receiver().value()->signature(),
              ::testing::ElementsAreArray(test_data::kU2fSignature));

  // Verify that we get the key handle used for signing.
  EXPECT_THAT(GetTestCredentialRawIdBytes(),
              ::testing::ElementsAreArray(
                  sign_callback_receiver().value()->raw_credential_id()));
}

TEST_F(U2fSignTest, TestSignSuccessWithFake) {
  auto private_key = crypto::ECPrivateKey::Create();
  std::string public_key;
  private_key->ExportRawPublicKey(&public_key);

  auto key_handle = fido_parsing_utils::CreateSHA256Hash(public_key);
  std::vector<std::vector<uint8_t>> handles{key_handle};
  auto request = CreateSignRequestWithKeys(handles);
  request->Start();
  discovery()->WaitForCallToStartAndSimulateSuccess();

  auto device = std::make_unique<VirtualU2fDevice>();
  device->mutable_state()->registrations.emplace(
      key_handle,
      VirtualFidoDevice::RegistrationData(
          std::move(private_key),
          fido_parsing_utils::Materialize(test_data::kApplicationParameter),
          42));
  discovery()->AddDevice(std::move(device));

  sign_callback_receiver().WaitForCallback();
  EXPECT_EQ(FidoReturnCode::kSuccess, sign_callback_receiver().status());

  // Just a sanity check, we don't verify the actual signature.
  ASSERT_GE(32u + 1u + 4u + 8u,  // Minimal ECDSA signature is 8 bytes
            sign_callback_receiver()
                .value()
                ->auth_data()
                .SerializeToByteArray()
                .size());
  EXPECT_EQ(0x01,
            sign_callback_receiver()
                .value()
                ->auth_data()
                .SerializeToByteArray()[32]);  // UP flag
  EXPECT_EQ(43, sign_callback_receiver()
                    .value()
                    ->auth_data()
                    .SerializeToByteArray()[36]);  // counter
}

TEST_F(U2fSignTest, TestDelayedSuccess) {
  auto request = CreateSignRequest();
  request->Start();
  discovery()->WaitForCallToStartAndSimulateSuccess();

  // Go through the state machine twice before success.
  auto device = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device, GetId()).WillRepeatedly(::testing::Return("device"));
  EXPECT_CALL(*device,
              DeviceTransactPtr(GetU2fSignCommandWithCorrectCredential(), _))
      .WillOnce(::testing::Invoke(MockFidoDevice::NotSatisfied))
      .WillOnce(::testing::Invoke(MockFidoDevice::NoErrorSign));
  EXPECT_CALL(*device, TryWinkRef(_))
      .Times(2)
      .WillRepeatedly(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device));

  sign_callback_receiver().WaitForCallback();
  EXPECT_EQ(FidoReturnCode::kSuccess, sign_callback_receiver().status());

  // Correct key was sent so a sign response is expected.
  EXPECT_THAT(sign_callback_receiver().value()->signature(),
              ::testing::ElementsAreArray(test_data::kU2fSignature));

  // Verify that we get the key handle used for signing.
  EXPECT_THAT(GetTestCredentialRawIdBytes(),
              ::testing::ElementsAreArray(
                  sign_callback_receiver().value()->raw_credential_id()));
}

TEST_F(U2fSignTest, TestMultipleHandles) {
  // Two wrong keys followed by a correct key ensuring the wrong keys will be
  // tested first.
  const auto correct_key_handle = GetTestCredentialRawIdBytes();
  auto request = CreateSignRequestWithKeys(
      {fido_parsing_utils::Materialize(test_data::kKeyHandleAlpha),
       fido_parsing_utils::Materialize(test_data::kKeyHandleBeta),
       correct_key_handle});
  request->Start();
  discovery()->WaitForCallToStartAndSimulateSuccess();

  auto device = std::make_unique<MockFidoDevice>();
  // Wrong key would respond with SW_WRONG_DATA.
  EXPECT_CALL(*device, GetId()).WillRepeatedly(::testing::Return("device"));
  EXPECT_CALL(*device,
              DeviceTransactPtr(fido_parsing_utils::Materialize(
                                    test_data::kU2fSignCommandApduWithKeyAlpha),
                                _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));
  EXPECT_CALL(*device,
              DeviceTransactPtr(fido_parsing_utils::Materialize(
                                    test_data::kU2fSignCommandApduWithKeyBeta),
                                _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));

  EXPECT_CALL(*device,
              DeviceTransactPtr(GetU2fSignCommandWithCorrectCredential(), _))
      .WillOnce(::testing::Invoke(MockFidoDevice::NoErrorSign));

  // Only one wink expected per device.
  EXPECT_CALL(*device, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device));

  sign_callback_receiver().WaitForCallback();
  EXPECT_EQ(FidoReturnCode::kSuccess, sign_callback_receiver().status());

  // Correct key was sent so a sign response is expected.
  EXPECT_THAT(sign_callback_receiver().value()->signature(),
              ::testing::ElementsAreArray(test_data::kU2fSignature));
  // Verify that we get the key handle used for signing.
  EXPECT_EQ(correct_key_handle,
            sign_callback_receiver().value()->raw_credential_id());
}

TEST_F(U2fSignTest, TestMultipleDevices) {
  const auto correct_key_handle = GetTestCredentialRawIdBytes();
  auto request = CreateSignRequestWithKeys(
      {GetTestCredentialRawIdBytes(),
       fido_parsing_utils::Materialize(test_data::kKeyHandleAlpha)});
  request->Start();

  auto device0 = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device0, GetId()).WillRepeatedly(::testing::Return("device0"));
  EXPECT_CALL(*device0,
              DeviceTransactPtr(GetU2fSignCommandWithCorrectCredential(), _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));
  EXPECT_CALL(*device0,
              DeviceTransactPtr(fido_parsing_utils::Materialize(
                                    test_data::kU2fSignCommandApduWithKeyAlpha),
                                _))
      .WillOnce(::testing::Invoke(MockFidoDevice::NotSatisfied));
  // One wink per device.
  EXPECT_CALL(*device0, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device0));

  // Second device will have a successful touch.
  auto device1 = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device1, GetId()).WillRepeatedly(::testing::Return("device1"));
  EXPECT_CALL(*device1,
              DeviceTransactPtr(GetU2fSignCommandWithCorrectCredential(), _))
      .WillOnce(::testing::Invoke(MockFidoDevice::NoErrorSign));

  // One wink per device.
  EXPECT_CALL(*device1, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device1));
  discovery()->WaitForCallToStartAndSimulateSuccess();

  sign_callback_receiver().WaitForCallback();
  EXPECT_EQ(FidoReturnCode::kSuccess, sign_callback_receiver().status());

  // Correct key was sent so a sign response is expected.
  EXPECT_THAT(sign_callback_receiver().value()->signature(),
              ::testing::ElementsAreArray(test_data::kU2fSignature));

  // Verify that we get the key handle used for signing.
  EXPECT_EQ(correct_key_handle,
            sign_callback_receiver().value()->raw_credential_id());
}

TEST_F(U2fSignTest, TestFakeEnroll) {
  auto request = CreateSignRequestWithKeys(
      {fido_parsing_utils::Materialize(test_data::kKeyHandleAlpha),
       fido_parsing_utils::Materialize(test_data::kKeyHandleBeta)});
  request->Start();

  auto device0 = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device0,
              DeviceTransactPtr(fido_parsing_utils::Materialize(
                                    test_data::kU2fSignCommandApduWithKeyAlpha),
                                _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));

  EXPECT_CALL(*device0,
              DeviceTransactPtr(fido_parsing_utils::Materialize(
                                    test_data::kU2fSignCommandApduWithKeyBeta),
                                _))
      .WillOnce(::testing::Invoke(MockFidoDevice::NotSatisfied));
  // One wink per device.
  EXPECT_CALL(*device0, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device0));

  // Second device will be have a successful touch.
  auto device1 = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device1, GetId()).WillRepeatedly(::testing::Return("device1"));
  // Both keys will be tried, when both fail, register is tried on that device.
  EXPECT_CALL(*device1,
              DeviceTransactPtr(fido_parsing_utils::Materialize(
                                    test_data::kU2fSignCommandApduWithKeyAlpha),
                                _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));
  EXPECT_CALL(*device1,
              DeviceTransactPtr(fido_parsing_utils::Materialize(
                                    test_data::kU2fSignCommandApduWithKeyBeta),
                                _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));
  EXPECT_CALL(*device1,
              DeviceTransactPtr(fido_parsing_utils::Materialize(
                                    test_data::kU2fFakeRegisterCommand),
                                _))
      .WillOnce(::testing::Invoke(MockFidoDevice::NoErrorRegister));

  // One wink per device.
  EXPECT_CALL(*device1, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device1));
  discovery()->WaitForCallToStartAndSimulateSuccess();

  sign_callback_receiver().WaitForCallback();
  // Device that responded had no correct keys.
  EXPECT_EQ(FidoReturnCode::kUserConsentButCredentialNotRecognized,
            sign_callback_receiver().status());
  EXPECT_FALSE(sign_callback_receiver().value());
}

TEST_F(U2fSignTest, TestFakeEnrollErroringOut) {
  auto request = CreateSignRequest();
  request->Start();
  // First device errors out on all requests (including the sign request and
  // fake registration attempt). The device should then be abandoned to prevent
  // the test from crashing or timing out.
  auto device0 = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device0, GetId()).WillRepeatedly(::testing::Return("device0"));
  EXPECT_CALL(*device0,
              DeviceTransactPtr(GetU2fSignCommandWithCorrectCredential(), _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));
  EXPECT_CALL(*device0,
              DeviceTransactPtr(fido_parsing_utils::Materialize(
                                    test_data::kU2fFakeRegisterCommand),
                                _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));
  // One wink per device.
  EXPECT_CALL(*device0, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device0));

  // Second device will have a successful touch and sign on the first attempt.
  auto device1 = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device1, GetId()).WillRepeatedly(::testing::Return("device1"));
  EXPECT_CALL(*device1,
              DeviceTransactPtr(GetU2fSignCommandWithCorrectCredential(), _))
      .WillOnce(::testing::Invoke(MockFidoDevice::NoErrorSign));
  // One wink per device.
  EXPECT_CALL(*device1, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device1));
  discovery()->WaitForCallToStartAndSimulateSuccess();

  // Correct key was sent so a sign response is expected.
  sign_callback_receiver().WaitForCallback();
  EXPECT_THAT(sign_callback_receiver().value()->signature(),
              ::testing::ElementsAreArray(test_data::kU2fSignature));
}

// Device returns success, but the response is unparse-able.
TEST_F(U2fSignTest, TestSignWithCorruptedResponse) {
  auto request = CreateSignRequest();
  request->Start();
  discovery()->WaitForCallToStartAndSimulateSuccess();

  auto device = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device, GetId()).WillRepeatedly(::testing::Return("device"));
  EXPECT_CALL(*device,
              DeviceTransactPtr(GetU2fSignCommandWithCorrectCredential(), _))
      .WillOnce(::testing::Invoke(MockFidoDevice::SignWithCorruptedResponse));
  EXPECT_CALL(*device, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device));

  sign_callback_receiver().WaitForCallback();
  EXPECT_EQ(FidoReturnCode::kAuthenticatorResponseInvalid,
            sign_callback_receiver().status());
  EXPECT_FALSE(sign_callback_receiver().value());
}

MATCHER_P(WithApplicationParameter, expected, "") {
  // See
  // https://fidoalliance.org/specs/fido-u2f-v1.2-ps-20170411/fido-u2f-raw-message-formats-v1.2-ps-20170411.html#request-message-framing
  // and
  // https://fidoalliance.org/specs/fido-u2f-v1.2-ps-20170411/fido-u2f-raw-message-formats-v1.2-ps-20170411.html#authentication-request-message---u2f_authenticate
  constexpr size_t kAppParamOffset = 4 /* CLA + INS + P1 + P2 */ +
                                     3 /* Extended Lc */ +
                                     32 /* Challenge Parameter */;
  constexpr size_t kAppParamLength = 32;
  if (arg.size() < kAppParamOffset + kAppParamLength) {
    return false;
  }

  auto application_parameter =
      base::make_span(arg).subspan(kAppParamOffset, kAppParamLength);

  return std::equal(application_parameter.begin(), application_parameter.end(),
                    expected.begin(), expected.end());
}

TEST_F(U2fSignTest, TestAlternativeApplicationParameter) {
  const std::vector<uint8_t> signing_key_handle(32, 0x0A);
  const std::vector<uint8_t> primary_app_param(32, 1);
  const std::vector<uint8_t> alt_app_param(32, 2);

  ForgeNextHidDiscovery();
  auto request = std::make_unique<U2fSign>(
      nullptr /* connector */,
      base::flat_set<FidoTransportProtocol>(
          {FidoTransportProtocol::kUsbHumanInterfaceDevice}),
      std::vector<std::vector<uint8_t>>({signing_key_handle}),
      std::vector<uint8_t>(32), primary_app_param, alt_app_param,
      sign_callback_receiver_.callback());
  request->Start();
  discovery()->WaitForCallToStartAndSimulateSuccess();

  auto device = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device, GetId()).WillRepeatedly(::testing::Return("device"));
  // The first request will use the primary app_param, which will be rejected.
  EXPECT_CALL(*device,
              DeviceTransactPtr(WithApplicationParameter(primary_app_param), _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));
  // After the rejection, the alternative should be tried.
  EXPECT_CALL(*device,
              DeviceTransactPtr(WithApplicationParameter(alt_app_param), _))
      .WillOnce(::testing::Invoke(MockFidoDevice::NoErrorSign));
  EXPECT_CALL(*device, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device));

  sign_callback_receiver().WaitForCallback();
  EXPECT_EQ(FidoReturnCode::kSuccess, sign_callback_receiver().status());

  EXPECT_THAT(sign_callback_receiver().value()->signature(),
              ::testing::ElementsAreArray(test_data::kU2fSignature));
  EXPECT_EQ(signing_key_handle,
            sign_callback_receiver().value()->raw_credential_id());
}

// This is a regression test in response to https://crbug.com/833398.
TEST_F(U2fSignTest, TestAlternativeApplicationParameterRejection) {
  const std::vector<uint8_t> signing_key_handle(32, 0x0A);
  const std::vector<uint8_t> primary_app_param(32, 1);
  const std::vector<uint8_t> alt_app_param(32, 2);

  ForgeNextHidDiscovery();
  auto request = std::make_unique<U2fSign>(
      nullptr /* connector */,
      base::flat_set<FidoTransportProtocol>(
          {FidoTransportProtocol::kUsbHumanInterfaceDevice}),
      std::vector<std::vector<uint8_t>>({signing_key_handle}),
      std::vector<uint8_t>(32), primary_app_param, alt_app_param,
      sign_callback_receiver_.callback());
  request->Start();
  discovery()->WaitForCallToStartAndSimulateSuccess();

  auto device = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device, GetId()).WillRepeatedly(::testing::Return("device"));
  EXPECT_CALL(*device, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));

  // The first request will use the primary app_param, which will be rejected.
  EXPECT_CALL(*device,
              DeviceTransactPtr(WithApplicationParameter(primary_app_param), _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));

  // After the rejection, the alternative should be tried, which will also be
  // rejected.
  EXPECT_CALL(*device,
              DeviceTransactPtr(WithApplicationParameter(alt_app_param), _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));

  // The second rejection will trigger a bogus register command. This will be
  // rejected as well, triggering the device to be abandoned.
  EXPECT_CALL(*device,
              DeviceTransactPtr(WithApplicationParameter(kBogusAppParam), _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));

  discovery()->AddDevice(std::move(device));
}

}  // namespace device
