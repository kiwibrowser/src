// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_CONNECT_TETHERING_OPERATION_H_
#define CHROMEOS_COMPONENTS_TETHER_CONNECT_TETHERING_OPERATION_H_

#include <map>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "base/time/clock.h"
#include "chromeos/components/tether/ble_connection_manager.h"
#include "chromeos/components/tether/message_transfer_operation.h"
#include "components/cryptauth/remote_device_ref.h"

namespace chromeos {

namespace tether {

class MessageWrapper;
class TetherHostResponseRecorder;

// Operation used to request that a tether host share its Internet connection.
// Attempts a connection to the RemoteDevice passed to its constructor and
// notifies observers when the RemoteDevice sends a response.
class ConnectTetheringOperation : public MessageTransferOperation {
 public:
  // Includes all error codes of ConnectTetheringResponse_ResponseCode, but
  // includes extra values, |NO_RESPONSE| and |INVALID_HOTSPOT_CREDENTIALS|.
  enum HostResponseErrorCode {
    PROVISIONING_FAILED = 0,
    TETHERING_TIMEOUT = 1,
    TETHERING_UNSUPPORTED = 2,
    NO_CELL_DATA = 3,
    ENABLING_HOTSPOT_FAILED = 4,
    ENABLING_HOTSPOT_TIMEOUT = 5,
    UNKNOWN_ERROR = 6,
    NO_RESPONSE = 7,
    INVALID_HOTSPOT_CREDENTIALS = 8
  };

  class Factory {
   public:
    static std::unique_ptr<ConnectTetheringOperation> NewInstance(
        cryptauth::RemoteDeviceRef device_to_connect,
        BleConnectionManager* connection_manager,
        TetherHostResponseRecorder* tether_host_response_recorder,
        bool setup_required);

    static void SetInstanceForTesting(Factory* factory);

   protected:
    virtual std::unique_ptr<ConnectTetheringOperation> BuildInstance(
        cryptauth::RemoteDeviceRef devices_to_connect,
        BleConnectionManager* connection_manager,
        TetherHostResponseRecorder* tether_host_response_recorder,
        bool setup_required);

   private:
    static Factory* factory_instance_;
  };

  class Observer {
   public:
    virtual void OnConnectTetheringRequestSent(
        cryptauth::RemoteDeviceRef remote_device) = 0;
    virtual void OnSuccessfulConnectTetheringResponse(
        cryptauth::RemoteDeviceRef remote_device,
        const std::string& ssid,
        const std::string& password) = 0;
    virtual void OnConnectTetheringFailure(
        cryptauth::RemoteDeviceRef remote_device,
        HostResponseErrorCode error_code) = 0;
  };

  ~ConnectTetheringOperation() override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  ConnectTetheringOperation(
      cryptauth::RemoteDeviceRef device_to_connect,
      BleConnectionManager* connection_manager,
      TetherHostResponseRecorder* tether_host_response_recorder,
      bool setup_required);

  // MessageTransferOperation:
  void OnDeviceAuthenticated(cryptauth::RemoteDeviceRef remote_device) override;
  void OnMessageReceived(std::unique_ptr<MessageWrapper> message_wrapper,
                         cryptauth::RemoteDeviceRef remote_device) override;
  void OnOperationFinished() override;
  MessageType GetMessageTypeForConnection() override;
  void OnMessageSent(int sequence_number) override;
  uint32_t GetTimeoutSeconds() override;

  void NotifyConnectTetheringRequestSent();
  void NotifyObserversOfSuccessfulResponse(const std::string& ssid,
                                           const std::string& password);
  void NotifyObserversOfConnectionFailure(HostResponseErrorCode error_code);

 private:
  friend class ConnectTetheringOperationTest;
  FRIEND_TEST_ALL_PREFIXES(ConnectTetheringOperationTest,
                           TestOperation_SetupRequired);

  HostResponseErrorCode ConnectTetheringResponseCodeToHostResponseErrorCode(
      ConnectTetheringResponse_ResponseCode error_code);

  void SetClockForTest(base::Clock* clock_for_test);

  // The amount of time this operation will wait for a response. The timeout
  // values are different depending on whether setup is needed on the host.
  static const uint32_t kSetupNotRequiredResponseTimeoutSeconds;
  static const uint32_t kSetupRequiredResponseTimeoutSeconds;

  cryptauth::RemoteDeviceRef remote_device_;
  TetherHostResponseRecorder* tether_host_response_recorder_;
  base::Clock* clock_;
  int connect_message_sequence_number_ = -1;
  bool setup_required_;

  // These values are saved in OnMessageReceived() and returned in
  // OnOperationFinished().
  std::string ssid_to_return_;
  std::string password_to_return_;
  HostResponseErrorCode error_code_to_return_;
  base::Time connect_tethering_request_start_time_;

  base::ObserverList<Observer> observer_list_;

  DISALLOW_COPY_AND_ASSIGN(ConnectTetheringOperation);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_CONNECT_TETHERING_OPERATION_H_
