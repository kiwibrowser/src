// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/ble_scanner_impl.h"

#include <memory>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "chromeos/components/tether/ble_constants.h"
#include "chromeos/components/tether/ble_synchronizer.h"
#include "chromeos/components/tether/tether_host_fetcher.h"
#include "components/cryptauth/local_device_data_provider.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/remote_beacon_seed_fetcher.h"
#include "components/cryptauth/remote_device_ref.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_discovery_session.h"
#include "device/bluetooth/bluetooth_uuid.h"

namespace chromeos {

namespace tether {

namespace {

// Valid advertisement service data must be at least 2 bytes.
// As of March 2018, valid background advertisement service data is exactly 2
// bytes, which identify the advertising device to the scanning device.
// Valid foreground advertisement service data must include at least 4 bytes:
// 2 bytes associated with the scanning device (used as a scan filter) and 2
// bytes which identify the advertising device to the scanning device.
const size_t kMinNumBytesInServiceData = 2;
const size_t kMaxNumBytesInBackgroundServiceData = 3;
const size_t kMinNumBytesInForegroundServiceData = 4;

}  // namespace

// static
BleScannerImpl::Factory* BleScannerImpl::Factory::factory_instance_ = nullptr;

// static
std::unique_ptr<BleScanner> BleScannerImpl::Factory::NewInstance(
    scoped_refptr<device::BluetoothAdapter> adapter,
    cryptauth::LocalDeviceDataProvider* local_device_data_provider,
    cryptauth::RemoteBeaconSeedFetcher* remote_beacon_seed_fetcher,
    BleSynchronizerBase* ble_synchronizer,
    TetherHostFetcher* tether_host_fetcher) {
  if (!factory_instance_)
    factory_instance_ = new Factory();

  return factory_instance_->BuildInstance(
      adapter, local_device_data_provider, remote_beacon_seed_fetcher,
      ble_synchronizer, tether_host_fetcher);
}

// static
void BleScannerImpl::Factory::SetInstanceForTesting(Factory* factory) {
  factory_instance_ = factory;
}

std::unique_ptr<BleScanner> BleScannerImpl::Factory::BuildInstance(
    scoped_refptr<device::BluetoothAdapter> adapter,
    cryptauth::LocalDeviceDataProvider* local_device_data_provider,
    cryptauth::RemoteBeaconSeedFetcher* remote_beacon_seed_fetcher,
    BleSynchronizerBase* ble_synchronizer,
    TetherHostFetcher* tether_host_fetcher) {
  return base::WrapUnique(new BleScannerImpl(
      adapter, local_device_data_provider, remote_beacon_seed_fetcher,
      ble_synchronizer, tether_host_fetcher));
}

BleScannerImpl::ServiceDataProviderImpl::ServiceDataProviderImpl() = default;

BleScannerImpl::ServiceDataProviderImpl::~ServiceDataProviderImpl() = default;

const std::vector<uint8_t>*
BleScannerImpl::ServiceDataProviderImpl::GetServiceDataForUUID(
    device::BluetoothDevice* bluetooth_device) {
  return bluetooth_device->GetServiceDataForUUID(
      device::BluetoothUUID(kAdvertisingServiceUuid));
}

BleScannerImpl::BleScannerImpl(
    scoped_refptr<device::BluetoothAdapter> adapter,
    cryptauth::LocalDeviceDataProvider* local_device_data_provider,
    cryptauth::RemoteBeaconSeedFetcher* remote_beacon_seed_fetcher,
    BleSynchronizerBase* ble_synchronizer,
    TetherHostFetcher* tether_host_fetcher)
    : adapter_(adapter),
      local_device_data_provider_(local_device_data_provider),
      remote_beacon_seed_fetcher_(remote_beacon_seed_fetcher),
      ble_synchronizer_(ble_synchronizer),
      tether_host_fetcher_(tether_host_fetcher),
      service_data_provider_(std::make_unique<ServiceDataProviderImpl>()),
      background_eid_generator_(
          std::make_unique<cryptauth::BackgroundEidGenerator>()),
      foreground_eid_generator_(
          std::make_unique<cryptauth::ForegroundEidGenerator>()),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_ptr_factory_(this) {
  adapter_->AddObserver(this);
}

BleScannerImpl::~BleScannerImpl() {
  adapter_->RemoveObserver(this);
}

bool BleScannerImpl::RegisterScanFilterForDevice(const std::string& device_id) {
  if (registered_remote_device_ids_.size() >= kMaxConcurrentAdvertisements) {
    // Each scan filter corresponds to an advertisement. Thus, the number of
    // concurrent advertisements cannot exceed the maximum number of concurrent
    // advertisements.
    PA_LOG(WARNING) << "Attempted to start a scan for a new device when the "
                    << "maximum number of devices have already been "
                    << "registered.";
    return false;
  }

  std::vector<cryptauth::BeaconSeed> local_device_beacon_seeds;
  if (!local_device_data_provider_->GetLocalDeviceData(
          nullptr, &local_device_beacon_seeds)) {
    PA_LOG(WARNING) << "Error fetching the local device's beacon seeds. Cannot "
                    << "generate scan without beacon seeds.";
    return false;
  }

  std::unique_ptr<cryptauth::ForegroundEidGenerator::EidData> scan_filters =
      foreground_eid_generator_->GenerateBackgroundScanFilter(
          local_device_beacon_seeds);
  if (!scan_filters) {
    PA_LOG(WARNING) << "Error generating background scan filters. Cannot scan";
    return false;
  }

  registered_remote_device_ids_.push_back(device_id);
  UpdateDiscoveryStatus();

  return true;
}

bool BleScannerImpl::UnregisterScanFilterForDevice(
    const std::string& device_id) {
  for (auto it = registered_remote_device_ids_.begin();
       it != registered_remote_device_ids_.end(); ++it) {
    if (*it == device_id) {
      registered_remote_device_ids_.erase(it);
      UpdateDiscoveryStatus();
      return true;
    }
  }

  return false;
}

bool BleScannerImpl::ShouldDiscoverySessionBeActive() {
  return !registered_remote_device_ids_.empty();
}

bool BleScannerImpl::IsDiscoverySessionActive() {
  ResetDiscoverySessionIfNotActive();

  if (discovery_session_) {
    // Once the session is stopped, the pointer is cleared.
    DCHECK(discovery_session_->IsActive());
    return true;
  }

  return false;
}

void BleScannerImpl::SetTestDoubles(
    std::unique_ptr<ServiceDataProvider> service_data_provider,
    std::unique_ptr<cryptauth::BackgroundEidGenerator> background_eid_generator,
    std::unique_ptr<cryptauth::ForegroundEidGenerator> foreground_eid_generator,
    scoped_refptr<base::TaskRunner> test_task_runner) {
  service_data_provider_ = std::move(service_data_provider);
  background_eid_generator_ = std::move(background_eid_generator);
  foreground_eid_generator_ = std::move(foreground_eid_generator);
  task_runner_ = test_task_runner;
}

bool BleScannerImpl::IsDeviceRegistered(const std::string& device_id) {
  return std::find(registered_remote_device_ids_.begin(),
                   registered_remote_device_ids_.end(),
                   device_id) != registered_remote_device_ids_.end();
}

void BleScannerImpl::DeviceAdded(device::BluetoothAdapter* adapter,
                                 device::BluetoothDevice* bluetooth_device) {
  DCHECK_EQ(adapter_.get(), adapter);
  HandleDeviceUpdated(bluetooth_device);
}

void BleScannerImpl::DeviceChanged(device::BluetoothAdapter* adapter,
                                   device::BluetoothDevice* bluetooth_device) {
  DCHECK_EQ(adapter_.get(), adapter);
  HandleDeviceUpdated(bluetooth_device);
}

void BleScannerImpl::ResetDiscoverySessionIfNotActive() {
  if (!discovery_session_ || discovery_session_->IsActive())
    return;

  PA_LOG(ERROR) << "BluetoothDiscoverySession became out of sync. Session is "
                << "no longer active, but it was never stopped successfully. "
                << "Resetting session.";

  // |discovery_session_| should be deleted whenever the session is no longer
  // active. However, due to Bluetooth bugs, this does not always occur
  // properly. When we detect that this situation has occurred, delete the
  // pointer and reset discovery state.
  discovery_session_.reset();
  discovery_session_weak_ptr_factory_.reset();
  is_initializing_discovery_session_ = false;
  is_stopping_discovery_session_ = false;
  weak_ptr_factory_.InvalidateWeakPtrs();

  ScheduleStatusChangeNotification(false /* discovery_session_active */);
}

void BleScannerImpl::UpdateDiscoveryStatus() {
  if (ShouldDiscoverySessionBeActive())
    EnsureDiscoverySessionActive();
  else
    EnsureDiscoverySessionNotActive();
}

void BleScannerImpl::EnsureDiscoverySessionActive() {
  // If the session is active or is in the process of becoming active, there is
  // nothing to do.
  if (IsDiscoverySessionActive() || is_initializing_discovery_session_)
    return;

  is_initializing_discovery_session_ = true;

  ble_synchronizer_->StartDiscoverySession(
      base::Bind(&BleScannerImpl::OnDiscoverySessionStarted,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&BleScannerImpl::OnStartDiscoverySessionError,
                 weak_ptr_factory_.GetWeakPtr()));
}

void BleScannerImpl::OnDiscoverySessionStarted(
    std::unique_ptr<device::BluetoothDiscoverySession> discovery_session) {
  is_initializing_discovery_session_ = false;
  PA_LOG(INFO) << "Started discovery session successfully.";

  discovery_session_ = std::move(discovery_session);
  discovery_session_weak_ptr_factory_ =
      std::make_unique<base::WeakPtrFactory<device::BluetoothDiscoverySession>>(
          discovery_session_.get());

  ScheduleStatusChangeNotification(true /* discovery_session_active */);

  UpdateDiscoveryStatus();
}

void BleScannerImpl::OnStartDiscoverySessionError() {
  PA_LOG(ERROR) << "Error starting discovery session. Initialization failed.";
  is_initializing_discovery_session_ = false;
  UpdateDiscoveryStatus();
}

void BleScannerImpl::EnsureDiscoverySessionNotActive() {
  // If there is no session, there is nothing to do.
  if (!IsDiscoverySessionActive() || is_stopping_discovery_session_)
    return;

  is_stopping_discovery_session_ = true;

  ble_synchronizer_->StopDiscoverySession(
      discovery_session_weak_ptr_factory_->GetWeakPtr(),
      base::Bind(&BleScannerImpl::OnDiscoverySessionStopped,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&BleScannerImpl::OnStopDiscoverySessionError,
                 weak_ptr_factory_.GetWeakPtr()));
}

void BleScannerImpl::OnDiscoverySessionStopped() {
  is_stopping_discovery_session_ = false;
  PA_LOG(INFO) << "Stopped discovery session successfully.";

  discovery_session_.reset();
  discovery_session_weak_ptr_factory_.reset();

  ScheduleStatusChangeNotification(false /* discovery_session_active */);

  UpdateDiscoveryStatus();
}

void BleScannerImpl::OnStopDiscoverySessionError() {
  PA_LOG(ERROR) << "Error stopping discovery session.";
  is_stopping_discovery_session_ = false;
  UpdateDiscoveryStatus();
}

void BleScannerImpl::HandleDeviceUpdated(
    device::BluetoothDevice* bluetooth_device) {
  DCHECK(bluetooth_device);

  const std::vector<uint8_t>* service_data =
      service_data_provider_->GetServiceDataForUUID(bluetooth_device);
  if (!service_data || service_data->size() < kMinNumBytesInServiceData) {
    // If there is no service data or the service data is of insufficient
    // length, there is not enough information to create a connection.
    return;
  }

  // Convert the service data from a std::vector<uint8_t> to a std::string.
  std::string service_data_str;
  char* string_contents_ptr =
      base::WriteInto(&service_data_str, service_data->size() + 1);
  memcpy(string_contents_ptr, service_data->data(), service_data->size());

  CheckForMatchingScanFilters(bluetooth_device, service_data_str);
}

void BleScannerImpl::CheckForMatchingScanFilters(
    device::BluetoothDevice* bluetooth_device,
    const std::string& service_data) {
  std::string device_id;
  bool is_background_advertisement = false;

  // First try, identifying |service_data| as a foreground advertisement.
  if (service_data.size() >= kMinNumBytesInForegroundServiceData) {
    std::vector<cryptauth::BeaconSeed> beacon_seeds;
    if (local_device_data_provider_->GetLocalDeviceData(nullptr,
                                                        &beacon_seeds)) {
      device_id =
          foreground_eid_generator_->IdentifyRemoteDeviceByAdvertisement(
              service_data, registered_remote_device_ids_, beacon_seeds);
    }
  }

  // If the device has not yet been identified, try identifying |service_data|
  // as a background advertisement.
  if (chromeos::switches::IsInstantTetheringBackgroundAdvertisingSupported() &&
      device_id.empty() && service_data.size() >= kMinNumBytesInServiceData &&
      service_data.size() <= kMaxNumBytesInBackgroundServiceData) {
    device_id = background_eid_generator_->IdentifyRemoteDeviceByAdvertisement(
        remote_beacon_seed_fetcher_, service_data,
        registered_remote_device_ids_);
    is_background_advertisement = true;
  }

  // If the service data does not correspond to an advertisement from a device
  // on this account, ignore it.
  if (device_id.empty())
    return;

  tether_host_fetcher_->FetchTetherHost(
      device_id, base::Bind(&BleScannerImpl::OnIdentifiedHostFetched,
                            weak_ptr_factory_.GetWeakPtr(), bluetooth_device,
                            device_id, is_background_advertisement));
}

void BleScannerImpl::OnIdentifiedHostFetched(
    device::BluetoothDevice* bluetooth_device,
    const std::string& device_id,
    bool is_background_advertisement,
    base::Optional<cryptauth::RemoteDeviceRef> identified_device) {
  if (!identified_device) {
    PA_LOG(ERROR) << "Unable to fetch RemoteDevice object with ID \""
                  << cryptauth::RemoteDeviceRef::TruncateDeviceIdForLogs(
                         device_id)
                  << "\".";
    return;
  }

  NotifyReceivedAdvertisementFromDevice(*identified_device, bluetooth_device,
                                        is_background_advertisement);
}

void BleScannerImpl::ScheduleStatusChangeNotification(
    bool discovery_session_active) {
  // Schedule the task to run after the current task has completed. This is
  // necessary because the completion of a Bluetooth task may cause the Tether
  // component to be shut down; if that occurs, then we cannot reference
  // instance variables in this class after the object has been deleted.
  // Completing the current command as part of the next task ensures that this
  // cannot occur. See crbug.com/776241.
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&BleScannerImpl::NotifyDiscoverySessionStateChanged,
                     weak_ptr_factory_.GetWeakPtr(), discovery_session_active));
}

}  // namespace tether

}  // namespace chromeos
