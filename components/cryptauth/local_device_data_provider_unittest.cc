// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/local_device_data_provider.h"

#include <memory>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "components/cryptauth/cryptauth_enroller.h"
#include "components/cryptauth/fake_cryptauth_device_manager.h"
#include "components/cryptauth/fake_cryptauth_enrollment_manager.h"
#include "components/cryptauth/fake_cryptauth_gcm_manager.h"
#include "components/cryptauth/fake_cryptauth_service.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/secure_message_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::NiceMock;
using testing::Return;

namespace cryptauth {

namespace {

const char kDefaultPublicKey[] = "publicKey";

const char kBeaconSeed1Data[] = "beaconSeed1Data";
const int64_t kBeaconSeed1StartMs = 1000L;
const int64_t kBeaconSeed1EndMs = 2000L;

const char kBeaconSeed2Data[] = "beaconSeed2Data";
const int64_t kBeaconSeed2StartMs = 2000L;
const int64_t kBeaconSeed2EndMs = 3000L;

BeaconSeed CreateBeaconSeed(const std::string& data,
                            int64_t start_ms,
                            int64_t end_ms) {
  BeaconSeed seed;
  seed.set_data(data);
  seed.set_start_time_millis(start_ms);
  seed.set_end_time_millis(end_ms);
  return seed;
}

}  // namespace

class CryptAuthLocalDeviceDataProviderTest : public testing::Test {
 protected:
  CryptAuthLocalDeviceDataProviderTest() {
    fake_beacon_seeds_.push_back(CreateBeaconSeed(
        kBeaconSeed1Data, kBeaconSeed1StartMs, kBeaconSeed1EndMs));
    fake_beacon_seeds_.push_back(CreateBeaconSeed(
        kBeaconSeed2Data, kBeaconSeed2StartMs, kBeaconSeed2EndMs));

    // Has no public key and no BeaconSeeds.
    ExternalDeviceInfo synced_device1;
    fake_synced_devices_.push_back(synced_device1);

    // Has no public key and some BeaconSeeds.
    ExternalDeviceInfo synced_device2;
    synced_device2.add_beacon_seeds()->CopyFrom(CreateBeaconSeed(
        kBeaconSeed1Data, kBeaconSeed1StartMs, kBeaconSeed1EndMs));
    synced_device2.add_beacon_seeds()->CopyFrom(CreateBeaconSeed(
        kBeaconSeed2Data, kBeaconSeed2StartMs, kBeaconSeed2EndMs));
    fake_synced_devices_.push_back(synced_device2);

    // Has another different public key and no BeaconSeeds.
    ExternalDeviceInfo synced_device3;
    synced_device3.set_public_key("anotherPublicKey");
    fake_synced_devices_.push_back(synced_device3);

    // Has different public key and BeaconSeeds.
    ExternalDeviceInfo synced_device4;
    synced_device4.set_public_key("otherPublicKey");
    synced_device4.add_beacon_seeds()->CopyFrom(CreateBeaconSeed(
        kBeaconSeed1Data, kBeaconSeed1StartMs, kBeaconSeed1EndMs));
    synced_device4.add_beacon_seeds()->CopyFrom(CreateBeaconSeed(
        kBeaconSeed2Data, kBeaconSeed2StartMs, kBeaconSeed2EndMs));
    fake_synced_devices_.push_back(synced_device4);

    // Has public key but no BeaconSeeds.
    ExternalDeviceInfo synced_device5;
    synced_device5.set_public_key(kDefaultPublicKey);
    fake_synced_devices_.push_back(synced_device5);

    // Has public key and BeaconSeeds.
    ExternalDeviceInfo synced_device6;
    synced_device6.set_public_key(kDefaultPublicKey);
    synced_device6.add_beacon_seeds()->CopyFrom(CreateBeaconSeed(
        kBeaconSeed1Data, kBeaconSeed1StartMs, kBeaconSeed1EndMs));
    synced_device6.add_beacon_seeds()->CopyFrom(CreateBeaconSeed(
        kBeaconSeed2Data, kBeaconSeed2StartMs, kBeaconSeed2EndMs));
    fake_synced_devices_.push_back(synced_device6);
  }

  void SetUp() override {
    fake_device_manager_ = std::make_unique<FakeCryptAuthDeviceManager>();
    fake_enrollment_manager_ =
        std::make_unique<FakeCryptAuthEnrollmentManager>();

    fake_cryptauth_service_ = std::make_unique<FakeCryptAuthService>();
    fake_cryptauth_service_->set_cryptauth_device_manager(
        fake_device_manager_.get());
    fake_cryptauth_service_->set_cryptauth_enrollment_manager(
        fake_enrollment_manager_.get());

    provider_ = base::WrapUnique(
        new LocalDeviceDataProvider(fake_cryptauth_service_.get()));
  }

  std::vector<BeaconSeed> fake_beacon_seeds_;
  std::vector<ExternalDeviceInfo> fake_synced_devices_;

  std::unique_ptr<FakeCryptAuthDeviceManager> fake_device_manager_;
  std::unique_ptr<FakeCryptAuthEnrollmentManager> fake_enrollment_manager_;
  std::unique_ptr<FakeCryptAuthService> fake_cryptauth_service_;

  std::unique_ptr<LocalDeviceDataProvider> provider_;

 private:
  DISALLOW_COPY_AND_ASSIGN(CryptAuthLocalDeviceDataProviderTest);
};

TEST_F(CryptAuthLocalDeviceDataProviderTest,
       TestGetLocalDeviceData_NoPublicKey) {
  fake_enrollment_manager_->set_user_public_key(std::string());
  fake_device_manager_->set_synced_devices(fake_synced_devices_);

  std::string public_key;
  std::vector<BeaconSeed> beacon_seeds;

  EXPECT_FALSE(provider_->GetLocalDeviceData(&public_key, &beacon_seeds));
}

TEST_F(CryptAuthLocalDeviceDataProviderTest,
       TestGetLocalDeviceData_NoSyncedDevices) {
  fake_enrollment_manager_->set_user_public_key(kDefaultPublicKey);

  std::string public_key;
  std::vector<BeaconSeed> beacon_seeds;

  EXPECT_FALSE(provider_->GetLocalDeviceData(&public_key, &beacon_seeds));
}

TEST_F(CryptAuthLocalDeviceDataProviderTest,
       TestGetLocalDeviceData_NoSyncedDeviceMatchingPublicKey) {
  fake_enrollment_manager_->set_user_public_key(kDefaultPublicKey);
  fake_device_manager_->set_synced_devices(std::vector<ExternalDeviceInfo>{
      fake_synced_devices_[0], fake_synced_devices_[1], fake_synced_devices_[2],
      fake_synced_devices_[3]});

  std::string public_key;
  std::vector<BeaconSeed> beacon_seeds;

  EXPECT_FALSE(provider_->GetLocalDeviceData(&public_key, &beacon_seeds));
}

TEST_F(CryptAuthLocalDeviceDataProviderTest,
       TestGetLocalDeviceData_SyncedDeviceIncludesPublicKeyButNoBeaconSeeds) {
  fake_enrollment_manager_->set_user_public_key(kDefaultPublicKey);
  fake_device_manager_->synced_devices().push_back(fake_synced_devices_[4]);

  std::string public_key;
  std::vector<BeaconSeed> beacon_seeds;

  EXPECT_FALSE(provider_->GetLocalDeviceData(&public_key, &beacon_seeds));
}

TEST_F(CryptAuthLocalDeviceDataProviderTest, TestGetLocalDeviceData_Success) {
  fake_enrollment_manager_->set_user_public_key(kDefaultPublicKey);
  fake_device_manager_->set_synced_devices(fake_synced_devices_);

  std::string public_key;
  std::vector<BeaconSeed> beacon_seeds;

  EXPECT_TRUE(provider_->GetLocalDeviceData(&public_key, &beacon_seeds));

  EXPECT_EQ(kDefaultPublicKey, public_key);

  ASSERT_EQ(fake_beacon_seeds_.size(), beacon_seeds.size());
  for (size_t i = 0; i < fake_beacon_seeds_.size(); i++) {
    // Note: google::protobuf::util::MessageDifferencer can only be used to diff
    // Message, but BeaconSeed derives from the incompatible MessageLite class.
    BeaconSeed expected = fake_beacon_seeds_[i];
    BeaconSeed actual = beacon_seeds[i];
    EXPECT_EQ(expected.data(), actual.data());
    EXPECT_EQ(expected.start_time_millis(), actual.start_time_millis());
    EXPECT_EQ(expected.end_time_millis(), actual.end_time_millis());
  }
}

}  // namespace cryptauth
