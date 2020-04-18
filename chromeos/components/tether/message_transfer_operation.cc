// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/message_transfer_operation.h"

#include <memory>
#include <set>

#include "chromeos/components/proximity_auth/logging/logging.h"
#include "chromeos/components/tether/connection_reason.h"
#include "chromeos/components/tether/message_wrapper.h"
#include "chromeos/components/tether/timer_factory.h"

namespace chromeos {

namespace tether {

namespace {

cryptauth::RemoteDeviceRefList RemoveDuplicatesFromVector(
    const cryptauth::RemoteDeviceRefList& remote_devices) {
  cryptauth::RemoteDeviceRefList updated_remote_devices;
  std::set<cryptauth::RemoteDeviceRef> remote_devices_set;
  for (const auto& remote_device : remote_devices) {
    // Only add the device to the output vector if it has not already been put
    // into the set.
    if (remote_devices_set.find(remote_device) == remote_devices_set.end()) {
      remote_devices_set.insert(remote_device);
      updated_remote_devices.push_back(remote_device);
    }
  }
  return updated_remote_devices;
}

}  // namespace

// static
const uint32_t MessageTransferOperation::kMaxEmptyScansPerDevice = 3;

// static
const uint32_t MessageTransferOperation::kMaxGattConnectionAttemptsPerDevice =
    6;

MessageTransferOperation::MessageTransferOperation(
    const cryptauth::RemoteDeviceRefList& devices_to_connect,
    BleConnectionManager* connection_manager)
    : remote_devices_(RemoveDuplicatesFromVector(devices_to_connect)),
      connection_manager_(connection_manager),
      timer_factory_(std::make_unique<TimerFactory>()),
      weak_ptr_factory_(this) {}

MessageTransferOperation::~MessageTransferOperation() {
  // If initialization never occurred, devices were never registered.
  if (!initialized_)
    return;

  connection_manager_->RemoveObserver(this);

  shutting_down_ = true;

  // Unregister any devices that are still registered; otherwise, Bluetooth
  // connections will continue to stay alive until the Tether component is shut
  // down (see crbug.com/761106). Note that a copy of |remote_devices_| is used
  // here because UnregisterDevice() will modify |remote_devices_| internally.
  cryptauth::RemoteDeviceRefList remote_devices_copy = remote_devices_;
  for (const auto& remote_device : remote_devices_copy)
    UnregisterDevice(remote_device);
}

void MessageTransferOperation::Initialize() {
  if (initialized_) {
    return;
  }
  initialized_ = true;

  // Store the message type for this connection as a private field. This is
  // necessary because when UnregisterDevice() is called in the destructor, the
  // derived class has already been destroyed, so invoking
  // GetMessageTypeForConnection() will fail due to it being a pure virtual
  // function at that time.
  message_type_for_connection_ = GetMessageTypeForConnection();

  connection_manager_->AddObserver(this);
  OnOperationStarted();

  for (const auto& remote_device : remote_devices_) {
    connection_manager_->RegisterRemoteDevice(
        remote_device.GetDeviceId(),
        MessageTypeToConnectionReason(message_type_for_connection_));

    cryptauth::SecureChannel::Status status;
    if (connection_manager_->GetStatusForDevice(remote_device.GetDeviceId(),
                                                &status) &&
        status == cryptauth::SecureChannel::Status::AUTHENTICATED) {
      StartTimerForDevice(remote_device);
      OnDeviceAuthenticated(remote_device);
    }
  }
}

void MessageTransferOperation::OnSecureChannelStatusChanged(
    const std::string& device_id,
    const cryptauth::SecureChannel::Status& old_status,
    const cryptauth::SecureChannel::Status& new_status,
    BleConnectionManager::StateChangeDetail status_change_detail) {
  base::Optional<cryptauth::RemoteDeviceRef> remote_device =
      GetRemoteDevice(device_id);
  if (!remote_device) {
    // If the device whose status has changed does not correspond to any of the
    // devices passed to this MessageTransferOperation instance, ignore the
    // status change.
    return;
  }

  switch (new_status) {
    case cryptauth::SecureChannel::Status::AUTHENTICATED:
      StartTimerForDevice(*remote_device);
      OnDeviceAuthenticated(*remote_device);
      break;
    case cryptauth::SecureChannel::Status::DISCONNECTED:
      HandleDeviceDisconnection(*remote_device, status_change_detail);
      break;
    default:
      // Note: In success cases, the channel advances from DISCONNECTED to
      // CONNECTING to CONNECTED to AUTHENTICATING to AUTHENTICATED. If the
      // channel fails to advance at any of those stages, it transitions back to
      // DISCONNECTED and starts over. There is no need for special handling for
      // any of these interim states since they will eventually progress to
      // either AUTHENTICATED or DISCONNECTED.
      break;
  }
}

void MessageTransferOperation::OnMessageReceived(const std::string& device_id,
                                                 const std::string& payload) {
  base::Optional<cryptauth::RemoteDeviceRef> remote_device =
      GetRemoteDevice(device_id);
  if (!remote_device) {
    // If the device from which the message has been received does not
    // correspond to any of the devices passed to this MessageTransferOperation
    // instance, ignore the message.
    return;
  }

  std::unique_ptr<MessageWrapper> message_wrapper =
      MessageWrapper::FromRawMessage(payload);
  if (message_wrapper) {
    OnMessageReceived(std::move(message_wrapper), *remote_device);
  }
}

void MessageTransferOperation::UnregisterDevice(
    cryptauth::RemoteDeviceRef remote_device) {
  // Note: This function may be called from the destructor. It is invalid to
  // invoke any virtual methods if |shutting_down_| is true.

  // Make a copy of |remote_device| before continuing, since the code below may
  // cause the original reference to be deleted.
  cryptauth::RemoteDeviceRef remote_device_copy = remote_device;

  remote_device_to_attempts_map_.erase(remote_device_copy);
  remote_devices_.erase(std::remove(remote_devices_.begin(),
                                    remote_devices_.end(), remote_device_copy),
                        remote_devices_.end());
  StopTimerForDeviceIfRunning(remote_device_copy);

  connection_manager_->UnregisterRemoteDevice(
      remote_device_copy.GetDeviceId(),
      MessageTypeToConnectionReason(message_type_for_connection_));

  if (!shutting_down_ && remote_devices_.empty())
    OnOperationFinished();
}

int MessageTransferOperation::SendMessageToDevice(
    cryptauth::RemoteDeviceRef remote_device,
    std::unique_ptr<MessageWrapper> message_wrapper) {
  return connection_manager_->SendMessage(remote_device.GetDeviceId(),
                                          message_wrapper->ToRawMessage());
}

uint32_t MessageTransferOperation::GetTimeoutSeconds() {
  return MessageTransferOperation::kDefaultTimeoutSeconds;
}

void MessageTransferOperation::HandleDeviceDisconnection(
    cryptauth::RemoteDeviceRef remote_device,
    BleConnectionManager::StateChangeDetail status_change_detail) {
  ConnectAttemptCounts& attempts_for_device =
      remote_device_to_attempts_map_[remote_device];

  switch (status_change_detail) {
    case BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE:
      PA_LOG(ERROR) << "State transitioned to DISCONNECTED, but no "
                    << "StateChangeDetail was provided. Treating this as a "
                    << "failure to discover the device.";
      FALLTHROUGH;
    case BleConnectionManager::StateChangeDetail::
        STATE_CHANGE_DETAIL_COULD_NOT_ATTEMPT_CONNECTION:
      ++attempts_for_device.empty_scan_attempts;
      PA_LOG(INFO) << "Connection attempt failed; could not discover the "
                   << "device with ID "
                   << remote_device.GetTruncatedDeviceIdForLogs() << ". "
                   << "Number of failures to establish connection: "
                   << attempts_for_device.empty_scan_attempts;

      if (attempts_for_device.empty_scan_attempts >= kMaxEmptyScansPerDevice) {
        PA_LOG(INFO) << "Reached retry limit for failing to discover the "
                     << "device with ID "
                     << remote_device.GetTruncatedDeviceIdForLogs() << ". "
                     << "Unregistering device.";
        UnregisterDevice(remote_device);
      }
      break;
    case BleConnectionManager::StateChangeDetail::
        STATE_CHANGE_DETAIL_GATT_CONNECTION_WAS_ATTEMPTED:
      ++attempts_for_device.gatt_connection_attempts;
      PA_LOG(INFO) << "Connection attempt failed; GATT connection error for "
                   << "device with ID "
                   << remote_device.GetTruncatedDeviceIdForLogs() << ". "
                   << "Number of GATT error: "
                   << attempts_for_device.gatt_connection_attempts;
      if (attempts_for_device.gatt_connection_attempts >=
          kMaxGattConnectionAttemptsPerDevice) {
        PA_LOG(INFO) << "Reached retry limit for GATT connection errors for "
                     << "device with ID "
                     << remote_device.GetTruncatedDeviceIdForLogs() << ". "
                     << "Unregistering device.";
        UnregisterDevice(remote_device);
      }
      break;
    case BleConnectionManager::StateChangeDetail::
        STATE_CHANGE_DETAIL_INTERRUPTED_BY_HIGHER_PRIORITY:
      // If the connection attempt was interrupted by a higher-priority message,
      // this is not a true failure. There is nothing to do until the next state
      // change occurs.
      break;
    case BleConnectionManager::StateChangeDetail::
        STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED:
      // This state change is expected to be handled as a result of calls to
      // UnregisterDevice(). There is no need for special handling.
      break;
    default:
      NOTREACHED();
      break;
  }
}

void MessageTransferOperation::StartTimerForDevice(
    cryptauth::RemoteDeviceRef remote_device) {
  PA_LOG(INFO) << "Starting timer for operation with message type "
               << message_type_for_connection_ << " from device with ID "
               << remote_device.GetTruncatedDeviceIdForLogs() << ".";

  remote_device_to_timer_map_.emplace(remote_device,
                                      timer_factory_->CreateOneShotTimer());
  remote_device_to_timer_map_[remote_device]->Start(
      FROM_HERE, base::TimeDelta::FromSeconds(GetTimeoutSeconds()),
      base::Bind(&MessageTransferOperation::OnTimeout,
                 weak_ptr_factory_.GetWeakPtr(), remote_device));
}

void MessageTransferOperation::StopTimerForDeviceIfRunning(
    cryptauth::RemoteDeviceRef remote_device) {
  if (!remote_device_to_timer_map_[remote_device])
    return;

  remote_device_to_timer_map_[remote_device]->Stop();
  remote_device_to_timer_map_.erase(remote_device);
}

void MessageTransferOperation::OnTimeout(
    cryptauth::RemoteDeviceRef remote_device) {
  PA_LOG(WARNING) << "Timed out operation for message type "
                  << message_type_for_connection_ << " from device with ID "
                  << remote_device.GetTruncatedDeviceIdForLogs() << ".";

  remote_device_to_timer_map_.erase(remote_device);
  UnregisterDevice(remote_device);
}

base::Optional<cryptauth::RemoteDeviceRef>
MessageTransferOperation::GetRemoteDevice(const std::string& device_id) {
  for (auto& remote_device : remote_devices_) {
    if (remote_device.GetDeviceId() == device_id)
      return remote_device;
  }

  return base::nullopt;
}

void MessageTransferOperation::SetTimerFactoryForTest(
    std::unique_ptr<TimerFactory> timer_factory_for_test) {
  timer_factory_ = std::move(timer_factory_for_test);
}

}  // namespace tether

}  // namespace chromeos
