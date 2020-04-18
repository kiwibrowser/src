// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/remote_device_ref.h"

#include <memory>

#include "base/macros.h"
#include "components/cryptauth/remote_device.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cryptauth {

class RemoteDeviceRefTest : public testing::Test {
 protected:
  RemoteDeviceRefTest() = default;

  // testing::Test:
  void SetUp() override {
    std::map<cryptauth::SoftwareFeature, cryptauth::SoftwareFeatureState>
        software_feature_to_state_map;
    software_feature_to_state_map
        [cryptauth::SoftwareFeature::BETTER_TOGETHER_CLIENT] =
            cryptauth::SoftwareFeatureState::kSupported;
    software_feature_to_state_map
        [cryptauth::SoftwareFeature::BETTER_TOGETHER_HOST] =
            cryptauth::SoftwareFeatureState::kEnabled;

    remote_device_ = std::make_shared<RemoteDevice>(
        "user_id", "name", "public_key", "persistent_symmetric_key",
        true /* unlock_key */, true /* supports_mobile_hotspot */,
        42000 /* last_update_time_millis */,
        software_feature_to_state_map /* software_features */);
    remote_device_->LoadBeaconSeeds({BeaconSeed(), BeaconSeed()});
  }

  std::shared_ptr<RemoteDevice> remote_device_;

  DISALLOW_COPY_AND_ASSIGN(RemoteDeviceRefTest);
};

TEST_F(RemoteDeviceRefTest, TestFields) {
  RemoteDeviceRef remote_device_ref(remote_device_);

  EXPECT_EQ(remote_device_->user_id, remote_device_ref.user_id());
  EXPECT_EQ(remote_device_->name, remote_device_ref.name());
  EXPECT_EQ(remote_device_->public_key, remote_device_ref.public_key());
  EXPECT_EQ(remote_device_->persistent_symmetric_key,
            remote_device_ref.persistent_symmetric_key());
  EXPECT_EQ(remote_device_->unlock_key, remote_device_ref.unlock_key());
  EXPECT_EQ(remote_device_->supports_mobile_hotspot,
            remote_device_ref.supports_mobile_hotspot());
  EXPECT_EQ(remote_device_->last_update_time_millis,
            remote_device_ref.last_update_time_millis());
  EXPECT_EQ(&remote_device_->beacon_seeds, &remote_device_ref.beacon_seeds());

  EXPECT_EQ(cryptauth::SoftwareFeatureState::kNotSupported,
            remote_device_ref.GetSoftwareFeatureState(
                cryptauth::SoftwareFeature::MAGIC_TETHER_CLIENT));
  EXPECT_EQ(cryptauth::SoftwareFeatureState::kSupported,
            remote_device_ref.GetSoftwareFeatureState(
                cryptauth::SoftwareFeature::BETTER_TOGETHER_CLIENT));
  EXPECT_EQ(cryptauth::SoftwareFeatureState::kEnabled,
            remote_device_ref.GetSoftwareFeatureState(
                cryptauth::SoftwareFeature::BETTER_TOGETHER_HOST));

  EXPECT_EQ(remote_device_->GetDeviceId(), remote_device_ref.GetDeviceId());
  EXPECT_EQ(
      RemoteDeviceRef::TruncateDeviceIdForLogs(remote_device_->GetDeviceId()),
      remote_device_ref.GetTruncatedDeviceIdForLogs());
}

TEST_F(RemoteDeviceRefTest, TestCopyAndAssign) {
  RemoteDeviceRef remote_device_ref_1(remote_device_);

  RemoteDeviceRef remote_device_ref_2 = remote_device_ref_1;
  EXPECT_EQ(remote_device_ref_2, remote_device_ref_1);

  RemoteDeviceRef remote_device_ref_3(remote_device_ref_1);
  EXPECT_EQ(remote_device_ref_3, remote_device_ref_1);
}

}  // namespace cryptauth
