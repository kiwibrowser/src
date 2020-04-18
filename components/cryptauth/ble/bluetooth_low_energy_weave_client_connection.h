// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef COMPONENTS_CRYPTAUTH_BLE_BLUETOOTH_LOW_ENERGY_WEAVE_CLIENT_CONNECTION_H_
#define COMPONENTS_CRYPTAUTH_BLE_BLUETOOTH_LOW_ENERGY_WEAVE_CLIENT_CONNECTION_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <string>

#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "components/cryptauth/ble/bluetooth_low_energy_characteristics_finder.h"
#include "components/cryptauth/ble/bluetooth_low_energy_weave_packet_generator.h"
#include "components/cryptauth/ble/bluetooth_low_energy_weave_packet_receiver.h"
#include "components/cryptauth/ble/fake_wire_message.h"
#include "components/cryptauth/ble/remote_attribute.h"
#include "components/cryptauth/connection.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_gatt_notify_session.h"
#include "device/bluetooth/bluetooth_remote_gatt_characteristic.h"
#include "device/bluetooth/bluetooth_uuid.h"

namespace base {
class TaskRunner;
}

namespace cryptauth {

namespace weave {

// Creates GATT connection on top of the BLE connection and act as a Client.
// uWeave communication follows the flow:
//
// Client                           | Server
// ---------------------------------|--------------------------------
// send connection request          |
//                                  | receive connection request
//                                  | send connection response
// receive connection response      |
// opt: send data                   | opt: send data
// receive data                     | receive data
// opt: close connection            | opt: close connection
class BluetoothLowEnergyWeaveClientConnection
    : public Connection,
      public device::BluetoothAdapter::Observer {
 public:
  class Factory {
   public:
    static std::unique_ptr<Connection> NewInstance(
        RemoteDeviceRef remote_device,
        scoped_refptr<device::BluetoothAdapter> adapter,
        const device::BluetoothUUID remote_service_uuid,
        device::BluetoothDevice* bluetooth_device,
        bool should_set_low_connection_latency);
    static void SetInstanceForTesting(Factory* factory);

   protected:
    virtual std::unique_ptr<Connection> BuildInstance(
        RemoteDeviceRef remote_device,
        scoped_refptr<device::BluetoothAdapter> adapter,
        const device::BluetoothUUID remote_service_uuid,
        device::BluetoothDevice* bluetooth_device,
        bool should_set_low_connection_latency);

   private:
    static Factory* factory_instance_;
  };

  enum SubStatus {
    DISCONNECTED,
    WAITING_CONNECTION_LATENCY,
    WAITING_GATT_CONNECTION,
    WAITING_CHARACTERISTICS,
    CHARACTERISTICS_FOUND,
    WAITING_NOTIFY_SESSION,
    NOTIFY_SESSION_READY,
    WAITING_CONNECTION_RESPONSE,
    CONNECTED_AND_IDLE,
    CONNECTED_AND_SENDING_MESSAGE,
  };

  // Constructs the Connection object; a subsequent call to Connect() is
  // necessary to initiate the BLE connection.
  BluetoothLowEnergyWeaveClientConnection(
      RemoteDeviceRef remote_device,
      scoped_refptr<device::BluetoothAdapter> adapter,
      const device::BluetoothUUID remote_service_uuid,
      device::BluetoothDevice* bluetooth_device,
      bool should_set_low_connection_latency);

  ~BluetoothLowEnergyWeaveClientConnection() override;

  // Connection:
  void Connect() override;
  void Disconnect() override;
  std::string GetDeviceAddress() override;

 protected:
  enum BleWeaveConnectionResult {
    BLE_WEAVE_CONNECTION_RESULT_CLOSED_NORMALLY = 0,
    BLE_WEAVE_CONNECTION_RESULT_TIMEOUT_SETTING_CONNECTION_LATENCY = 1,
    BLE_WEAVE_CONNECTION_RESULT_TIMEOUT_CREATING_GATT_CONNECTION = 2,
    BLE_WEAVE_CONNECTION_RESULT_TIMEOUT_STARTING_NOTIFY_SESSION = 3,
    BLE_WEAVE_CONNECTION_RESULT_TIMEOUT_FINDING_GATT_CHARACTERISTICS = 4,
    BLE_WEAVE_CONNECTION_RESULT_TIMEOUT_WAITING_FOR_CONNECTION_RESPONSE = 5,
    BLE_WEAVE_CONNECTION_RESULT_ERROR_BLUETOOTH_DEVICE_NOT_AVAILABLE = 6,
    BLE_WEAVE_CONNECTION_RESULT_ERROR_CREATING_GATT_CONNECTION = 7,
    BLE_WEAVE_CONNECTION_RESULT_ERROR_STARTING_NOTIFY_SESSION = 8,
    BLE_WEAVE_CONNECTION_RESULT_ERROR_FINDING_GATT_CHARACTERISTICS = 9,
    BLE_WEAVE_CONNECTION_RESULT_ERROR_WRITING_GATT_CHARACTERISTIC = 10,
    BLE_WEAVE_CONNECTION_RESULT_ERROR_GATT_CHARACTERISTIC_NOT_AVAILABLE = 11,
    BLE_WEAVE_CONNECTION_RESULT_ERROR_WRITE_QUEUE_OUT_OF_SYNC = 12,
    BLE_WEAVE_CONNECTION_RESULT_ERROR_DEVICE_LOST = 13,
    BLE_WEAVE_CONNECTION_RESULT_ERROR_CONNECTION_DROPPED = 14,
    BLE_WEAVE_CONNECTION_RESULT_TIMEOUT_WAITING_FOR_MESSAGE_TO_SEND = 15,
    BLE_WEAVE_CONNECTION_RESULT_MAX
  };

  // Destroys the connection immediately; if there was an active connection, it
  // will be disconnected after this call. Note that this function may notify
  // observers of a connection status change.
  void DestroyConnection(BleWeaveConnectionResult result);

  SubStatus sub_status() { return sub_status_; }

  void SetupTestDoubles(
      scoped_refptr<base::TaskRunner> test_task_runner,
      std::unique_ptr<base::Timer> test_timer,
      std::unique_ptr<BluetoothLowEnergyWeavePacketGenerator> test_generator,
      std::unique_ptr<BluetoothLowEnergyWeavePacketReceiver> test_receiver);

  virtual BluetoothLowEnergyCharacteristicsFinder* CreateCharacteristicsFinder(
      const BluetoothLowEnergyCharacteristicsFinder::SuccessCallback&
          success_callback,
      const BluetoothLowEnergyCharacteristicsFinder::ErrorCallback&
          error_callback);

  // Connection:
  void SendMessageImpl(std::unique_ptr<WireMessage> message) override;
  void OnDidSendMessage(const WireMessage& message, bool success) override;

  // device::BluetoothAdapter::Observer:
  void DeviceChanged(device::BluetoothAdapter* adapter,
                     device::BluetoothDevice* device) override;
  void DeviceRemoved(device::BluetoothAdapter* adapter,
                     device::BluetoothDevice* device) override;
  void GattCharacteristicValueChanged(
      device::BluetoothAdapter* adapter,
      device::BluetoothRemoteGattCharacteristic* characteristic,
      const Packet& value) override;

  bool should_set_low_connection_latency() {
    return should_set_low_connection_latency_;
  }

 private:
  friend class CryptAuthBluetoothLowEnergyWeaveClientConnectionTest;
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           CreateAndDestroyWithoutConnectCallDoesntCrash);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           DisconnectWithoutConnectDoesntCrash);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           ConnectSuccess);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           ConnectSuccessDisconnect);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           DisconnectCalledTwice);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           ConnectSuccessDisconnect_DoNotSetLowLatency);
  FRIEND_TEST_ALL_PREFIXES(
      CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
      ConnectIncompleteDisconnectFromWaitingCharacteristicsState);
  FRIEND_TEST_ALL_PREFIXES(
      CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
      ConnectIncompleteDisconnectFromWaitingNotifySessionState);
  FRIEND_TEST_ALL_PREFIXES(
      CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
      ConnectIncompleteDisconnectFromWaitingConnectionResponseState);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           ConnectFailsCharacteristicsNotFound);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           ConnectFailsCharacteristicsFoundThenUnavailable);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           ConnectFailsNotifySessionError);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           ConnectFailsErrorSendingConnectionRequest);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           ReceiveMessageSmallerThanCharacteristicSize);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           ReceiveMessageLargerThanCharacteristicSize);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           SendMessageSmallerThanCharacteristicSize);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           SendMessageLargerThanCharacteristicSize);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           SendMessageKeepsFailing);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           ReceiveCloseConnectionTest);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           ReceiverErrorTest);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           ReceiverErrorWithPendingWritesTest);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           ObserverDeletesConnectionOnDisconnect);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           ObserverDeletesConnectionOnMessageSent);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           WriteConnectionCloseMaxNumberOfTimes);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           ConnectAfterADelayWhenThrottled);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           SetConnectionLatencyError);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           Timeout_ConnectionLatency);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           Timeout_GattConnection);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           Timeout_GattCharacteristics);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           Timeout_NotifySession);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           Timeout_ConnectionResponse);
  FRIEND_TEST_ALL_PREFIXES(CryptAuthBluetoothLowEnergyWeaveClientConnectionTest,
                           Timeout_SendingMessage);
  enum WriteRequestType {
    REGULAR,
    MESSAGE_COMPLETE,
    CONNECTION_REQUEST,
    CONNECTION_CLOSE
  };

  // GATT_CONNECTION_RESULT_UNKNOWN indicates that the Bluetooth platform
  // returned a code that is not recognized.
  enum GattConnectionResult {
    GATT_CONNECTION_RESULT_SUCCESS = 0,
    GATT_CONNECTION_RESULT_ERROR_AUTH_CANCELED = 1,
    GATT_CONNECTION_RESULT_ERROR_AUTH_FAILED = 2,
    GATT_CONNECTION_RESULT_ERROR_AUTH_REJECTED = 3,
    GATT_CONNECTION_RESULT_ERROR_AUTH_TIMEOUT = 4,
    GATT_CONNECTION_RESULT_ERROR_FAILED = 5,
    GATT_CONNECTION_RESULT_ERROR_INPROGRESS = 6,
    GATT_CONNECTION_RESULT_ERROR_UNKNOWN = 7,
    GATT_CONNECTION_RESULT_ERROR_UNSUPPORTED_DEVICE = 8,
    GATT_CONNECTION_RESULT_UNKNOWN = 9,
    GATT_CONNECTION_RESULT_MAX
  };

  // GATT_SERVICE_OPERATION_RESULT_UNKNOWN indicates that the Bluetooth
  // platform returned a code that is not recognized.
  enum GattServiceOperationResult {
    GATT_SERVICE_OPERATION_RESULT_SUCCESS = 0,
    GATT_SERVICE_OPERATION_RESULT_GATT_ERROR_UNKNOWN = 1,
    GATT_SERVICE_OPERATION_RESULT_GATT_ERROR_FAILED = 2,
    GATT_SERVICE_OPERATION_RESULT_GATT_ERROR_IN_PROGRESS = 3,
    GATT_SERVICE_OPERATION_RESULT_GATT_ERROR_INVALID_LENGTH = 4,
    GATT_SERVICE_OPERATION_RESULT_GATT_ERROR_NOT_PERMITTED = 5,
    GATT_SERVICE_OPERATION_RESULT_GATT_ERROR_NOT_AUTHORIZED = 6,
    GATT_SERVICE_OPERATION_RESULT_GATT_ERROR_NOT_PAIRED = 7,
    GATT_SERVICE_OPERATION_RESULT_GATT_ERROR_NOT_SUPPORTED = 8,
    GATT_SERVICE_OPERATION_RESULT_UNKNOWN = 9,
    GATT_SERVICE_OPERATION_RESULT_MAX
  };

  // Represents a request to write |value| to a some characteristic.
  // |is_last_write_for_wire_messsage| indicates whether this request
  // corresponds to the last write request for some wire message.
  struct WriteRequest {
    WriteRequest(const Packet& val,
                 WriteRequestType request_type,
                 WireMessage* associated_wire_message);
    WriteRequest(const Packet& val, WriteRequestType request_type);
    WriteRequest(const WireMessage& other);
    virtual ~WriteRequest();

    Packet value;
    WriteRequestType request_type;
    WireMessage* associated_wire_message;
    int number_of_failed_attempts = 0;
  };

  static std::string SubStatusToString(SubStatus sub_status);

  // Returns the timeout for the given SubStatus. If no timeout is needed for
  // |sub_status|, base::TimeDelta::Max() is returned.
  static base::TimeDelta GetTimeoutForSubStatus(SubStatus sub_status);

  // Sets the current status; if |status| corresponds to one of Connection's
  // Status types, observers will be notified of the change.
  void SetSubStatus(SubStatus status);
  void OnTimeoutForSubStatus(SubStatus status);

  // These functions are used to set up the connection so that it is ready to
  // send/receive data.
  void SetConnectionLatency();
  void CreateGattConnection();
  void OnGattConnectionCreated(
      std::unique_ptr<device::BluetoothGattConnection> gatt_connection);
  void OnSetConnectionLatencyError();
  void OnCreateGattConnectionError(
      device::BluetoothDevice::ConnectErrorCode error_code);
  void OnCharacteristicsFound(const RemoteAttribute& service,
                              const RemoteAttribute& tx_characteristic,
                              const RemoteAttribute& rx_characteristic);
  void OnCharacteristicsFinderError(const RemoteAttribute& tx_characteristic,
                                    const RemoteAttribute& rx_characteristic);
  void StartNotifySession();
  void OnNotifySessionStarted(
      std::unique_ptr<device::BluetoothGattNotifySession> notify_session);
  void OnNotifySessionError(device::BluetoothGattService::GattErrorCode);

  // Sends the connection request message (the first message in the uWeave
  // handshake).
  void SendConnectionRequest();

  // Completes and updates the status accordingly.
  void CompleteConnection();

  // If no write is in progress and there are queued packets, sends the next
  // packet; if there is already a write in progress or there are no queued
  // packets, this function is a no-op.
  void ProcessNextWriteRequest();

  void SendPendingWriteRequest();
  void OnRemoteCharacteristicWritten();
  void OnWriteRemoteCharacteristicError(
      device::BluetoothRemoteGattService::GattErrorCode error);
  void ClearQueueAndSendConnectionClose();

  void RecordBleWeaveConnectionResult(BleWeaveConnectionResult result);
  void RecordGattConnectionResult(GattConnectionResult result);
  GattConnectionResult BluetoothDeviceConnectErrorCodeToGattConnectionResult(
      device::BluetoothDevice::ConnectErrorCode error_code);
  void RecordGattNotifySessionResult(GattServiceOperationResult result);
  void RecordGattWriteCharacteristicResult(GattServiceOperationResult result);
  GattServiceOperationResult
  BluetoothRemoteDeviceGattServiceGattErrorCodeToGattServiceOperationResult(
      device::BluetoothRemoteGattService::GattErrorCode error_code);

  // Private getters for the Bluetooth classes corresponding to this connection.
  device::BluetoothRemoteGattService* GetRemoteService();
  device::BluetoothRemoteGattCharacteristic* GetGattCharacteristic(
      const std::string& identifier);
  device::BluetoothDevice* GetBluetoothDevice();

  // Get the reason that the other side of the connection decided to close the
  // connection.
  std::string GetReasonForClose();

  // The device to which to connect.
  device::BluetoothDevice* bluetooth_device_;

  bool should_set_low_connection_latency_;

  bool has_triggered_disconnection_ = false;

  // Tracks if the result of this connection has been recorded (using
  // BleWeaveConnectionResult). The result of a connection should only be
  // recorded once.
  bool has_recorded_connection_result_ = false;

  scoped_refptr<device::BluetoothAdapter> adapter_;
  RemoteAttribute remote_service_;
  std::unique_ptr<BluetoothLowEnergyWeavePacketGenerator> packet_generator_;
  std::unique_ptr<BluetoothLowEnergyWeavePacketReceiver> packet_receiver_;
  RemoteAttribute tx_characteristic_;
  RemoteAttribute rx_characteristic_;
  scoped_refptr<base::TaskRunner> task_runner_;
  std::unique_ptr<base::Timer> timer_;

  // These pointers start out null and are created during the connection
  // process.
  std::unique_ptr<device::BluetoothGattConnection> gatt_connection_;
  std::unique_ptr<BluetoothLowEnergyCharacteristicsFinder>
      characteristic_finder_;
  std::unique_ptr<device::BluetoothGattNotifySession> notify_session_;

  SubStatus sub_status_;

  // The WriteRequest that is currently being sent as well as those queued to be
  // sent. Each WriteRequest corresponds to one uWeave packet to be sent.
  std::unique_ptr<WriteRequest> pending_write_request_;
  base::queue<std::unique_ptr<WriteRequest>> queued_write_requests_;

  // WireMessages queued to be sent. Each WireMessage correponds to one or more
  // WriteRequests. WireMessages remain in this queue until the last
  // corresponding WriteRequest has been sent.
  base::queue<std::unique_ptr<WireMessage>> queued_wire_messages_;

  base::WeakPtrFactory<BluetoothLowEnergyWeaveClientConnection>
      weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothLowEnergyWeaveClientConnection);
};

}  // namespace weave

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_BLE_BLUETOOTH_LOW_ENERGY_WEAVE_CLIENT_CONNECTION_H_
