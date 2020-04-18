// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/bluetooth/tray_bluetooth_helper.h"

#include "ash/shell.h"
#include "ash/system/bluetooth/bluetooth_power_controller.h"
#include "ash/system/tray/system_tray_controller.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/metrics/user_metrics.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_discovery_session.h"
#include "device/bluetooth/chromeos/bluetooth_utils.h"

namespace ash {
namespace {

// System tray shows a limited number of bluetooth devices.
const int kMaximumDevicesShown = 50;

void BluetoothSetDiscoveringError() {
  LOG(ERROR) << "BluetoothSetDiscovering failed.";
}

void BluetoothDeviceConnectError(
    device::BluetoothDevice::ConnectErrorCode error_code) {}

ash::SystemTrayNotifier* GetSystemTrayNotifier() {
  return Shell::Get()->system_tray_notifier();
}

BluetoothDeviceInfo GetBluetoothDeviceInfo(device::BluetoothDevice* device) {
  BluetoothDeviceInfo info;
  info.address = device->GetAddress();
  info.display_name = device->GetNameForDisplay();
  info.connected = device->IsConnected();
  info.connecting = device->IsConnecting();
  info.paired = device->IsPaired();
  info.device_type = device->GetDeviceType();
  return info;
}

}  // namespace

BluetoothDeviceInfo::BluetoothDeviceInfo()
    : connected(false), connecting(false), paired(false) {}

BluetoothDeviceInfo::BluetoothDeviceInfo(const BluetoothDeviceInfo& other) =
    default;

BluetoothDeviceInfo::~BluetoothDeviceInfo() = default;

TrayBluetoothHelper::TrayBluetoothHelper() : weak_ptr_factory_(this) {}

TrayBluetoothHelper::~TrayBluetoothHelper() {
  if (adapter_)
    adapter_->RemoveObserver(this);
}

void TrayBluetoothHelper::Initialize() {
  device::BluetoothAdapterFactory::GetAdapter(
      base::Bind(&TrayBluetoothHelper::InitializeOnAdapterReady,
                 weak_ptr_factory_.GetWeakPtr()));
}

void TrayBluetoothHelper::InitializeOnAdapterReady(
    scoped_refptr<device::BluetoothAdapter> adapter) {
  adapter_ = adapter;
  CHECK(adapter_);
  adapter_->AddObserver(this);
}

BluetoothDeviceList TrayBluetoothHelper::GetAvailableBluetoothDevices() const {
  BluetoothDeviceList device_list;
  device::BluetoothAdapter::DeviceList devices =
      device::FilterBluetoothDeviceList(adapter_->GetDevices(),
                                        device::BluetoothFilterType::KNOWN,
                                        kMaximumDevicesShown);
  for (device::BluetoothDevice* device : devices)
    device_list.push_back(GetBluetoothDeviceInfo(device));

  return device_list;
}

void TrayBluetoothHelper::StartBluetoothDiscovering() {
  if (HasBluetoothDiscoverySession()) {
    LOG(WARNING) << "Already have active Bluetooth device discovery session.";
    return;
  }
  VLOG(1) << "Requesting new Bluetooth device discovery session.";
  should_run_discovery_ = true;
  adapter_->StartDiscoverySession(
      base::Bind(&TrayBluetoothHelper::OnStartDiscoverySession,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&BluetoothSetDiscoveringError));
}

void TrayBluetoothHelper::StopBluetoothDiscovering() {
  should_run_discovery_ = false;
  if (!HasBluetoothDiscoverySession()) {
    LOG(WARNING) << "No active Bluetooth device discovery session.";
    return;
  }
  VLOG(1) << "Stopping Bluetooth device discovery session.";
  discovery_session_->Stop(base::DoNothing(),
                           base::Bind(&BluetoothSetDiscoveringError));
}

void TrayBluetoothHelper::ConnectToBluetoothDevice(const std::string& address) {
  device::BluetoothDevice* device = adapter_->GetDevice(address);
  if (!device || device->IsConnecting() ||
      (device->IsConnected() && device->IsPaired())) {
    return;
  }
  if (device->IsPaired() && !device->IsConnectable())
    return;
  if (device->IsPaired() || !device->IsPairable()) {
    base::RecordAction(
        base::UserMetricsAction("StatusArea_Bluetooth_Connect_Known"));
    device->Connect(NULL, base::DoNothing(),
                    base::Bind(&BluetoothDeviceConnectError));
    return;
  }
  // Show pairing dialog for the unpaired device.
  Shell::Get()->system_tray_controller()->ShowBluetoothPairingDialog(
      device->GetAddress(), device->GetNameForDisplay(), device->IsPaired(),
      device->IsConnected());
}

bool TrayBluetoothHelper::GetBluetoothAvailable() {
  return adapter_ && adapter_->IsPresent();
}

bool TrayBluetoothHelper::GetBluetoothEnabled() {
  return adapter_ && adapter_->IsPowered();
}

void TrayBluetoothHelper::SetBluetoothEnabled(bool enabled) {
  if (GetBluetoothEnabled() != enabled) {
    Shell::Get()->metrics()->RecordUserMetricsAction(
        enabled ? UMA_STATUS_AREA_BLUETOOTH_ENABLED
                : UMA_STATUS_AREA_BLUETOOTH_DISABLED);
  }
  Shell::Get()->bluetooth_power_controller()->SetBluetoothEnabled(enabled);
}

bool TrayBluetoothHelper::HasBluetoothDiscoverySession() {
  return discovery_session_ && discovery_session_->IsActive();
}

////////////////////////////////////////////////////////////////////////////////
// BluetoothAdapter::Observer:

void TrayBluetoothHelper::AdapterPresentChanged(
    device::BluetoothAdapter* adapter,
    bool present) {
  GetSystemTrayNotifier()->NotifyRefreshBluetooth();
}

void TrayBluetoothHelper::AdapterPoweredChanged(
    device::BluetoothAdapter* adapter,
    bool powered) {
  GetSystemTrayNotifier()->NotifyRefreshBluetooth();
}

void TrayBluetoothHelper::AdapterDiscoveringChanged(
    device::BluetoothAdapter* adapter,
    bool discovering) {
  GetSystemTrayNotifier()->NotifyBluetoothDiscoveringChanged();
}

void TrayBluetoothHelper::DeviceAdded(device::BluetoothAdapter* adapter,
                                      device::BluetoothDevice* device) {
  GetSystemTrayNotifier()->NotifyRefreshBluetooth();
}

void TrayBluetoothHelper::DeviceChanged(device::BluetoothAdapter* adapter,
                                        device::BluetoothDevice* device) {
  GetSystemTrayNotifier()->NotifyRefreshBluetooth();
}

void TrayBluetoothHelper::DeviceRemoved(device::BluetoothAdapter* adapter,
                                        device::BluetoothDevice* device) {
  GetSystemTrayNotifier()->NotifyRefreshBluetooth();
}

void TrayBluetoothHelper::OnStartDiscoverySession(
    std::unique_ptr<device::BluetoothDiscoverySession> discovery_session) {
  // If the discovery session was returned after a request to stop discovery
  // (e.g. the user dismissed the Bluetooth detailed view before the call
  // returned), don't claim the discovery session and let it clean up.
  if (!should_run_discovery_)
    return;
  VLOG(1) << "Claiming new Bluetooth device discovery session.";
  discovery_session_ = std::move(discovery_session);
  GetSystemTrayNotifier()->NotifyBluetoothDiscoveringChanged();
}

}  // namespace ash
