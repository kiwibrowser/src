// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/background_eid_generator.h"

#include <cstring>
#include <memory>

#include "base/strings/string_util.h"
#include "base/time/default_clock.h"
#include "base/time/time.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/raw_eid_generator.h"
#include "components/cryptauth/raw_eid_generator_impl.h"
#include "components/cryptauth/remote_beacon_seed_fetcher.h"
#include "components/cryptauth/remote_device_ref.h"

namespace cryptauth {

namespace {

// The duration of a EID period in milliseconds.
const int64_t kEidPeriodMs = 15 * 60 * 1000;  // 15 minutes

// The number of periods to look forward and backwards when calculating the
// neartest EIDs.
const int kEidLookAhead = 2;

// Returns the BeaconSeed valid for |timestamp_ms|, or nullptr if none can be
// found.
const BeaconSeed* GetBeaconSeedForTimestamp(
    int64_t timestamp_ms,
    const std::vector<BeaconSeed>& beacon_seeds) {
  for (const BeaconSeed& seed : beacon_seeds) {
    if (timestamp_ms >= seed.start_time_millis() &&
        timestamp_ms <= seed.end_time_millis()) {
      return &seed;
    }
  }
  return nullptr;
}

}  // namespace

BackgroundEidGenerator::BackgroundEidGenerator()
    : BackgroundEidGenerator(std::make_unique<RawEidGeneratorImpl>(),
                             base::DefaultClock::GetInstance()) {}

BackgroundEidGenerator::~BackgroundEidGenerator() {}

BackgroundEidGenerator::BackgroundEidGenerator(
    std::unique_ptr<RawEidGenerator> raw_eid_generator,
    base::Clock* clock)
    : raw_eid_generator_(std::move(raw_eid_generator)), clock_(clock) {}

std::vector<DataWithTimestamp> BackgroundEidGenerator::GenerateNearestEids(
    const std::vector<BeaconSeed>& beacon_seeds) const {
  int64_t now_timestamp_ms = clock_->Now().ToJavaTime();
  std::vector<DataWithTimestamp> eids;

  for (int i = -kEidLookAhead; i <= kEidLookAhead; ++i) {
    int64_t timestamp_ms = now_timestamp_ms + i * kEidPeriodMs;
    std::unique_ptr<DataWithTimestamp> eid =
        GenerateEid(timestamp_ms, beacon_seeds);
    if (eid)
      eids.push_back(*eid);
  }

  return eids;
}

std::unique_ptr<DataWithTimestamp> BackgroundEidGenerator::GenerateEid(
    int64_t timestamp_ms,
    const std::vector<BeaconSeed>& beacon_seeds) const {
  const BeaconSeed* beacon_seed =
      GetBeaconSeedForTimestamp(timestamp_ms, beacon_seeds);
  if (!beacon_seed) {
    PA_LOG(WARNING) << "  " << timestamp_ms << ": outside of BeaconSeed range.";
    return nullptr;
  }

  int64_t seed_start_time_ms = beacon_seed->start_time_millis();
  int64_t offset_time_ms = timestamp_ms - seed_start_time_ms;
  int64_t start_of_period_ms =
      seed_start_time_ms + (offset_time_ms / kEidPeriodMs) * kEidPeriodMs;

  std::string eid = raw_eid_generator_->GenerateEid(
      beacon_seed->data(), start_of_period_ms, nullptr);

  return std::make_unique<DataWithTimestamp>(eid, start_of_period_ms,
                                             start_of_period_ms + kEidPeriodMs);
}

std::string BackgroundEidGenerator::IdentifyRemoteDeviceByAdvertisement(
    cryptauth::RemoteBeaconSeedFetcher* remote_beacon_seed_fetcher,
    const std::string& advertisement_service_data,
    const std::vector<std::string>& device_ids) const {
  // Resize the service data to analyze only the first |kNumBytesInEidValue|
  // bytes. If there are any bytes after those first |kNumBytesInEidValue|
  // bytes, they are flags, so they are not needed to identify the device which
  // sent a message.
  std::string service_data_without_flags = advertisement_service_data;
  service_data_without_flags.resize(RawEidGenerator::kNumBytesInEidValue);

  const auto device_id_it = std::find_if(
      device_ids.begin(), device_ids.end(),
      [this, remote_beacon_seed_fetcher,
       &service_data_without_flags](auto device_id) {
        std::vector<BeaconSeed> beacon_seeds;
        if (!remote_beacon_seed_fetcher->FetchSeedsForDeviceId(device_id,
                                                               &beacon_seeds)) {
          PA_LOG(WARNING) << "Error fetching beacon seeds for device with ID "
                          << RemoteDeviceRef::TruncateDeviceIdForLogs(
                                 device_id);
          return false;
        }

        std::vector<DataWithTimestamp> eids = GenerateNearestEids(beacon_seeds);
        const auto eid_it = std::find_if(
            eids.begin(), eids.end(), [&service_data_without_flags](auto eid) {
              return eid.data == service_data_without_flags;
            });

        return eid_it != eids.end();
      });

  // Return empty string if no matching device is found.
  return device_id_it != device_ids.end() ? *device_id_it : std::string();
}

}  // cryptauth
