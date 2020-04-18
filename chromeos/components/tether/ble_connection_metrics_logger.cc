// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/ble_connection_metrics_logger.h"

#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/default_clock.h"
#include "chromeos/components/proximity_auth/logging/logging.h"

namespace chromeos {

namespace tether {

BleConnectionMetricsLogger::BleConnectionMetricsLogger()
    : clock_(base::DefaultClock::GetInstance()) {}

BleConnectionMetricsLogger::~BleConnectionMetricsLogger() = default;

void BleConnectionMetricsLogger::OnConnectionAttemptStarted(
    const std::string& device_id) {
  device_id_to_started_scan_time_map_[device_id] = clock_->Now();
}

void BleConnectionMetricsLogger::OnAdvertisementReceived(
    const std::string& device_id,
    bool is_background_advertisement) {
  device_id_to_received_advertisement_time_map_[device_id] = clock_->Now();
  RecordStartScanToReceiveAdvertisementDuration(device_id,
                                                is_background_advertisement);
}

void BleConnectionMetricsLogger::OnConnection(
    const std::string& device_id,
    bool is_background_advertisement) {
  device_id_to_status_connected_time_map_[device_id] = clock_->Now();
  RecordStartScanToConnectionDuration(device_id, is_background_advertisement);
}

void BleConnectionMetricsLogger::OnSecureChannelCreated(
    const std::string& device_id,
    bool is_background_advertisement) {
  RecordConnectionToAuthenticationDuration(device_id,
                                           is_background_advertisement);
  RecordGattConnectionAttemptSuccessRate(true /* success */,
                                         is_background_advertisement);

  device_id_to_status_authenticated_map_[device_id] = true;
}

void BleConnectionMetricsLogger::OnDeviceDisconnected(
    const std::string& device_id,
    BleConnectionManager::StateChangeDetail state_change_detail,
    bool is_background_advertisement) {
  if (state_change_detail ==
      BleConnectionManager::StateChangeDetail::
          STATE_CHANGE_DETAIL_GATT_CONNECTION_WAS_ATTEMPTED) {
    RecordGattConnectionAttemptSuccessRate(false /* success */,
                                           is_background_advertisement);
  } else if (state_change_detail ==
             BleConnectionManager::StateChangeDetail::
                 STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED) {
    bool did_device_disconnect_after_success =
        base::ContainsKey(device_id_to_status_authenticated_map_, device_id) &&
        device_id_to_status_authenticated_map_[device_id];
    RecordGattConnectionAttemptEffectiveSuccessRateWithRetries(
        did_device_disconnect_after_success /* success */,
        is_background_advertisement);
  }

  device_id_to_started_scan_time_map_.erase(device_id);
  device_id_to_received_advertisement_time_map_.erase(device_id);
  device_id_to_status_connected_time_map_.erase(device_id);
  device_id_to_status_authenticated_map_.erase(device_id);
}

void BleConnectionMetricsLogger::RecordStartScanToReceiveAdvertisementDuration(
    const std::string device_id,
    bool is_background_advertisement) {
  if (!base::ContainsKey(device_id_to_started_scan_time_map_, device_id) ||
      !base::ContainsKey(device_id_to_received_advertisement_time_map_,
                         device_id)) {
    PA_LOG(ERROR) << "Failed to record start scan to advertisement duration: "
                  << "times are invalid";
    NOTREACHED();
    return;
  }

  base::TimeDelta start_scan_to_received_advertisment_duration =
      device_id_to_received_advertisement_time_map_[device_id] -
      device_id_to_started_scan_time_map_[device_id];

  if (is_background_advertisement) {
    UMA_HISTOGRAM_TIMES(
        "InstantTethering.Performance.StartScanToReceiveAdvertisementDuration."
        "Background",
        start_scan_to_received_advertisment_duration);
  } else {
    UMA_HISTOGRAM_TIMES(
        "InstantTethering.Performance.StartScanToReceiveAdvertisementDuration",
        start_scan_to_received_advertisment_duration);
  }
}

void BleConnectionMetricsLogger::RecordStartScanToConnectionDuration(
    const std::string device_id,
    bool is_background_advertisement) {
  if (!base::ContainsKey(device_id_to_started_scan_time_map_, device_id) ||
      !base::ContainsKey(device_id_to_received_advertisement_time_map_,
                         device_id) ||
      !base::ContainsKey(device_id_to_status_connected_time_map_, device_id)) {
    PA_LOG(ERROR) << "Failed to record start scan to connection duration: "
                  << "times are invalid";
    NOTREACHED();
    return;
  }

  base::TimeDelta received_advertisment_to_connection_duration =
      device_id_to_status_connected_time_map_[device_id] -
      device_id_to_received_advertisement_time_map_[device_id];
  base::TimeDelta start_scan_to_connection_duration =
      device_id_to_status_connected_time_map_[device_id] -
      device_id_to_started_scan_time_map_[device_id];

  if (is_background_advertisement) {
    UMA_HISTOGRAM_MEDIUM_TIMES(
        "InstantTethering.Performance.ReceiveAdvertisementToConnectionDuration."
        "Background",
        received_advertisment_to_connection_duration);
    UMA_HISTOGRAM_MEDIUM_TIMES(
        "InstantTethering.Performance.StartScanToConnectionDuration.Background",
        start_scan_to_connection_duration);
  } else {
    UMA_HISTOGRAM_MEDIUM_TIMES(
        "InstantTethering.Performance.ReceiveAdvertisementToConnectionDuration",
        received_advertisment_to_connection_duration);
    UMA_HISTOGRAM_MEDIUM_TIMES(
        "InstantTethering.Performance.AdvertisementToConnectionDuration",
        start_scan_to_connection_duration);
  }

  device_id_to_started_scan_time_map_.erase(device_id);
  device_id_to_received_advertisement_time_map_.erase(device_id);
}

void BleConnectionMetricsLogger::RecordConnectionToAuthenticationDuration(
    const std::string device_id,
    bool is_background_advertisement) {
  if (!base::ContainsKey(device_id_to_status_connected_time_map_, device_id)) {
    PA_LOG(ERROR) << "Failed to record connection to authentication duration: "
                  << "connection start time is invalid";
    NOTREACHED();
    return;
  }

  base::TimeDelta connection_to_authentication_duration =
      clock_->Now() - device_id_to_status_connected_time_map_[device_id];
  if (is_background_advertisement) {
    UMA_HISTOGRAM_TIMES(
        "InstantTethering.Performance.ConnectionToAuthenticationDuration."
        "Background",
        connection_to_authentication_duration);
  } else {
    UMA_HISTOGRAM_TIMES(
        "InstantTethering.Performance.ConnectionToAuthenticationDuration",
        connection_to_authentication_duration);
  }

  device_id_to_status_connected_time_map_.erase(device_id);
}

void BleConnectionMetricsLogger::RecordGattConnectionAttemptSuccessRate(
    bool success,
    bool is_background_advertisement) {
  if (is_background_advertisement) {
    UMA_HISTOGRAM_BOOLEAN(
        "InstantTethering.GattConnectionAttempt.SuccessRate.Background",
        success);
  } else {
    UMA_HISTOGRAM_BOOLEAN("InstantTethering.GattConnectionAttempt.SuccessRate",
                          success);
  }
}

void BleConnectionMetricsLogger::
    RecordGattConnectionAttemptEffectiveSuccessRateWithRetries(
        bool success,
        bool is_background_advertisement) {
  if (is_background_advertisement) {
    UMA_HISTOGRAM_BOOLEAN(
        "InstantTethering.GattConnectionAttempt."
        "EffectiveSuccessRateWithRetries.Background",
        success);
  } else {
    UMA_HISTOGRAM_BOOLEAN(
        "InstantTethering.GattConnectionAttempt."
        "EffectiveSuccessRateWithRetries",
        success);
  }
}

void BleConnectionMetricsLogger::SetClockForTesting(base::Clock* test_clock) {
  clock_ = test_clock;
}

}  // namespace tether

}  // namespace chromeos