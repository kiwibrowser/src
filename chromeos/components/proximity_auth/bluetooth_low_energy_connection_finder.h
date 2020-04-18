// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_PROXIMITY_AUTH_BLUETOOTH_LOW_ENERGY_CONNECTION_FINDER_H_
#define CHROMEOS_COMPONENTS_PROXIMITY_AUTH_BLUETOOTH_LOW_ENERGY_CONNECTION_FINDER_H_

#include <memory>
#include <set>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "components/cryptauth/background_eid_generator.h"
#include "components/cryptauth/connection.h"
#include "components/cryptauth/connection_finder.h"
#include "components/cryptauth/connection_observer.h"
#include "components/cryptauth/remote_beacon_seed_fetcher.h"
#include "components/cryptauth/remote_device_ref.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_discovery_session.h"
#include "device/bluetooth/bluetooth_gatt_connection.h"

namespace proximity_auth {

// This cryptauth::ConnectionFinder implementation is specialized in finding a
// Bluetooth Low Energy remote device. We look for remote devices advertising
// the expected EID for the current or nearby time intervals.
class BluetoothLowEnergyConnectionFinder
    : public cryptauth::ConnectionFinder,
      public cryptauth::ConnectionObserver,
      public device::BluetoothAdapter::Observer {
 public:
  // Finds (and connects) to a Bluetooth low energy device, based on the EID
  // advertised by the remote device.
  //
  // |remote_device|: The BLE remote device.
  BluetoothLowEnergyConnectionFinder(cryptauth::RemoteDeviceRef remote_device);

  ~BluetoothLowEnergyConnectionFinder() override;

  // Finds a connection to the remote device.
  void Find(const cryptauth::ConnectionFinder::ConnectionCallback&
                connection_callback) override;

  // cryptauth::ConnectionObserver:
  void OnConnectionStatusChanged(
      cryptauth::Connection* connection,
      cryptauth::Connection::Status old_status,
      cryptauth::Connection::Status new_status) override;

  // device::BluetoothAdapter::Observer:
  void AdapterPoweredChanged(device::BluetoothAdapter* adapter,
                             bool powered) override;
  void DeviceAdded(device::BluetoothAdapter* adapter,
                   device::BluetoothDevice* device) override;
  void DeviceChanged(device::BluetoothAdapter* adapter,
                     device::BluetoothDevice* device) override;

 protected:
  BluetoothLowEnergyConnectionFinder(
      const cryptauth::RemoteDeviceRef remote_device,
      const std::string& service_uuid,
      std::unique_ptr<cryptauth::BackgroundEidGenerator> eid_generator);

  // Creates a proximity_auth::Connection with the device given by
  // |device_address|. Exposed for testing.
  virtual std::unique_ptr<cryptauth::Connection> CreateConnection(
      device::BluetoothDevice* bluetooth_device);

  // Checks if |device| is advertising the right EID.
  virtual bool IsRightDevice(device::BluetoothDevice* device);

 private:
  // Callback to be called when the Bluetooth adapter is initialized.
  void OnAdapterInitialized(scoped_refptr<device::BluetoothAdapter> adapter);

  // Checks if |remote_device| contains |remote_service_uuid| and creates a
  // connection in that case.
  void HandleDeviceUpdated(device::BluetoothDevice* remote_device);

  // Callback called when a new discovery session is started.
  void OnDiscoverySessionStarted(
      std::unique_ptr<device::BluetoothDiscoverySession> discovery_session);

  // Callback called when there is an error starting a new discovery session.
  void OnStartDiscoverySessionError();

  // Starts a discovery session for |adapter_|.
  void StartDiscoverySession();

  // Stops the discovery session given by |discovery_session_|.
  void StopDiscoverySession();

  // Restarts the discovery session after creating |connection_| fails.
  void RestartDiscoverySessionAsync();

  // Used to invoke |connection_callback_| asynchronously, decoupling the
  // callback invocation from the ConnectionObserver callstack.
  void InvokeCallbackAsync();

  // The remote BLE device being searched. It maybe empty, in this case the
  // remote device should advertise |remote_service_uuid_| and
  // |advertised_name_|.
  cryptauth::RemoteDeviceRef remote_device_;

  // The UUID of the service used by the Weave socket.
  std::string service_uuid_;

  // Generates the expected EIDs that may be advertised by |remote_device_|. If
  // an EID matches, we know its a device we should connect to.
  std::unique_ptr<cryptauth::BackgroundEidGenerator> eid_generator_;

  // The Bluetooth adapter over which the Bluetooth connection will be made.
  scoped_refptr<device::BluetoothAdapter> adapter_;

  // The discovery session associated to this object.
  std::unique_ptr<device::BluetoothDiscoverySession> discovery_session_;

  // The connection with |remote_device|.
  std::unique_ptr<cryptauth::Connection> connection_;

  // Callback called when the connection is established.
  cryptauth::ConnectionFinder::ConnectionCallback connection_callback_;

  base::WeakPtrFactory<BluetoothLowEnergyConnectionFinder> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothLowEnergyConnectionFinder);
};

}  // namespace proximity_auth

#endif  // CHROMEOS_COMPONENTS_PROXIMITY_AUTH_BLUETOOTH_LOW_ENERGY_CONNECTION_FINDER_H_
