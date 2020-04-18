// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/proximity_auth/bluetooth_low_energy_connection_finder.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "components/cryptauth/background_eid_generator.h"
#include "components/cryptauth/ble/bluetooth_low_energy_weave_client_connection.h"
#include "components/cryptauth/connection.h"
#include "components/cryptauth/raw_eid_generator.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_common.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_discovery_session.h"
#include "device/bluetooth/bluetooth_uuid.h"

using device::BluetoothAdapter;
using device::BluetoothDevice;
using device::BluetoothGattConnection;
using device::BluetoothDiscoveryFilter;

namespace proximity_auth {
namespace {
const char kAdvertisementUUID[] = "0000fe50-0000-1000-8000-00805f9b34fb";
const char kBLEGattServiceUUID[] = "b3b7e28e-a000-3e17-bd86-6e97b9e28c11";
const int kRestartDiscoveryOnErrorDelaySeconds = 2;
}  // namespace

BluetoothLowEnergyConnectionFinder::BluetoothLowEnergyConnectionFinder(
    cryptauth::RemoteDeviceRef remote_device)
    : BluetoothLowEnergyConnectionFinder(
          remote_device,
          kBLEGattServiceUUID,
          std::make_unique<cryptauth::BackgroundEidGenerator>()) {}

BluetoothLowEnergyConnectionFinder::BluetoothLowEnergyConnectionFinder(
    cryptauth::RemoteDeviceRef remote_device,
    const std::string& service_uuid,
    std::unique_ptr<cryptauth::BackgroundEidGenerator> eid_generator)
    : remote_device_(remote_device),
      service_uuid_(service_uuid),
      eid_generator_(std::move(eid_generator)),
      weak_ptr_factory_(this) {}

BluetoothLowEnergyConnectionFinder::~BluetoothLowEnergyConnectionFinder() {
  if (discovery_session_) {
    StopDiscoverySession();
  }

  if (connection_) {
    connection_->RemoveObserver(this);
    connection_.reset();
  }

  if (adapter_) {
    adapter_->RemoveObserver(this);
    adapter_ = nullptr;
  }
}

void BluetoothLowEnergyConnectionFinder::Find(
    const cryptauth::ConnectionFinder::ConnectionCallback&
        connection_callback) {
  if (!device::BluetoothAdapterFactory::IsBluetoothSupported()) {
    PA_LOG(WARNING) << "Bluetooth is unsupported on this platform. Aborting.";
    return;
  }
  PA_LOG(INFO) << "Finding connection";

  connection_callback_ = connection_callback;

  device::BluetoothAdapterFactory::GetAdapter(
      base::Bind(&BluetoothLowEnergyConnectionFinder::OnAdapterInitialized,
                 weak_ptr_factory_.GetWeakPtr()));
}

// It's not necessary to observe |AdapterPresentChanged| too. When |adapter_| is
// present, but not powered, it's not possible to scan for new devices.
void BluetoothLowEnergyConnectionFinder::AdapterPoweredChanged(
    BluetoothAdapter* adapter,
    bool powered) {
  DCHECK_EQ(adapter_.get(), adapter);
  PA_LOG(INFO) << "Adapter powered: " << powered;

  // Important: do not rely on |adapter->IsDiscoverying()| to verify if there is
  // an active discovery session. We need to create our own with an specific
  // filter.
  if (powered && (!discovery_session_ || !discovery_session_->IsActive()))
    StartDiscoverySession();
}

void BluetoothLowEnergyConnectionFinder::DeviceAdded(BluetoothAdapter* adapter,
                                                     BluetoothDevice* device) {
  DCHECK_EQ(adapter_.get(), adapter);
  DCHECK(device);

  // Note: Only consider |device| when it was actually added/updated during a
  // scanning, otherwise the device is stale and the GATT connection will fail.
  // For instance, when |adapter_| change status from unpowered to powered,
  // |DeviceAdded| is called for each paired |device|.
  if (adapter_->IsPowered() && discovery_session_ &&
      discovery_session_->IsActive())
    HandleDeviceUpdated(device);
}

void BluetoothLowEnergyConnectionFinder::DeviceChanged(
    BluetoothAdapter* adapter,
    BluetoothDevice* device) {
  DCHECK_EQ(adapter_.get(), adapter);
  DCHECK(device);

  // Note: Only consider |device| when it was actually added/updated during a
  // scanning, otherwise the device is stale and the GATT connection will fail.
  // For instance, when |adapter_| change status from unpowered to powered,
  // |DeviceAdded| is called for each paired |device|.
  if (adapter_->IsPowered() && discovery_session_ &&
      discovery_session_->IsActive())
    HandleDeviceUpdated(device);
}

void BluetoothLowEnergyConnectionFinder::HandleDeviceUpdated(
    BluetoothDevice* device) {
  // Ensuring only one call to |CreateConnection()| is made. A new |connection_|
  // can be created only when the previous one disconnects, triggering a call to
  // |OnConnectionStatusChanged|.
  if (connection_)
    return;

  if (IsRightDevice(device)) {
    PA_LOG(INFO) << "Connecting to device " << device->GetAddress();
    connection_ = CreateConnection(device);
    connection_->AddObserver(this);
    connection_->Connect();

    StopDiscoverySession();
  }
}

bool BluetoothLowEnergyConnectionFinder::IsRightDevice(
    BluetoothDevice* device) {
  if (!device)
    return false;

  device::BluetoothUUID advertisement_uuid(kAdvertisementUUID);
  const std::vector<uint8_t>* service_data =
      device->GetServiceDataForUUID(advertisement_uuid);
  if (!service_data)
    return false;

  std::string service_data_string(service_data->begin(), service_data->end());
  std::vector<cryptauth::DataWithTimestamp> nearest_eids =
      eid_generator_->GenerateNearestEids(remote_device_.beacon_seeds());
  for (const cryptauth::DataWithTimestamp& eid : nearest_eids) {
    if (eid.data == service_data_string) {
      PA_LOG(INFO) << "Found a matching EID: " << eid.DataInHex();
      return true;
    }
  }
  return false;
}

void BluetoothLowEnergyConnectionFinder::OnAdapterInitialized(
    scoped_refptr<BluetoothAdapter> adapter) {
  PA_LOG(INFO) << "Adapter ready";
  adapter_ = adapter;
  adapter_->AddObserver(this);
  StartDiscoverySession();
}

void BluetoothLowEnergyConnectionFinder::OnDiscoverySessionStarted(
    std::unique_ptr<device::BluetoothDiscoverySession> discovery_session) {
  PA_LOG(INFO) << "Discovery session started";
  discovery_session_ = std::move(discovery_session);
}

void BluetoothLowEnergyConnectionFinder::OnStartDiscoverySessionError() {
  PA_LOG(WARNING) << "Error starting discovery session, restarting in "
                  << kRestartDiscoveryOnErrorDelaySeconds << " seconds.";
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(
          &BluetoothLowEnergyConnectionFinder::RestartDiscoverySessionAsync,
          weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(kRestartDiscoveryOnErrorDelaySeconds));
}

void BluetoothLowEnergyConnectionFinder::StartDiscoverySession() {
  DCHECK(adapter_);
  if (discovery_session_ && discovery_session_->IsActive()) {
    PA_LOG(INFO) << "Discovery session already active";
    return;
  }

  // Discover only low energy (LE) devices.
  std::unique_ptr<BluetoothDiscoveryFilter> filter(
      new BluetoothDiscoveryFilter(device::BLUETOOTH_TRANSPORT_LE));

  adapter_->StartDiscoverySessionWithFilter(
      std::move(filter),
      base::Bind(&BluetoothLowEnergyConnectionFinder::OnDiscoverySessionStarted,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(
          &BluetoothLowEnergyConnectionFinder::OnStartDiscoverySessionError,
          weak_ptr_factory_.GetWeakPtr()));
}

void BluetoothLowEnergyConnectionFinder::StopDiscoverySession() {
  PA_LOG(INFO) << "Stopping discovery session";
  // Destroying the discovery session also stops it.
  discovery_session_.reset();
}

std::unique_ptr<cryptauth::Connection>
BluetoothLowEnergyConnectionFinder::CreateConnection(
    device::BluetoothDevice* bluetooth_device) {
  return cryptauth::weave::BluetoothLowEnergyWeaveClientConnection::Factory::
      NewInstance(remote_device_, adapter_,
                  device::BluetoothUUID(service_uuid_), bluetooth_device,
                  true /* should_set_low_connection_latency */);
}

void BluetoothLowEnergyConnectionFinder::OnConnectionStatusChanged(
    cryptauth::Connection* connection,
    cryptauth::Connection::Status old_status,
    cryptauth::Connection::Status new_status) {
  DCHECK_EQ(connection, connection_.get());
  PA_LOG(INFO) << "OnConnectionStatusChanged: " << old_status << " -> "
               << new_status;

  if (!connection_callback_.is_null() && connection_->IsConnected()) {
    adapter_->RemoveObserver(this);
    connection_->RemoveObserver(this);

    // If we invoke the callback now, the callback function may install its own
    // observer to |connection_|. Because we are in the ConnectionObserver
    // callstack, this new observer will receive this connection event.
    // Therefore, we need to invoke the callback or restart discovery
    // asynchronously.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&BluetoothLowEnergyConnectionFinder::InvokeCallbackAsync,
                       weak_ptr_factory_.GetWeakPtr()));
  } else if (old_status == cryptauth::Connection::Status::IN_PROGRESS) {
    PA_LOG(WARNING) << "Connection failed. Retrying.";
    connection_->RemoveObserver(this);
    connection_.reset();
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(
            &BluetoothLowEnergyConnectionFinder::RestartDiscoverySessionAsync,
            weak_ptr_factory_.GetWeakPtr()));
  }
}

void BluetoothLowEnergyConnectionFinder::RestartDiscoverySessionAsync() {
  bool discovery_active = discovery_session_ && discovery_session_->IsActive();
  if (!connection_ && !discovery_active) {
    PA_LOG(INFO) << "Restarting discovery session.";
    StartDiscoverySession();
  }
}

void BluetoothLowEnergyConnectionFinder::InvokeCallbackAsync() {
  connection_callback_.Run(std::move(connection_));
}

}  // namespace proximity_auth
