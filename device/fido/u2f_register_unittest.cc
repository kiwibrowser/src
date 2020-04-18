// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/u2f_register.h"

#include <iterator>
#include <utility>

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "device/fido/authenticator_make_credential_response.h"
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

namespace device {

using ::testing::_;

namespace {

std::vector<uint8_t> GetTestRegisterRequest() {
  return fido_parsing_utils::Materialize(test_data::kU2fRegisterCommandApdu);
}

using TestRegisterCallback = ::device::test::StatusAndValueCallbackReceiver<
    FidoReturnCode,
    base::Optional<AuthenticatorMakeCredentialResponse>>;

}  // namespace

class U2fRegisterTest : public ::testing::Test {
 public:
  void ForgeNextHidDiscovery() {
    discovery_ = scoped_fake_discovery_factory_.ForgeNextHidDiscovery();
  }

  std::unique_ptr<U2fRegister> CreateRegisterRequest() {
    return CreateRegisterRequestWithRegisteredKeys(
        std::vector<std::vector<uint8_t>>());
  }

  // Creates a U2F register request with `none` attestation, and the given
  // previously |registered_keys|.
  std::unique_ptr<U2fRegister> CreateRegisterRequestWithRegisteredKeys(
      std::vector<std::vector<uint8_t>> registered_keys) {
    ForgeNextHidDiscovery();
    return std::make_unique<U2fRegister>(
        nullptr /* connector */,
        base::flat_set<FidoTransportProtocol>(
            {FidoTransportProtocol::kUsbHumanInterfaceDevice}),
        registered_keys,
        fido_parsing_utils::Materialize(test_data::kChallengeParameter),
        fido_parsing_utils::Materialize(test_data::kApplicationParameter),
        false /* is_individual_attestation */,
        register_callback_receiver_.callback());
  }

  test::FakeFidoDiscovery* discovery() const { return discovery_; }

  TestRegisterCallback& register_callback_receiver() {
    return register_callback_receiver_;
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::vector<std::vector<uint8_t>> key_handles_;
  base::flat_set<FidoTransportProtocol> protocols_;
  test::ScopedFakeFidoDiscoveryFactory scoped_fake_discovery_factory_;
  test::FakeFidoDiscovery* discovery_;
  TestRegisterCallback register_callback_receiver_;
};


TEST_F(U2fRegisterTest, TestRegisterSuccess) {
  auto request = CreateRegisterRequest();
  request->Start();
  discovery()->WaitForCallToStartAndSimulateSuccess();

  auto device = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device, GetId()).WillRepeatedly(testing::Return("device"));
  EXPECT_CALL(*device, DeviceTransactPtr(GetTestRegisterRequest(), _))
      .WillOnce(testing::Invoke(MockFidoDevice::NoErrorRegister));
  EXPECT_CALL(*device, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device));

  register_callback_receiver().WaitForCallback();
  EXPECT_EQ(FidoReturnCode::kSuccess, register_callback_receiver().status());
  ASSERT_TRUE(register_callback_receiver().value());
  EXPECT_THAT(register_callback_receiver().value()->raw_credential_id(),
              ::testing::ElementsAreArray(test_data::kU2fSignKeyHandle));
}

TEST_F(U2fRegisterTest, TestRegisterSuccessWithFake) {
  auto request = CreateRegisterRequest();
  request->Start();
  discovery()->WaitForCallToStartAndSimulateSuccess();

  auto device = std::make_unique<VirtualU2fDevice>();
  discovery()->AddDevice(std::move(device));

  register_callback_receiver().WaitForCallback();
  EXPECT_EQ(FidoReturnCode::kSuccess, register_callback_receiver().status());

  // We don't verify the response from the fake, but do a quick sanity check.
  ASSERT_TRUE(register_callback_receiver().value());
  EXPECT_EQ(32ul,
            register_callback_receiver().value()->raw_credential_id().size());
}

TEST_F(U2fRegisterTest, TestDelayedSuccess) {
  auto request = CreateRegisterRequest();
  request->Start();
  discovery()->WaitForCallToStartAndSimulateSuccess();

  auto device = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device, GetId()).WillRepeatedly(testing::Return("device"));
  // Go through the state machine twice before success.
  EXPECT_CALL(*device, DeviceTransactPtr(GetTestRegisterRequest(), _))
      .WillOnce(::testing::Invoke(MockFidoDevice::NotSatisfied))
      .WillOnce(::testing::Invoke(MockFidoDevice::NoErrorRegister));
  EXPECT_CALL(*device, TryWinkRef(_))
      .Times(2)
      .WillRepeatedly(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device));

  register_callback_receiver().WaitForCallback();
  EXPECT_EQ(FidoReturnCode::kSuccess, register_callback_receiver().status());
  ASSERT_TRUE(register_callback_receiver().value());
  EXPECT_THAT(register_callback_receiver().value()->raw_credential_id(),
              ::testing::ElementsAreArray(test_data::kU2fSignKeyHandle));
}

TEST_F(U2fRegisterTest, TestMultipleDevices) {
  auto request = CreateRegisterRequest();
  request->Start();

  auto device0 = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device0, GetId()).WillRepeatedly(::testing::Return("device0"));
  EXPECT_CALL(*device0, DeviceTransactPtr(GetTestRegisterRequest(), _))
      .WillOnce(::testing::Invoke(MockFidoDevice::NotSatisfied));
  // One wink per device.
  EXPECT_CALL(*device0, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device0));

  // Second device will have a successful touch.
  auto device1 = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device1, GetId()).WillRepeatedly(::testing::Return("device1"));
  EXPECT_CALL(*device1, DeviceTransactPtr(GetTestRegisterRequest(), _))
      .WillOnce(::testing::Invoke(MockFidoDevice::NoErrorRegister));
  // One wink per device.
  EXPECT_CALL(*device1, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device1));
  discovery()->WaitForCallToStartAndSimulateSuccess();

  register_callback_receiver().WaitForCallback();
  EXPECT_EQ(FidoReturnCode::kSuccess, register_callback_receiver().status());
  ASSERT_TRUE(register_callback_receiver().value());
  EXPECT_THAT(register_callback_receiver().value()->raw_credential_id(),
              ::testing::ElementsAreArray(test_data::kU2fSignKeyHandle));
}

// Tests a scenario where a single device is connected and registration call
// is received with three unknown key handles. We expect that three check only
// sign-in calls be processed before registration.
TEST_F(U2fRegisterTest, TestSingleDeviceRegistrationWithExclusionList) {
  // Simulate three unknown key handles.
  auto request = CreateRegisterRequestWithRegisteredKeys(
      {fido_parsing_utils::Materialize(test_data::kKeyHandleAlpha),
       fido_parsing_utils::Materialize(test_data::kKeyHandleBeta)});
  request->Start();
  discovery()->WaitForCallToStartAndSimulateSuccess();

  auto device = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device, GetId()).WillRepeatedly(::testing::Return("device"));
  // DeviceTransact() will be called three times including two check
  // only sign-in calls and one registration call. For the first two calls,
  // device will invoke MockFidoDevice::WrongData as the authenticator did not
  // create the two key handles provided in the exclude list. At the third
  // call, MockFidoDevice::NoErrorRegister will be invoked after registration.
  EXPECT_CALL(*device.get(),
              DeviceTransactPtr(
                  fido_parsing_utils::Materialize(
                      test_data::kU2fCheckOnlySignCommandApduWithKeyAlpha),
                  _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));

  EXPECT_CALL(
      *device.get(),
      DeviceTransactPtr(fido_parsing_utils::Materialize(
                            test_data::kU2fCheckOnlySignCommandApduWithKeyBeta),
                        _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));

  EXPECT_CALL(*device.get(), DeviceTransactPtr(GetTestRegisterRequest(), _))
      .WillOnce(::testing::Invoke(MockFidoDevice::NoErrorRegister));

  // TryWink() will be called twice. First during the check only sign-in. After
  // check only sign operation is complete, request state is changed to IDLE,
  // and TryWink() is called again before Register() is called.
  EXPECT_CALL(*device, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device));

  register_callback_receiver().WaitForCallback();
  ASSERT_TRUE(register_callback_receiver().value());
  EXPECT_EQ(FidoReturnCode::kSuccess, register_callback_receiver().status());
  EXPECT_THAT(register_callback_receiver().value()->raw_credential_id(),
              ::testing::ElementsAreArray(test_data::kU2fSignKeyHandle));
}

// Tests a scenario where two devices are connected and registration call is
// received with three unknown key handles. We assume that user will proceed the
// registration with second device, "device1".
TEST_F(U2fRegisterTest, TestMultipleDeviceRegistrationWithExclusionList) {
  // Simulate three unknown key handles.
  auto request = CreateRegisterRequestWithRegisteredKeys(
      {fido_parsing_utils::Materialize(test_data::kKeyHandleAlpha),
       fido_parsing_utils::Materialize(test_data::kKeyHandleBeta),
       fido_parsing_utils::Materialize(test_data::kKeyHandleGamma)});
  request->Start();

  auto device0 = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device0, GetId()).WillRepeatedly(::testing::Return("device0"));
  // DeviceTransact() will be called four times: three times to check for
  // duplicate key handles and once for registration. Since user
  // will register using "device1", the fourth call will invoke
  // MockFidoDevice::NotSatisfied.
  EXPECT_CALL(*device0.get(),
              DeviceTransactPtr(
                  fido_parsing_utils::Materialize(
                      test_data::kU2fCheckOnlySignCommandApduWithKeyAlpha),
                  _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));

  EXPECT_CALL(
      *device0.get(),
      DeviceTransactPtr(fido_parsing_utils::Materialize(
                            test_data::kU2fCheckOnlySignCommandApduWithKeyBeta),
                        _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));

  EXPECT_CALL(*device0.get(),
              DeviceTransactPtr(
                  fido_parsing_utils::Materialize(
                      test_data::kU2fCheckOnlySignCommandApduWithKeyGamma),
                  _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));

  EXPECT_CALL(*device0.get(), DeviceTransactPtr(GetTestRegisterRequest(), _))
      .WillOnce(::testing::Invoke(MockFidoDevice::NotSatisfied));

  // TryWink() will be called twice on both devices -- during check only
  // sign-in operation and during registration attempt.
  EXPECT_CALL(*device0, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device0));

  auto device1 = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device1, GetId()).WillRepeatedly(testing::Return("device1"));
  // We assume that user registers with second device. Therefore, the fourth
  // DeviceTransact() will invoke MockFidoDevice::NoErrorRegister after
  // successful registration.
  EXPECT_CALL(*device1.get(),
              DeviceTransactPtr(
                  fido_parsing_utils::Materialize(
                      test_data::kU2fCheckOnlySignCommandApduWithKeyAlpha),
                  _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));

  EXPECT_CALL(
      *device1.get(),
      DeviceTransactPtr(fido_parsing_utils::Materialize(
                            test_data::kU2fCheckOnlySignCommandApduWithKeyBeta),
                        _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));

  EXPECT_CALL(*device1.get(),
              DeviceTransactPtr(
                  fido_parsing_utils::Materialize(
                      test_data::kU2fCheckOnlySignCommandApduWithKeyGamma),
                  _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));

  EXPECT_CALL(*device1.get(), DeviceTransactPtr(GetTestRegisterRequest(), _))
      .WillOnce(::testing::Invoke(MockFidoDevice::NoErrorRegister));

  // TryWink() will be called twice on both devices -- during check only
  // sign-in operation and during registration attempt.
  EXPECT_CALL(*device1, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device1));
  discovery()->WaitForCallToStartAndSimulateSuccess();

  register_callback_receiver().WaitForCallback();
  EXPECT_EQ(FidoReturnCode::kSuccess, register_callback_receiver().status());
  ASSERT_TRUE(register_callback_receiver().value());
  EXPECT_THAT(register_callback_receiver().value()->raw_credential_id(),
              ::testing::ElementsAreArray(test_data::kU2fSignKeyHandle));
}

// Tests a scenario where single device is connected and registration is called
// with a key in the exclude list that was created by this device. We assume
// that the duplicate key is the last key handle in the exclude list. Therefore,
// after duplicate key handle is found, the process is expected to terminate
// after calling bogus registration which checks for user presence.
TEST_F(U2fRegisterTest, TestSingleDeviceRegistrationWithDuplicateHandle) {
  // Simulate three unknown key handles followed by a duplicate key.
  auto request = CreateRegisterRequestWithRegisteredKeys(
      {fido_parsing_utils::Materialize(test_data::kKeyHandleAlpha),
       fido_parsing_utils::Materialize(test_data::kKeyHandleBeta),
       fido_parsing_utils::Materialize(test_data::kKeyHandleGamma)});
  request->Start();
  discovery()->WaitForCallToStartAndSimulateSuccess();

  auto device = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device, GetId()).WillRepeatedly(::testing::Return("device"));
  // For four keys in exclude list, the first three keys will invoke
  // MockFidoDevice::WrongData and the final duplicate key handle will invoke
  // MockFidoDevice::NoErrorSign. Once duplicate key handle is found, bogus
  // registration is called to confirm user presence. This invokes
  // MockFidoDevice::NoErrorRegister.
  EXPECT_CALL(*device.get(),
              DeviceTransactPtr(
                  fido_parsing_utils::Materialize(
                      test_data::kU2fCheckOnlySignCommandApduWithKeyAlpha),
                  _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));

  EXPECT_CALL(
      *device.get(),
      DeviceTransactPtr(fido_parsing_utils::Materialize(
                            test_data::kU2fCheckOnlySignCommandApduWithKeyBeta),
                        _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));

  EXPECT_CALL(*device.get(),
              DeviceTransactPtr(
                  fido_parsing_utils::Materialize(
                      test_data::kU2fCheckOnlySignCommandApduWithKeyGamma),
                  _))
      .WillOnce(::testing::Invoke(MockFidoDevice::NoErrorSign));

  EXPECT_CALL(*device.get(),
              DeviceTransactPtr(fido_parsing_utils::Materialize(
                                    test_data::kU2fFakeRegisterCommand),
                                _))
      .WillOnce(::testing::Invoke(MockFidoDevice::NoErrorRegister));

  // Since duplicate key handle is found, registration process is terminated
  // before actual Register() is called on the device. Therefore, TryWink() is
  // invoked once.
  EXPECT_CALL(*device, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device));

  register_callback_receiver().WaitForCallback();
  EXPECT_EQ(FidoReturnCode::kUserConsentButCredentialExcluded,
            register_callback_receiver().status());
  EXPECT_FALSE(register_callback_receiver().value());
}

// Tests a scenario where one (device1) of the two devices connected has created
// a key handle provided in exclude list. We assume that duplicate key is the
// third key handle provided in the exclude list.
TEST_F(U2fRegisterTest, TestMultipleDeviceRegistrationWithDuplicateHandle) {
  // Simulate two unknown key handles followed by a duplicate key.
  auto request = CreateRegisterRequestWithRegisteredKeys(
      {fido_parsing_utils::Materialize(test_data::kKeyHandleAlpha),
       fido_parsing_utils::Materialize(test_data::kKeyHandleBeta),
       fido_parsing_utils::Materialize(test_data::kKeyHandleGamma)});
  request->Start();

  // Since the first device did not create any of the key handles provided in
  // exclude list, we expect that check only sign() should be called
  // four times, and all the calls to DeviceTransact() invoke
  // MockFidoDevice::WrongData.
  auto device0 = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device0, GetId()).WillRepeatedly(testing::Return("device0"));
  EXPECT_CALL(*device0.get(),
              DeviceTransactPtr(
                  fido_parsing_utils::Materialize(
                      test_data::kU2fCheckOnlySignCommandApduWithKeyAlpha),
                  _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));

  EXPECT_CALL(
      *device0.get(),
      DeviceTransactPtr(fido_parsing_utils::Materialize(
                            test_data::kU2fCheckOnlySignCommandApduWithKeyBeta),
                        _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));

  EXPECT_CALL(*device0.get(),
              DeviceTransactPtr(
                  fido_parsing_utils::Materialize(
                      test_data::kU2fCheckOnlySignCommandApduWithKeyGamma),
                  _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));

  EXPECT_CALL(*device0, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device0));

  // Since the last key handle in exclude list is a duplicate key, we expect
  // that the first three calls to check only sign() invoke
  // MockFidoDevice::WrongData and that fourth sign() call invoke
  // MockFidoDevice::NoErrorSign. After duplicate key is found, process is
  // terminated after user presence is verified using bogus registration, which
  // invokes MockFidoDevice::NoErrorRegister.
  auto device1 = std::make_unique<MockFidoDevice>();
  EXPECT_CALL(*device1, GetId()).WillRepeatedly(::testing::Return("device1"));
  EXPECT_CALL(*device1.get(),
              DeviceTransactPtr(
                  fido_parsing_utils::Materialize(
                      test_data::kU2fCheckOnlySignCommandApduWithKeyAlpha),
                  _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));

  EXPECT_CALL(
      *device1.get(),
      DeviceTransactPtr(fido_parsing_utils::Materialize(
                            test_data::kU2fCheckOnlySignCommandApduWithKeyBeta),
                        _))
      .WillOnce(::testing::Invoke(MockFidoDevice::WrongData));

  EXPECT_CALL(*device1.get(),
              DeviceTransactPtr(
                  fido_parsing_utils::Materialize(
                      test_data::kU2fCheckOnlySignCommandApduWithKeyGamma),
                  _))
      .WillOnce(::testing::Invoke(MockFidoDevice::NoErrorSign));

  EXPECT_CALL(*device1.get(),
              DeviceTransactPtr(fido_parsing_utils::Materialize(
                                    test_data::kU2fFakeRegisterCommand),
                                _))
      .WillOnce(::testing::Invoke(MockFidoDevice::NoErrorRegister));

  EXPECT_CALL(*device1, TryWinkRef(_))
      .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
  discovery()->AddDevice(std::move(device1));
  discovery()->WaitForCallToStartAndSimulateSuccess();

  register_callback_receiver().WaitForCallback();
  EXPECT_EQ(FidoReturnCode::kUserConsentButCredentialExcluded,
            register_callback_receiver().status());
  EXPECT_FALSE(register_callback_receiver().value());
}

MATCHER_P(IndicatesIndividualAttestation, expected, "") {
  return arg.size() > 2 && ((arg[2] & 0x80) == 0x80) == expected;
}

TEST_F(U2fRegisterTest, TestIndividualAttestation) {
  // Test that the individual attestation flag is correctly reflected in the
  // resulting registration APDU.
  for (const auto& individual_attestation : {false, true}) {
    SCOPED_TRACE(individual_attestation);

    ForgeNextHidDiscovery();
    TestRegisterCallback cb;
    auto request = std::make_unique<U2fRegister>(
        nullptr /* connector */,
        base::flat_set<FidoTransportProtocol>(
            {FidoTransportProtocol::kUsbHumanInterfaceDevice}) /* transports */,
        key_handles_,
        fido_parsing_utils::Materialize(test_data::kChallengeParameter),
        fido_parsing_utils::Materialize(test_data::kApplicationParameter),
        individual_attestation, cb.callback());
    request->Start();
    discovery()->WaitForCallToStartAndSimulateSuccess();

    auto device = std::make_unique<MockFidoDevice>();
    EXPECT_CALL(*device, GetId()).WillRepeatedly(::testing::Return("device"));
    EXPECT_CALL(*device,
                DeviceTransactPtr(
                    IndicatesIndividualAttestation(individual_attestation), _))
        .WillOnce(::testing::Invoke(MockFidoDevice::NoErrorRegister));
    EXPECT_CALL(*device, TryWinkRef(_))
        .WillOnce(::testing::Invoke(MockFidoDevice::WinkDoNothing));
    discovery()->AddDevice(std::move(device));

    cb.WaitForCallback();
    EXPECT_EQ(FidoReturnCode::kSuccess, cb.status());
    ASSERT_TRUE(cb.value());
    EXPECT_THAT(cb.value()->raw_credential_id(),
                ::testing::ElementsAreArray(test_data::kU2fSignKeyHandle));
  }
}

}  // namespace device
