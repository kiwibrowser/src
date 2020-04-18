// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUETOOTH_LOW_ENERGY_DEVICE_MAC_H_
#define DEVICE_BLUETOOTH_BLUETOOTH_LOW_ENERGY_DEVICE_MAC_H_

#if defined(OS_IOS)
#import <CoreBluetooth/CoreBluetooth.h>
#else  // !defined(OS_IOS)
#import <IOBluetooth/IOBluetooth.h>
#endif  // defined(OS_IOS)

#include <stdint.h>

#include <set>

#include "base/mac/scoped_nsobject.h"
#include "base/mac/sdk_forward_declarations.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "crypto/sha2.h"
#include "device/bluetooth/bluetooth_device_mac.h"

@class BluetoothLowEnergyPeripheralDelegate;

namespace device {

class BluetoothAdapterMac;
class BluetoothRemoteGattServiceMac;
class BluetoothRemoteGattCharacteristicMac;
class BluetoothRemoteGattDescriptorMac;

class DEVICE_BLUETOOTH_EXPORT BluetoothLowEnergyDeviceMac
    : public BluetoothDeviceMac {
 public:
  BluetoothLowEnergyDeviceMac(BluetoothAdapterMac* adapter,
                              CBPeripheral* peripheral);
  ~BluetoothLowEnergyDeviceMac() override;

  // BluetoothDevice overrides.
  std::string GetIdentifier() const override;
  uint32_t GetBluetoothClass() const override;
  std::string GetAddress() const override;
  BluetoothDevice::VendorIDSource GetVendorIDSource() const override;
  uint16_t GetVendorID() const override;
  uint16_t GetProductID() const override;
  uint16_t GetDeviceID() const override;
  uint16_t GetAppearance() const override;
  base::Optional<std::string> GetName() const override;
  bool IsPaired() const override;
  bool IsConnected() const override;
  bool IsGattConnected() const override;
  bool IsConnectable() const override;
  bool IsConnecting() const override;
  bool ExpectingPinCode() const override;
  bool ExpectingPasskey() const override;
  bool ExpectingConfirmation() const override;
  void GetConnectionInfo(const ConnectionInfoCallback& callback) override;
  void SetConnectionLatency(ConnectionLatency connection_latency,
                            const base::Closure& callback,
                            const ErrorCallback& error_callback) override;
  void Connect(PairingDelegate* pairing_delegate,
               const base::Closure& callback,
               const ConnectErrorCallback& error_callback) override;
  void SetPinCode(const std::string& pincode) override;
  void SetPasskey(uint32_t passkey) override;
  void ConfirmPairing() override;
  void RejectPairing() override;
  void CancelPairing() override;
  void Disconnect(const base::Closure& callback,
                  const ErrorCallback& error_callback) override;
  void Forget(const base::Closure& callback,
              const ErrorCallback& error_callback) override;
  void ConnectToService(
      const BluetoothUUID& uuid,
      const ConnectToServiceCallback& callback,
      const ConnectToServiceErrorCallback& error_callback) override;
  void ConnectToServiceInsecurely(
      const device::BluetoothUUID& uuid,
      const ConnectToServiceCallback& callback,
      const ConnectToServiceErrorCallback& error_callback) override;

 protected:
  // BluetoothDevice override.
  void CreateGattConnectionImpl() override;
  void DisconnectGatt() override;

  // Methods used by BluetoothLowEnergyPeripheralBridge.
  void DidDiscoverPrimaryServices(NSError* error);
  void DidModifyServices(NSArray* invalidatedServices);
  void DidDiscoverCharacteristics(CBService* cb_service, NSError* error);
  void DidUpdateValue(CBCharacteristic* characteristic, NSError* error);
  void DidWriteValue(CBCharacteristic* characteristic, NSError* error);
  void DidUpdateNotificationState(CBCharacteristic* characteristic,
                                  NSError* error);
  void DidDiscoverDescriptors(CBCharacteristic* characteristic, NSError* error);
  void DidUpdateValueForDescriptor(CBDescriptor* cb_descriptor, NSError* error);
  void DidWriteValueForDescriptor(CBDescriptor* descriptor, NSError* error);

  static std::string GetPeripheralIdentifier(CBPeripheral* peripheral);

  // Hashes and truncates the peripheral identifier to deterministically
  // construct an address. The use of fake addresses is a temporary fix before
  // we switch to using bluetooth identifiers throughout Chrome.
  // http://crbug.com/507824
  static std::string GetPeripheralHashAddress(CBPeripheral* peripheral);

 private:
  friend class BluetoothAdapterMac;
  friend class BluetoothAdapterMacTest;
  friend class BluetoothLowEnergyPeripheralBridge;
  friend class BluetoothRemoteGattServiceMac;
  friend class BluetoothTestMac;
  friend class BluetoothRemoteGattServiceMac;

  // Called by the adapter when the device is connected.
  void DidConnectPeripheral();

  // Calls macOS to discover primary services.
  void DiscoverPrimaryServices();

  // Sends notification if this device is ready with all services discovered.
  void SendNotificationIfDiscoveryComplete();

  // Returns the Bluetooth adapter.
  BluetoothAdapterMac* GetMacAdapter();

  // Returns the CoreBluetooth Peripheral.
  CBPeripheral* GetPeripheral();

  // Returns BluetoothRemoteGattServiceMac based on the CBService.
  BluetoothRemoteGattServiceMac* GetBluetoothRemoteGattServiceMac(
      CBService* service) const;

  // Returns BluetoothRemoteGattCharacteristicMac based on the CBCharacteristic.
  BluetoothRemoteGattCharacteristicMac* GetBluetoothRemoteGattCharacteristicMac(
      CBCharacteristic* cb_characteristic) const;

  // Returns BluetoothRemoteGattDescriptorMac based on the CBDescriptor.
  BluetoothRemoteGattDescriptorMac* GetBluetoothRemoteGattDescriptorMac(
      CBDescriptor* cb_descriptor) const;

  // Callback used when the CoreBluetooth Peripheral is disconnected.
  void DidDisconnectPeripheral(NSError* error);

  // CoreBluetooth data structure.
  base::scoped_nsobject<CBPeripheral> peripheral_;

  // Objective-C delegate for the CBPeripheral.
  base::scoped_nsobject<BluetoothLowEnergyPeripheralDelegate>
      peripheral_delegate_;

  // Whether the device is connected.
  bool connected_;

  // The peripheral's identifier, as returned by [CBPeripheral identifier].
  std::string identifier_;

  // A local address for the device created by hashing the peripheral
  // identifier.
  std::string hash_address_;

  // Increases each time -[CBPeripheral discoverServices:] is called, and
  // decreases each time DidDiscoverPrimaryServices() is called. Once the
  // value is set to 0, characteristics and properties are discovered.
  int discovery_pending_count_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothLowEnergyDeviceMac);
};

// Stream operator for logging.
DEVICE_BLUETOOTH_EXPORT std::ostream& operator<<(
    std::ostream& out,
    const BluetoothLowEnergyDeviceMac& device);

}  // namespace device

#endif  // DEVICE_BLUETOOTH_BLUETOOTH_LOW_ENERGY_DEVICE_MAC_H_
