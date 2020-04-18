// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUEZ_BLUETOOTH_ADVERTISEMENT_BLUEZ_H_
#define DEVICE_BLUETOOTH_BLUEZ_BLUETOOTH_ADVERTISEMENT_BLUEZ_H_

#include <memory>

#include "base/macros.h"
#include "dbus/object_path.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_advertisement.h"
#include "device/bluetooth/bluetooth_export.h"
#include "device/bluetooth/dbus/bluetooth_le_advertisement_service_provider.h"

namespace bluez {
class BluetoothLEAdvertisementServiceProvider;
}

namespace bluez {

class BluetoothAdapterBlueZ;

// The BluetoothAdvertisementBlueZ class implements BluetoothAdvertisement
// for platforms that use BlueZ.
class DEVICE_BLUETOOTH_EXPORT BluetoothAdvertisementBlueZ
    : public device::BluetoothAdvertisement,
      public bluez::BluetoothLEAdvertisementServiceProvider::Delegate {
 public:
  BluetoothAdvertisementBlueZ(
      std::unique_ptr<device::BluetoothAdvertisement::Data> data,
      scoped_refptr<BluetoothAdapterBlueZ> adapter);

  // BluetoothAdvertisement overrides:
  void Unregister(const SuccessCallback& success_callback,
                  const ErrorCallback& error_callback) override;

  // bluez::BluetoothLEAdvertisementServiceProvider::Delegate overrides:
  void Released() override;

  void Register(const base::Closure& success_callback,
                const device::BluetoothAdapter::AdvertisementErrorCallback&
                    error_callback);

  // Used from tests to be able to trigger events on the fake advertisement
  // provider.
  bluez::BluetoothLEAdvertisementServiceProvider* provider() {
    return provider_.get();
  }

 private:
  ~BluetoothAdvertisementBlueZ() override;

  // Adapter this advertisement is advertising on.
  dbus::ObjectPath adapter_path_;
  std::unique_ptr<bluez::BluetoothLEAdvertisementServiceProvider> provider_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothAdvertisementBlueZ);
};

}  // namespace bluez

#endif  // DEVICE_BLUETOOTH_BLUEZ_BLUETOOTH_ADVERTISEMENT_BLUEZ_H_
