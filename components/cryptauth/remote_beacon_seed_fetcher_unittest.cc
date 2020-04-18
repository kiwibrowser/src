// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/remote_beacon_seed_fetcher.h"

#include <memory>

#include "base/macros.h"
#include "components/cryptauth/cryptauth_client.h"
#include "components/cryptauth/fake_cryptauth_device_manager.h"
#include "components/cryptauth/remote_device_ref.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::StrictMock;
using testing::Return;

namespace cryptauth {

namespace {

const std::string fake_beacon_seed1_data = "fakeBeaconSeed1Data";
const int64_t fake_beacon_seed1_start_ms = 1000L;
const int64_t fake_beacon_seed1_end_ms = 2000L;

const std::string fake_beacon_seed2_data = "fakeBeaconSeed2Data";
const int64_t fake_beacon_seed2_start_ms = 2000L;
const int64_t fake_beacon_seed2_end_ms = 3000L;

const std::string fake_beacon_seed3_data = "fakeBeaconSeed3Data";
const int64_t fake_beacon_seed3_start_ms = 1000L;
const int64_t fake_beacon_seed3_end_ms = 2000L;

const std::string fake_beacon_seed4_data = "fakeBeaconSeed4Data";
const int64_t fake_beacon_seed4_start_ms = 2000L;
const int64_t fake_beacon_seed4_end_ms = 3000L;

const std::string public_key1 = "publicKey1";
const std::string public_key2 = "publicKey2";

ExternalDeviceInfo CreateFakeInfo1() {
  BeaconSeed seed1;
  seed1.set_data(fake_beacon_seed1_data);
  seed1.set_start_time_millis(fake_beacon_seed1_start_ms);
  seed1.set_end_time_millis(fake_beacon_seed1_end_ms);

  BeaconSeed seed2;
  seed2.set_data(fake_beacon_seed2_data);
  seed2.set_start_time_millis(fake_beacon_seed2_start_ms);
  seed2.set_end_time_millis(fake_beacon_seed2_end_ms);

  ExternalDeviceInfo info1;
  info1.set_public_key(public_key1);
  info1.add_beacon_seeds()->CopyFrom(seed1);
  info1.add_beacon_seeds()->CopyFrom(seed2);
  return info1;
}

ExternalDeviceInfo CreateFakeInfo2() {
  BeaconSeed seed3;
  seed3.set_data(fake_beacon_seed3_data);
  seed3.set_start_time_millis(fake_beacon_seed3_start_ms);
  seed3.set_end_time_millis(fake_beacon_seed3_end_ms);

  BeaconSeed seed4;
  seed4.set_data(fake_beacon_seed4_data);
  seed4.set_start_time_millis(fake_beacon_seed4_start_ms);
  seed4.set_end_time_millis(fake_beacon_seed4_end_ms);

  ExternalDeviceInfo info2;
  info2.set_public_key(public_key2);
  info2.add_beacon_seeds()->CopyFrom(seed3);
  info2.add_beacon_seeds()->CopyFrom(seed4);
  return info2;
}

}  // namespace

class CryptAuthRemoteBeaconSeedFetcherTest : public testing::Test {
 protected:
  CryptAuthRemoteBeaconSeedFetcherTest()
      : fake_info1_(CreateFakeInfo1()), fake_info2_(CreateFakeInfo2()) {}

  void SetUp() override {
    fake_device_manager_ = std::make_unique<FakeCryptAuthDeviceManager>();
    fetcher_ = std::make_unique<StrictMock<RemoteBeaconSeedFetcher>>(
        fake_device_manager_.get());
  }

  std::unique_ptr<RemoteBeaconSeedFetcher> fetcher_;
  std::unique_ptr<FakeCryptAuthDeviceManager> fake_device_manager_;

  const ExternalDeviceInfo fake_info1_;
  const ExternalDeviceInfo fake_info2_;

 private:
  DISALLOW_COPY_AND_ASSIGN(CryptAuthRemoteBeaconSeedFetcherTest);
};

TEST_F(CryptAuthRemoteBeaconSeedFetcherTest, TestRemoteDeviceWithNoPublicKey) {
  std::vector<BeaconSeed> seeds;
  EXPECT_FALSE(fetcher_->FetchSeedsForDeviceId(std::string(), &seeds));
}

TEST_F(CryptAuthRemoteBeaconSeedFetcherTest, TestNoSyncedDevices) {
  std::vector<BeaconSeed> seeds;
  EXPECT_FALSE(fetcher_->FetchSeedsForDeviceId(
      RemoteDeviceRef::GenerateDeviceId(public_key1), &seeds));
}

TEST_F(CryptAuthRemoteBeaconSeedFetcherTest, TestDeviceHasDifferentPublicKey) {
  fake_device_manager_->set_synced_devices(
      std::vector<ExternalDeviceInfo>{fake_info1_, fake_info2_});

  std::vector<BeaconSeed> seeds;
  EXPECT_FALSE(fetcher_->FetchSeedsForDeviceId(
      RemoteDeviceRef::GenerateDeviceId("differentPublicKey"), &seeds));
}

TEST_F(CryptAuthRemoteBeaconSeedFetcherTest, TestSuccess) {
  fake_device_manager_->set_synced_devices(
      std::vector<ExternalDeviceInfo>{fake_info1_, fake_info2_});

  std::vector<BeaconSeed> seeds1;
  ASSERT_TRUE(fetcher_->FetchSeedsForDeviceId(
      RemoteDeviceRef::GenerateDeviceId(public_key1), &seeds1));
  ASSERT_EQ(2u, seeds1.size());
  EXPECT_EQ(fake_beacon_seed1_data, seeds1[0].data());
  EXPECT_EQ(fake_beacon_seed1_start_ms, seeds1[0].start_time_millis());
  EXPECT_EQ(fake_beacon_seed1_end_ms, seeds1[0].end_time_millis());
  EXPECT_EQ(fake_beacon_seed2_data, seeds1[1].data());
  EXPECT_EQ(fake_beacon_seed2_start_ms, seeds1[1].start_time_millis());
  EXPECT_EQ(fake_beacon_seed2_end_ms, seeds1[1].end_time_millis());

  std::vector<BeaconSeed> seeds2;
  ASSERT_TRUE(fetcher_->FetchSeedsForDeviceId(
      RemoteDeviceRef::GenerateDeviceId(public_key2), &seeds2));
  ASSERT_EQ(2u, seeds2.size());
  EXPECT_EQ(fake_beacon_seed3_data, seeds2[0].data());
  EXPECT_EQ(fake_beacon_seed3_start_ms, seeds2[0].start_time_millis());
  EXPECT_EQ(fake_beacon_seed3_end_ms, seeds2[0].end_time_millis());
  EXPECT_EQ(fake_beacon_seed4_data, seeds2[1].data());
  EXPECT_EQ(fake_beacon_seed4_start_ms, seeds2[1].start_time_millis());
  EXPECT_EQ(fake_beacon_seed4_end_ms, seeds2[1].end_time_millis());
}

}  // namespace cryptauth
