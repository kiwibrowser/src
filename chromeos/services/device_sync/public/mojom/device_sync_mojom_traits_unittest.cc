// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/device_sync/public/mojom/device_sync_mojom_traits.h"

#include "base/time/time.h"
#include "chromeos/services/device_sync/public/mojom/device_sync.mojom.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/remote_device.h"
#include "mojo/public/cpp/base/time_mojom_traits.h"
#include "mojo/public/cpp/test_support/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kTestBeaconSeedData[] = "data";
const int64_t kTestBeaconSeedStartTimeMillis = 1L;
const int64_t kTestBeaconSeedEndTimeMillis = 2L;

cryptauth::BeaconSeed CreateTestBeaconSeed() {
  cryptauth::BeaconSeed beacon_seed;

  beacon_seed.set_data(kTestBeaconSeedData);
  beacon_seed.set_start_time_millis(kTestBeaconSeedStartTimeMillis);
  beacon_seed.set_end_time_millis(kTestBeaconSeedEndTimeMillis);

  return beacon_seed;
}

}  // namespace

TEST(DeviceSyncMojomStructTraitsTest, BeaconSeed) {
  cryptauth::BeaconSeed input = CreateTestBeaconSeed();

  cryptauth::BeaconSeed output;
  EXPECT_TRUE(mojo::test::SerializeAndDeserialize<
              chromeos::device_sync::mojom::BeaconSeed>(&input, &output));

  EXPECT_EQ(kTestBeaconSeedData, output.data());
  EXPECT_EQ(kTestBeaconSeedStartTimeMillis, output.start_time_millis());
  EXPECT_EQ(kTestBeaconSeedEndTimeMillis, output.end_time_millis());
}

TEST(DeviceSyncMojomStructTraitsTest, RemoteDevice) {
  std::map<cryptauth::SoftwareFeature, cryptauth::SoftwareFeatureState>
      software_features = std::map<cryptauth::SoftwareFeature,
                                   cryptauth::SoftwareFeatureState>();
  software_features[cryptauth::SoftwareFeature::BETTER_TOGETHER_CLIENT] =
      cryptauth::SoftwareFeatureState::kSupported;
  software_features[cryptauth::SoftwareFeature::BETTER_TOGETHER_HOST] =
      cryptauth::SoftwareFeatureState::kEnabled;

  cryptauth::RemoteDevice input;
  input.user_id = "userId";
  input.name = "name";
  input.public_key = "publicKey";
  input.persistent_symmetric_key = "persistentSymmetricKey";
  input.unlock_key = true;
  input.supports_mobile_hotspot = true;
  input.last_update_time_millis = 3L;
  input.software_features = software_features;
  input.LoadBeaconSeeds({CreateTestBeaconSeed()});

  cryptauth::RemoteDevice output;
  EXPECT_TRUE(mojo::test::SerializeAndDeserialize<
              chromeos::device_sync::mojom::RemoteDevice>(&input, &output));

  EXPECT_EQ("userId", output.user_id);
  EXPECT_EQ("name", output.name);
  EXPECT_EQ("publicKey", output.public_key);
  EXPECT_EQ("persistentSymmetricKey", output.persistent_symmetric_key);
  EXPECT_TRUE(output.unlock_key);
  EXPECT_TRUE(output.supports_mobile_hotspot);
  EXPECT_EQ(3L, output.last_update_time_millis);
  EXPECT_EQ(software_features, output.software_features);
  ASSERT_EQ(1u, output.beacon_seeds.size());
  EXPECT_EQ(kTestBeaconSeedData, output.beacon_seeds[0].data());
  EXPECT_EQ(kTestBeaconSeedStartTimeMillis,
            output.beacon_seeds[0].start_time_millis());
  EXPECT_EQ(kTestBeaconSeedEndTimeMillis,
            output.beacon_seeds[0].end_time_millis());
}

TEST(DeviceSyncMojomEnumTraitsTest, SoftwareFeature) {
  static constexpr cryptauth::SoftwareFeature kTestSoftwareFeatures[] = {
      cryptauth::SoftwareFeature::UNKNOWN_FEATURE,
      cryptauth::SoftwareFeature::BETTER_TOGETHER_HOST,
      cryptauth::SoftwareFeature::BETTER_TOGETHER_CLIENT,
      cryptauth::SoftwareFeature::EASY_UNLOCK_HOST,
      cryptauth::SoftwareFeature::EASY_UNLOCK_CLIENT,
      cryptauth::SoftwareFeature::MAGIC_TETHER_HOST,
      cryptauth::SoftwareFeature::MAGIC_TETHER_CLIENT,
      cryptauth::SoftwareFeature::SMS_CONNECT_HOST,
      cryptauth::SoftwareFeature::SMS_CONNECT_CLIENT};

  for (auto feature_in : kTestSoftwareFeatures) {
    cryptauth::SoftwareFeature feature_out;

    chromeos::device_sync::mojom::SoftwareFeature serialized_feature =
        mojo::EnumTraits<chromeos::device_sync::mojom::SoftwareFeature,
                         cryptauth::SoftwareFeature>::ToMojom(feature_in);
    ASSERT_TRUE((mojo::EnumTraits<
                 chromeos::device_sync::mojom::SoftwareFeature,
                 cryptauth::SoftwareFeature>::FromMojom(serialized_feature,
                                                        &feature_out)));
    EXPECT_EQ(feature_in, feature_out);
  }
}
