// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_DISCONNECT_TETHERING_REQUEST_SENDER_IMPL_H_
#define CHROMEOS_COMPONENTS_TETHER_DISCONNECT_TETHERING_REQUEST_SENDER_IMPL_H_

#include <map>

#include "base/optional.h"
#include "chromeos/components/tether/disconnect_tethering_operation.h"
#include "chromeos/components/tether/disconnect_tethering_request_sender.h"

namespace chromeos {

namespace tether {

class BleConnectionManager;
class TetherHostFetcher;

class DisconnectTetheringRequestSenderImpl
    : public DisconnectTetheringRequestSender,
      public DisconnectTetheringOperation::Observer {
 public:
  class Factory {
   public:
    static std::unique_ptr<DisconnectTetheringRequestSender> NewInstance(
        BleConnectionManager* ble_connection_manager,
        TetherHostFetcher* tether_host_fetcher);

    static void SetInstanceForTesting(Factory* factory);

   protected:
    virtual std::unique_ptr<DisconnectTetheringRequestSender> BuildInstance(
        BleConnectionManager* ble_connection_manager,
        TetherHostFetcher* tether_host_fetcher);

   private:
    static Factory* factory_instance_;
  };

  ~DisconnectTetheringRequestSenderImpl() override;

  // DisconnectTetheringRequestSender:
  void SendDisconnectRequestToDevice(const std::string& device_id) override;
  bool HasPendingRequests() override;

  // DisconnectTetheringOperation::Observer:
  void OnOperationFinished(const std::string& device_id, bool success) override;

 protected:
  DisconnectTetheringRequestSenderImpl(
      BleConnectionManager* ble_connection_manager,
      TetherHostFetcher* tether_host_fetcher);

 private:
  void OnTetherHostFetched(
      const std::string& device_id,
      base::Optional<cryptauth::RemoteDeviceRef> tether_host);

  BleConnectionManager* ble_connection_manager_;
  TetherHostFetcher* tether_host_fetcher_;

  int num_pending_host_fetches_ = 0;
  std::map<std::string, std::unique_ptr<DisconnectTetheringOperation>>
      device_id_to_operation_map_;

  base::WeakPtrFactory<DisconnectTetheringRequestSenderImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DisconnectTetheringRequestSenderImpl);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_DISCONNECT_TETHERING_REQUEST_SENDER_IMPL_H_
