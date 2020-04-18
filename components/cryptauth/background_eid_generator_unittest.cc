// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/background_eid_generator.h"

#include <memory>
#include <string>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/test/simple_test_clock.h"
#include "base/time/time.h"
#include "components/cryptauth/mock_remote_beacon_seed_fetcher.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/raw_eid_generator_impl.h"
#include "components/cryptauth/remote_device_ref.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cryptauth {

namespace {
const int64_t kEidPeriodMs = base::TimeDelta::FromMinutes(15).InMilliseconds();
const int64_t kBeaconSeedDurationMs =
    base::TimeDelta::FromDays(14).InMilliseconds();

// The number of nearest EIDs returned by GenerateNearestEids().
const size_t kEidCount = 5;

// Midnight on 1/1/2020.
const int64_t kStartPeriodMs = 1577836800000L;
// 1:43am on 1/1/2020.
const int64_t kCurrentTimeMs = 1577843000000L;

const std::string kFirstSeed = "firstSeed";
const std::string kSecondSeed = "secondSeed";
const std::string kThirdSeed = "thirdSeed";
const std::string kFourthSeed = "fourthSeed";

const std::string kDeviceId1 = "deviceId1";
const std::string kDeviceId2 = "deviceId2";

BeaconSeed CreateBeaconSeed(const std::string& data,
                            const int64_t start_timestamp_ms,
                            const int64_t end_timestamp_ms) {
  BeaconSeed seed;
  seed.set_data(data);
  seed.set_start_time_millis(start_timestamp_ms);
  seed.set_end_time_millis(end_timestamp_ms);
  return seed;
}

DataWithTimestamp CreateDataWithTimestamp(
    const std::string& eid_seed,
    int64_t start_of_period_timestamp_ms) {
  std::unique_ptr<RawEidGenerator> raw_eid_generator =
      std::make_unique<RawEidGeneratorImpl>();
  std::string data = raw_eid_generator->GenerateEid(
      eid_seed, start_of_period_timestamp_ms, nullptr /* extra_entropy */);
  return DataWithTimestamp(data, start_of_period_timestamp_ms,
                           start_of_period_timestamp_ms + kEidPeriodMs);
}

class TestRawEidGenerator : public RawEidGeneratorImpl {
 public:
  TestRawEidGenerator() {}
  ~TestRawEidGenerator() override {}

  // RawEidGenerator:
  std::string GenerateEid(const std::string& eid_seed,
                          int64_t start_of_period_timestamp_ms,
                          std::string const* extra_entropy) override {
    EXPECT_FALSE(extra_entropy);
    return RawEidGeneratorImpl::GenerateEid(
        eid_seed, start_of_period_timestamp_ms, extra_entropy);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestRawEidGenerator);
};

}  //  namespace

class CryptAuthBackgroundEidGeneratorTest : public testing::Test {
 protected:
  CryptAuthBackgroundEidGeneratorTest() {
    beacon_seeds_.push_back(CreateBeaconSeed(
        kFirstSeed, kStartPeriodMs - kBeaconSeedDurationMs, kStartPeriodMs));
    beacon_seeds_.push_back(CreateBeaconSeed(
        kSecondSeed, kStartPeriodMs, kStartPeriodMs + kBeaconSeedDurationMs));
    beacon_seeds_.push_back(
        CreateBeaconSeed(kThirdSeed, kStartPeriodMs + kBeaconSeedDurationMs,
                         kStartPeriodMs + 2 * kBeaconSeedDurationMs));
    beacon_seeds_.push_back(CreateBeaconSeed(
        kFourthSeed, kStartPeriodMs + 2 * kBeaconSeedDurationMs,
        kStartPeriodMs + 3 * kBeaconSeedDurationMs));
  }

  void SetUp() override {
    SetTestTime(kCurrentTimeMs);

    mock_seed_fetcher_ =
        std::make_unique<cryptauth::MockRemoteBeaconSeedFetcher>();

    eid_generator_.reset(new BackgroundEidGenerator(
        std::make_unique<TestRawEidGenerator>(), &test_clock_));
  }

  void SetTestTime(int64_t timestamp_ms) {
    base::Time time = base::Time::UnixEpoch() +
                      base::TimeDelta::FromMilliseconds(timestamp_ms);
    test_clock_.SetNow(time);
  }

  std::unique_ptr<BackgroundEidGenerator> eid_generator_;
  std::unique_ptr<MockRemoteBeaconSeedFetcher> mock_seed_fetcher_;
  base::SimpleTestClock test_clock_;
  std::vector<BeaconSeed> beacon_seeds_;
};

TEST_F(CryptAuthBackgroundEidGeneratorTest,
       GenerateNearestEids_BeaconSeedsExpired) {
  SetTestTime(beacon_seeds_[beacon_seeds_.size() - 1].end_time_millis() +
              kEidCount * kEidPeriodMs);
  std::vector<DataWithTimestamp> eids =
      eid_generator_->GenerateNearestEids(beacon_seeds_);
  EXPECT_EQ(0u, eids.size());
}

TEST_F(CryptAuthBackgroundEidGeneratorTest,
       GenerateNearestEids_BeaconSeedsValidInFuture) {
  SetTestTime(beacon_seeds_[0].start_time_millis() - kEidCount * kEidPeriodMs);
  std::vector<DataWithTimestamp> eids =
      eid_generator_->GenerateNearestEids(beacon_seeds_);
  EXPECT_EQ(0u, eids.size());
}

TEST_F(CryptAuthBackgroundEidGeneratorTest,
       GenerateNearestEids_EidsUseSameBeaconSeed) {
  int64_t start_period_ms =
      beacon_seeds_[0].start_time_millis() + kEidCount * kEidPeriodMs;
  SetTestTime(start_period_ms + kEidPeriodMs / 2);

  std::vector<DataWithTimestamp> eids =
      eid_generator_->GenerateNearestEids(beacon_seeds_);

  std::string seed = beacon_seeds_[0].data();
  EXPECT_EQ(kEidCount, eids.size());
  EXPECT_EQ(CreateDataWithTimestamp(seed, start_period_ms - 2 * kEidPeriodMs),
            eids[0]);
  EXPECT_EQ(CreateDataWithTimestamp(seed, start_period_ms - 1 * kEidPeriodMs),
            eids[1]);
  EXPECT_EQ(CreateDataWithTimestamp(seed, start_period_ms + 0 * kEidPeriodMs),
            eids[2]);
  EXPECT_EQ(CreateDataWithTimestamp(seed, start_period_ms + 1 * kEidPeriodMs),
            eids[3]);
  EXPECT_EQ(CreateDataWithTimestamp(seed, start_period_ms + 2 * kEidPeriodMs),
            eids[4]);
}

TEST_F(CryptAuthBackgroundEidGeneratorTest,
       GenerateNearestEids_EidsAcrossBeaconSeeds) {
  int64_t end_period_ms = beacon_seeds_[0].end_time_millis();
  int64_t start_period_ms = beacon_seeds_[1].start_time_millis();
  SetTestTime(start_period_ms + kEidPeriodMs / 2);

  std::vector<DataWithTimestamp> eids =
      eid_generator_->GenerateNearestEids(beacon_seeds_);

  std::string seed0 = beacon_seeds_[0].data();
  std::string seed1 = beacon_seeds_[1].data();
  EXPECT_EQ(kEidCount, eids.size());
  EXPECT_EQ(CreateDataWithTimestamp(seed0, end_period_ms - 2 * kEidPeriodMs),
            eids[0]);
  EXPECT_EQ(CreateDataWithTimestamp(seed0, end_period_ms - 1 * kEidPeriodMs),
            eids[1]);
  EXPECT_EQ(CreateDataWithTimestamp(seed1, start_period_ms + 0 * kEidPeriodMs),
            eids[2]);
  EXPECT_EQ(CreateDataWithTimestamp(seed1, start_period_ms + 1 * kEidPeriodMs),
            eids[3]);
  EXPECT_EQ(CreateDataWithTimestamp(seed1, start_period_ms + 2 * kEidPeriodMs),
            eids[4]);
}

TEST_F(CryptAuthBackgroundEidGeneratorTest,
       GenerateNearestEids_CurrentTimeAtStartOfRange) {
  int64_t start_period_ms = beacon_seeds_[0].start_time_millis();
  SetTestTime(start_period_ms + kEidPeriodMs / 2);

  std::vector<DataWithTimestamp> eids =
      eid_generator_->GenerateNearestEids(beacon_seeds_);

  std::string seed = beacon_seeds_[0].data();
  EXPECT_EQ(3u, eids.size());
  EXPECT_EQ(CreateDataWithTimestamp(seed, start_period_ms + 0 * kEidPeriodMs),
            eids[0]);
  EXPECT_EQ(CreateDataWithTimestamp(seed, start_period_ms + 1 * kEidPeriodMs),
            eids[1]);
  EXPECT_EQ(CreateDataWithTimestamp(seed, start_period_ms + 2 * kEidPeriodMs),
            eids[2]);
}

TEST_F(CryptAuthBackgroundEidGeneratorTest,
       GenerateNearestEids_CurrentTimeAtEndOfRange) {
  int64_t start_period_ms = beacon_seeds_[3].end_time_millis() - kEidPeriodMs;
  SetTestTime(start_period_ms + kEidPeriodMs / 2);

  std::vector<DataWithTimestamp> eids =
      eid_generator_->GenerateNearestEids(beacon_seeds_);

  std::string seed = beacon_seeds_[3].data();
  EXPECT_EQ(3u, eids.size());
  EXPECT_EQ(CreateDataWithTimestamp(seed, start_period_ms - 2 * kEidPeriodMs),
            eids[0]);
  EXPECT_EQ(CreateDataWithTimestamp(seed, start_period_ms - 1 * kEidPeriodMs),
            eids[1]);
  EXPECT_EQ(CreateDataWithTimestamp(seed, start_period_ms - 0 * kEidPeriodMs),
            eids[2]);
}

// Test the case where the account has other devices, but their beacon seeds
// don't match the incoming advertisement. |beacon_seeds_[0]| corresponds to
// |kDeviceId1|. Since |kDeviceId1| is not present in the device ids passed to
// IdentifyRemoteDeviceByAdvertisement(), no match is expected to be found.
TEST_F(CryptAuthBackgroundEidGeneratorTest,
       IdentifyRemoteDeviceByAdvertisement_NoMatchingRemoteDevices) {
  SetTestTime(kStartPeriodMs + kEidPeriodMs / 2);
  DataWithTimestamp advertisement_eid = CreateDataWithTimestamp(
      beacon_seeds_[0].data(), kStartPeriodMs - kEidPeriodMs);
  mock_seed_fetcher_->SetSeedsForDeviceId(kDeviceId1, &beacon_seeds_);

  EXPECT_EQ(std::string(), eid_generator_->IdentifyRemoteDeviceByAdvertisement(
                               mock_seed_fetcher_.get(), advertisement_eid.data,
                               {kDeviceId2}));
}

TEST_F(CryptAuthBackgroundEidGeneratorTest,
       IdentifyRemoteDeviceByAdvertisement_Success) {
  SetTestTime(kStartPeriodMs + kEidPeriodMs / 2);
  DataWithTimestamp advertisement_eid = CreateDataWithTimestamp(
      beacon_seeds_[0].data(), kStartPeriodMs - kEidPeriodMs);
  mock_seed_fetcher_->SetSeedsForDeviceId(kDeviceId1, &beacon_seeds_);

  EXPECT_EQ(kDeviceId1, eid_generator_->IdentifyRemoteDeviceByAdvertisement(
                            mock_seed_fetcher_.get(), advertisement_eid.data,
                            {kDeviceId1, kDeviceId2}));
}

}  // namespace cryptauth
