// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/fake_ble_connection_manager.h"

#include "chromeos/components/tether/timer_factory.h"
#include "device/bluetooth/bluetooth_adapter.h"

namespace chromeos {

namespace tether {

FakeBleConnectionManager::StatusAndRegisteredConnectionReasons::
    StatusAndRegisteredConnectionReasons()
    : status(cryptauth::SecureChannel::Status::DISCONNECTED) {}

FakeBleConnectionManager::StatusAndRegisteredConnectionReasons::
    StatusAndRegisteredConnectionReasons(
        const StatusAndRegisteredConnectionReasons& other) = default;

FakeBleConnectionManager::StatusAndRegisteredConnectionReasons::
    ~StatusAndRegisteredConnectionReasons() = default;

FakeBleConnectionManager::FakeBleConnectionManager()
    : BleConnectionManager(nullptr,
                           nullptr,
                           nullptr,
                           nullptr,
                           nullptr,
                           nullptr) {}

FakeBleConnectionManager::~FakeBleConnectionManager() = default;

void FakeBleConnectionManager::SetDeviceStatus(
    const std::string& device_id,
    const cryptauth::SecureChannel::Status& status,
    BleConnectionManager::StateChangeDetail state_change_detail) {
  const auto iter = device_id_map_.find(device_id);
  DCHECK(iter != device_id_map_.end());

  cryptauth::SecureChannel::Status old_status = iter->second.status;
  if (old_status == status) {
    // If the status has not changed, do not do anything.
    return;
  }

  iter->second.status = status;
  NotifySecureChannelStatusChanged(device_id, old_status, status,
                                   state_change_detail);
}

void FakeBleConnectionManager::ReceiveMessage(const std::string& device_id,
                                              const std::string& payload) {
  DCHECK(device_id_map_.find(device_id) != device_id_map_.end());
  NotifyMessageReceived(device_id, payload);
}

void FakeBleConnectionManager::SetMessageSent(int sequence_number) {
  DCHECK(sequence_number < next_sequence_number_);
  NotifyMessageSent(sequence_number);
}

void FakeBleConnectionManager::SimulateUnansweredConnectionAttempts(
    const std::string& device_id,
    size_t num_attempts) {
  for (size_t i = 0; i < num_attempts; ++i) {
    SetDeviceStatus(device_id, cryptauth::SecureChannel::Status::CONNECTING,
                    StateChangeDetail::STATE_CHANGE_DETAIL_NONE);
    SetDeviceStatus(
        device_id, cryptauth::SecureChannel::Status::DISCONNECTED,
        StateChangeDetail::STATE_CHANGE_DETAIL_COULD_NOT_ATTEMPT_CONNECTION);
  }
}

void FakeBleConnectionManager::SimulateGattErrorConnectionAttempts(
    const std::string& device_id,
    size_t num_attempts) {
  for (size_t i = 0; i < num_attempts; ++i) {
    SetDeviceStatus(device_id, cryptauth::SecureChannel::Status::CONNECTING,
                    StateChangeDetail::STATE_CHANGE_DETAIL_NONE);
    SetDeviceStatus(device_id, cryptauth::SecureChannel::Status::CONNECTED,
                    StateChangeDetail::STATE_CHANGE_DETAIL_NONE);
    SetDeviceStatus(device_id, cryptauth::SecureChannel::Status::AUTHENTICATING,
                    StateChangeDetail::STATE_CHANGE_DETAIL_NONE);
    SetDeviceStatus(
        device_id, cryptauth::SecureChannel::Status::DISCONNECTED,
        StateChangeDetail::STATE_CHANGE_DETAIL_GATT_CONNECTION_WAS_ATTEMPTED);
  }
}

bool FakeBleConnectionManager::IsRegistered(const std::string& device_id) {
  return base::ContainsKey(device_id_map_, device_id);
}

void FakeBleConnectionManager::RegisterRemoteDevice(
    const std::string& device_id,
    const ConnectionReason& connection_reason) {
  StatusAndRegisteredConnectionReasons& value = device_id_map_[device_id];
  value.registered_message_types.insert(connection_reason);
}

void FakeBleConnectionManager::UnregisterRemoteDevice(
    const std::string& device_id,
    const ConnectionReason& connection_reason) {
  StatusAndRegisteredConnectionReasons& value = device_id_map_[device_id];
  value.registered_message_types.erase(connection_reason);
  if (value.registered_message_types.empty())
    device_id_map_.erase(device_id);
}

int FakeBleConnectionManager::SendMessage(const std::string& device_id,
                                          const std::string& message) {
  sent_messages_.push_back({device_id, message});
  return next_sequence_number_++;
}

bool FakeBleConnectionManager::GetStatusForDevice(
    const std::string& device_id,
    cryptauth::SecureChannel::Status* status) const {
  const auto iter = device_id_map_.find(device_id);
  if (iter == device_id_map_.end())
    return false;

  *status = iter->second.status;
  return true;
}

}  // namespace tether

}  // namespace chromeos
