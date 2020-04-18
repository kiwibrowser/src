// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/ble/ble_advertisement_generator.h"

#include <memory>

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "components/cryptauth/mock_foreground_eid_generator.h"
#include "components/cryptauth/mock_local_device_data_provider.h"
#include "components/cryptauth/mock_remote_beacon_seed_fetcher.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/remote_device_ref.h"
#include "components/cryptauth/remote_device_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::StrictMock;

namespace cryptauth {

namespace {

const char kFakePublicKey[] = "fakePublicKey";

std::vector<BeaconSeed> CreateFakeBeaconSeedsForDevice(
    RemoteDeviceRef remote_device) {
  BeaconSeed seed1;
  seed1.set_data("seed1Data" + remote_device.GetTruncatedDeviceIdForLogs());
  seed1.set_start_time_millis(1000L);
  seed1.set_start_time_millis(2000L);

  BeaconSeed seed2;
  seed2.set_data("seed2Data" + remote_device.GetTruncatedDeviceIdForLogs());
  seed2.set_start_time_millis(2000L);
  seed2.set_start_time_millis(3000L);

  std::vector<BeaconSeed> seeds = {seed1, seed2};
  return seeds;
}

}  // namespace

class CryptAuthBleAdvertisementGeneratorTest : public testing::Test {
 protected:
  CryptAuthBleAdvertisementGeneratorTest()
      : fake_device_(CreateRemoteDeviceRefListForTest(1)[0]),
        fake_advertisement_("advertisement1", 1000L, 2000L) {}

  void SetUp() override {
    mock_seed_fetcher_ = std::make_unique<MockRemoteBeaconSeedFetcher>();
    std::vector<BeaconSeed> device_0_beacon_seeds =
        CreateFakeBeaconSeedsForDevice(fake_device_);
    mock_seed_fetcher_->SetSeedsForDeviceId(fake_device_.GetDeviceId(),
                                            &device_0_beacon_seeds);

    mock_local_data_provider_ = std::make_unique<MockLocalDeviceDataProvider>();
    mock_local_data_provider_->SetPublicKey(
        std::make_unique<std::string>(kFakePublicKey));

    generator_ = base::WrapUnique(new BleAdvertisementGenerator());

    mock_eid_generator_ = new MockForegroundEidGenerator();
    generator_->SetEidGeneratorForTesting(
        base::WrapUnique(mock_eid_generator_));
  }

  void TearDown() override { generator_.reset(); }

  std::unique_ptr<DataWithTimestamp> GenerateBleAdvertisement() {
    return generator_->GenerateBleAdvertisementInternal(
        fake_device_.GetDeviceId(), mock_local_data_provider_.get(),
        mock_seed_fetcher_.get());
  }

  const RemoteDeviceRef fake_device_;
  const DataWithTimestamp fake_advertisement_;

  std::unique_ptr<MockRemoteBeaconSeedFetcher> mock_seed_fetcher_;
  std::unique_ptr<MockLocalDeviceDataProvider> mock_local_data_provider_;

  MockForegroundEidGenerator* mock_eid_generator_;

  std::unique_ptr<BleAdvertisementGenerator> generator_;

 private:
  DISALLOW_COPY_AND_ASSIGN(CryptAuthBleAdvertisementGeneratorTest);
};

TEST_F(CryptAuthBleAdvertisementGeneratorTest, TestCannotFetchPublicKey) {
  mock_local_data_provider_->SetPublicKey(nullptr);
  EXPECT_EQ(nullptr, GenerateBleAdvertisement());
}

TEST_F(CryptAuthBleAdvertisementGeneratorTest, EmptyPublicKey) {
  mock_local_data_provider_->SetPublicKey(std::make_unique<std::string>(""));
  EXPECT_EQ(nullptr, GenerateBleAdvertisement());
}

TEST_F(CryptAuthBleAdvertisementGeneratorTest, NoBeaconSeeds) {
  mock_seed_fetcher_->SetSeedsForDeviceId(fake_device_.GetDeviceId(), nullptr);
  EXPECT_EQ(nullptr, GenerateBleAdvertisement());
}

TEST_F(CryptAuthBleAdvertisementGeneratorTest, EmptyBeaconSeeds) {
  std::vector<BeaconSeed> empty_seeds;
  mock_seed_fetcher_->SetSeedsForDeviceId(fake_device_.GetDeviceId(),
                                          &empty_seeds);
  EXPECT_EQ(nullptr, GenerateBleAdvertisement());
}

TEST_F(CryptAuthBleAdvertisementGeneratorTest, CannotGenerateAdvertisement) {
  mock_eid_generator_->set_advertisement(nullptr);
  EXPECT_EQ(nullptr, GenerateBleAdvertisement());
}

TEST_F(CryptAuthBleAdvertisementGeneratorTest, AdvertisementGenerated) {
  mock_eid_generator_->set_advertisement(
      std::make_unique<DataWithTimestamp>(fake_advertisement_));
  EXPECT_EQ(fake_advertisement_, *GenerateBleAdvertisement());
}

}  // namespace cryptauth
