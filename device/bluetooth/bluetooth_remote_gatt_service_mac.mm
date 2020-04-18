// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_remote_gatt_service_mac.h"

#import <CoreBluetooth/CoreBluetooth.h>
#include <vector>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/sys_string_conversions.h"
#include "device/bluetooth/bluetooth_adapter_mac.h"
#include "device/bluetooth/bluetooth_low_energy_device_mac.h"
#include "device/bluetooth/bluetooth_remote_gatt_characteristic_mac.h"
#include "device/bluetooth/bluetooth_uuid.h"

namespace device {

BluetoothRemoteGattServiceMac::BluetoothRemoteGattServiceMac(
    BluetoothLowEnergyDeviceMac* bluetooth_device_mac,
    CBService* service,
    bool is_primary)
    : bluetooth_device_mac_(bluetooth_device_mac),
      service_(service, base::scoped_policy::RETAIN),
      is_primary_(is_primary),
      discovery_pending_count_(0) {
  uuid_ = BluetoothAdapterMac::BluetoothUUIDWithCBUUID([service_ UUID]);
  identifier_ = base::SysNSStringToUTF8(
      [NSString stringWithFormat:@"%s-%p", uuid_.canonical_value().c_str(),
                                 service_.get()]);
}

BluetoothRemoteGattServiceMac::~BluetoothRemoteGattServiceMac() {}

std::string BluetoothRemoteGattServiceMac::GetIdentifier() const {
  return identifier_;
}

BluetoothUUID BluetoothRemoteGattServiceMac::GetUUID() const {
  return uuid_;
}

bool BluetoothRemoteGattServiceMac::IsPrimary() const {
  return is_primary_;
}

BluetoothDevice* BluetoothRemoteGattServiceMac::GetDevice() const {
  return bluetooth_device_mac_;
}

std::vector<BluetoothRemoteGattCharacteristic*>
BluetoothRemoteGattServiceMac::GetCharacteristics() const {
  std::vector<BluetoothRemoteGattCharacteristic*> gatt_characteristics;
  for (const auto& iter : gatt_characteristic_macs_) {
    BluetoothRemoteGattCharacteristic* gatt_characteristic =
        static_cast<BluetoothRemoteGattCharacteristic*>(iter.second.get());
    gatt_characteristics.push_back(gatt_characteristic);
  }
  return gatt_characteristics;
}

std::vector<BluetoothRemoteGattService*>
BluetoothRemoteGattServiceMac::GetIncludedServices() const {
  NOTIMPLEMENTED();
  return std::vector<BluetoothRemoteGattService*>();
}

BluetoothRemoteGattCharacteristic*
BluetoothRemoteGattServiceMac::GetCharacteristic(
    const std::string& identifier) const {
  auto searched_pair = gatt_characteristic_macs_.find(identifier);
  if (searched_pair == gatt_characteristic_macs_.end()) {
    return nullptr;
  }
  return static_cast<BluetoothRemoteGattCharacteristic*>(
      searched_pair->second.get());
}

void BluetoothRemoteGattServiceMac::DiscoverCharacteristics() {
  VLOG(1) << *this << ": DiscoverCharacteristics.";
  SetDiscoveryComplete(false);
  ++discovery_pending_count_;
  [GetCBPeripheral() discoverCharacteristics:nil forService:GetService()];
}

void BluetoothRemoteGattServiceMac::DidDiscoverCharacteristics() {
  if (IsDiscoveryComplete() || discovery_pending_count_ == 0) {
    // This should never happen, just in case it happens with a device, this
    // notification should be ignored.
    VLOG(1)
        << *this
        << ": Unmatch DiscoverCharacteristics and DidDiscoverCharacteristics.";
    return;
  }
  VLOG(1) << *this << ": DidDiscoverCharacteristics.";
  --discovery_pending_count_;
  std::unordered_set<std::string> characteristic_identifier_to_remove;
  for (const auto& iter : gatt_characteristic_macs_) {
    characteristic_identifier_to_remove.insert(iter.first);
  }

  for (CBCharacteristic* cb_characteristic in GetService().characteristics) {
    BluetoothRemoteGattCharacteristicMac* gatt_characteristic_mac =
        GetBluetoothRemoteGattCharacteristicMac(cb_characteristic);
    if (gatt_characteristic_mac) {
      VLOG(1) << *gatt_characteristic_mac
              << ": Known characteristic, properties "
              << gatt_characteristic_mac->GetProperties();
      const std::string& identifier = gatt_characteristic_mac->GetIdentifier();
      characteristic_identifier_to_remove.erase(identifier);
      gatt_characteristic_mac->DiscoverDescriptors();
      continue;
    }
    gatt_characteristic_mac =
        new BluetoothRemoteGattCharacteristicMac(this, cb_characteristic);
    const std::string& identifier = gatt_characteristic_mac->GetIdentifier();
    auto result_iter = gatt_characteristic_macs_.insert(
        {identifier, base::WrapUnique(gatt_characteristic_mac)});
    DCHECK(result_iter.second);
    VLOG(1) << *gatt_characteristic_mac << ": New characteristic, properties "
            << gatt_characteristic_mac->GetProperties();
    if (discovery_pending_count_ == 0) {
      gatt_characteristic_mac->DiscoverDescriptors();
    }
    GetMacAdapter()->NotifyGattCharacteristicAdded(gatt_characteristic_mac);
  }

  for (const std::string& identifier : characteristic_identifier_to_remove) {
    auto pair_to_remove = gatt_characteristic_macs_.find(identifier);
    std::unique_ptr<BluetoothRemoteGattCharacteristicMac>
        characteristic_to_remove;
    pair_to_remove->second.swap(characteristic_to_remove);
    VLOG(1) << *characteristic_to_remove << ": Removed characteristic.";
    gatt_characteristic_macs_.erase(pair_to_remove);
    GetMacAdapter()->NotifyGattCharacteristicRemoved(
        characteristic_to_remove.get());
  }
  SendNotificationIfComplete();
}

void BluetoothRemoteGattServiceMac::DidDiscoverDescriptors(
    CBCharacteristic* characteristic) {
  if (IsDiscoveryComplete()) {
    // This should never happen, just in case it happens with a device, this
    // notification should be ignored.
    VLOG(1) << *this
            << ": Discovery complete, ignoring DidDiscoverDescriptors.";
    return;
  }
  BluetoothRemoteGattCharacteristicMac* gatt_characteristic =
      GetBluetoothRemoteGattCharacteristicMac(characteristic);
  DCHECK(gatt_characteristic);
  gatt_characteristic->DidDiscoverDescriptors();
  SendNotificationIfComplete();
}

void BluetoothRemoteGattServiceMac::SendNotificationIfComplete() {
  DCHECK(!IsDiscoveryComplete());
  // Notify when all characteristics have been fully discovered.
  SetDiscoveryComplete(
      discovery_pending_count_ == 0 &&
      std::find_if_not(
          gatt_characteristic_macs_.begin(), gatt_characteristic_macs_.end(),
          [](const std::pair<
              const std::string,
              std::unique_ptr<BluetoothRemoteGattCharacteristicMac>>& pair) {
            return pair.second->IsDiscoveryComplete();
          }) == gatt_characteristic_macs_.end());
  if (IsDiscoveryComplete()) {
    VLOG(1) << *this << ": Discovery complete.";
    GetMacAdapter()->NotifyGattServiceChanged(this);
  }
}

BluetoothAdapterMac* BluetoothRemoteGattServiceMac::GetMacAdapter() const {
  return bluetooth_device_mac_->GetMacAdapter();
}

CBPeripheral* BluetoothRemoteGattServiceMac::GetCBPeripheral() const {
  return bluetooth_device_mac_->GetPeripheral();
}

CBService* BluetoothRemoteGattServiceMac::GetService() const {
  return service_;
}

BluetoothRemoteGattCharacteristicMac*
BluetoothRemoteGattServiceMac::GetBluetoothRemoteGattCharacteristicMac(
    CBCharacteristic* cb_characteristic) const {
  auto found = std::find_if(
      gatt_characteristic_macs_.begin(), gatt_characteristic_macs_.end(),
      [cb_characteristic](
          const std::pair<
              const std::string,
              std::unique_ptr<BluetoothRemoteGattCharacteristicMac>>& pair) {
        return pair.second->GetCBCharacteristic() == cb_characteristic;
      });
  if (found == gatt_characteristic_macs_.end()) {
    return nullptr;
  } else {
    return found->second.get();
  }
}

BluetoothRemoteGattDescriptorMac*
BluetoothRemoteGattServiceMac::GetBluetoothRemoteGattDescriptorMac(
    CBDescriptor* cb_descriptor) const {
  CBCharacteristic* cb_characteristic = [cb_descriptor characteristic];
  BluetoothRemoteGattCharacteristicMac* gatt_characteristic_mac =
      GetBluetoothRemoteGattCharacteristicMac(cb_characteristic);
  if (!gatt_characteristic_mac) {
    return nullptr;
  }
  return gatt_characteristic_mac->GetBluetoothRemoteGattDescriptorMac(
      cb_descriptor);
}

DEVICE_BLUETOOTH_EXPORT std::ostream& operator<<(
    std::ostream& out,
    const BluetoothRemoteGattServiceMac& service) {
  const BluetoothLowEnergyDeviceMac* bluetooth_device_mac_ =
      static_cast<const BluetoothLowEnergyDeviceMac*>(service.GetDevice());
  return out << "<BluetoothRemoteGattServiceMac "
             << service.GetUUID().canonical_value() << "/" << &service
             << ", device: " << bluetooth_device_mac_->GetAddress() << "/"
             << bluetooth_device_mac_ << ">";
}

}  // namespace device
