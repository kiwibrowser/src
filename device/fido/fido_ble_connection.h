// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_FIDO_BLE_CONNECTION_H_
#define DEVICE_FIDO_FIDO_BLE_CONNECTION_H_

#include <stdint.h>

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/component_export.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_gatt_service.h"

namespace device {

class BluetoothGattConnection;
class BluetoothGattNotifySession;
class BluetoothRemoteGattCharacteristic;
class BluetoothRemoteGattService;

// A connection to the U2F service of an authenticator over BLE. Detailed
// specification of the BLE device can be found here:
// https://fidoalliance.org/specs/fido-u2f-v1.2-ps-20170411/fido-u2f-bt-protocol-v1.2-ps-20170411.html#h2_gatt-service-description
//
// Currently this code does not handle devices that need pairing. This is fine
// for non-BlueZ platforms, as here accessing a protected characteristic will
// trigger an OS level dialog if pairing is required. However, for BlueZ
// platforms pairing must have been done externally, for example using the
// `bluetoothctl` command.
//
// TODO(crbug.com/763303): Add support for pairing from within this class and
// provide users with an option to manually specify a PIN code.
class COMPONENT_EXPORT(DEVICE_FIDO) FidoBleConnection
    : public BluetoothAdapter::Observer {
 public:
  enum class ServiceRevision {
    VERSION_1_0,
    VERSION_1_1,
    VERSION_1_2,
  };

  // This callback informs clients repeatedly about changes in the device
  // connection. This class makes an initial connection attempt on construction,
  // which result in returned via this callback. Future invocations happen if
  // devices connect or disconnect from the adapter.
  using ConnectionStatusCallback = base::RepeatingCallback<void(bool)>;
  using WriteCallback = base::OnceCallback<void(bool)>;
  using ReadCallback = base::RepeatingCallback<void(std::vector<uint8_t>)>;
  using ControlPointLengthCallback =
      base::OnceCallback<void(base::Optional<uint16_t>)>;
  using ServiceRevisionsCallback =
      base::OnceCallback<void(std::set<ServiceRevision>)>;

  FidoBleConnection(std::string device_address,
                    ConnectionStatusCallback connection_status_callback,
                    ReadCallback read_callback);
  ~FidoBleConnection() override;

  const std::string& address() const { return address_; }

  virtual void Connect();
  virtual void ReadControlPointLength(ControlPointLengthCallback callback);
  virtual void ReadServiceRevisions(ServiceRevisionsCallback callback);
  virtual void WriteControlPoint(const std::vector<uint8_t>& data,
                                 WriteCallback callback);
  virtual void WriteServiceRevision(ServiceRevision service_revision,
                                    WriteCallback callback);

 protected:
  explicit FidoBleConnection(std::string device_address);

 private:
  // BluetoothAdapter::Observer:
  void DeviceAdded(BluetoothAdapter* adapter, BluetoothDevice* device) override;
  void DeviceAddressChanged(BluetoothAdapter* adapter,
                            BluetoothDevice* device,
                            const std::string& old_address) override;
  void DeviceChanged(BluetoothAdapter* adapter,
                     BluetoothDevice* device) override;
  void GattCharacteristicValueChanged(
      BluetoothAdapter* adapter,
      BluetoothRemoteGattCharacteristic* characteristic,
      const std::vector<uint8_t>& value) override;
  void GattServicesDiscovered(BluetoothAdapter* adapter,
                              BluetoothDevice* device) override;

  void OnGetAdapter(scoped_refptr<BluetoothAdapter> adapter);

  void CreateGattConnection();
  void OnCreateGattConnection(
      std::unique_ptr<BluetoothGattConnection> connection);
  void OnCreateGattConnectionError(
      BluetoothDevice::ConnectErrorCode error_code);

  void ConnectToU2fService();

  void OnStartNotifySession(
      std::unique_ptr<BluetoothGattNotifySession> notify_session);
  void OnStartNotifySessionError(
      BluetoothGattService::GattErrorCode error_code);

  void OnConnectionError();

  const BluetoothRemoteGattService* GetFidoService() const;

  static void OnReadControlPointLength(ControlPointLengthCallback callback,
                                       const std::vector<uint8_t>& value);
  static void OnReadControlPointLengthError(
      ControlPointLengthCallback callback,
      BluetoothGattService::GattErrorCode error_code);

  void OnReadServiceRevision(base::OnceClosure callback,
                             const std::vector<uint8_t>& value);
  void OnReadServiceRevisionError(
      base::OnceClosure callback,
      BluetoothGattService::GattErrorCode error_code);

  void OnReadServiceRevisionBitfield(base::OnceClosure callback,
                                     const std::vector<uint8_t>& value);
  void OnReadServiceRevisionBitfieldError(
      base::OnceClosure callback,
      BluetoothGattService::GattErrorCode error_code);
  void ReturnServiceRevisions(ServiceRevisionsCallback callback);

  static void OnWrite(WriteCallback callback);
  static void OnWriteError(WriteCallback callback,
                           BluetoothGattService::GattErrorCode error_code);

  std::string address_;
  ConnectionStatusCallback connection_status_callback_;
  ReadCallback read_callback_;

  scoped_refptr<BluetoothAdapter> adapter_;
  std::unique_ptr<BluetoothGattConnection> connection_;
  std::unique_ptr<BluetoothGattNotifySession> notify_session_;

  base::Optional<std::string> u2f_service_id_;
  base::Optional<std::string> control_point_length_id_;
  base::Optional<std::string> control_point_id_;
  base::Optional<std::string> status_id_;
  base::Optional<std::string> service_revision_id_;
  base::Optional<std::string> service_revision_bitfield_id_;

  std::set<ServiceRevision> service_revisions_;

  base::WeakPtrFactory<FidoBleConnection> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FidoBleConnection);
};

}  // namespace device

#endif  // DEVICE_FIDO_FIDO_BLE_CONNECTION_H_
