// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_BLE_CONNECTION_MANAGER_H_
#define CHROMEOS_COMPONENTS_TETHER_BLE_CONNECTION_MANAGER_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "chromeos/components/tether/ble_advertisement_device_queue.h"
#include "chromeos/components/tether/ble_advertiser.h"
#include "chromeos/components/tether/ble_scanner.h"
#include "chromeos/components/tether/connection_priority.h"
#include "chromeos/components/tether/connection_reason.h"
#include "chromeos/components/tether/proto/tether.pb.h"
#include "components/cryptauth/secure_channel.h"

namespace cryptauth {
class CryptAuthService;
}  // namespace cryptauth

namespace device {
class BluetoothAdapter;
class BluetoothDevice;
}  // namespace device

namespace chromeos {

namespace tether {

class AdHocBleAdvertiser;
class TimerFactory;

// Manages connections to remote devices. When a device is registered,
// BleConnectionManager intiates a connection to that device. If the connection
// succeeds and authenticates successfully, messages can be sent/received
// to/from the device. If the connection does not succeed, BleConnectionManager
// continues attempting connections until a connection is established or the
// device is unregistered.
//
// To use this class, construct an instance and observe it via AddObserver().
// Then, register device(s) to connect to via RegisterRemoteDevice() and wait
// for the OnSecureChannelStatusChanged() callback to be invoked. If the
// status for a device changes from |DISCONNECTED| to |CONNECTING| then back to
// |DISCONNECTED|, a connection attempt has failed. Clients should set a retry
// limit and unregister a device via |UnregisterRemoteDevice()| if multiple
// connection attempts have failed. If, instead, a connection succeeds the
// status changes to |AUTHENTICATED|, the device can safely send and receive
// messages. To send a message, call SendMessage(), and to listen for received
// messages, implement the OnMessageReceived() callback.
//
// Note that a single device can be registered for multiple connection reasons.
// If a device is registered for more than one reason, its connections (and
// connection attempts) will remain active until all connection reasons have
// been unregistered for the device.
class BleConnectionManager : public BleScanner::Observer {
 public:
  // Extra data about a state change which is passed to observers in
  // OnSecureChannelStatusChanged(). If no extra data applies to the state
  // change, STATE_CHANGE_DETAIL_NONE is used.
  enum class StateChangeDetail {
    STATE_CHANGE_DETAIL_NONE,
    STATE_CHANGE_DETAIL_COULD_NOT_ATTEMPT_CONNECTION,
    STATE_CHANGE_DETAIL_GATT_CONNECTION_WAS_ATTEMPTED,
    STATE_CHANGE_DETAIL_INTERRUPTED_BY_HIGHER_PRIORITY,
    STATE_CHANGE_DETAIL_DEVICE_WAS_UNREGISTERED
  };

  class Observer {
   public:
    virtual void OnSecureChannelStatusChanged(
        const std::string& device_id,
        const cryptauth::SecureChannel::Status& old_status,
        const cryptauth::SecureChannel::Status& new_status,
        StateChangeDetail state_change_detail) = 0;

    virtual void OnMessageReceived(const std::string& device_id,
                                   const std::string& payload) = 0;

    // Called when a message has been sent successfully; |sequence_number|
    // corresponds to the value returned by an earlier call to SendMessage().
    virtual void OnMessageSent(int sequence_number) = 0;
  };

  // Observes events that are relevant to metrics reporting.
  class MetricsObserver {
   public:
    virtual void OnConnectionAttemptStarted(const std::string& device_id) {}
    virtual void OnAdvertisementReceived(const std::string& device_id,
                                         bool is_background_advertisement) {}
    virtual void OnConnection(const std::string& device_id,
                              bool is_background_advertisement) {}
    virtual void OnSecureChannelCreated(const std::string& device_id,
                                        bool is_background_advertisement) {}
    virtual void OnDeviceDisconnected(const std::string& device_id,
                                      StateChangeDetail state_change_detail,
                                      bool is_background_advertisement) {}
  };

  BleConnectionManager(
      cryptauth::CryptAuthService* cryptauth_service,
      scoped_refptr<device::BluetoothAdapter> adapter,
      BleAdvertisementDeviceQueue* ble_advertisement_device_queue,
      BleAdvertiser* ble_advertiser,
      BleScanner* ble_scanner,
      AdHocBleAdvertiser* ad_hoc_ble_advertisement);
  virtual ~BleConnectionManager();

  // Registers |device_id| for |connection_reason|. Once registered, this
  // instance will continue to attempt to connect and authenticate to that
  // device until the device is unregistered.
  virtual void RegisterRemoteDevice(const std::string& device_id,
                                    const ConnectionReason& connection_reason);

  // Unregisters |device_id| for |connection_reason|. Once registered, a device
  // will continue trying to connect until *ALL* of its ConnectionReasons have
  // been unregistered.
  virtual void UnregisterRemoteDevice(
      const std::string& device_id,
      const ConnectionReason& connection_reason);

  // Sends |message| to the device with ID |device_id|. This function can only
  // be called if the given device is authenticated. This function returns a
  // sequence number for the message; if this message is sent successfully,
  // observers will be notified and provided this number. Note that -1 is
  // returned when the message cannot be sent.
  virtual int SendMessage(const std::string& device_id,
                          const std::string& message);

  // Gets the device with ID |device_id|'s status and stores it to |status|,
  // returning whether that device is registered. If this function returns
  // |false|, no value is saved to |status|.
  virtual bool GetStatusForDevice(
      const std::string& device_id,
      cryptauth::SecureChannel::Status* status) const;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void AddMetricsObserver(MetricsObserver* observer);
  void RemoveMetricsObserver(MetricsObserver* observer);

  // BleScanner::Observer:
  void OnReceivedAdvertisementFromDevice(
      cryptauth::RemoteDeviceRef remote_device,
      device::BluetoothDevice* bluetooth_device,
      bool is_background_advertisement) override;

 protected:
  void NotifyAdvertisementReceived(const std::string& device_id,
                                   bool is_background_advertisement);
  void NotifyMessageReceived(std::string device_id, std::string payload);
  void NotifySecureChannelStatusChanged(
      std::string device_id,
      cryptauth::SecureChannel::Status old_status,
      cryptauth::SecureChannel::Status new_status,
      StateChangeDetail state_change_detail);
  void NotifyMessageSent(int sequence_number);

 private:
  friend class BleConnectionManagerTest;

  static const int64_t kAdvertisingTimeoutMillis;
  static const int64_t kFailImmediatelyTimeoutMillis;

  // Data associated with a registered device. Each registered device has an
  // associated |ConnectionMetadata| stored in |device_id_to_metadata_map_|, and
  // the |ConnectionMetadata| is removed when the device is unregistered. A
  // |ConnectionMetadata| stores the associated |SecureChannel| for registered
  // devices which have an active connection.
  class ConnectionMetadata final : public cryptauth::SecureChannel::Observer {
   public:
    ConnectionMetadata(const std::string& device_id,
                       std::unique_ptr<base::Timer> timer,
                       base::WeakPtr<BleConnectionManager> manager);
    ~ConnectionMetadata();

    void RegisterConnectionReason(const ConnectionReason& connection_reason);
    void UnregisterConnectionReason(const ConnectionReason& connection_reason);
    ConnectionPriority GetConnectionPriority();
    bool HasReasonForConnection() const;

    bool HasEstablishedConnection() const;
    cryptauth::SecureChannel::Status GetStatus() const;

    void StartConnectionAttemptTimer(bool use_short_error_timeout);
    void StopConnectionAttemptTimer();
    bool HasSecureChannel();
    void SetSecureChannel(
        std::unique_ptr<cryptauth::SecureChannel> secure_channel);
    int SendMessage(const std::string& payload);
    void Disconnect();

    // cryptauth::SecureChannel::Observer:
    void OnSecureChannelStatusChanged(
        cryptauth::SecureChannel* secure_channel,
        const cryptauth::SecureChannel::Status& old_status,
        const cryptauth::SecureChannel::Status& new_status) override;
    void OnMessageReceived(cryptauth::SecureChannel* secure_channel,
                           const std::string& feature,
                           const std::string& payload) override;
    void OnMessageSent(cryptauth::SecureChannel* secure_channel,
                       int sequence_number) override;
    void OnGattCharacteristicsNotAvailable() override;

   private:
    friend class BleConnectionManagerTest;

    void OnConnectionAttemptTimeout();

    std::string device_id_;
    std::set<ConnectionReason> active_connection_reasons_;
    std::unique_ptr<cryptauth::SecureChannel> secure_channel_;
    std::unique_ptr<base::Timer> connection_attempt_timeout_timer_;
    base::WeakPtr<BleConnectionManager> manager_;

    base::WeakPtrFactory<ConnectionMetadata> weak_ptr_factory_;
  };

  ConnectionMetadata* GetConnectionMetadata(const std::string& device_id) const;
  ConnectionMetadata* AddMetadataForDevice(const std::string& device_id);

  void UpdateConnectionAttempts();
  void UpdateAdvertisementQueue();

  void StartConnectionAttempt(const std::string& device_id);
  void EndUnsuccessfulAttempt(const std::string& device_id,
                              StateChangeDetail state_change_detail);
  void StopConnectionAttemptAndMoveToEndOfQueue(const std::string& device_id);

  void OnConnectionAttemptTimeout(const std::string& device_id);
  void OnSecureChannelStatusChanged(
      const std::string& device_id,
      const cryptauth::SecureChannel::Status& old_status,
      const cryptauth::SecureChannel::Status& new_status,
      StateChangeDetail state_change_detail);
  void OnGattCharacteristicsNotAvailable(const std::string& device_id);

  void SetTestTimerFactoryForTesting(
      std::unique_ptr<TimerFactory> test_timer_factory);

  // Record various operation durations. These need to be separate methods
  // because internally they use a macro which maintains a static state that
  // does not tolerate different histogram names being passed to it.
  void RecordStartScanToConnectionDuration(const std::string device_id);
  void RecordConnectionToAuthenticationDuration(const std::string device_id);

  cryptauth::CryptAuthService* cryptauth_service_;
  scoped_refptr<device::BluetoothAdapter> adapter_;
  BleAdvertisementDeviceQueue* ble_advertisement_device_queue_;
  BleAdvertiser* ble_advertiser_;
  BleScanner* ble_scanner_;
  AdHocBleAdvertiser* ad_hoc_ble_advertisement_;

  std::unique_ptr<TimerFactory> timer_factory_;

  bool has_registered_observer_;
  std::map<std::string, std::unique_ptr<ConnectionMetadata>>
      device_id_to_metadata_map_;

  // Records if the advertisement from a device was a background advertisement.
  std::map<std::string, bool> device_id_to_is_background_advertisement_map_;

  base::ObserverList<Observer> observer_list_;
  base::ObserverList<MetricsObserver> metrics_observer_list_;
  base::WeakPtrFactory<BleConnectionManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BleConnectionManager);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_BLE_CONNECTION_MANAGER_H_
