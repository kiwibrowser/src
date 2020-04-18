// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/test/fake_remote_gatt_service.h"

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_uuid.h"
#include "device/bluetooth/public/mojom/test/fake_bluetooth.mojom.h"
#include "device/bluetooth/test/fake_remote_gatt_characteristic.h"

namespace bluetooth {

FakeRemoteGattService::FakeRemoteGattService(
    const std::string& service_id,
    const device::BluetoothUUID& service_uuid,
    bool is_primary,
    device::BluetoothDevice* device)
    : service_id_(service_id),
      service_uuid_(service_uuid),
      is_primary_(is_primary),
      device_(device),
      last_characteristic_id_(0) {}

FakeRemoteGattService::~FakeRemoteGattService() = default;

bool FakeRemoteGattService::AllResponsesConsumed() {
  return std::all_of(
      fake_characteristics_.begin(), fake_characteristics_.end(),
      [](const auto& e) { return e.second->AllResponsesConsumed(); });
}

std::string FakeRemoteGattService::AddFakeCharacteristic(
    const device::BluetoothUUID& characteristic_uuid,
    mojom::CharacteristicPropertiesPtr properties) {
  FakeCharacteristicMap::iterator it;
  bool inserted;

  // Attribute instance Ids need to be unique.
  std::string new_characteristic_id = base::StringPrintf(
      "%s_%zu", GetIdentifier().c_str(), ++last_characteristic_id_);

  std::tie(it, inserted) = fake_characteristics_.emplace(
      new_characteristic_id, std::make_unique<FakeRemoteGattCharacteristic>(
                                 new_characteristic_id, characteristic_uuid,
                                 std::move(properties), this));

  DCHECK(inserted);
  return it->second->GetIdentifier();
}

bool FakeRemoteGattService::RemoveFakeCharacteristic(
    const std::string& identifier) {
  GetCharacteristic(identifier);
  const auto& it = fake_characteristics_.find(identifier);
  if (it == fake_characteristics_.end()) {
    return false;
  }

  fake_characteristics_.erase(it);
  return true;
}

std::string FakeRemoteGattService::GetIdentifier() const {
  return service_id_;
}

device::BluetoothUUID FakeRemoteGattService::GetUUID() const {
  return service_uuid_;
}

bool FakeRemoteGattService::IsPrimary() const {
  return is_primary_;
}

device::BluetoothDevice* FakeRemoteGattService::GetDevice() const {
  return device_;
}

std::vector<device::BluetoothRemoteGattCharacteristic*>
FakeRemoteGattService::GetCharacteristics() const {
  std::vector<device::BluetoothRemoteGattCharacteristic*> characteristics;
  for (const auto& it : fake_characteristics_)
    characteristics.push_back(it.second.get());
  return characteristics;
}

std::vector<device::BluetoothRemoteGattService*>
FakeRemoteGattService::GetIncludedServices() const {
  NOTREACHED();
  return std::vector<device::BluetoothRemoteGattService*>();
}

device::BluetoothRemoteGattCharacteristic*
FakeRemoteGattService::GetCharacteristic(const std::string& identifier) const {
  const auto& it = fake_characteristics_.find(identifier);
  if (it == fake_characteristics_.end())
    return nullptr;

  return it->second.get();
}

}  // namespace bluetooth
