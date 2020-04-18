// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/ble_connection_manager.h"

#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "chromeos/components/tether/ad_hoc_ble_advertiser.h"
#include "chromeos/components/tether/ble_constants.h"
#include "chromeos/components/tether/timer_factory.h"
#include "components/cryptauth/ble/bluetooth_low_energy_weave_client_connection.h"
#include "components/cryptauth/cryptauth_service.h"
#include "components/cryptauth/remote_device_ref.h"
#include "device/bluetooth/bluetooth_uuid.h"

namespace chromeos {

namespace tether {

namespace {

const char kTetherFeature[] = "magic_tether";

std::string StateChangeDetailToString(
    BleConnectionManager::StateChangeDetail state_change_detail) {
  switch (state_change_detail) {
    case BleConnectionManager::StateChangeDetail::STATE_CHANGE_DETAIL_NONE:
      return "[none]";
    case BleConnectionManager::StateChangeDetail::
        STATE_CHANGE_DETAIL_COULD_NOT_ATTEMPT_CONNECTION:
      return "[could not attempt connection]";
    case BleConnectionManager::StateChangeDetail::
        STATE_CHANGE_DETAIL_GATT_CONNECTION_WAS_ATTEMPTED:
      return "[GATT connection was attempted]";
    case BleConnectionManager::StateChangeDetail::
        STATE_CHANGE_DETAIL_INTERRUPTED_BY_HIGHER_PRIORITY:
      return "[attempt interrupted by higher priority]";
    case BleConnectionManager::StateChangeDetail::
        STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED:
      return "[device was unregistered]";
    default:
      NOTREACHED();
      return std::string();
  }
}

}  // namespace

const int64_t BleConnectionManager::kAdvertisingTimeoutMillis = 12000;
const int64_t BleConnectionManager::kFailImmediatelyTimeoutMillis = 0;

BleConnectionManager::ConnectionMetadata::ConnectionMetadata(
    const std::string& device_id,
    std::unique_ptr<base::Timer> timer,
    base::WeakPtr<BleConnectionManager> manager)
    : device_id_(device_id),
      connection_attempt_timeout_timer_(std::move(timer)),
      manager_(manager),
      weak_ptr_factory_(this) {}

BleConnectionManager::ConnectionMetadata::~ConnectionMetadata() = default;

void BleConnectionManager::ConnectionMetadata::RegisterConnectionReason(
    const ConnectionReason& connection_reason) {
  active_connection_reasons_.insert(connection_reason);
}

void BleConnectionManager::ConnectionMetadata::UnregisterConnectionReason(
    const ConnectionReason& connection_reason) {
  active_connection_reasons_.erase(connection_reason);
}

ConnectionPriority
BleConnectionManager::ConnectionMetadata::GetConnectionPriority() {
  return HighestPriorityForConnectionReasons(active_connection_reasons_);
}

bool BleConnectionManager::ConnectionMetadata::HasReasonForConnection() const {
  return !active_connection_reasons_.empty();
}

bool BleConnectionManager::ConnectionMetadata::HasEstablishedConnection()
    const {
  return secure_channel_.get();
}

cryptauth::SecureChannel::Status
BleConnectionManager::ConnectionMetadata::GetStatus() const {
  if (connection_attempt_timeout_timer_->IsRunning()) {
    // If the timer is running, a connection attempt is in progress but a
    // channel has not been established.
    return cryptauth::SecureChannel::Status::CONNECTING;
  } else if (!HasEstablishedConnection()) {
    // If there is no timer and a channel has not been established, the channel
    // is disconnected.
    return cryptauth::SecureChannel::Status::DISCONNECTED;
  }

  // If a channel has been established, return its status.
  return secure_channel_->status();
}

void BleConnectionManager::ConnectionMetadata::StartConnectionAttemptTimer(
    bool fail_immediately) {
  DCHECK(!secure_channel_);
  DCHECK(!connection_attempt_timeout_timer_->IsRunning());

  int64_t timeout_millis = fail_immediately ? kFailImmediatelyTimeoutMillis
                                            : kAdvertisingTimeoutMillis;

  connection_attempt_timeout_timer_->Start(
      FROM_HERE, base::TimeDelta::FromMilliseconds(timeout_millis),
      base::Bind(&ConnectionMetadata::OnConnectionAttemptTimeout,
                 weak_ptr_factory_.GetWeakPtr()));
}

void BleConnectionManager::ConnectionMetadata::StopConnectionAttemptTimer() {
  DCHECK(!secure_channel_);
  connection_attempt_timeout_timer_->Stop();
}

void BleConnectionManager::ConnectionMetadata::OnConnectionAttemptTimeout() {
  manager_->OnConnectionAttemptTimeout(device_id_);
}

bool BleConnectionManager::ConnectionMetadata::HasSecureChannel() {
  return secure_channel_ != nullptr;
}

void BleConnectionManager::ConnectionMetadata::SetSecureChannel(
    std::unique_ptr<cryptauth::SecureChannel> secure_channel) {
  DCHECK(!secure_channel_);

  // The connection has succeeded, so cancel the timeout.
  connection_attempt_timeout_timer_->Stop();

  secure_channel_ = std::move(secure_channel);
  secure_channel_->AddObserver(this);
  secure_channel_->Initialize();
}

int BleConnectionManager::ConnectionMetadata::SendMessage(
    const std::string& payload) {
  DCHECK(GetStatus() == cryptauth::SecureChannel::Status::AUTHENTICATED);
  return secure_channel_->SendMessage(std::string(kTetherFeature), payload);
}

void BleConnectionManager::ConnectionMetadata::Disconnect() {
  DCHECK(HasSecureChannel());
  secure_channel_->Disconnect();
}

void BleConnectionManager::ConnectionMetadata::OnSecureChannelStatusChanged(
    cryptauth::SecureChannel* secure_channel,
    const cryptauth::SecureChannel::Status& old_status,
    const cryptauth::SecureChannel::Status& new_status) {
  DCHECK(secure_channel_.get() == secure_channel);

  if (new_status == cryptauth::SecureChannel::Status::CONNECTING) {
    // BleConnectionManager already broadcasts "disconnected => connecting"
    // status updates when a connection attempt begins, so there is no need to
    // handle this case.
    return;
  }

  // Make a copy of the two statuses. If |secure_channel_.reset()| is called
  // below, the SecureChannel instance will be destroyed and |old_status| and
  // |new_status| may refer to memory which has been deleted.
  const cryptauth::SecureChannel::Status old_status_copy = old_status;
  const cryptauth::SecureChannel::Status new_status_copy = new_status;

  StateChangeDetail state_change_detail =
      StateChangeDetail::STATE_CHANGE_DETAIL_NONE;

  if (new_status == cryptauth::SecureChannel::Status::DISCONNECTED) {
    secure_channel_->RemoveObserver(this);
    secure_channel_.reset();
    state_change_detail =
        StateChangeDetail::STATE_CHANGE_DETAIL_GATT_CONNECTION_WAS_ATTEMPTED;
  }

  manager_->OnSecureChannelStatusChanged(device_id_, old_status_copy,
                                         new_status_copy, state_change_detail);
}

void BleConnectionManager::ConnectionMetadata::OnMessageReceived(
    cryptauth::SecureChannel* secure_channel,
    const std::string& feature,
    const std::string& payload) {
  DCHECK(secure_channel_.get() == secure_channel);
  if (feature != std::string(kTetherFeature)) {
    // If the message received was not a tether feature, ignore it.
    return;
  }

  manager_->NotifyMessageReceived(device_id_, payload);
}

void BleConnectionManager::ConnectionMetadata::OnMessageSent(
    cryptauth::SecureChannel* secure_channel,
    int sequence_number) {
  DCHECK(secure_channel_.get() == secure_channel);
  PA_LOG(INFO) << "Message sent successfully to device with ID \""
               << cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(
                      device_id_)
               << "\"; message sequence number: " << sequence_number;
  manager_->NotifyMessageSent(sequence_number);
}

void BleConnectionManager::ConnectionMetadata::
    OnGattCharacteristicsNotAvailable() {
  manager_->OnGattCharacteristicsNotAvailable(device_id_);
}

BleConnectionManager::BleConnectionManager(
    cryptauth::CryptAuthService* cryptauth_service,
    scoped_refptr<device::BluetoothAdapter> adapter,
    BleAdvertisementDeviceQueue* ble_advertisement_device_queue,
    BleAdvertiser* ble_advertiser,
    BleScanner* ble_scanner,
    AdHocBleAdvertiser* ad_hoc_ble_advertisement)
    : cryptauth_service_(cryptauth_service),
      adapter_(adapter),
      ble_advertisement_device_queue_(ble_advertisement_device_queue),
      ble_advertiser_(ble_advertiser),
      ble_scanner_(ble_scanner),
      ad_hoc_ble_advertisement_(ad_hoc_ble_advertisement),
      timer_factory_(std::make_unique<TimerFactory>()),
      has_registered_observer_(false),
      weak_ptr_factory_(this) {}

BleConnectionManager::~BleConnectionManager() {
  if (has_registered_observer_) {
    ble_scanner_->RemoveObserver(this);
  }
}

void BleConnectionManager::RegisterRemoteDevice(
    const std::string& device_id,
    const ConnectionReason& connection_reason) {
  if (!has_registered_observer_) {
    ble_scanner_->AddObserver(this);
  }
  has_registered_observer_ = true;

  PA_LOG(INFO) << "Register - Device ID: \""
               << cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(device_id)
               << "\", Reason: " << ConnectionReasonToString(connection_reason);

  ConnectionMetadata* connection_metadata = GetConnectionMetadata(device_id);
  if (!connection_metadata)
    connection_metadata = AddMetadataForDevice(device_id);

  connection_metadata->RegisterConnectionReason(connection_reason);
  UpdateConnectionAttempts();
}

void BleConnectionManager::UnregisterRemoteDevice(
    const std::string& device_id,
    const ConnectionReason& connection_reason) {
  ConnectionMetadata* connection_metadata = GetConnectionMetadata(device_id);
  if (!connection_metadata) {
    PA_LOG(WARNING) << "Tried to unregister device, but was not registered - "
                    << "Device ID: \""
                    << cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(
                           device_id)
                    << "\", Reason: "
                    << ConnectionReasonToString(connection_reason);
    return;
  }

  PA_LOG(INFO) << "Unregister - Device ID: \""
               << cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(device_id)
               << "\", Reason: " << ConnectionReasonToString(connection_reason);

  connection_metadata->UnregisterConnectionReason(connection_reason);
  if (!connection_metadata->HasReasonForConnection()) {
    if (connection_metadata->HasEstablishedConnection()) {
      connection_metadata->Disconnect();
    } else {
      // |device_id| references memory that will be deleted below; make a copy.
      const std::string device_id_copy = device_id;
      cryptauth::SecureChannel::Status status_before_erase =
          connection_metadata->GetStatus();
      device_id_to_metadata_map_.erase(device_id_copy);

      if (status_before_erase == cryptauth::SecureChannel::Status::CONNECTING) {
        StopConnectionAttemptAndMoveToEndOfQueue(device_id_copy);
        NotifySecureChannelStatusChanged(
            device_id_copy, cryptauth::SecureChannel::Status::CONNECTING,
            cryptauth::SecureChannel::Status::DISCONNECTED,
            StateChangeDetail::STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED);
      }
    }
  }

  UpdateConnectionAttempts();
}

int BleConnectionManager::SendMessage(const std::string& device_id,
                                      const std::string& message) {
  ConnectionMetadata* connection_metadata = GetConnectionMetadata(device_id);
  if (!connection_metadata ||
      connection_metadata->GetStatus() !=
          cryptauth::SecureChannel::Status::AUTHENTICATED) {
    PA_LOG(ERROR) << "SendMessage(): Error - no authenticated channel. "
                  << "Device ID: \""
                  << cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(
                         device_id)
                  << "\", Message: \"" << message << "\"";
    return -1;
  }

  PA_LOG(INFO) << "SendMessage(): Device ID: \""
               << cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(device_id)
               << "\", Message: \"" << message << "\"";
  return connection_metadata->SendMessage(message);
}

bool BleConnectionManager::GetStatusForDevice(
    const std::string& device_id,
    cryptauth::SecureChannel::Status* status) const {
  ConnectionMetadata* connection_metadata = GetConnectionMetadata(device_id);
  if (!connection_metadata)
    return false;

  *status = connection_metadata->GetStatus();
  return true;
}

void BleConnectionManager::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void BleConnectionManager::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void BleConnectionManager::AddMetricsObserver(MetricsObserver* observer) {
  metrics_observer_list_.AddObserver(observer);
}

void BleConnectionManager::RemoveMetricsObserver(MetricsObserver* observer) {
  metrics_observer_list_.RemoveObserver(observer);
}

void BleConnectionManager::OnReceivedAdvertisementFromDevice(
    cryptauth::RemoteDeviceRef remote_device,
    device::BluetoothDevice* bluetooth_device,
    bool is_background_advertisement) {
  const std::string device_id = remote_device.GetDeviceId();

  ConnectionMetadata* connection_metadata = GetConnectionMetadata(device_id);
  if (!connection_metadata) {
    // If an advertisement  is received from a device that is not registered,
    // ignore it.
    PA_LOG(WARNING) << "Received an advertisement from a device which is not "
                    << "registered. Bluetooth address: "
                    << bluetooth_device->GetAddress() << ", Remote Device "
                    << "ID: \"" << remote_device.GetTruncatedDeviceIdForLogs()
                    << "\".";
    return;
  }

  if (connection_metadata->HasSecureChannel()) {
    PA_LOG(WARNING) << "Received another advertisement from a registered "
                    << "device which is already being actively communicated "
                    << "with. Bluetooth address: "
                    << bluetooth_device->GetAddress() << ", Remote Device "
                    << "ID: \"" << remote_device.GetTruncatedDeviceIdForLogs()
                    << "\".";
    return;
  }

  PA_LOG(INFO) << "Received advertisement - Device ID: \""
               << remote_device.GetTruncatedDeviceIdForLogs()
               << "\". Starting authentication handshake.";

  device_id_to_is_background_advertisement_map_[device_id] =
      is_background_advertisement;
  NotifyAdvertisementReceived(device_id, is_background_advertisement);

  // Stop trying to connect to that device, since it has been found.
  StopConnectionAttemptAndMoveToEndOfQueue(device_id);

  // Create a connection to that device.
  std::unique_ptr<cryptauth::Connection> connection = cryptauth::weave::
      BluetoothLowEnergyWeaveClientConnection::Factory::NewInstance(
          remote_device, adapter_, device::BluetoothUUID(kGattServerUuid),
          bluetooth_device, false /* should_set_low_connection_latency */);
  std::unique_ptr<cryptauth::SecureChannel> secure_channel =
      cryptauth::SecureChannel::Factory::NewInstance(std::move(connection),
                                                     cryptauth_service_);
  connection_metadata->SetSecureChannel(std::move(secure_channel));

  UpdateConnectionAttempts();
}

BleConnectionManager::ConnectionMetadata*
BleConnectionManager::GetConnectionMetadata(
    const std::string& device_id) const {
  const auto map_iter = device_id_to_metadata_map_.find(device_id);
  if (map_iter == device_id_to_metadata_map_.end())
    return nullptr;

  return map_iter->second.get();
}

BleConnectionManager::ConnectionMetadata*
BleConnectionManager::AddMetadataForDevice(const std::string& device_id) {
  ConnectionMetadata* existing_data = GetConnectionMetadata(device_id);
  if (existing_data)
    return existing_data;

  // Create the metadata.
  std::unique_ptr<ConnectionMetadata> metadata = base::WrapUnique(
      new ConnectionMetadata(device_id, timer_factory_->CreateOneShotTimer(),
                             weak_ptr_factory_.GetWeakPtr()));
  ConnectionMetadata* metadata_raw_ptr = metadata.get();

  // Add it to the map.
  device_id_to_metadata_map_.emplace(
      std::pair<std::string, std::unique_ptr<ConnectionMetadata>>(
          device_id, std::move(metadata)));

  return metadata_raw_ptr;
}

void BleConnectionManager::UpdateConnectionAttempts() {
  UpdateAdvertisementQueue();

  std::vector<std::string> should_advertise_to =
      ble_advertisement_device_queue_->GetDeviceIdsToWhichToAdvertise();
  DCHECK(should_advertise_to.size() <= kMaxConcurrentAdvertisements);

  // Generate a list of devices which are advertising but are not present in
  // |should_advertise_to|.
  std::vector<std::string> device_ids_to_stop;
  for (const auto& map_entry : device_id_to_metadata_map_) {
    if (map_entry.second->GetStatus() ==
            cryptauth::SecureChannel::Status::CONNECTING &&
        !map_entry.second->HasEstablishedConnection() &&
        std::find(should_advertise_to.begin(), should_advertise_to.end(),
                  map_entry.first) == should_advertise_to.end()) {
      device_ids_to_stop.push_back(map_entry.first);
    }
  }

  // For each device that should not be advertised to, end the connection
  // attempt. Note that this is done outside of the map iteration above because
  // it is possible that EndSuccessfulAttempt() will cause that map to be
  // modified during iteration.
  for (const auto& device_id_to_stop : device_ids_to_stop) {
    PA_LOG(INFO) << "Connection attempt for device ID \""
                 << cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(
                        device_id_to_stop)
                 << "\" interrupted by higher-priority connection.";
    EndUnsuccessfulAttempt(
        device_id_to_stop,
        StateChangeDetail::STATE_CHANGE_DETAIL_INTERRUPTED_BY_HIGHER_PRIORITY);
  }

  for (const auto& device_id : should_advertise_to) {
    ConnectionMetadata* associated_data = GetConnectionMetadata(device_id);
    if (associated_data->GetStatus() !=
        cryptauth::SecureChannel::Status::CONNECTING) {
      // If there is no active attempt to connect to a device at the front of
      // the queue, start a connection attempt.
      StartConnectionAttempt(device_id);
    }
  }
}

void BleConnectionManager::UpdateAdvertisementQueue() {
  std::vector<BleAdvertisementDeviceQueue::PrioritizedDeviceId> prioritized_ids;
  for (const auto& map_entry : device_id_to_metadata_map_) {
    if (map_entry.second->HasEstablishedConnection()) {
      // If there is already an active connection to the device, there is no
      // need to advertise to the device to bootstrap a connection.
      continue;
    }

    prioritized_ids.emplace_back(map_entry.first,
                                 map_entry.second->GetConnectionPriority());
  }

  ble_advertisement_device_queue_->SetPrioritizedDeviceIds(prioritized_ids);
}

void BleConnectionManager::StartConnectionAttempt(
    const std::string& device_id) {
  ConnectionMetadata* connection_metadata = GetConnectionMetadata(device_id);
  DCHECK(connection_metadata);

  PA_LOG(INFO) << "Attempting connection - Device ID: \""
               << cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(device_id)
               << "\"";

  bool success = ble_scanner_->RegisterScanFilterForDevice(device_id) &&
                 ble_advertiser_->StartAdvertisingToDevice(device_id);

  // Start a timer; if a connection is unable to be created before the timer
  // fires, a timeout occurs. Note that if this class is unable to start both
  // the scanner and advertiser successfully (i.e., |success| is |false|), a
  // the connection fails immediately insetad of waiting for a timeout, which
  // has the effect of quickly sending out "disconnected => connecting =>
  // disconnecting" status updates. The timer is used here instead of a special
  // case in order to route all connection failures through the same code path.
  connection_metadata->StartConnectionAttemptTimer(
      !success /* fail_immediately */);

  // Send a "disconnected => connecting" update to alert clients that a
  // connection attempt for |device_id| is underway.
  NotifySecureChannelStatusChanged(
      device_id, cryptauth::SecureChannel::Status::DISCONNECTED,
      cryptauth::SecureChannel::Status::CONNECTING,
      StateChangeDetail::STATE_CHANGE_DETAIL_NONE);
}

void BleConnectionManager::EndUnsuccessfulAttempt(
    const std::string& device_id,
    StateChangeDetail state_change_detail) {
  GetConnectionMetadata(device_id)->StopConnectionAttemptTimer();
  StopConnectionAttemptAndMoveToEndOfQueue(device_id);

  // Send a "connecting => disconnected" update to alert clients that a
  // connection attempt for |device_id| has failed.
  NotifySecureChannelStatusChanged(
      device_id, cryptauth::SecureChannel::Status::CONNECTING,
      cryptauth::SecureChannel::Status::DISCONNECTED, state_change_detail);
}

void BleConnectionManager::StopConnectionAttemptAndMoveToEndOfQueue(
    const std::string& device_id) {
  ble_scanner_->UnregisterScanFilterForDevice(device_id);
  ble_advertiser_->StopAdvertisingToDevice(device_id);
  ble_advertisement_device_queue_->MoveDeviceToEnd(device_id);
}

void BleConnectionManager::OnConnectionAttemptTimeout(
    const std::string& device_id) {
  PA_LOG(INFO) << "Connection attempt timeout - Device ID \""
               << cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(device_id)
               << "\".";
  EndUnsuccessfulAttempt(
      device_id,
      StateChangeDetail::STATE_CHANGE_DETAIL_COULD_NOT_ATTEMPT_CONNECTION);
  UpdateConnectionAttempts();
}

void BleConnectionManager::OnSecureChannelStatusChanged(
    const std::string& device_id,
    const cryptauth::SecureChannel::Status& old_status,
    const cryptauth::SecureChannel::Status& new_status,
    StateChangeDetail state_change_detail) {
  ConnectionMetadata* connection_metadata = GetConnectionMetadata(device_id);
  DCHECK(connection_metadata);

  // Create copies of the references passed to this function. If the map entry
  // is erased below, the references will point to deleted memory.
  const std::string device_id_copy = device_id;
  const cryptauth::SecureChannel::Status old_status_copy = old_status;
  const cryptauth::SecureChannel::Status new_status_copy = new_status;

  if (!connection_metadata->HasReasonForConnection() &&
      new_status == cryptauth::SecureChannel::Status::DISCONNECTED) {
    device_id_to_metadata_map_.erase(device_id_copy);
    state_change_detail =
        StateChangeDetail::STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED;
  }

  NotifySecureChannelStatusChanged(device_id_copy, old_status_copy,
                                   new_status_copy, state_change_detail);
  UpdateConnectionAttempts();
}

void BleConnectionManager::OnGattCharacteristicsNotAvailable(
    const std::string& device_id) {
  PA_LOG(WARNING) << "Previous connection attempt failed due to unavailable "
                  << "GATT services for device ID \""
                  << cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(
                         device_id)
                  << "\".";
  ad_hoc_ble_advertisement_->RequestGattServicesForDevice(device_id);
}

void BleConnectionManager::NotifyAdvertisementReceived(
    const std::string& device_id,
    bool is_background_advertisement) {
  for (auto& observer : metrics_observer_list_)
    observer.OnAdvertisementReceived(device_id, is_background_advertisement);
}

void BleConnectionManager::NotifyMessageReceived(std::string device_id,
                                                 std::string payload) {
  PA_LOG(INFO) << "Message received - Device ID: \""
               << cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(device_id)
               << "\", Message: \"" << payload << "\".";
  for (auto& observer : observer_list_)
    observer.OnMessageReceived(device_id, payload);
}

void BleConnectionManager::NotifySecureChannelStatusChanged(
    std::string device_id,
    cryptauth::SecureChannel::Status old_status,
    cryptauth::SecureChannel::Status new_status,
    StateChangeDetail state_change_detail) {
  PA_LOG(INFO) << "Status change - Device ID: \""
               << cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(device_id)
               << "\": " << cryptauth::SecureChannel::StatusToString(old_status)
               << " => " << cryptauth::SecureChannel::StatusToString(new_status)
               << ", State change detail: "
               << StateChangeDetailToString(state_change_detail);

  for (auto& observer : metrics_observer_list_) {
    if (old_status == cryptauth::SecureChannel::Status::DISCONNECTED &&
        new_status == cryptauth::SecureChannel::Status::CONNECTING) {
      observer.OnConnectionAttemptStarted(device_id);
    } else if (new_status == cryptauth::SecureChannel::Status::CONNECTED) {
      observer.OnConnection(
          device_id, device_id_to_is_background_advertisement_map_[device_id]);
    } else if (new_status == cryptauth::SecureChannel::Status::AUTHENTICATED) {
      observer.OnSecureChannelCreated(
          device_id, device_id_to_is_background_advertisement_map_[device_id]);
    } else if (new_status == cryptauth::SecureChannel::Status::DISCONNECTED) {
      observer.OnDeviceDisconnected(
          device_id, state_change_detail,
          device_id_to_is_background_advertisement_map_[device_id]);
    }
  }

  for (auto& observer : observer_list_) {
    observer.OnSecureChannelStatusChanged(device_id, old_status, new_status,
                                          state_change_detail);
  }
}

void BleConnectionManager::NotifyMessageSent(int sequence_number) {
  for (auto& observer : observer_list_)
    observer.OnMessageSent(sequence_number);
}

void BleConnectionManager::SetTestTimerFactoryForTesting(
    std::unique_ptr<TimerFactory> test_timer_factory) {
  timer_factory_ = std::move(test_timer_factory);
}

}  // namespace tether

}  // namespace chromeos
