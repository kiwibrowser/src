// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/software_feature_manager_impl.h"

#include "base/bind.h"
#include "base/macros.h"
#include "components/cryptauth/mock_cryptauth_client.h"
#include "components/cryptauth/remote_device_ref.h"
#include "components/cryptauth/remote_device_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Invoke;

namespace cryptauth {

namespace {

const char kSuccessResult[] = "success";
const char kErrorSetSoftwareFeatureState[] = "setSoftwareFeatureStateError";
const char kErrorFindEligibleDevices[] = "findEligibleDevicesError";

std::vector<cryptauth::ExternalDeviceInfo>
CreateExternalDeviceInfosForRemoteDevices(
    const cryptauth::RemoteDeviceRefList remote_devices) {
  std::vector<cryptauth::ExternalDeviceInfo> device_infos;
  for (const auto& remote_device : remote_devices) {
    // Add an ExternalDeviceInfo with the same public key as the RemoteDevice.
    cryptauth::ExternalDeviceInfo info;
    info.set_public_key(remote_device.public_key());
    device_infos.push_back(info);
  }
  return device_infos;
}

}  // namespace

class CryptAuthSoftwareFeatureManagerImplTest
    : public testing::Test,
      public MockCryptAuthClientFactory::Observer {
 public:
  CryptAuthSoftwareFeatureManagerImplTest()
      : all_test_external_device_infos_(
            CreateExternalDeviceInfosForRemoteDevices(
                cryptauth::CreateRemoteDeviceRefListForTest(5))),
        test_eligible_external_devices_infos_(
            {all_test_external_device_infos_[0],
             all_test_external_device_infos_[1],
             all_test_external_device_infos_[2]}),
        test_ineligible_external_devices_infos_(
            {all_test_external_device_infos_[3],
             all_test_external_device_infos_[4]}) {}

  void SetUp() override {
    mock_cryptauth_client_factory_ =
        std::make_unique<MockCryptAuthClientFactory>(
            MockCryptAuthClientFactory::MockType::MAKE_NICE_MOCKS);
    mock_cryptauth_client_factory_->AddObserver(this);
    software_feature_manager_ =
        SoftwareFeatureManagerImpl::Factory::NewInstance(
            mock_cryptauth_client_factory_.get());
  }

  void TearDown() override {
    mock_cryptauth_client_factory_->RemoveObserver(this);
  }

  void OnCryptAuthClientCreated(MockCryptAuthClient* client) override {
    ON_CALL(*client, ToggleEasyUnlock(_, _, _))
        .WillByDefault(Invoke(
            this,
            &CryptAuthSoftwareFeatureManagerImplTest::MockToggleEasyUnlock));
    ON_CALL(*client, FindEligibleUnlockDevices(_, _, _))
        .WillByDefault(Invoke(this, &CryptAuthSoftwareFeatureManagerImplTest::
                                        MockFindEligibleUnlockDevices));
  }

  // Mock CryptAuthClient::ToggleEasyUnlock() implementation.
  void MockToggleEasyUnlock(
      const ToggleEasyUnlockRequest& request,
      const CryptAuthClient::ToggleEasyUnlockCallback& callback,
      const CryptAuthClient::ErrorCallback& error_callback) {
    last_toggle_request_ = request;
    toggle_easy_unlock_callback_ = callback;
    error_callback_ = error_callback;
    error_code_ = kErrorSetSoftwareFeatureState;
  }

  // Mock CryptAuthClient::FindEligibleUnlockDevices() implementation.
  void MockFindEligibleUnlockDevices(
      const FindEligibleUnlockDevicesRequest& request,
      const CryptAuthClient::FindEligibleUnlockDevicesCallback& callback,
      const CryptAuthClient::ErrorCallback& error_callback) {
    last_find_request_ = request;
    find_eligible_unlock_devices_callback_ = callback;
    error_callback_ = error_callback;
    error_code_ = kErrorFindEligibleDevices;
  }

  FindEligibleUnlockDevicesResponse CreateFindEligibleUnlockDevicesResponse() {
    FindEligibleUnlockDevicesResponse find_eligible_unlock_devices_response;
    for (const auto& device_info : test_eligible_external_devices_infos_) {
      find_eligible_unlock_devices_response.add_eligible_devices()->CopyFrom(
          device_info);
    }
    for (const auto& device_info : test_ineligible_external_devices_infos_) {
      find_eligible_unlock_devices_response.add_ineligible_devices()
          ->mutable_device()
          ->CopyFrom(device_info);
    }
    return find_eligible_unlock_devices_response;
  }

  void VerifyDeviceEligibility() {
    // Ensure that resulting devices are not empty.  Otherwise, following for
    // loop checks will succeed on empty resulting devices.
    EXPECT_TRUE(result_eligible_devices_.size() > 0);
    EXPECT_TRUE(result_ineligible_devices_.size() > 0);
    for (const auto& device_info : result_eligible_devices_) {
      EXPECT_TRUE(
          std::find_if(
              test_eligible_external_devices_infos_.begin(),
              test_eligible_external_devices_infos_.end(),
              [&device_info](const cryptauth::ExternalDeviceInfo& device) {
                return device.public_key() == device_info.public_key();
              }) != test_eligible_external_devices_infos_.end());
    }
    for (const auto& ineligible_device : result_ineligible_devices_) {
      EXPECT_TRUE(
          std::find_if(test_ineligible_external_devices_infos_.begin(),
                       test_ineligible_external_devices_infos_.end(),
                       [&ineligible_device](
                           const cryptauth::ExternalDeviceInfo& device) {
                         return device.public_key() ==
                                ineligible_device.device().public_key();
                       }) != test_ineligible_external_devices_infos_.end());
    }
    result_eligible_devices_.clear();
    result_ineligible_devices_.clear();
  }

  void SetSoftwareFeatureState(SoftwareFeature feature,
                               const ExternalDeviceInfo& device_info,
                               bool enabled,
                               bool is_exclusive = false) {
    software_feature_manager_->SetSoftwareFeatureState(
        device_info.public_key(), feature, enabled,
        base::Bind(
            &CryptAuthSoftwareFeatureManagerImplTest::OnSoftwareFeatureStateSet,
            base::Unretained(this)),
        base::Bind(&CryptAuthSoftwareFeatureManagerImplTest::OnError,
                   base::Unretained(this)),
        is_exclusive);
  }

  void FindEligibleDevices(SoftwareFeature feature) {
    software_feature_manager_->FindEligibleDevices(
        feature,
        base::Bind(
            &CryptAuthSoftwareFeatureManagerImplTest::OnEligibleDevicesFound,
            base::Unretained(this)),
        base::Bind(&CryptAuthSoftwareFeatureManagerImplTest::OnError,
                   base::Unretained(this)));
  }

  void OnSoftwareFeatureStateSet() { result_ = kSuccessResult; }

  void OnEligibleDevicesFound(
      const std::vector<ExternalDeviceInfo>& eligible_devices,
      const std::vector<IneligibleDevice>& ineligible_devices) {
    result_ = kSuccessResult;
    result_eligible_devices_ = eligible_devices;
    result_ineligible_devices_ = ineligible_devices;
  }

  void OnError(const std::string& error_message) { result_ = error_message; }

  void InvokeSetSoftwareFeatureCallback() {
    CryptAuthClient::ToggleEasyUnlockCallback success_callback =
        toggle_easy_unlock_callback_;
    ASSERT_TRUE(!success_callback.is_null());
    toggle_easy_unlock_callback_.Reset();
    success_callback.Run(ToggleEasyUnlockResponse());
  }

  void InvokeFindEligibleDevicesCallback(
      const FindEligibleUnlockDevicesResponse& retrieved_devices_response) {
    CryptAuthClient::FindEligibleUnlockDevicesCallback success_callback =
        find_eligible_unlock_devices_callback_;
    ASSERT_TRUE(!success_callback.is_null());
    find_eligible_unlock_devices_callback_.Reset();
    success_callback.Run(retrieved_devices_response);
  }

  void InvokeErrorCallback() {
    CryptAuthClient::ErrorCallback error_callback = error_callback_;
    ASSERT_TRUE(!error_callback.is_null());
    error_callback_.Reset();
    error_callback.Run(error_code_);
  }

  std::string GetResultAndReset() {
    std::string result;
    result.swap(result_);
    return result;
  }

  const std::vector<cryptauth::ExternalDeviceInfo>
      all_test_external_device_infos_;
  const std::vector<ExternalDeviceInfo> test_eligible_external_devices_infos_;
  const std::vector<ExternalDeviceInfo> test_ineligible_external_devices_infos_;

  std::unique_ptr<MockCryptAuthClientFactory> mock_cryptauth_client_factory_;
  std::unique_ptr<cryptauth::SoftwareFeatureManager> software_feature_manager_;

  CryptAuthClient::ErrorCallback error_callback_;

  // Set when a CryptAuthClient function returns. If empty, no callback has been
  // invoked.
  std::string result_;

  // The code passed to the error callback; varies depending on what
  // CryptAuthClient function is invoked.
  std::string error_code_;

  // For SetSoftwareFeatureState() tests.
  ToggleEasyUnlockRequest last_toggle_request_;
  CryptAuthClient::ToggleEasyUnlockCallback toggle_easy_unlock_callback_;

  // For FindEligibleDevices() tests.
  FindEligibleUnlockDevicesRequest last_find_request_;
  CryptAuthClient::FindEligibleUnlockDevicesCallback
      find_eligible_unlock_devices_callback_;
  std::vector<ExternalDeviceInfo> result_eligible_devices_;
  std::vector<IneligibleDevice> result_ineligible_devices_;

 private:
  DISALLOW_COPY_AND_ASSIGN(CryptAuthSoftwareFeatureManagerImplTest);
};

TEST_F(CryptAuthSoftwareFeatureManagerImplTest, TestOrderUponMultipleRequests) {
  SetSoftwareFeatureState(SoftwareFeature::BETTER_TOGETHER_HOST,
                          test_eligible_external_devices_infos_[0],
                          true /* enable */);
  FindEligibleDevices(SoftwareFeature::BETTER_TOGETHER_HOST);
  SetSoftwareFeatureState(SoftwareFeature::BETTER_TOGETHER_CLIENT,
                          test_eligible_external_devices_infos_[1],
                          false /* enable */);
  FindEligibleDevices(SoftwareFeature::BETTER_TOGETHER_CLIENT);

  EXPECT_EQ(SoftwareFeature::BETTER_TOGETHER_HOST,
            last_toggle_request_.feature());
  EXPECT_EQ(true, last_toggle_request_.enable());
  EXPECT_EQ(false, last_toggle_request_.is_exclusive());
  InvokeSetSoftwareFeatureCallback();
  EXPECT_EQ(kSuccessResult, GetResultAndReset());

  EXPECT_EQ(SoftwareFeature::BETTER_TOGETHER_HOST,
            last_find_request_.feature());
  InvokeFindEligibleDevicesCallback(CreateFindEligibleUnlockDevicesResponse());
  EXPECT_EQ(kSuccessResult, GetResultAndReset());
  VerifyDeviceEligibility();

  EXPECT_EQ(SoftwareFeature::BETTER_TOGETHER_CLIENT,
            last_toggle_request_.feature());
  EXPECT_EQ(false, last_toggle_request_.enable());
  EXPECT_EQ(false, last_toggle_request_.is_exclusive());
  InvokeSetSoftwareFeatureCallback();
  EXPECT_EQ(kSuccessResult, GetResultAndReset());

  EXPECT_EQ(SoftwareFeature::BETTER_TOGETHER_CLIENT,
            last_find_request_.feature());
  InvokeFindEligibleDevicesCallback(CreateFindEligibleUnlockDevicesResponse());
  EXPECT_EQ(kSuccessResult, GetResultAndReset());
  VerifyDeviceEligibility();
}

TEST_F(CryptAuthSoftwareFeatureManagerImplTest,
       TestMultipleSetUnlocksRequests) {
  SetSoftwareFeatureState(SoftwareFeature::BETTER_TOGETHER_HOST,
                          test_eligible_external_devices_infos_[0],
                          true /* enable */);
  SetSoftwareFeatureState(SoftwareFeature::BETTER_TOGETHER_CLIENT,
                          test_eligible_external_devices_infos_[1],
                          false /* enable */);
  SetSoftwareFeatureState(SoftwareFeature::BETTER_TOGETHER_HOST,
                          test_eligible_external_devices_infos_[2],
                          true /* enable */);

  EXPECT_EQ(SoftwareFeature::BETTER_TOGETHER_HOST,
            last_toggle_request_.feature());
  EXPECT_EQ(true, last_toggle_request_.enable());
  EXPECT_EQ(false, last_toggle_request_.is_exclusive());
  InvokeErrorCallback();
  EXPECT_EQ(kErrorSetSoftwareFeatureState, GetResultAndReset());

  EXPECT_EQ(SoftwareFeature::BETTER_TOGETHER_CLIENT,
            last_toggle_request_.feature());
  EXPECT_EQ(false, last_toggle_request_.enable());
  EXPECT_EQ(false, last_toggle_request_.is_exclusive());
  InvokeSetSoftwareFeatureCallback();
  EXPECT_EQ(kSuccessResult, GetResultAndReset());

  EXPECT_EQ(SoftwareFeature::BETTER_TOGETHER_HOST,
            last_toggle_request_.feature());
  EXPECT_EQ(true, last_toggle_request_.enable());
  EXPECT_EQ(false, last_toggle_request_.is_exclusive());
  InvokeSetSoftwareFeatureCallback();
  EXPECT_EQ(kSuccessResult, GetResultAndReset());
}

TEST_F(CryptAuthSoftwareFeatureManagerImplTest,
       TestMultipleFindEligibleForUnlockDevicesRequests) {
  FindEligibleDevices(SoftwareFeature::BETTER_TOGETHER_HOST);
  FindEligibleDevices(SoftwareFeature::BETTER_TOGETHER_CLIENT);
  FindEligibleDevices(SoftwareFeature::BETTER_TOGETHER_HOST);

  EXPECT_EQ(SoftwareFeature::BETTER_TOGETHER_HOST,
            last_find_request_.feature());
  InvokeFindEligibleDevicesCallback(CreateFindEligibleUnlockDevicesResponse());
  EXPECT_EQ(kSuccessResult, GetResultAndReset());
  VerifyDeviceEligibility();

  EXPECT_EQ(SoftwareFeature::BETTER_TOGETHER_CLIENT,
            last_find_request_.feature());
  InvokeErrorCallback();
  EXPECT_EQ(kErrorFindEligibleDevices, GetResultAndReset());

  EXPECT_EQ(SoftwareFeature::BETTER_TOGETHER_HOST,
            last_find_request_.feature());
  InvokeFindEligibleDevicesCallback(CreateFindEligibleUnlockDevicesResponse());
  EXPECT_EQ(kSuccessResult, GetResultAndReset());
  VerifyDeviceEligibility();
}

TEST_F(CryptAuthSoftwareFeatureManagerImplTest, TestOrderViaMultipleErrors) {
  SetSoftwareFeatureState(SoftwareFeature::BETTER_TOGETHER_HOST,
                          test_eligible_external_devices_infos_[0],
                          true /* enable */);
  FindEligibleDevices(SoftwareFeature::BETTER_TOGETHER_HOST);

  EXPECT_EQ(SoftwareFeature::BETTER_TOGETHER_HOST,
            last_toggle_request_.feature());
  InvokeErrorCallback();
  EXPECT_EQ(kErrorSetSoftwareFeatureState, GetResultAndReset());

  EXPECT_EQ(SoftwareFeature::BETTER_TOGETHER_HOST,
            last_find_request_.feature());
  InvokeErrorCallback();
  EXPECT_EQ(kErrorFindEligibleDevices, GetResultAndReset());
}

TEST_F(CryptAuthSoftwareFeatureManagerImplTest, TestIsExclusive) {
  SetSoftwareFeatureState(SoftwareFeature::BETTER_TOGETHER_HOST,
                          test_eligible_external_devices_infos_[0],
                          true /* enable */, true /* is_exclusive */);

  EXPECT_EQ(SoftwareFeature::BETTER_TOGETHER_HOST,
            last_toggle_request_.feature());
  EXPECT_EQ(true, last_toggle_request_.enable());
  EXPECT_EQ(true, last_toggle_request_.is_exclusive());
  InvokeErrorCallback();
  EXPECT_EQ(kErrorSetSoftwareFeatureState, GetResultAndReset());
}

TEST_F(CryptAuthSoftwareFeatureManagerImplTest, TestEasyUnlockSpecialCase) {
  SetSoftwareFeatureState(SoftwareFeature::EASY_UNLOCK_HOST,
                          test_eligible_external_devices_infos_[0],
                          false /* enable */);

  EXPECT_EQ(SoftwareFeature::EASY_UNLOCK_HOST, last_toggle_request_.feature());
  EXPECT_EQ(false, last_toggle_request_.enable());
  // apply_to_all() should be false when disabling EasyUnlock host capabilities.
  EXPECT_EQ(true, last_toggle_request_.apply_to_all());
  InvokeErrorCallback();
  EXPECT_EQ(kErrorSetSoftwareFeatureState, GetResultAndReset());
}

}  // namespace cryptauth
