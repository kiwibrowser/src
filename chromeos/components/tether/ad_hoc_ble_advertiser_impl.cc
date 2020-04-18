// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/ad_hoc_ble_advertiser_impl.h"

#include "base/bind.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "chromeos/components/tether/error_tolerant_ble_advertisement_impl.h"
#include "chromeos/components/tether/timer_factory.h"
#include "components/cryptauth/ble/ble_advertisement_generator.h"
#include "components/cryptauth/remote_device_ref.h"

namespace chromeos {

namespace tether {

namespace {

// Empirically, phones generally pick up scan results in 1-6 seconds. 12 seconds
// adds an extra buffer to that time to ensure that the advertisement is
// discovered by the Tether host.
constexpr const int64_t kNumSecondsToAdvertise = 12;

}  // namespace

AdHocBleAdvertiserImpl::AdvertisementWithTimer::AdvertisementWithTimer(
    std::unique_ptr<ErrorTolerantBleAdvertisement> advertisement,
    std::unique_ptr<base::Timer> timer)
    : advertisement(std::move(advertisement)), timer(std::move(timer)) {}

AdHocBleAdvertiserImpl::AdvertisementWithTimer::~AdvertisementWithTimer() {}

AdHocBleAdvertiserImpl::AdHocBleAdvertiserImpl(
    cryptauth::LocalDeviceDataProvider* local_device_data_provider,
    cryptauth::RemoteBeaconSeedFetcher* remote_beacon_seed_fetcher,
    BleSynchronizerBase* ble_synchronizer)
    : local_device_data_provider_(local_device_data_provider),
      remote_beacon_seed_fetcher_(remote_beacon_seed_fetcher),
      ble_synchronizer_(ble_synchronizer),
      timer_factory_(std::make_unique<TimerFactory>()),
      weak_ptr_factory_(this) {}

AdHocBleAdvertiserImpl::~AdHocBleAdvertiserImpl() {}

void AdHocBleAdvertiserImpl::RequestGattServicesForDevice(
    const std::string& device_id) {
  // If an advertisement to that device is already in progress, there is nothing
  // to do.
  if (device_id_to_advertisement_with_timer_map_.find(device_id) !=
      device_id_to_advertisement_with_timer_map_.end()) {
    PA_LOG(INFO) << "Advertisement already in progress to device with ID \""
                 << cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(
                        device_id)
                 << "\".";
    return;
  }

  // Generate a new advertisement to device with ID |device_id|.
  std::unique_ptr<cryptauth::DataWithTimestamp> service_data =
      cryptauth::BleAdvertisementGenerator::GenerateBleAdvertisement(
          device_id, local_device_data_provider_, remote_beacon_seed_fetcher_);
  if (!service_data) {
    PA_LOG(WARNING) << "Cannot generate advertisement for device with ID \""
                    << cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(
                           device_id)
                    << "\"; GATT services cannot be requested.";
    return;
  }

  std::unique_ptr<ErrorTolerantBleAdvertisement> advertisement =
      ErrorTolerantBleAdvertisementImpl::Factory::NewInstance(
          device_id, std::move(service_data), ble_synchronizer_);

  PA_LOG(INFO) << "Requesting GATT services for device with ID \""
               << cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(device_id)
               << "\" by creating a new advertisement to that device.";

  std::unique_ptr<base::Timer> timer = timer_factory_->CreateOneShotTimer();
  timer->Start(FROM_HERE, base::TimeDelta::FromSeconds(kNumSecondsToAdvertise),
               base::Bind(&AdHocBleAdvertiserImpl::OnTimerFired,
                          weak_ptr_factory_.GetWeakPtr(), device_id));

  device_id_to_advertisement_with_timer_map_.emplace(
      std::piecewise_construct, std::forward_as_tuple(device_id),
      std::forward_as_tuple(std::move(advertisement), std::move(timer)));
}

bool AdHocBleAdvertiserImpl::HasPendingRequests() {
  return !device_id_to_advertisement_with_timer_map_.empty();
}

void AdHocBleAdvertiserImpl::OnTimerFired(const std::string& device_id) {
  auto it = device_id_to_advertisement_with_timer_map_.find(device_id);
  if (it == device_id_to_advertisement_with_timer_map_.end()) {
    PA_LOG(ERROR) << "Timer fired for device with ID \""
                  << cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(
                         device_id)
                  << "\", but that device is not present in the map.";
    return;
  }

  PA_LOG(INFO) << "Stopping workaround advertisement for device with ID \""
               << cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(device_id)
               << "\".";

  it->second.advertisement->Stop(
      base::Bind(&AdHocBleAdvertiserImpl::OnAdvertisementStopped,
                 weak_ptr_factory_.GetWeakPtr(), device_id));
}

void AdHocBleAdvertiserImpl::OnAdvertisementStopped(
    const std::string& device_id) {
  auto it = device_id_to_advertisement_with_timer_map_.find(device_id);
  if (it == device_id_to_advertisement_with_timer_map_.end()) {
    PA_LOG(ERROR) << "Advertisement stopped for device ID \""
                  << cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(
                         device_id)
                  << "\", but that device is not present in the map.";
    return;
  }

  device_id_to_advertisement_with_timer_map_.erase(it);

  if (device_id_to_advertisement_with_timer_map_.empty())
    NotifyAsynchronousShutdownComplete();
}

void AdHocBleAdvertiserImpl::SetTimerFactoryForTesting(
    std::unique_ptr<TimerFactory> test_timer_factory) {
  timer_factory_ = std::move(test_timer_factory);
}

}  // namespace tether

}  // namespace chromeos
