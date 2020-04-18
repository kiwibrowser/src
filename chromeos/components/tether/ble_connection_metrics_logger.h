// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_BLE_CONNECTION_METRICS_LOGGER_H_
#define CHROMEOS_COMPONENTS_TETHER_BLE_CONNECTION_METRICS_LOGGER_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "chromeos/components/tether/ble_connection_manager.h"

namespace base {
class Clock;
}

namespace chromeos {

namespace tether {

// Responsible for logging metrics for various stages of a BLE connection to
// another device (both success and performance metrics). Listens on events
// BleConnectionManager to keep track of when events occur, and if they end in
// success or failure.
class BleConnectionMetricsLogger
    : public BleConnectionManager::MetricsObserver {
 public:
  BleConnectionMetricsLogger();
  virtual ~BleConnectionMetricsLogger();

  // BleConnectionManager::MetricsObserver:
  void OnConnectionAttemptStarted(const std::string& device_id) override;
  void OnAdvertisementReceived(const std::string& device_id,
                               bool is_background_advertisement) override;
  void OnConnection(const std::string& device_id,
                    bool is_background_advertisement) override;
  void OnSecureChannelCreated(const std::string& device_id,
                              bool is_background_advertisement) override;
  void OnDeviceDisconnected(
      const std::string& device_id,
      BleConnectionManager::StateChangeDetail state_change_detail,
      bool is_background_advertisement) override;

 private:
  friend class BleConnectionMetricsLoggerTest;

  void RecordStartScanToReceiveAdvertisementDuration(
      const std::string device_id,
      bool is_background_advertisement);
  void RecordStartScanToConnectionDuration(const std::string device_id,
                                           bool is_background_advertisement);
  void RecordConnectionToAuthenticationDuration(
      const std::string device_id,
      bool is_background_advertisement);
  void RecordGattConnectionAttemptSuccessRate(bool success,
                                              bool is_background_advertisement);
  void RecordGattConnectionAttemptEffectiveSuccessRateWithRetries(
      bool success,
      bool is_background_advertisement);

  void SetClockForTesting(base::Clock* test_clock);

  base::Clock* clock_;
  std::map<std::string, base::Time> device_id_to_started_scan_time_map_;
  std::map<std::string, base::Time>
      device_id_to_received_advertisement_time_map_;
  std::map<std::string, base::Time> device_id_to_status_connected_time_map_;
  std::map<std::string, bool> device_id_to_status_authenticated_map_;

  DISALLOW_COPY_AND_ASSIGN(BleConnectionMetricsLogger);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_BLE_CONNECTION_METRICS_LOGGER_H_