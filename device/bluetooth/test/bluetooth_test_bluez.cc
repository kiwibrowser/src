// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/test/bluetooth_test_bluez.h"

#include <string>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "dbus/object_path.h"
#include "device/bluetooth/bluez/bluetooth_adapter_bluez.h"
#include "device/bluetooth/bluez/bluetooth_device_bluez.h"
#include "device/bluetooth/bluez/bluetooth_gatt_characteristic_bluez.h"
#include "device/bluetooth/bluez/bluetooth_gatt_descriptor_bluez.h"
#include "device/bluetooth/bluez/bluetooth_local_gatt_characteristic_bluez.h"
#include "device/bluetooth/bluez/bluetooth_local_gatt_descriptor_bluez.h"
#include "device/bluetooth/bluez/bluetooth_local_gatt_service_bluez.h"
#include "device/bluetooth/dbus/bluez_dbus_manager.h"
#include "device/bluetooth/dbus/fake_bluetooth_adapter_client.h"
#include "device/bluetooth/dbus/fake_bluetooth_device_client.h"
#include "device/bluetooth/dbus/fake_bluetooth_gatt_characteristic_service_provider.h"
#include "device/bluetooth/dbus/fake_bluetooth_gatt_descriptor_service_provider.h"
#include "device/bluetooth/dbus/fake_bluetooth_gatt_manager_client.h"
#include "device/bluetooth/test/test_bluetooth_local_gatt_service_delegate.h"

namespace device {

namespace {

void AdapterCallback(const base::Closure& quit_closure) {
  quit_closure.Run();
}

void GetValueCallback(
    const base::Closure& quit_closure,
    const BluetoothLocalGattService::Delegate::ValueCallback& value_callback,
    const std::vector<uint8_t>& value) {
  value_callback.Run(value);
  quit_closure.Run();
}

void ClosureCallback(const base::Closure& quit_closure,
                     const base::Closure& callback) {
  callback.Run();
  quit_closure.Run();
}

dbus::ObjectPath GetDevicePath(BluetoothDevice* device) {
  bluez::BluetoothDeviceBlueZ* device_bluez =
      static_cast<bluez::BluetoothDeviceBlueZ*>(device);
  return device_bluez->object_path();
}

}  // namespace

BluetoothTestBlueZ::BluetoothTestBlueZ()
    : fake_bluetooth_device_client_(nullptr) {}

BluetoothTestBlueZ::~BluetoothTestBlueZ() = default;

void BluetoothTestBlueZ::SetUp() {
  BluetoothTestBase::SetUp();
  std::unique_ptr<bluez::BluezDBusManagerSetter> dbus_setter =
      bluez::BluezDBusManager::GetSetterForTesting();

  fake_bluetooth_adapter_client_ = new bluez::FakeBluetoothAdapterClient;
  dbus_setter->SetBluetoothAdapterClient(
      std::unique_ptr<bluez::BluetoothAdapterClient>(
          fake_bluetooth_adapter_client_));

  fake_bluetooth_device_client_ = new bluez::FakeBluetoothDeviceClient;
  dbus_setter->SetBluetoothDeviceClient(
      std::unique_ptr<bluez::BluetoothDeviceClient>(
          fake_bluetooth_device_client_));

  // Make the fake adapter post tasks without delay in order to avoid timing
  // issues.
  fake_bluetooth_adapter_client_->SetSimulationIntervalMs(0);
}

void BluetoothTestBlueZ::TearDown() {
  for (const auto& connection : gatt_connections_) {
    if (connection->IsConnected())
      connection->Disconnect();
  }
  gatt_connections_.clear();

  for (const auto& session : discovery_sessions_) {
    if (session->IsActive())
      session->Stop(base::DoNothing(), base::DoNothing());
  }
  discovery_sessions_.clear();

  adapter_ = nullptr;
  bluez::BluezDBusManager::Shutdown();
  BluetoothTestBase::TearDown();
}

bool BluetoothTestBlueZ::PlatformSupportsLowEnergy() {
  return true;
}

void BluetoothTestBlueZ::InitWithFakeAdapter() {
  base::RunLoop run_loop;
  adapter_ = new bluez::BluetoothAdapterBlueZ(
      base::Bind(&AdapterCallback, run_loop.QuitClosure()));
  run_loop.Run();
  adapter_->SetPowered(true, base::DoNothing(), base::DoNothing());
}

BluetoothDevice* BluetoothTestBlueZ::SimulateLowEnergyDevice(
    int device_ordinal) {
  if (device_ordinal > 7 || device_ordinal < 1)
    return nullptr;

  base::Optional<std::string> device_name = std::string(kTestDeviceName);
  std::string device_address = kTestDeviceAddress1;
  std::vector<std::string> service_uuids;
  BluetoothTransport device_type = BLUETOOTH_TRANSPORT_LE;
  std::map<std::string, std::vector<uint8_t>> service_data;
  std::map<uint16_t, std::vector<uint8_t>> manufacturer_data;

  switch (device_ordinal) {
    case 1:
      service_uuids.push_back(kTestUUIDGenericAccess);
      service_uuids.push_back(kTestUUIDGenericAttribute);
      service_data[kTestUUIDHeartRate] = {0x01};
      manufacturer_data[kTestManufacturerId] = {1, 2, 3, 4};
      break;
    case 2:
      service_uuids.push_back(kTestUUIDImmediateAlert);
      service_uuids.push_back(kTestUUIDLinkLoss);
      service_data[kTestUUIDHeartRate] = {};
      service_data[kTestUUIDImmediateAlert] = {0x00, 0x02};
      manufacturer_data[kTestManufacturerId] = {};
      break;
    case 3:
      device_name = std::string(kTestDeviceNameEmpty);
      break;
    case 4:
      device_name = std::string(kTestDeviceNameEmpty);
      device_address = kTestDeviceAddress2;
      break;
    case 5:
      device_name = base::nullopt;
      break;
    case 6:
      device_address = kTestDeviceAddress2;
      device_type = BLUETOOTH_TRANSPORT_DUAL;
      break;
    case 7:
      device_name.emplace(kTestDeviceNameU2f);
      service_uuids.push_back(kTestUUIDU2f);
      service_data[kTestUUIDU2fControlPointLength] = {0x00, 0x14};
      break;
  }

  BluetoothDevice* device = adapter_->GetDevice(device_address);
  if (device) {
    fake_bluetooth_device_client_->UpdateServiceAndManufacturerData(
        GetDevicePath(device), service_uuids, service_data, manufacturer_data);
    return device;
  }

  fake_bluetooth_device_client_->CreateTestDevice(
      dbus::ObjectPath(bluez::FakeBluetoothAdapterClient::kAdapterPath),
      /* name */ device_name,
      /* alias */ device_name.value_or("") + "(alias)", device_address,
      service_uuids, device_type, service_data, manufacturer_data);

  return adapter_->GetDevice(device_address);
}

BluetoothDevice* BluetoothTestBlueZ::SimulateClassicDevice() {
  std::string device_name = kTestDeviceName;
  std::string device_address = kTestDeviceAddress3;
  std::vector<std::string> service_uuids;
  std::map<std::string, std::vector<uint8_t>> service_data;
  std::map<uint16_t, std::vector<uint8_t>> manufacturer_data;

  if (!adapter_->GetDevice(device_address)) {
    fake_bluetooth_device_client_->CreateTestDevice(
        dbus::ObjectPath(bluez::FakeBluetoothAdapterClient::kAdapterPath),
        device_name /* name */, device_name /* alias */, device_address,
        service_uuids, BLUETOOTH_TRANSPORT_CLASSIC, service_data,
        manufacturer_data);
  }
  return adapter_->GetDevice(device_address);
}

void BluetoothTestBlueZ::SimulateLocalGattCharacteristicValueReadRequest(
    BluetoothDevice* from_device,
    BluetoothLocalGattCharacteristic* characteristic,
    const BluetoothLocalGattService::Delegate::ValueCallback& value_callback,
    const base::Closure& error_callback) {
  bluez::BluetoothLocalGattCharacteristicBlueZ* characteristic_bluez =
      static_cast<bluez::BluetoothLocalGattCharacteristicBlueZ*>(
          characteristic);
  bluez::FakeBluetoothGattManagerClient* fake_bluetooth_gatt_manager_client =
      static_cast<bluez::FakeBluetoothGattManagerClient*>(
          bluez::BluezDBusManager::Get()->GetBluetoothGattManagerClient());
  bluez::FakeBluetoothGattCharacteristicServiceProvider*
      characteristic_provider =
          fake_bluetooth_gatt_manager_client->GetCharacteristicServiceProvider(
              characteristic_bluez->object_path());

  bluez::BluetoothLocalGattServiceBlueZ* service_bluez =
      static_cast<bluez::BluetoothLocalGattServiceBlueZ*>(
          characteristic->GetService());
  static_cast<TestBluetoothLocalGattServiceDelegate*>(
      service_bluez->GetDelegate())
      ->set_expected_characteristic(characteristic);

  base::RunLoop run_loop;
  characteristic_provider->GetValue(
      GetDevicePath(from_device),
      base::Bind(&GetValueCallback, run_loop.QuitClosure(), value_callback),
      base::Bind(&ClosureCallback, run_loop.QuitClosure(), error_callback));
  run_loop.Run();
}

void BluetoothTestBlueZ::SimulateLocalGattCharacteristicValueWriteRequest(
    BluetoothDevice* from_device,
    BluetoothLocalGattCharacteristic* characteristic,
    const std::vector<uint8_t>& value_to_write,
    const base::Closure& success_callback,
    const base::Closure& error_callback) {
  bluez::BluetoothLocalGattCharacteristicBlueZ* characteristic_bluez =
      static_cast<bluez::BluetoothLocalGattCharacteristicBlueZ*>(
          characteristic);
  bluez::FakeBluetoothGattManagerClient* fake_bluetooth_gatt_manager_client =
      static_cast<bluez::FakeBluetoothGattManagerClient*>(
          bluez::BluezDBusManager::Get()->GetBluetoothGattManagerClient());
  bluez::FakeBluetoothGattCharacteristicServiceProvider*
      characteristic_provider =
          fake_bluetooth_gatt_manager_client->GetCharacteristicServiceProvider(
              characteristic_bluez->object_path());

  bluez::BluetoothLocalGattServiceBlueZ* service_bluez =
      static_cast<bluez::BluetoothLocalGattServiceBlueZ*>(
          characteristic->GetService());
  static_cast<TestBluetoothLocalGattServiceDelegate*>(
      service_bluez->GetDelegate())
      ->set_expected_characteristic(characteristic);

  base::RunLoop run_loop;
  characteristic_provider->SetValue(
      GetDevicePath(from_device), value_to_write,
      base::Bind(&ClosureCallback, run_loop.QuitClosure(), success_callback),
      base::Bind(&ClosureCallback, run_loop.QuitClosure(), error_callback));
  run_loop.Run();
}

void BluetoothTestBlueZ::SimulateLocalGattDescriptorValueReadRequest(
    BluetoothDevice* from_device,
    BluetoothLocalGattDescriptor* descriptor,
    const BluetoothLocalGattService::Delegate::ValueCallback& value_callback,
    const base::Closure& error_callback) {
  bluez::BluetoothLocalGattDescriptorBlueZ* descriptor_bluez =
      static_cast<bluez::BluetoothLocalGattDescriptorBlueZ*>(descriptor);
  bluez::FakeBluetoothGattManagerClient* fake_bluetooth_gatt_manager_client =
      static_cast<bluez::FakeBluetoothGattManagerClient*>(
          bluez::BluezDBusManager::Get()->GetBluetoothGattManagerClient());
  bluez::FakeBluetoothGattDescriptorServiceProvider* descriptor_provider =
      fake_bluetooth_gatt_manager_client->GetDescriptorServiceProvider(
          descriptor_bluez->object_path());

  bluez::BluetoothLocalGattServiceBlueZ* service_bluez =
      static_cast<bluez::BluetoothLocalGattServiceBlueZ*>(
          descriptor->GetCharacteristic()->GetService());
  static_cast<TestBluetoothLocalGattServiceDelegate*>(
      service_bluez->GetDelegate())
      ->set_expected_descriptor(descriptor);

  base::RunLoop run_loop;
  descriptor_provider->GetValue(
      GetDevicePath(from_device),
      base::Bind(&GetValueCallback, run_loop.QuitClosure(), value_callback),
      base::Bind(&ClosureCallback, run_loop.QuitClosure(), error_callback));
  run_loop.Run();
}

void BluetoothTestBlueZ::SimulateLocalGattDescriptorValueWriteRequest(
    BluetoothDevice* from_device,
    BluetoothLocalGattDescriptor* descriptor,
    const std::vector<uint8_t>& value_to_write,
    const base::Closure& success_callback,
    const base::Closure& error_callback) {
  bluez::BluetoothLocalGattDescriptorBlueZ* descriptor_bluez =
      static_cast<bluez::BluetoothLocalGattDescriptorBlueZ*>(descriptor);
  bluez::FakeBluetoothGattManagerClient* fake_bluetooth_gatt_manager_client =
      static_cast<bluez::FakeBluetoothGattManagerClient*>(
          bluez::BluezDBusManager::Get()->GetBluetoothGattManagerClient());
  bluez::FakeBluetoothGattDescriptorServiceProvider* descriptor_provider =
      fake_bluetooth_gatt_manager_client->GetDescriptorServiceProvider(
          descriptor_bluez->object_path());

  bluez::BluetoothLocalGattServiceBlueZ* service_bluez =
      static_cast<bluez::BluetoothLocalGattServiceBlueZ*>(
          descriptor->GetCharacteristic()->GetService());
  static_cast<TestBluetoothLocalGattServiceDelegate*>(
      service_bluez->GetDelegate())
      ->set_expected_descriptor(descriptor);

  base::RunLoop run_loop;
  descriptor_provider->SetValue(
      GetDevicePath(from_device), value_to_write,
      base::Bind(&ClosureCallback, run_loop.QuitClosure(), success_callback),
      base::Bind(&ClosureCallback, run_loop.QuitClosure(), error_callback));
  run_loop.Run();
}

bool BluetoothTestBlueZ::SimulateLocalGattCharacteristicNotificationsRequest(
    BluetoothLocalGattCharacteristic* characteristic,
    bool start) {
  bluez::BluetoothLocalGattCharacteristicBlueZ* characteristic_bluez =
      static_cast<bluez::BluetoothLocalGattCharacteristicBlueZ*>(
          characteristic);
  bluez::FakeBluetoothGattManagerClient* fake_bluetooth_gatt_manager_client =
      static_cast<bluez::FakeBluetoothGattManagerClient*>(
          bluez::BluezDBusManager::Get()->GetBluetoothGattManagerClient());
  bluez::FakeBluetoothGattCharacteristicServiceProvider*
      characteristic_provider =
          fake_bluetooth_gatt_manager_client->GetCharacteristicServiceProvider(
              characteristic_bluez->object_path());

  bluez::BluetoothLocalGattServiceBlueZ* service_bluez =
      static_cast<bluez::BluetoothLocalGattServiceBlueZ*>(
          characteristic->GetService());
  static_cast<TestBluetoothLocalGattServiceDelegate*>(
      service_bluez->GetDelegate())
      ->set_expected_characteristic(characteristic);

  return characteristic_provider->NotificationsChange(start);
}

std::vector<uint8_t> BluetoothTestBlueZ::LastNotifactionValueForCharacteristic(
    BluetoothLocalGattCharacteristic* characteristic) {
  bluez::BluetoothLocalGattCharacteristicBlueZ* characteristic_bluez =
      static_cast<bluez::BluetoothLocalGattCharacteristicBlueZ*>(
          characteristic);
  bluez::FakeBluetoothGattManagerClient* fake_bluetooth_gatt_manager_client =
      static_cast<bluez::FakeBluetoothGattManagerClient*>(
          bluez::BluezDBusManager::Get()->GetBluetoothGattManagerClient());
  bluez::FakeBluetoothGattCharacteristicServiceProvider*
      characteristic_provider =
          fake_bluetooth_gatt_manager_client->GetCharacteristicServiceProvider(
              characteristic_bluez->object_path());

  return characteristic_provider ? characteristic_provider->sent_value()
                                 : std::vector<uint8_t>();
}

std::vector<BluetoothLocalGattService*>
BluetoothTestBlueZ::RegisteredGattServices() {
  std::vector<BluetoothLocalGattService*> services;
  bluez::BluetoothAdapterBlueZ* adapter_bluez =
      static_cast<bluez::BluetoothAdapterBlueZ*>(adapter_.get());

  for (const auto& iter : adapter_bluez->registered_gatt_services_)
    services.push_back(iter.second);
  return services;
}

}  // namespace device
