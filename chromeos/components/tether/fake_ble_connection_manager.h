// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_FAKE_BLE_CONNECTION_MANAGER_H_
#define CHROMEOS_COMPONENTS_TETHER_FAKE_BLE_CONNECTION_MANAGER_H_

#include <map>
#include <set>

#include "base/macros.h"
#include "chromeos/components/tether/ble_connection_manager.h"

namespace chromeos {

namespace tether {

// Test double for BleConnectionManager.
class FakeBleConnectionManager : public BleConnectionManager {
 public:
  FakeBleConnectionManager();
  ~FakeBleConnectionManager() override;

  struct SentMessage {
    std::string device_id;
    std::string message;
  };

  void SetDeviceStatus(
      const std::string& device_id,
      const cryptauth::SecureChannel::Status& status,
      BleConnectionManager::StateChangeDetail state_change_detail);
  void ReceiveMessage(const std::string& device_id, const std::string& payload);
  void SetMessageSent(int sequence_number);

  // Simulates |num_attempts| consecutive failed "unanswered" connection
  // attempts for the device with ID |device_id|. Specifically, this function
  // updates the device's status to CONNECTING then DISCONNECTED on each
  // attempt.
  void SimulateUnansweredConnectionAttempts(const std::string& device_id,
                                            size_t num_attempts);

  // Simulates |num_attempts| consecutive failed "GATT error" connection
  // attempts for the device with ID |device_id|. Specifically, this function
  // updates the device's status to CONNECTING, then CONNECTED, then
  // AUTHENTICATING, then DISCONNECTED on each attempt.
  void SimulateGattErrorConnectionAttempts(const std::string& device_id,
                                           size_t num_attempts);

  std::vector<SentMessage>& sent_messages() { return sent_messages_; }
  // Returns -1 if no sequence numbers have been used yet.
  int last_sequence_number() { return next_sequence_number_ - 1; }

  bool IsRegistered(const std::string& device_id);

  // BleConnectionManager:
  void RegisterRemoteDevice(const std::string& device_id,
                            const ConnectionReason& connection_reason) override;
  void UnregisterRemoteDevice(
      const std::string& device_id,
      const ConnectionReason& connection_reason) override;
  int SendMessage(const std::string& device_id,
                  const std::string& message) override;
  bool GetStatusForDevice(
      const std::string& device_id,
      cryptauth::SecureChannel::Status* status) const override;

  using BleConnectionManager::NotifyAdvertisementReceived;

 private:
  struct StatusAndRegisteredConnectionReasons {
    StatusAndRegisteredConnectionReasons();
    StatusAndRegisteredConnectionReasons(
        const StatusAndRegisteredConnectionReasons& other);
    ~StatusAndRegisteredConnectionReasons();

    cryptauth::SecureChannel::Status status;
    std::set<ConnectionReason> registered_message_types;
  };

  int next_sequence_number_ = 0;
  std::map<std::string, StatusAndRegisteredConnectionReasons> device_id_map_;
  std::vector<SentMessage> sent_messages_;

  DISALLOW_COPY_AND_ASSIGN(FakeBleConnectionManager);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_FAKE_BLE_CONNECTION_MANAGER_H_
