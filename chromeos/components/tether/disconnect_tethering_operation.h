// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_DISCONNECT_TETHERING_OPERATION_H_
#define CHROMEOS_COMPONENTS_TETHER_DISCONNECT_TETHERING_OPERATION_H_

#include "base/observer_list.h"
#include "base/time/clock.h"
#include "chromeos/components/tether/message_transfer_operation.h"

namespace chromeos {

namespace tether {

class BleConnectionManager;

// Operation which sends a disconnect message to a tether host.
class DisconnectTetheringOperation : public MessageTransferOperation {
 public:
  class Factory {
   public:
    static std::unique_ptr<DisconnectTetheringOperation> NewInstance(
        cryptauth::RemoteDeviceRef device_to_connect,
        BleConnectionManager* connection_manager);

    static void SetInstanceForTesting(Factory* factory);

   protected:
    virtual std::unique_ptr<DisconnectTetheringOperation> BuildInstance(
        cryptauth::RemoteDeviceRef device_to_connect,
        BleConnectionManager* connection_manager);

   private:
    static Factory* factory_instance_;
  };

  class Observer {
   public:
    // Alerts observers when the operation has finished for device with ID
    // |device_id|. |success| is true when the operation successfully sends the
    // message and false otherwise.
    virtual void OnOperationFinished(const std::string& device_id,
                                     bool success) = 0;
  };

  ~DisconnectTetheringOperation() override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  DisconnectTetheringOperation(cryptauth::RemoteDeviceRef device_to_connect,
                               BleConnectionManager* connection_manager);

  void NotifyObserversOperationFinished(bool success);

  // MessageTransferOperation:
  void OnDeviceAuthenticated(cryptauth::RemoteDeviceRef remote_device) override;
  void OnOperationFinished() override;
  MessageType GetMessageTypeForConnection() override;
  void OnMessageSent(int sequence_number) override;

 private:
  friend class DisconnectTetheringOperationTest;

  void SetClockForTest(base::Clock* clock_for_test);

  base::ObserverList<Observer> observer_list_;
  cryptauth::RemoteDeviceRef remote_device_;
  int disconnect_message_sequence_number_ = -1;
  bool has_sent_message_;

  base::Clock* clock_;
  base::Time disconnect_start_time_;

  DISALLOW_COPY_AND_ASSIGN(DisconnectTetheringOperation);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_DISCONNECT_TETHERING_OPERATION_H_
