// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/remote_device_loader.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "components/cryptauth/fake_secure_message_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cryptauth {
namespace {

// Prefixes for RemoteDevice fields.
const char kDeviceNamePrefix[] = "device";
const char kPublicKeyPrefix[] = "pk";

// The id of the user who the remote devices belong to.
const char kUserId[] = "example@gmail.com";

// The public key of the user's local device.
const char kUserPublicKey[] = "User public key";

// BeaconSeed values.
const int64_t kBeaconSeedStartTimeMs = 1000;
const int64_t kBeaconSeedEndTimeMs = 2000;
const char kBeaconSeedData[] = "Beacon Seed Data";

// Creates and returns an ExternalDeviceInfo proto with the fields appended with
// |suffix|.
cryptauth::ExternalDeviceInfo CreateDeviceInfo(const std::string& suffix) {
  cryptauth::ExternalDeviceInfo device_info;
  device_info.set_friendly_device_name(std::string(kDeviceNamePrefix) + suffix);
  device_info.set_public_key(std::string(kPublicKeyPrefix) + suffix);
  device_info.add_beacon_seeds();
  BeaconSeed* beacon_seed = device_info.mutable_beacon_seeds(0);
  beacon_seed->set_start_time_millis(kBeaconSeedStartTimeMs);
  beacon_seed->set_end_time_millis(kBeaconSeedEndTimeMs);
  beacon_seed->set_data(kBeaconSeedData);
  return device_info;
}

}  // namespace

class CryptAuthRemoteDeviceLoaderTest : public testing::Test {
 public:
  CryptAuthRemoteDeviceLoaderTest()
      : secure_message_delegate_(new cryptauth::FakeSecureMessageDelegate()),
        user_private_key_(secure_message_delegate_->GetPrivateKeyForPublicKey(
            kUserPublicKey)) {}

  ~CryptAuthRemoteDeviceLoaderTest() {}

  void OnRemoteDevicesLoaded(
      const cryptauth::RemoteDeviceList& remote_devices) {
    remote_devices_ = remote_devices;
    LoadCompleted();
  }

  MOCK_METHOD0(LoadCompleted, void());

 protected:
  // Handles deriving the PSK. Ownership will be passed to the
  // RemoteDeviceLoader under test.
  std::unique_ptr<cryptauth::FakeSecureMessageDelegate>
      secure_message_delegate_;

  // The private key of the user local device.
  std::string user_private_key_;

  // Stores the result of the RemoteDeviceLoader.
  cryptauth::RemoteDeviceList remote_devices_;

  DISALLOW_COPY_AND_ASSIGN(CryptAuthRemoteDeviceLoaderTest);
};

TEST_F(CryptAuthRemoteDeviceLoaderTest, LoadZeroDevices) {
  std::vector<cryptauth::ExternalDeviceInfo> device_infos;
  RemoteDeviceLoader loader(device_infos, user_private_key_, kUserId,
                            std::move(secure_message_delegate_));

  EXPECT_CALL(*this, LoadCompleted());
  loader.Load(
      false, base::Bind(&CryptAuthRemoteDeviceLoaderTest::OnRemoteDevicesLoaded,
                        base::Unretained(this)));

  EXPECT_EQ(0u, remote_devices_.size());
}

TEST_F(CryptAuthRemoteDeviceLoaderTest, LoadOneDeviceWithBeaconSeeds) {
  std::vector<cryptauth::ExternalDeviceInfo> device_infos(
      1, CreateDeviceInfo("0"));
  RemoteDeviceLoader loader(device_infos, user_private_key_, kUserId,
                            std::move(secure_message_delegate_));

  EXPECT_CALL(*this, LoadCompleted());
  loader.Load(
      true, base::Bind(&CryptAuthRemoteDeviceLoaderTest::OnRemoteDevicesLoaded,
                       base::Unretained(this)));

  EXPECT_EQ(1u, remote_devices_.size());
  EXPECT_FALSE(remote_devices_[0].persistent_symmetric_key.empty());
  EXPECT_EQ(device_infos[0].friendly_device_name(), remote_devices_[0].name);
  EXPECT_EQ(device_infos[0].public_key(), remote_devices_[0].public_key);
  EXPECT_TRUE(remote_devices_[0].are_beacon_seeds_loaded);
  ASSERT_EQ(1u, remote_devices_[0].beacon_seeds.size());

  const BeaconSeed& beacon_seed = remote_devices_[0].beacon_seeds[0];
  EXPECT_EQ(kBeaconSeedData, beacon_seed.data());
  EXPECT_EQ(kBeaconSeedStartTimeMs, beacon_seed.start_time_millis());
  EXPECT_EQ(kBeaconSeedEndTimeMs, beacon_seed.end_time_millis());
}

TEST_F(CryptAuthRemoteDeviceLoaderTest, LoadDevicesWithAndWithoutBeaconSeeds) {
  std::vector<cryptauth::ExternalDeviceInfo> device_infos(
      1, CreateDeviceInfo("0"));

  RemoteDeviceLoader loader1(device_infos, user_private_key_, kUserId,
                             std::make_unique<FakeSecureMessageDelegate>());
  EXPECT_CALL(*this, LoadCompleted());
  loader1.Load(
      false /* should_load_beacon_seeds */,
      base::Bind(&CryptAuthRemoteDeviceLoaderTest::OnRemoteDevicesLoaded,
                 base::Unretained(this)));
  RemoteDevice remote_device_without_beacon_seed = remote_devices_[0];

  RemoteDeviceLoader loader2(device_infos, user_private_key_, kUserId,
                             std::make_unique<FakeSecureMessageDelegate>());
  EXPECT_CALL(*this, LoadCompleted());
  loader2.Load(
      true /* should_load_beacon_seeds */,
      base::Bind(&CryptAuthRemoteDeviceLoaderTest::OnRemoteDevicesLoaded,
                 base::Unretained(this)));
  RemoteDevice remote_device_with_beacon_seed = remote_devices_[0];

  EXPECT_EQ(remote_device_without_beacon_seed,
            remote_device_without_beacon_seed);
  EXPECT_EQ(remote_device_with_beacon_seed, remote_device_with_beacon_seed);
  EXPECT_FALSE(remote_device_with_beacon_seed ==
               remote_device_without_beacon_seed);
}

TEST_F(CryptAuthRemoteDeviceLoaderTest, BooleanAttributes) {
  cryptauth::ExternalDeviceInfo first = CreateDeviceInfo("0");
  first.set_unlock_key(true);
  first.set_mobile_hotspot_supported(true);

  cryptauth::ExternalDeviceInfo second = CreateDeviceInfo("1");
  second.set_unlock_key(false);
  second.set_mobile_hotspot_supported(false);

  std::vector<cryptauth::ExternalDeviceInfo> device_infos{first, second};

  RemoteDeviceLoader loader(device_infos, user_private_key_, kUserId,
                            std::move(secure_message_delegate_));

  EXPECT_CALL(*this, LoadCompleted());
  loader.Load(
      false, base::Bind(&CryptAuthRemoteDeviceLoaderTest::OnRemoteDevicesLoaded,
                        base::Unretained(this)));

  EXPECT_EQ(2u, remote_devices_.size());

  EXPECT_FALSE(remote_devices_[0].persistent_symmetric_key.empty());
  EXPECT_TRUE(remote_devices_[0].unlock_key);
  EXPECT_TRUE(remote_devices_[0].supports_mobile_hotspot);

  EXPECT_FALSE(remote_devices_[1].persistent_symmetric_key.empty());
  EXPECT_FALSE(remote_devices_[1].unlock_key);
  EXPECT_FALSE(remote_devices_[1].supports_mobile_hotspot);
}

TEST_F(CryptAuthRemoteDeviceLoaderTest, LastUpdateTimeMillis) {
  cryptauth::ExternalDeviceInfo first = CreateDeviceInfo("0");
  first.set_last_update_time_millis(1000);

  cryptauth::ExternalDeviceInfo second = CreateDeviceInfo("1");
  second.set_last_update_time_millis(2000);

  std::vector<cryptauth::ExternalDeviceInfo> device_infos{first, second};

  RemoteDeviceLoader loader(device_infos, user_private_key_, kUserId,
                            std::move(secure_message_delegate_));

  EXPECT_CALL(*this, LoadCompleted());
  loader.Load(
      false, base::Bind(&CryptAuthRemoteDeviceLoaderTest::OnRemoteDevicesLoaded,
                        base::Unretained(this)));

  EXPECT_EQ(2u, remote_devices_.size());

  EXPECT_EQ(1000, remote_devices_[0].last_update_time_millis);

  EXPECT_EQ(2000, remote_devices_[1].last_update_time_millis);
}

TEST_F(CryptAuthRemoteDeviceLoaderTest, SoftwareFeatures) {
  const std::vector<SoftwareFeature> kSupportedSoftwareFeatures{
      BETTER_TOGETHER_HOST, BETTER_TOGETHER_CLIENT};
  const std::vector<SoftwareFeature> kEnabledSoftwareFeatures{
      BETTER_TOGETHER_HOST};

  cryptauth::ExternalDeviceInfo first = CreateDeviceInfo("0");
  for (const auto& software_feature : kSupportedSoftwareFeatures)
    first.add_supported_software_features(software_feature);
  for (const auto& software_feature : kEnabledSoftwareFeatures)
    first.add_enabled_software_features(software_feature);

  std::vector<cryptauth::ExternalDeviceInfo> device_infos{first};

  RemoteDeviceLoader loader(device_infos, user_private_key_, kUserId,
                            std::move(secure_message_delegate_));

  EXPECT_CALL(*this, LoadCompleted());
  loader.Load(
      false, base::Bind(&CryptAuthRemoteDeviceLoaderTest::OnRemoteDevicesLoaded,
                        base::Unretained(this)));

  EXPECT_EQ(1u, remote_devices_.size());

  EXPECT_EQ(SoftwareFeatureState::kSupported,
            remote_devices_[0].software_features[BETTER_TOGETHER_CLIENT]);
  EXPECT_EQ(SoftwareFeatureState::kEnabled,
            remote_devices_[0].software_features[BETTER_TOGETHER_HOST]);
  EXPECT_EQ(SoftwareFeatureState::kNotSupported,
            remote_devices_[0].software_features[MAGIC_TETHER_HOST]);
}

}  // namespace cryptauth
