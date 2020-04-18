// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAIRING_BLUETOOTH_CONTROLLER_PAIRING_CONTROLLER_H_
#define COMPONENTS_PAIRING_BLUETOOTH_CONTROLLER_PAIRING_CONTROLLER_H_

#include <stdint.h>

#include <memory>
#include <set>

#include "base/macros.h"
#include "base/observer_list.h"
#include "base/threading/thread_checker.h"
#include "components/pairing/controller_pairing_controller.h"
#include "components/pairing/proto_decoder.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_socket.h"

namespace net {
class IOBuffer;
}

namespace pairing_chromeos {

class BluetoothControllerPairingController
    : public ControllerPairingController,
      public ProtoDecoder::Observer,
      public device::BluetoothAdapter::Observer,
      public device::BluetoothDevice::PairingDelegate  {
 public:
  BluetoothControllerPairingController();
  ~BluetoothControllerPairingController() override;

 private:
  void ChangeStage(Stage new_stage);
  device::BluetoothDevice* GetController();
  void Reset();
  void DeviceFound(device::BluetoothDevice* device);
  void DeviceLost(device::BluetoothDevice* device);
  void SendBuffer(scoped_refptr<net::IOBuffer> io_buffer, int size);
  void CompleteSetup();

  void OnSetPowered();
  void OnGetAdapter(scoped_refptr<device::BluetoothAdapter> adapter);
  void OnStartDiscoverySession(
      std::unique_ptr<device::BluetoothDiscoverySession> discovery_session);
  void OnConnect();
  void OnConnectToService(scoped_refptr<device::BluetoothSocket> socket);
  void OnSendComplete(int bytes_sent);
  void OnReceiveComplete(int bytes, scoped_refptr<net::IOBuffer> io_buffer);

  void OnError();
  void OnErrorWithMessage(const std::string& message);
  void OnConnectError(device::BluetoothDevice::ConnectErrorCode error_code);
  void OnReceiveError(device::BluetoothSocket::ErrorReason reason,
                      const std::string& error_message);

  // ControllerPairingController:
  void AddObserver(ControllerPairingController::Observer* observer) override;
  void RemoveObserver(ControllerPairingController::Observer* observer) override;
  Stage GetCurrentStage() override;
  void StartPairing() override;
  DeviceIdList GetDiscoveredDevices() override;
  void ChooseDeviceForPairing(const std::string& device_id) override;
  void RepeatDiscovery() override;
  std::string GetConfirmationCode() override;
  void SetConfirmationCodeIsCorrect(bool correct) override;
  void SetHostNetwork(const std::string& onc_spec) override;
  void SetHostConfiguration(bool accepted_eula,
                            const std::string& lang,
                            const std::string& timezone,
                            bool send_reports,
                            const std::string& keyboard_layout) override;
  void OnAuthenticationDone(const std::string& domain,
                            const std::string& auth_token) override;
  void StartSession() override;

  // ProtoDecoder::Observer:
  void OnHostStatusMessage(const pairing_api::HostStatus& message) override;
  void OnConfigureHostMessage(
      const pairing_api::ConfigureHost& message) override;
  void OnPairDevicesMessage(const pairing_api::PairDevices& message) override;
  void OnCompleteSetupMessage(
      const pairing_api::CompleteSetup& message) override;
  void OnErrorMessage(const pairing_api::Error& message) override;
  void OnAddNetworkMessage(const pairing_api::AddNetwork& message) override;
  void OnRebootMessage(const pairing_api::Reboot& message) override;

  // BluetoothAdapter::Observer:
  void DeviceAdded(device::BluetoothAdapter* adapter,
                   device::BluetoothDevice* device) override;
  void DeviceRemoved(device::BluetoothAdapter* adapter,
                     device::BluetoothDevice* device) override;

  // device::BluetoothDevice::PairingDelegate:
  void RequestPinCode(device::BluetoothDevice* device) override;
  void RequestPasskey(device::BluetoothDevice* device) override;
  void DisplayPinCode(device::BluetoothDevice* device,
                      const std::string& pincode) override;
  void DisplayPasskey(device::BluetoothDevice* device,
                      uint32_t passkey) override;
  void KeysEntered(device::BluetoothDevice* device, uint32_t entered) override;
  void ConfirmPasskey(device::BluetoothDevice* device,
                      uint32_t passkey) override;
  void AuthorizePairing(device::BluetoothDevice* device) override;

  Stage current_stage_;
  scoped_refptr<device::BluetoothAdapter> adapter_;
  std::unique_ptr<device::BluetoothDiscoverySession> discovery_session_;
  scoped_refptr<device::BluetoothSocket> socket_;
  std::string controller_device_id_;

  std::string confirmation_code_;
  std::set<std::string> discovered_devices_;

  std::unique_ptr<ProtoDecoder> proto_decoder_;

  base::ThreadChecker thread_checker_;
  base::ObserverList<ControllerPairingController::Observer> observers_;
  base::WeakPtrFactory<BluetoothControllerPairingController> ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothControllerPairingController);
};

}  // namespace pairing_chromeos

#endif  // COMPONENTS_PAIRING_BLUETOOTH_CONTROLLER_PAIRING_CONTROLLER_H_
